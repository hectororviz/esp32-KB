#pragma once

#include <stdint.h>

#define KC_NO        0x0000u
#define KC_CALC      0x0001u
#define KC_HID_BASE  0x0100u

#define KC_HID(k)    (KC_HID_BASE | (uint16_t)(k))

#define HID_ESC            0x29
#define HID_ENTER          0x28
#define HID_TAB            0x2B
#define HID_BACKSPACE      0x2A
#define HID_HOME           0x4A
#define HID_END            0x4D
#define HID_PAGE_UP        0x4B
#define HID_PAGE_DOWN      0x4E
#define HID_ARROW_LEFT     0x50
#define HID_ARROW_UP       0x52
#define HID_ARROW_RIGHT    0x4F
#define HID_ARROW_DOWN     0x51

#define HID_A              0x04
#define HID_B              0x05
#define HID_C              0x06
#define HID_D              0x07
#define HID_E              0x08
#define HID_F              0x09

#define HID_MINUS          0x2D

#define HID_1              0x1E
#define HID_2              0x1F
#define HID_3              0x20
#define HID_4              0x21
#define HID_5              0x22
#define HID_6              0x23
#define HID_7              0x24
#define HID_8              0x25
#define HID_9              0x26
#define HID_0              0x27

#define HID_KP_NUMLOCK     0x53
#define HID_KP_DIVIDE      0x54
#define HID_KP_MULTIPLY    0x55
#define HID_KP_SUBTRACT    0x56
#define HID_KP_ADD         0x57
#define HID_KP_ENTER       0x58
#define HID_KP_1           0x59
#define HID_KP_2           0x5A
#define HID_KP_3           0x5B
#define HID_KP_4           0x5C
#define HID_KP_5           0x5D
#define HID_KP_6           0x5E
#define HID_KP_7           0x5F
#define HID_KP_8           0x60
#define HID_KP_9           0x61
#define HID_KP_0           0x62
#define HID_KP_DOT         0x63
#define HID_KP_EQUAL       0x67

#define HID_LEFT_CTRL      0xE0
#define HID_LEFT_SHIFT     0xE1
#define HID_LEFT_ALT       0xE2
#define HID_LEFT_GUI       0xE3
#define HID_RIGHT_CTRL     0xE4
#define HID_RIGHT_SHIFT    0xE5
#define HID_RIGHT_ALT      0xE6
#define HID_RIGHT_GUI      0xE7

#define CC_VOLUME_UP       0xE9
#define CC_VOLUME_DOWN     0xEA
