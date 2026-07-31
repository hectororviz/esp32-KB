#pragma once

#include <stdint.h>

#include "esp_err.h"

#define DISPLAY_WIDTH  128
#define DISPLAY_HEIGHT 64

typedef struct {
    int sda;
    int scl;
    uint8_t addr;
} display_config_t;

esp_err_t display_init(const display_config_t *cfg);
void display_begin(void);
void display_text(int x, int y, const char *s);
void display_update(void);
void display_off(void);
void display_on(void);
void display_set_brightness(uint8_t level);
void display_test_pattern(void);
