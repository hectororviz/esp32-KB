#pragma once

#include <stdbool.h>
#include <stdint.h>

#define SETTINGS_SLEEP_OFF  0
#define SETTINGS_SLEEP_30S  1
#define SETTINGS_SLEEP_5MIN 2
#define SETTINGS_SLEEP_10MIN 3

typedef struct {
    uint8_t contrast;
    bool invert_encoder;
    uint8_t sleep_timeout;
} settings_t;

extern settings_t g_settings;

void settings_load(void);
void settings_save(void);
