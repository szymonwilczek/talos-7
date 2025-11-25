#include "pico/unique_id.h"
#include "pin_definitions.h"
#include "tusb.h"

extern volatile uint8_t g_detected_platform;

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device = {.bLength = sizeof(tusb_desc_device_t),
                                        .bDescriptorType = TUSB_DESC_DEVICE,
                                        .bcdUSB = 0x0200,
                                        .bDeviceClass = TUSB_CLASS_MISC,
                                        .bDeviceSubClass = MISC_SUBCLASS_COMMON,
                                        .bDeviceProtocol = MISC_PROTOCOL_IAD,
                                        .bMaxPacketSize0 =
                                            CFG_TUD_ENDPOINT0_SIZE,

                                        .idVendor = USB_VID,
                                        .idProduct = USB_PID,
                                        .bcdDevice = 0x0101,
                                        .iManufacturer = 0x01,
                                        .iProduct = 0x02,
                                        .iSerialNumber = 0x03,

                                        .bNumConfigurations = 0x01};

// invoked when received GET DEVICE DESCRIPTOR
uint8_t const *tud_descriptor_device_cb(void) {
  return (uint8_t const *)&desc_device;
}

//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+
uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1)), // klawiatura
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(2))     // myszka
};

// invoked when received GET HID REPORT DESCRIPTOR
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
  (void)instance;
  return desc_hid_report;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+
enum {
  ITF_NUM_CDC = 0,
  ITF_NUM_CDC_DATA,
  ITF_NUM_HID,
  ITF_NUM_MIDI,
  ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN                                                            \
  (TUD_CONFIG_DESC_LEN + TUD_ASSOC_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN + \
   TUD_MIDI_DESC_LEN)

#define EPNUM_CDC_NOTIF 0x81
#define EPNUM_CDC_OUT 0x02
#define EPNUM_CDC_IN 0x82
#define EPNUM_HID 0x83
#define EPNUM_MIDI_OUT 0x03
#define EPNUM_MIDI_IN 0x84

uint8_t const desc_configuration[] = {
    // config number, interface count, string index, total length, attribute,
    // power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    // it connects CDC interfaces into one group for Windows
    TUD_ASSOCIATION_DESCRIPTOR(ITF_NUM_CDC, 2, TUSB_CLASS_CDC, CDC_COMM_SUBCLASS_ABSTRACT_CONTROL_MODEL, CDC_COMM_PROTOCOL_ATCOMMAND, 0),

    // interface number, string index, EP notification address and size, EP data
    // address (out, in) and size
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT,
                       EPNUM_CDC_IN, 64),

    // interface number, string index, protocol, report descriptor len, EP In
    // address, size & polling interval
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 5, HID_ITF_PROTOCOL_NONE,
                       sizeof(desc_hid_report), EPNUM_HID,
                       CFG_TUD_HID_EP_BUFSIZE, 10),

    // interface number, string index, EP Out & In address, size
    TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 6, EPNUM_MIDI_OUT, EPNUM_MIDI_IN, 64)};

// invoked when received GET CONFIGURATION DESCRIPTOR
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;
  return desc_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+
char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04}, // 0: English (0x0409)
    "Szymon Wilczek",           // 1: Manufacturer
    "Talos 7",                  // 2: Product
    "123456",                   // 3: Serial
    "Talos Config Interface",   // 4: CDC Interface
    "Talos 7 HID",              // 5: HID Interface
    "Talos 7 MIDI"              // 6: MIDI Interface
};

static uint16_t _desc_str[32];

// invoked when received GET STRING DESCRIPTOR request
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;

  uint8_t chr_count;

  // --- Windows (OS Fingerprinting) ---
  // zawsze pyta o deskryptor 0xEE (Microsoft OS String)
  if (index == 0xEE) {
    g_detected_platform = 1;
    return NULL;
  }
  // ------------------------------------------------------------

  if (index == 0) {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  } else {
    if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
      return NULL;

    const char *str;
    char serial_buf[33]; // bufor na unique id (max 16 bajtow hex + null)

    // obsluga unique id dla numeru seryjnego (index 3)
    if (index == 3) {
      pico_get_unique_board_id_string(serial_buf, sizeof(serial_buf));
      str = serial_buf;
    } else {
      str = string_desc_arr[index];
    }

    // cap at max char
    chr_count = strlen(str);
    if (chr_count > 31)
      chr_count = 31;

    // convert ASCII string into UTF-16
    for (uint8_t i = 0; i < chr_count; i++) {
      _desc_str[1 + i] = str[i];
    }
  }

  // first byte is length (including header), second byte is string type
  _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

  return _desc_str;
}

//--------------------------------------------------------------------+
// USB HID Callbacks
//--------------------------------------------------------------------+

// invoked when received GET_REPORT control request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

// invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)bufsize;
}
