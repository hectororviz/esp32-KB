#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "esp_timer.h"
#include "esp_mac.h"
#include "driver/temperature_sensor.h"

#include "keycodes.h"
#include "apps.h"
#include "settings.h"
#include "display.h"
#include "power.h"

#define APP_VERSION "0.2.0"

#define CALC_DISPLAY_LEN 32
#define ROW_PITCH 8

typedef enum {
    CALC_MODE_DEC = 0,
    CALC_MODE_HEX,
    CALC_MODE_BIN,
    CALC_MODE_MAX,
} calc_mode_t;

typedef struct {
    int64_t acc;
    int64_t entry;
    char pending_op;
    bool fresh;
} calc_engine_t;

typedef enum {
    MENU_ROOT = 0,
    MENU_DISPLAY,
    MENU_ENCODER,
    MENU_SLEEP,
    MENU_INFO,
} menu_page_t;

typedef enum {
    EDIT_NONE = 0,
    EDIT_CONTRAST,
    EDIT_ENCODER,
    EDIT_SLEEP,
} edit_kind_t;

static app_mode_t s_mode = APP_KEYBOARD;
static calc_mode_t s_calc_mode = CALC_MODE_DEC;
static calc_engine_t s_calc;

static int s_battery_pct;
static bool s_charging;
static bool s_ble_connected;
static bool s_usb_mounted;

static char s_status[CALC_DISPLAY_LEN];
static char s_calc_result[CALC_DISPLAY_LEN];

static menu_page_t s_menu_page = MENU_ROOT;
static int s_menu_cursor;
static edit_kind_t s_edit = EDIT_NONE;

static temperature_sensor_handle_t s_tsens;

static void refresh_status(void)
{
    int n = 0;
    n += snprintf(&s_status[n], sizeof(s_status) - (size_t)n, "BLE:%s USB:%s",
                  s_ble_connected ? "Y" : "-", s_usb_mounted ? "Y" : "-");
    n += snprintf(&s_status[n], sizeof(s_status) - (size_t)n, " %c%d%%",
                  s_charging ? '+' : ' ', s_battery_pct);
}

/* ---------- calculadora ---------- */

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
    if (kc == KC_HID(HID_KP_EQUAL) || kc == KC_HID(HID_KP_ENTER)) {
        calc_evaluate();
        s_calc.entry = s_calc.acc;
        s_calc.pending_op = '\0';
        s_calc.fresh = true;
        return;
    }

    if (kc == KC_HID(HID_BACKSPACE)) {
        if (!s_calc.fresh) {
            s_calc.entry /= 10;
        }
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

/* ---------- menú ---------- */

static int menu_items(menu_page_t page)
{
    switch (page) {
    case MENU_ROOT:
        return 5;
    case MENU_DISPLAY:
    case MENU_ENCODER:
    case MENU_SLEEP:
        return 2;
    default:
        return 0;
    }
}

static void menu_open(void)
{
    s_mode = APP_MENU;
    s_menu_page = MENU_ROOT;
    s_menu_cursor = 0;
    s_edit = EDIT_NONE;
}

static void menu_adjust(int delta)
{
    switch (s_edit) {
    case EDIT_CONTRAST: {
        int v = (int)g_settings.contrast + delta;
        if (v < 0) {
            v = 0;
        }
        if (v > 255) {
            v = 255;
        }
        g_settings.contrast = (uint8_t)v;
        display_set_brightness(g_settings.contrast);
        break;
    }
    case EDIT_ENCODER:
        g_settings.invert_encoder = !g_settings.invert_encoder;
        break;
    case EDIT_SLEEP: {
        int v = ((int)g_settings.sleep_timeout + delta + 4) % 4;
        g_settings.sleep_timeout = (uint8_t)v;
        break;
    }
    default:
        break;
    }
}

static void menu_confirm(void)
{
    if (s_edit != EDIT_NONE) {
        settings_save();
        s_edit = EDIT_NONE;
        return;
    }
    switch (s_menu_page) {
    case MENU_ROOT:
        switch (s_menu_cursor) {
        case 0:
            s_menu_page = MENU_INFO;
            break;
        case 1:
            s_menu_page = MENU_DISPLAY;
            s_menu_cursor = 0;
            break;
        case 2:
            s_menu_page = MENU_ENCODER;
            s_menu_cursor = 0;
            break;
        case 3:
            s_menu_page = MENU_SLEEP;
            s_menu_cursor = 0;
            break;
        default:
            s_mode = APP_KEYBOARD;
            break;
        }
        break;
    case MENU_DISPLAY:
        if (s_menu_cursor == 0) {
            s_edit = EDIT_CONTRAST;
        } else {
            s_menu_page = MENU_ROOT;
            s_menu_cursor = 1;
        }
        break;
    case MENU_ENCODER:
        if (s_menu_cursor == 0) {
            s_edit = EDIT_ENCODER;
        } else {
            s_menu_page = MENU_ROOT;
            s_menu_cursor = 2;
        }
        break;
    case MENU_SLEEP:
        if (s_menu_cursor == 0) {
            s_edit = EDIT_SLEEP;
        } else {
            s_menu_page = MENU_ROOT;
            s_menu_cursor = 3;
        }
        break;
    case MENU_INFO:
        s_menu_page = MENU_ROOT;
        s_menu_cursor = 0;
        break;
    }
}

static void render_menu(void)
{
    char line[CALC_DISPLAY_LEN];

    if (s_edit != EDIT_NONE) {
        switch (s_edit) {
        case EDIT_CONTRAST:
            snprintf(line, sizeof(line), "Contraste: %u", (unsigned)g_settings.contrast);
            display_text(0, 0, line);
            break;
        case EDIT_ENCODER:
            snprintf(line, sizeof(line), "Invertir: %s", g_settings.invert_encoder ? "Si" : "No");
            display_text(0, 0, line);
            break;
        case EDIT_SLEEP: {
            static const char *const names[] = { "Apagado", "30s", "5min", "10min" };
            snprintf(line, sizeof(line), "Timeout: %s",
                     names[g_settings.sleep_timeout % 4]);
            display_text(0, 0, line);
            break;
        }
        default:
            break;
        }
        display_text(0, 16, "Girar=ajustar");
        display_text(0, 24, "SW=guardar");
        return;
    }

    static const char *const root_items[] = { "Info", "Pantalla", "Encoder", "Sleep", "Salir" };
    static const char *sub_items[] = { NULL, "Volver" };
    const char *const *items = NULL;
    int n = 0;

    switch (s_menu_page) {
    case MENU_ROOT:
        items = root_items;
        n = 5;
        break;
    case MENU_DISPLAY:
        sub_items[0] = "Contraste";
        items = sub_items;
        n = 2;
        break;
    case MENU_ENCODER:
        sub_items[0] = "Invertir";
        items = sub_items;
        n = 2;
        break;
    case MENU_SLEEP:
        sub_items[0] = "Timeout";
        items = sub_items;
        n = 2;
        break;
    case MENU_INFO:
        break;
    }

    if (s_menu_page == MENU_INFO) {
        int mv = power_battery_millivolts();
        int pct = power_battery_percent();
        display_text(0, 0, "Info");
        snprintf(line, sizeof(line), "Bat: %d.%dV %d%%", mv / 1000, (mv / 100) % 10, pct);
        display_text(0, 8, line);
        const char *chg = power_is_charging() ? "Cargando"
                        : power_is_charging_done() ? "Llena"
                        : power_is_usb_powered() ? "USB"
                                                 : "Bateria";
        snprintf(line, sizeof(line), "Carga: %s", chg);
        display_text(0, 16, line);
        if (s_tsens) {
            float c = 0;
            if (temperature_sensor_get_celsius(s_tsens, &c) == ESP_OK) {
                snprintf(line, sizeof(line), "Temp: %.0fC", c);
            } else {
                snprintf(line, sizeof(line), "Temp: --");
            }
        } else {
            snprintf(line, sizeof(line), "Temp: --");
        }
        display_text(0, 24, line);
        uint8_t mac[6] = { 0 };
        if (esp_read_mac(mac, ESP_MAC_BT) == ESP_OK) {
            snprintf(line, sizeof(line), "BLE:%02X%02X%02X%02X%02X%02X",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            display_text(0, 32, line);
        }
        uint64_t secs = esp_timer_get_time() / 1000000ULL;
        snprintf(line, sizeof(line), "v%s %02u:%02u", APP_VERSION,
                 (unsigned)((secs / 3600) % 100), (unsigned)((secs / 60) % 60));
        display_text(0, 40, line);
        display_text(0, 56, "SW: volver");
        return;
    }

    for (int i = 0; i < n; i++) {
        if (i == s_menu_cursor) {
            snprintf(line, sizeof(line), "> %s", items[i]);
        } else {
            snprintf(line, sizeof(line), "  %s", items[i]);
        }
        display_text(0, i * ROW_PITCH, line);
    }
}

/* ---------- render ---------- */

static void render_hud(void)
{
    char line[CALC_DISPLAY_LEN];
    snprintf(line, sizeof(line), "%s", s_status);
    if (s_battery_pct < 20) {
        snprintf(line + strlen(line), sizeof(line) - strlen(line), " BAT!");
    }
    if (s_battery_pct < 10 && ((esp_timer_get_time() / 500000ULL) & 1ULL)) {
        line[0] = '\0';
    }
    display_text(0, 0, line);
    display_text(0, 16, "Numpad-20");
}

static void render_calc(void)
{
    char line[CALC_DISPLAY_LEN];
    const char *mode = s_calc_mode == CALC_MODE_HEX ? "HEX"
                     : s_calc_mode == CALC_MODE_BIN ? "BIN"
                                                    : "DEC";
    snprintf(line, sizeof(line), "Calc %s", mode);
    display_text(0, 0, line);
    calc_format(s_calc.fresh ? s_calc.acc : s_calc.entry, line, sizeof(line), s_calc_mode);
    display_text(0, 16, line);
    display_text(0, 56, "SW=base N=pegar");
}

void apps_render(void)
{
    switch (s_mode) {
    case APP_CALC:
        render_calc();
        break;
    case APP_MENU:
        render_menu();
        break;
    default:
        render_hud();
        break;
    }
}

/* ---------- API ---------- */

void apps_init(void)
{
    s_mode = APP_KEYBOARD;
    s_calc_mode = CALC_MODE_DEC;
    s_calc.acc = 0;
    s_calc.entry = 0;
    s_calc.pending_op = '\0';
    s_calc.fresh = true;
    s_battery_pct = 100;
    s_menu_page = MENU_ROOT;
    s_menu_cursor = 0;
    s_edit = EDIT_NONE;
    refresh_status();

    temperature_sensor_config_t tsens_cfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 85);
    if (temperature_sensor_install(&tsens_cfg, &s_tsens) != ESP_OK) {
        s_tsens = NULL;
    } else if (temperature_sensor_enable(s_tsens) != ESP_OK) {
        temperature_sensor_uninstall(s_tsens);
        s_tsens = NULL;
    }
}

void apps_on_key(uint16_t keycode)
{
    if (s_mode == APP_CALC) {
        calc_press(keycode);
    }
}

void apps_toggle_calc(void)
{
    if (s_mode == APP_CALC) {
        s_mode = APP_KEYBOARD;
        return;
    }
    s_mode = APP_CALC;
    s_calc.acc = 0;
    s_calc.entry = 0;
    s_calc.pending_op = '\0';
    s_calc.fresh = true;
}

bool apps_calc_active(void)
{
    return s_mode == APP_CALC;
}

app_mode_t apps_mode(void)
{
    return s_mode;
}

const char *apps_calc_result_string(void)
{
    calc_format(s_calc.fresh ? s_calc.acc : s_calc.entry, s_calc_result,
                sizeof(s_calc_result), s_calc_mode);
    return s_calc_result;
}

bool apps_encoder_turn(bool cw)
{
    if (s_mode != APP_MENU) {
        return false;
    }
    int delta = cw ? 1 : -1;
    if (s_edit != EDIT_NONE) {
        menu_adjust(delta);
        return true;
    }
    int max = menu_items(s_menu_page);
    if (max > 1) {
        s_menu_cursor = (s_menu_cursor + delta + max) % max;
    }
    return true;
}

void apps_encoder_press(void)
{
    if (s_mode == APP_MENU) {
        menu_confirm();
    } else if (s_mode == APP_CALC) {
        s_calc_mode = (calc_mode_t)((s_calc_mode + 1) % CALC_MODE_MAX);
    } else {
        menu_open();
    }
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
