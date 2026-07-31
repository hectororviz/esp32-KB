#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "hid_route.h"
#include "keycodes.h"

#include "hid_usb.h"
#include "hid_ble.h"

static const char *TAG = "hid_route";

static hid_report_t s_last_kbd;

esp_err_t hid_route_init(void)
{
    memset(&s_last_kbd, 0, sizeof(s_last_kbd));
    return ESP_OK;
}

void hid_route_update(const hid_report_t *kbd)
{
    if (kbd && memcmp(kbd, &s_last_kbd, sizeof(hid_report_t)) != 0) {
        s_last_kbd = *kbd;
        hid_usb_keyboard_send(kbd->modifier, kbd->keycodes);
        hid_ble_keyboard_send(kbd->modifier, kbd->keycodes);
    }
}

void hid_route_consumer_event(uint16_t consumer)
{
    hid_usb_consumer_send(consumer);
    hid_ble_consumer_send(consumer);
    ESP_LOGI(TAG, "consumer usage 0x%04X", (unsigned)consumer);
}

void hid_route_type_string(const char *s)
{
    const uint8_t shift_bit = (uint8_t)(1u << (HID_LEFT_SHIFT - HID_LEFT_CTRL));
    for (; *s != '\0'; s++) {
        char c = *s;
        uint8_t mod = 0;
        uint8_t usage = 0;
        if (c >= '0' && c <= '9') {
            usage = (c == '0') ? HID_0 : (uint8_t)(HID_1 + (c - '1'));
        } else if (c >= 'A' && c <= 'F') {
            usage = (uint8_t)(HID_A + (c - 'A'));
            mod = shift_bit;
        } else if (c == '-') {
            usage = HID_MINUS;
        } else {
            continue;
        }
        uint8_t keys[HID_REPORT_MAX_KEYS] = { usage };
        hid_usb_keyboard_send(mod, keys);
        hid_ble_keyboard_send(mod, keys);
        vTaskDelay(pdMS_TO_TICKS(10));
        uint8_t empty[HID_REPORT_MAX_KEYS] = { 0 };
        hid_usb_keyboard_send(0, empty);
        hid_ble_keyboard_send(0, empty);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    memset(&s_last_kbd, 0, sizeof(s_last_kbd));
}
