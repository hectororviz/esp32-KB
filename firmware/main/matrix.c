#include <string.h>

#include "esp_bit_defs.h"
#include "esp_rom_sys.h"
#include "driver/gpio.h"

#include "board_config.h"
#include "matrix.h"

static const gpio_num_t s_row_pins[MATRIX_ROWS] = MATRIX_ROW_PINS;
static const gpio_num_t s_col_pins[MATRIX_COLS] = MATRIX_COL_PINS;

static uint32_t s_raw_state;
static uint32_t s_state;
static uint8_t s_count[MATRIX_KEYS];

static uint32_t matrix_scan_raw(void)
{
    uint32_t state = 0;
    for (int r = 0; r < MATRIX_ROWS; r++) {
        gpio_set_level(s_row_pins[r], 0);
        esp_rom_delay_us(5);
        for (int c = 0; c < MATRIX_COLS; c++) {
            if (gpio_get_level(s_col_pins[c]) == 0) {
                state |= (1u << (r * MATRIX_COLS + c));
            }
        }
        gpio_set_level(s_row_pins[r], 1);
    }
    return state;
}

void matrix_init(void)
{
    for (int r = 0; r < MATRIX_ROWS; r++) {
        gpio_config_t cfg = {
            .pin_bit_mask = BIT64(s_row_pins[r]),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = false,
            .pull_down_en = false,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&cfg);
        gpio_set_level(s_row_pins[r], 1);
    }
    for (int c = 0; c < MATRIX_COLS; c++) {
        gpio_config_t cfg = {
            .pin_bit_mask = BIT64(s_col_pins[c]),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = true,
            .pull_down_en = false,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&cfg);
    }
    memset(s_count, 0, sizeof(s_count));
}

bool matrix_scan(void)
{
    s_raw_state = matrix_scan_raw();
    bool changed = false;
    for (int i = 0; i < MATRIX_KEYS; i++) {
        uint32_t mask = (1u << i);
        bool raw = (s_raw_state & mask) != 0;
        bool deb = (s_state & mask) != 0;
        if (raw == deb) {
            s_count[i] = 0;
        } else if (++s_count[i] >= DEBOUNCE_SAMPLES) {
            if (raw) {
                s_state |= mask;
            } else {
                s_state &= ~mask;
            }
            changed = true;
            s_count[i] = 0;
        }
    }
    return changed;
}

uint32_t matrix_get_state(void)
{
    return s_state;
}
