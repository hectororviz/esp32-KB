#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "keycodes.h"
#include "apps.h"

typedef enum {
    CALC_MODE_DEC = 0,
    CALC_MODE_HEX,
    CALC_MODE_BIN,
    CALC_MODE_MAX,
} calc_mode_t;

#define CALC_DISPLAY_LEN 32

typedef struct {
    int64_t acc;
    int64_t entry;
    char pending_op;
    bool fresh;
} calc_engine_t;

static bool s_calc_active;
static calc_mode_t s_mode;
static calc_engine_t s_calc;

static bool s_fn_active;
static int s_battery_pct;
static bool s_charging;
static bool s_ble_connected;
static bool s_usb_mounted;

static char s_status[CALC_DISPLAY_LEN];
static char s_calc_line[CALC_DISPLAY_LEN];

static void calc_format(int64_t value, char *out, size_t len, calc_mode_t mode)
{
    switch (mode) {
    case CALC_MODE_HEX:
        snprintf(out, len, "%llX", (unsigned long long)value);
        break;
    case CALC_MODE_BIN: {
        char tmp[CALC_DISPLAY_LEN];
        int64_t v = value;
        int i = 0;
        do {
            tmp[i++] = (v & 1) ? '1' : '0';
            v >>= 1;
        } while (v != 0 && i < (int)sizeof(tmp) - 1);
        tmp[i] = '\0';
        for (int j = 0; j < i; j++) {
            out[j] = tmp[i - 1 - j];
        }
        out[i] = '\0';
        break;
    }
    case CALC_MODE_DEC:
    default:
        snprintf(out, len, "%lld", (long long)value);
        break;
    }
}

static void calc_evaluate(void)
{
    if (s_calc.pending_op == '\0' || s_calc.fresh) {
        s_calc.acc = s_calc.entry;
        return;
    }
    switch (s_calc.pending_op) {
    case '+':
        s_calc.acc += s_calc.entry;
        break;
    case '-':
        s_calc.acc -= s_calc.entry;
        break;
    case '*':
        s_calc.acc *= s_calc.entry;
        break;
    case '/':
        if (s_calc.entry != 0) {
            s_calc.acc /= s_calc.entry;
        }
        break;
    default:
        break;
    }
}

static void calc_press(uint16_t kc)
{
    if (kc == KC_HID(HID_ESC)) {
        s_calc.acc = 0;
        s_calc.entry = 0;
        s_calc.pending_op = '\0';
        s_calc.fresh = true;
        return;
    }

    if (kc == KC_HID(HID_KP_EQUAL) || kc == KC_HID(HID_KP_ENTER)) {
        calc_evaluate();
        s_calc.entry = s_calc.acc;
        s_calc.pending_op = '\0';
        s_calc.fresh = true;
        return;
    }

    if (kc == KC_HID(HID_KP_0)) {
        if (s_calc.fresh) {
            s_calc.entry = 0;
            s_calc.fresh = false;
        } else {
            s_calc.entry = s_calc.entry * 10;
        }
        return;
    }
    if (kc >= KC_HID(HID_KP_1) && kc <= KC_HID(HID_KP_9)) {
        int digit = (int)(kc - KC_HID(HID_KP_1)) + 1;
        if (s_calc.fresh) {
            s_calc.entry = digit;
            s_calc.fresh = false;
        } else {
            s_calc.entry = s_calc.entry * 10 + digit;
        }
        return;
    }

    switch (kc) {
    case KC_HID(HID_KP_ADD):
        s_calc.pending_op = '+';
        break;
    case KC_HID(HID_KP_SUBTRACT):
        s_calc.pending_op = '-';
        break;
    case KC_HID(HID_KP_MULTIPLY):
        s_calc.pending_op = '*';
        break;
    case KC_HID(HID_KP_DIVIDE):
        s_calc.pending_op = '/';
        break;
    default:
        return;
    }
    if (!s_calc.fresh) {
        calc_evaluate();
        s_calc.entry = s_calc.acc;
    }
    s_calc.fresh = true;
}

static void refresh_status(void)
{
    int n = 0;
    n += snprintf(&s_status[n], sizeof(s_status) - (size_t)n, "BLE:%s USB:%s",
                  s_ble_connected ? "Y" : "-", s_usb_mounted ? "Y" : "-");
    if (s_calc_active) {
        const char *mode = s_mode == CALC_MODE_HEX ? "HEX" : s_mode == CALC_MODE_BIN ? "BIN" : "DEC";
        n += snprintf(&s_status[n], sizeof(s_status) - (size_t)n, " %s", mode);
    }
    n += snprintf(&s_status[n], sizeof(s_status) - (size_t)n, " %c%d%%",
                  s_charging ? '+' : ' ', s_battery_pct);
}

void apps_init(void)
{
    s_calc_active = false;
    s_mode = CALC_MODE_DEC;
    s_calc.acc = 0;
    s_calc.entry = 0;
    s_calc.pending_op = '\0';
    s_calc.fresh = true;
    s_battery_pct = 100;
    strcpy(s_calc_line, "0");
    refresh_status();
}

void apps_on_key(uint16_t kc)
{
    if (s_calc_active) {
        calc_press(kc);
    }
}

void apps_toggle_calc(void)
{
    s_calc_active = !s_calc_active;
    if (s_calc_active) {
        s_calc.acc = 0;
        s_calc.entry = 0;
        s_calc.pending_op = '\0';
        s_calc.fresh = true;
    }
    refresh_status();
}

void apps_calc_mode_cycle(void)
{
    if (!s_calc_active) {
        return;
    }
    s_mode = (calc_mode_t)((s_mode + 1) % CALC_MODE_MAX);
    refresh_status();
}

bool apps_calc_active(void)
{
    return s_calc_active;
}

void apps_set_fn(bool active)
{
    s_fn_active = active;
    refresh_status();
}

void apps_set_battery_pct(int pct)
{
    if (pct < 0) {
        pct = 0;
    }
    if (pct > 100) {
        pct = 100;
    }
    s_battery_pct = pct;
    refresh_status();
}

void apps_set_charging(bool charging)
{
    s_charging = charging;
    refresh_status();
}

void apps_set_ble_connected(bool connected)
{
    s_ble_connected = connected;
    refresh_status();
}

void apps_set_usb_mounted(bool mounted)
{
    s_usb_mounted = mounted;
    refresh_status();
}

const char *apps_status_line(void)
{
    return s_status;
}

const char *apps_calc_line(void)
{
    if (s_calc_active) {
        calc_format(s_calc.fresh ? s_calc.acc : s_calc.entry, s_calc_line, sizeof(s_calc_line), s_mode);
    } else {
        snprintf(s_calc_line, sizeof(s_calc_line), "%s%s",
                 s_fn_active ? "FN " : "", "Numpad-20");
    }
    return s_calc_line;
}
