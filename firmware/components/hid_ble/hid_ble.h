#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

#define BLE_HID_MAX_KEYS 6

esp_err_t hid_ble_init(void);
bool hid_ble_is_connected(void);
esp_err_t hid_ble_keyboard_send(uint8_t modifier, const uint8_t keycodes[BLE_HID_MAX_KEYS]);
esp_err_t hid_ble_consumer_send(uint16_t usage);
