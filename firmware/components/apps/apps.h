#pragma once

#include <stdint.h>
#include <stdbool.h>

void apps_init(void);
void apps_on_key(uint16_t keycode);
void apps_toggle_calc(void);
void apps_calc_mode_cycle(void);
bool apps_calc_active(void);

void apps_set_fn(bool active);
void apps_set_battery_pct(int pct);
void apps_set_charging(bool charging);
void apps_set_ble_connected(bool connected);
void apps_set_usb_mounted(bool mounted);

const char *apps_status_line(void);
const char *apps_calc_line(void);
