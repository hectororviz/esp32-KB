#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

typedef enum {
    ENC_NONE = 0,
    ENC_CW,
    ENC_CCW,
} encoder_dir_t;

typedef void (*encoder_cb_t)(encoder_dir_t dir, void *arg);

void encoder_init(void);
void encoder_set_callback(encoder_cb_t cb, void *arg);
encoder_dir_t encoder_get_direction(void);
bool encoder_poll_switch(void);
