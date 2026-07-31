#pragma once

#include <stdint.h>
#include <stdbool.h>

#define DEBOUNCE_SAMPLES 3

void matrix_init(void);
bool matrix_scan(void);
uint32_t matrix_get_state(void);
