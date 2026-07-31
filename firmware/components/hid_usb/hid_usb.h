#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

#define USB_HID_MAX_KEYS 6

esp_err_t hid_usb_init(void);
bool hid_usb_is_mounted(void);
esp_err_t hid_usb_keyboard_send(uint8_t modifier, const uint8_t keycodes[USB_HID_MAX_KEYS]);
esp_err_t hid_usb_consumer_send(uint16_t usage);
