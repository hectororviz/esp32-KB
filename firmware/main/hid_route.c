#include <string.h>

#include "esp_log.h"

#include "hid_route.h"

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
