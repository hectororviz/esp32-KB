#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "matrix.h"
#include "keycodes.h"
#include "hid_route.h"

void keymap_init(void);
uint16_t keymap_resolve(uint8_t key_idx, bool fn_active);
void keymap_build_report(uint32_t pressed, bool fn_active, hid_report_t *kbd, uint16_t *consumer);
