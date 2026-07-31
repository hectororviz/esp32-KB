#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    APP_KEYBOARD = 0,
    APP_MENU,
    APP_CALC,
} app_mode_t;

void apps_init(void);
void apps_on_key(uint16_t keycode);
void apps_toggle_calc(void);
bool apps_calc_active(void);
app_mode_t apps_mode(void);
const char *apps_calc_result_string(void);

bool apps_encoder_turn(bool cw);
void apps_encoder_press(void);

void apps_set_battery_pct(int pct);
void apps_set_charging(bool charging);
void apps_set_ble_connected(bool connected);
void apps_set_usb_mounted(bool mounted);

void apps_render(void);
