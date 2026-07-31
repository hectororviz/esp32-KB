#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

#define HID_REPORT_MAX_KEYS 6

typedef struct {
    uint8_t modifier;
    uint8_t keycodes[HID_REPORT_MAX_KEYS];
} hid_report_t;

esp_err_t hid_route_init(void);
void hid_route_update(const hid_report_t *kbd);
void hid_route_consumer_event(uint16_t consumer);
void hid_route_type_string(const char *s);
