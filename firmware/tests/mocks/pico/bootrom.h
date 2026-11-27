#ifndef MOCK_PICO_BOOTROM_H
#define MOCK_PICO_BOOTROM_H

#include <stdint.h>

static inline void reset_usb_boot(uint32_t usb_activity_gpio_pin_mask,
				  uint32_t disable_interface_mask)
{
	(void)usb_activity_gpio_pin_mask;
	(void)disable_interface_mask;
}

#endif // MOCK_PICO_BOOTROM_H
