#include "executor/actions/exec_script.h"

#include "cdc/cdc_transport.h"
#include "executor/actions/exec_hid_core.h"
#include "executor/actions/exec_text.h"
#include "tusb.h"
#include <stdint.h>

void exec_script(const char *script, uint8_t platform,
                 const key_step_t *shortcut, uint8_t shortcut_len) {
  cdc_log("[SCRIPT] Executing script (platform=%d)\n", platform);

  if (platform == 0) { // LINUX

    // 1. terminal shortcut
    if (shortcut_len > 0) {
      for (int i = 0; i < shortcut_len; i++) {
        press_sequence(shortcut[i].modifiers, shortcut[i].keycode);

        // minimal delay between keys
        if (i < shortcut_len - 1)
          sleep_ms(100);
      }
    } else {
      // fallback: ctrl+alt+t
      press_sequence(0x05, 23);
    }

    sleep_ms(1500); // waiting for gui response

    // 2. temporary file
    type_text_content("cat << 'EOF' > /tmp/m.sh\n", platform);
    sleep_ms(200);

    // 3. type content
    type_text_content(script, platform);

    // 4. close file (enter -> eof -> enter)
    press_sequence(0, 40); // enter
    type_text_content("EOF\n", platform);
    sleep_ms(200);

    // 5. run and cleanup
    type_text_content("chmod +x /tmp/m.sh && /tmp/m.sh && rm /tmp/m.sh\n",
                      platform);

  } else if (platform == 1) { // WINDOWS
    press_sequence(0x08, 21); // Win + R
    sleep_ms(500);

    type_text_content("powershell\n", platform);
    sleep_ms(1500);

    type_text_content(script, platform);
    press_sequence(0, 40);
  } else if (platform == 2) { // MACOS
    press_sequence(0x08, 44); // Cmd + Space
    sleep_ms(300);

    type_text_content("Terminal", platform);
    sleep_ms(100);

    press_sequence(0, 40);
    sleep_ms(1000);

    type_text_content("cat << 'EOF' > /tmp/m.sh\n", platform);
    type_text_content(script, platform);

    press_sequence(0, 40);
    type_text_content("EOF\n", platform);
    sleep_ms(100);

    type_text_content("sh /tmp/m.sh && rm /tmp/m.sh\n", platform);
  }
}
