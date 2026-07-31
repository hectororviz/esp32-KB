#include <string.h>

#include "keymap.h"

static const uint16_t s_keymap[MATRIX_KEYS] = {
    KC_HID(HID_KP_NUMLOCK), KC_HID(HID_KP_DIVIDE),   KC_HID(HID_KP_MULTIPLY), KC_HID(HID_KP_SUBTRACT),
    KC_HID(HID_KP_7),       KC_HID(HID_KP_8),        KC_HID(HID_KP_9),        KC_HID(HID_KP_ADD),
    KC_HID(HID_KP_4),       KC_HID(HID_KP_5),        KC_HID(HID_KP_6),        KC_HID(HID_KP_EQUAL),
    KC_HID(HID_KP_1),       KC_HID(HID_KP_2),        KC_HID(HID_KP_3),        KC_HID(HID_KP_ENTER),
    KC_HID(HID_KP_0),       KC_HID(HID_KP_DOT),      KC_CALC,                 KC_HID(HID_BACKSPACE),
};

void keymap_init(void)
{
}

uint16_t keymap_resolve(uint8_t key_idx)
{
    if (key_idx >= MATRIX_KEYS) {
        return KC_NO;
    }
    return s_keymap[key_idx];
}

void keymap_build_report(uint32_t pressed, hid_report_t *kbd)
{
    kbd->modifier = 0;
    memset(kbd->keycodes, 0, sizeof(kbd->keycodes));
    uint8_t count = 0;
    for (int i = 0; i < MATRIX_KEYS; i++) {
        if ((pressed & (1u << i)) == 0) {
            continue;
        }
        uint16_t kc = keymap_resolve(i);
        if (kc == KC_CALC) {
            continue;
        }
        if (kc >= KC_HID_BASE) {
            uint8_t usage = (uint8_t)(kc & 0xFF);
            if (usage >= HID_LEFT_CTRL && usage <= HID_RIGHT_GUI) {
                kbd->modifier |= (1u << (usage - HID_LEFT_CTRL));
            } else if (count < HID_REPORT_MAX_KEYS) {
                kbd->keycodes[count++] = usage;
            }
        }
    }
}
