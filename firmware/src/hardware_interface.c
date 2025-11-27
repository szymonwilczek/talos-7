#include "hardware_interface.h"

#include "cdc/cdc_transport.h"
#include "hardware/timer.h"
#include "macro_config.h"
#include "oled_display.h"
#include "pico/stdlib.h"
#include "pin_definitions.h"
#include <stdio.h>
#include <string.h>

// button states
static volatile bool button_states[NUM_BUTTONS] = {false};
static bool button_prev_states[NUM_BUTTONS] = {false};

// LED states
static bool led_states[NUM_BUTTONS] = {false};

// Timer structure
static struct repeating_timer buttons_timer;

// BUTTON SCANNING CALLBACK (ISR)
static bool buttons_timer_callback(struct repeating_timer *t) {
  (void)t;

  for (int i = 0; i < NUM_BUTTONS; i++) {
    button_prev_states[i] = button_states[i];

    bool raw_state = !gpio_get(BUTTON_PINS[i]);

    button_states[i] = raw_state;
  }

  return true;
}

const char *get_key_name(uint8_t keycode) {
  static const char *key_names[] = {
      "A",     "B",   "C",         "D",   "E",     "F",   "G",   "H",   "I",
      "J",     "K",   "L",         "M",   "N",     "O",   "P",   "Q",   "R",
      "S",     "T",   "U",         "V",   "W",     "X",   "Y",   "Z",   "1",
      "2",     "3",   "4",         "5",   "6",     "7",   "8",   "9",   "0",
      "Enter", "Esc", "Backspace", "Tab", "Space", "F1",  "F2",  "F3",  "F4",
      "F5",    "F6",  "F7",        "F8",  "F9",    "F10", "F11", "F12", "→",
      "←",     "↓",   "↑"};
  if (keycode >= 4 && keycode <= 29)
    return key_names[keycode - 4]; // A-Z
  if (keycode >= 30 && keycode <= 39)
    return key_names[keycode - 30 + 26]; // 1-0
  if (keycode == 40)
    return "Enter";
  if (keycode == 41)
    return "Esc";
  if (keycode == 42)
    return "Backspace";
  if (keycode == 43)
    return "Tab";
  if (keycode == 44)
    return "Space";
  if (keycode >= 58 && keycode <= 69)
    return key_names[keycode - 58 + 36]; // F1-F12
  if (keycode >= 79 && keycode <= 82)
    return key_names[keycode - 79 + 48]; // arrows
  if (keycode == 224)
    return "Ctrl";
  if (keycode == 225)
    return "Shift";
  if (keycode == 226)
    return "Alt";
  if (keycode == 227)
    return "Meta";
  return "Key";
}

const char *format_sequence_short(key_step_t *sequence, uint8_t len) {
  static char buf[32];
  buf[0] = '\0';
  if (len == 0)
    return buf;

  // pierwszy klawisz + modyfikatory (vim like)
  char mods[8] = "";
  if (sequence[0].modifiers & 1)
    strcat(mods, "C"); // ctrl
  if (sequence[0].modifiers & 2)
    strcat(mods, "S"); // shift
  if (sequence[0].modifiers & 4)
    strcat(mods, "A"); // alt
  if (sequence[0].modifiers & 8)
    strcat(mods, "M"); // meta (gui)
  if (strlen(mods) > 0) {
    sprintf(buf, "<%s-%s>", mods, get_key_name(sequence[0].keycode));
  } else {
    strcpy(buf, get_key_name(sequence[0].keycode));
  }

  // nastepne klawisze
  for (int i = 1; i < len && i < 3; i++) {
    strcat(buf, "+");
    strcat(buf, get_key_name(sequence[i].keycode));
  }
  if (len > 3)
    strcat(buf, "...");
  return buf;
}

// ==================== BUTTONS ====================

void buttons_init(void) {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    gpio_init(BUTTON_PINS[i]);
    gpio_set_dir(BUTTON_PINS[i], GPIO_IN);
    gpio_pull_up(BUTTON_PINS[i]); // pull-up (button connects to GND)
  }

  gpio_init(BTN_OS_TOGGLE_PIN);
  gpio_set_dir(BTN_OS_TOGGLE_PIN, GPIO_IN);
  gpio_pull_up(BTN_OS_TOGGLE_PIN);

  add_repeating_timer_ms(5, buttons_timer_callback, NULL, &buttons_timer);

  cdc_log("[BUTTONS] Initialized (%d buttons)\n", NUM_BUTTONS);
}

bool button_is_pressed(uint8_t button_index) {
  if (button_index >= NUM_BUTTONS)
    return false;
  return button_states[button_index];
}

// ==================== LED ====================

void leds_init(void) {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    gpio_init(LED_PINS[i]);
    gpio_set_dir(LED_PINS[i], GPIO_OUT);
    gpio_put(LED_PINS[i], false);
  }
  cdc_log("[LEDS] Initialized (%d LEDs)\n", NUM_BUTTONS);
}

void led_set(uint8_t led_index, bool state) {
  if (led_index >= NUM_BUTTONS)
    return;
  led_states[led_index] = state;
  gpio_put(LED_PINS[led_index], state);
}

void led_toggle(uint8_t led_index) {
  led_set(led_index, !led_states[led_index]);
}

void leds_update_for_layer(uint8_t layer) {
  // turn off all LEDs, then turn on LED for current layer (if layer < 7)
  for (int i = 0; i < NUM_BUTTONS; i++) {
    led_set(i, false);
  }

  if (layer < NUM_BUTTONS - 1) {
    led_set(layer, true);
  }
}

bool map_char_to_hid(char c, uint8_t *keycode, uint8_t *modifiers) {
  *modifiers = 0;
  if (c >= 'a' && c <= 'z') {
    *keycode = c - 'a' + 4;
  } else if (c >= 'A' && c <= 'Z') {
    *keycode = c - 'A' + 4;
    *modifiers = 0x02; // SHIFT
  } else if (c >= '1' && c <= '9') {
    *keycode = c - '1' + 30;
  } else if (c == '0') {
    *keycode = 39;
  } else if (c == ' ') {
    *keycode = 44;
  } else if (c == '\n' || c == '\r') {
    *keycode = 40; // Enter
  } else if (c == '!') {
    *keycode = 30; // 1
    *modifiers = 0x02;
  } else if (c == '@') {
    *keycode = 31; // 2
    *modifiers = 0x02;
  } else if (c == '#') {
    *keycode = 32; // 3
    *modifiers = 0x02;
  } else if (c == '$') {
    *keycode = 33; // 4
    *modifiers = 0x02;
  } else if (c == '%') {
    *keycode = 34; // 5
    *modifiers = 0x02;
  } else if (c == '^') {
    *keycode = 35; // 6
    *modifiers = 0x02;
  } else if (c == '&') {
    *keycode = 36; // 7
    *modifiers = 0x02;
  } else if (c == '*') {
    *keycode = 37; // 8
    *modifiers = 0x02;
  } else if (c == '(') {
    *keycode = 38; // 9
    *modifiers = 0x02;
  } else if (c == ')') {
    *keycode = 39; // 0
    *modifiers = 0x02;
  } else if (c == '_') {
    *keycode = 45; // -
    *modifiers = 0x02;
  } else if (c == '+') {
    *keycode = 46; // =
    *modifiers = 0x02;
  } else if (c == '{') {
    *keycode = 47; // [
    *modifiers = 0x02;
  } else if (c == '}') {
    *keycode = 48; // ]
    *modifiers = 0x02;
  } else if (c == '|') {
    *keycode = 49; // \
    *modifiers = 0x02;
  } else if (c == ':') {
    *keycode = 51; // ;
    *modifiers = 0x02;
  } else if (c == '"') {
    *keycode = 52; // '
    *modifiers = 0x02;
  } else if (c == '~') {
    *keycode = 53; // `
    *modifiers = 0x02;
  } else if (c == '<') {
    *keycode = 54; // ,
    *modifiers = 0x02;
  } else if (c == '>') {
    *keycode = 55; // .
    *modifiers = 0x02;
  } else if (c == '?') {
    *keycode = 56; // /
    *modifiers = 0x02;
  } else if (c == '\t') {
    *keycode = 43; // Tab
  } else if (c == '\b') {
    *keycode = 42; // Backspace
  } else if (c == '.') {
    *keycode = 55;
  } else if (c == ',') {
    *keycode = 54;
  } else if (c == '-') {
    *keycode = 45;
  } else if (c == '=') {
    *keycode = 46;
  } else if (c == '[') {
    *keycode = 47;
  } else if (c == ']') {
    *keycode = 48;
  } else if (c == '\\') {
    *keycode = 49;
  } else if (c == ';') {
    *keycode = 51;
  } else if (c == '\'') {
    *keycode = 52;
  } else if (c == '`') {
    *keycode = 53;
  } else if (c == '/') {
    *keycode = 56;
  } else {
    return false; // Nieobsługiwany znak
  }
  return true;
}

uint32_t utf8_to_codepoint(const char **str) {
  unsigned char c = **str;
  (*str)++;
  if (c < 0x80)
    return c;
  if (c < 0xC0)
    return 0; // Invalid
  int len = 0;
  if (c < 0xE0)
    len = 1;
  else if (c < 0xF0)
    len = 2;
  else if (c < 0xF8)
    len = 3;
  else
    return 0;
  uint32_t code = c & (0xFF >> (len + 1));
  for (int i = 0; i < len; i++) {
    c = **str;
    (*str)++;
    if ((c & 0xC0) != 0x80)
      return 0;
    code = (code << 6) | (c & 0x3F);
  }
  return code;
}

// ==================== INIT ====================

void hardware_init(void) {
  cdc_log("[HARDWARE] Initializing...\n");
  buttons_init();
  leds_init();
  oled_init();
  cdc_log("[HARDWARE] Initialization complete\n");
}
