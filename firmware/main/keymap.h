#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "matrix.h"
#include "keycodes.h"
#include "hid_route.h"

void keymap_init(void);
uint16_t keymap_resolve(uint8_t key_idx);
void keymap_build_report(uint32_t pressed, hid_report_t *kbd);
