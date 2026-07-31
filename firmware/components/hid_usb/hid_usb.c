#include <string.h>

#include "esp_log.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"

#include "hid_usb.h"

static const char *TAG = "hid_usb";

#define REPORT_ID_KEYBOARD 1
#define REPORT_ID_CONSUMER 2

#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

static const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_KEYBOARD)),
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(REPORT_ID_CONSUMER)),
};

static const char *hid_string_descriptor[5] = {
    (char[]){0x09, 0x04},
    "Horviz",
    "Numpad-20",
    "000000000001",
    "Numpad-20 HID",
};

static const uint8_t hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_descriptor), 0x81, 16, 10),
};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}

esp_err_t hid_usb_init(void)
{
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = hid_string_descriptor,
        .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
        .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
        .fs_configuration_descriptor = hid_configuration_descriptor,
        .hs_configuration_descriptor = hid_configuration_descriptor,
        .qualifier_descriptor = NULL,
#else
        .configuration_descriptor = hid_configuration_descriptor,
#endif
    };
    esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "tinyusb_driver_install failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

bool hid_usb_is_mounted(void)
{
    return tud_mounted();
}

esp_err_t hid_usb_keyboard_send(uint8_t modifier, const uint8_t keycodes[USB_HID_MAX_KEYS])
{
    if (!tud_mounted()) {
        return ESP_OK;
    }
    if (!tud_hid_ready()) {
        return ESP_ERR_INVALID_STATE;
    }
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, keycodes);
    return ESP_OK;
}

esp_err_t hid_usb_consumer_send(uint16_t usage)
{
    if (!tud_mounted()) {
        return ESP_OK;
    }
    if (!tud_hid_ready()) {
        return ESP_ERR_INVALID_STATE;
    }
    uint16_t report = usage;
    tud_hid_report(REPORT_ID_CONSUMER, &report, sizeof(report));
    return ESP_OK;
}
