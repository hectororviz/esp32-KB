#pragma once

#include <stdint.h>
#include <stdbool.h>

#define MATRIX_ROWS 5
#define MATRIX_COLS 4
#define MATRIX_KEYS (MATRIX_ROWS * MATRIX_COLS)

#define DEBOUNCE_SAMPLES 3

void matrix_init(void);
bool matrix_scan(void);
uint32_t matrix_get_state(void);
void matrix_enter_sleep(void);
