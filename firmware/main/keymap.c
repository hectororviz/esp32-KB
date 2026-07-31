#include <string.h>

#include "keymap.h"

static const uint16_t s_keymap[2][MATRIX_KEYS] = {
    {
        KC_HID(HID_KP_NUMLOCK), KC_HID(HID_KP_DIVIDE),   KC_HID(HID_KP_MULTIPLY), KC_HID(HID_KP_SUBTRACT),
        KC_HID(HID_KP_7),       KC_HID(HID_KP_8),        KC_HID(HID_KP_9),        KC_HID(HID_KP_ADD),
        KC_HID(HID_KP_4),       KC_HID(HID_KP_5),        KC_HID(HID_KP_6),        KC_HID(HID_KP_EQUAL),
        KC_HID(HID_KP_1),       KC_HID(HID_KP_2),        KC_HID(HID_KP_3),        KC_HID(HID_KP_ENTER),
        KC_HID(HID_KP_0),       KC_HID(HID_KP_DOT),      KC_FN,                   KC_HID(HID_ESC),
    },
    {
        KC_CC(CC_AC_HOME),      KC_CC(CC_SCAN_PREVIOUS), KC_CC(CC_SCAN_NEXT),     KC_CC(CC_MUTE),
        KC_CC(CC_VOLUME_UP),    KC_CC(CC_PLAY_PAUSE),    KC_CC(CC_STOP),          KC_CC(CC_VOLUME_DOWN),
        KC_HID(HID_ARROW_LEFT), KC_HID(HID_ARROW_DOWN),  KC_HID(HID_ARROW_UP),    KC_HID(HID_ARROW_RIGHT),
        KC_HID(HID_HOME),       KC_HID(HID_END),         KC_HID(HID_PAGE_UP),     KC_HID(HID_PAGE_DOWN),
        KC_HID(HID_TAB),        KC_HID(HID_KP_DOT),      KC_NO,                   KC_CALC,
    },
};

void keymap_init(void)
{
}

uint16_t keymap_resolve(uint8_t key_idx, bool fn_active)
{
    return s_keymap[fn_active ? 1 : 0][key_idx];
}

void keymap_build_report(uint32_t pressed, bool fn_active, hid_report_t *kbd, uint16_t *consumer)
{
    kbd->modifier = 0;
    memset(kbd->keycodes, 0, sizeof(kbd->keycodes));
    *consumer = 0;
    uint8_t count = 0;
    for (int i = 0; i < MATRIX_KEYS; i++) {
        if ((pressed & (1u << i)) == 0) {
            continue;
        }
        uint16_t kc = keymap_resolve(i, fn_active);
        if (kc == KC_FN || kc == KC_CALC) {
            continue;
        }
        if (kc >= KC_HID_BASE && kc < KC_CC_BASE) {
            uint8_t usage = (uint8_t)(kc & 0xFF);
            if (usage >= HID_LEFT_CTRL && usage <= HID_RIGHT_GUI) {
                kbd->modifier |= (1u << (usage - HID_LEFT_CTRL));
            } else if (count < 6) {
                kbd->keycodes[count++] = usage;
            }
        } else if (kc >= KC_CC_BASE && *consumer == 0) {
            *consumer = (uint16_t)(kc - KC_CC_BASE);
        }
    }
}
