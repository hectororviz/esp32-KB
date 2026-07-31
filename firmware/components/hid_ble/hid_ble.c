#include <string.h>

#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gatts_api.h"
#include "esp_hid_common.h"
#include "esp_hidd.h"
#include "esp_hidd_gatts.h"

#include "hid_ble.h"

static const char *TAG = "hid_ble";

#define BLE_DEVICE_NAME       "Numpad-20"
#define BLE_MANUFACTURER_NAME "Horviz"
#define BLE_SERIAL_NUMBER     "000000000001"

#define RPT_ID_KEYBOARD 1
#define RPT_ID_CONSUMER 2

static const uint8_t rpt_map_keyboard[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x06,       // Usage (Keyboard)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x01,       //   Report ID (1)
    0x05, 0x07,       //   Usage Page (Key Codes)
    0x19, 0xE0,       //   Usage Minimum (224)
    0x29, 0xE7,       //   Usage Maximum (231)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x08,       //   Report Count (8)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x95, 0x01,       //   Report Count (1)
    0x75, 0x08,       //   Report Size (8)
    0x81, 0x01,       //   Input (Const,Array,Abs)
    0x95, 0x06,       //   Report Count (6)
    0x75, 0x08,       //   Report Size (8)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x65,       //   Logical Maximum (101)
    0x05, 0x07,       //   Usage Page (Key Codes)
    0x19, 0x00,       //   Usage Minimum (0)
    0x29, 0x65,       //   Usage Maximum (101)
    0x81, 0x00,       //   Input (Data,Array,Abs)
    0xC0,             // End Collection
};

static const uint8_t rpt_map_consumer[] = {
    0x05, 0x0C,       // Usage Page (Consumer)
    0x09, 0x01,       // Usage (Consumer Control)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x02,       //   Report ID (2)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0xFF,       //   Logical Maximum (255)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x02,       //   Report Count (2)
    0x0A, 0xFF, 0xFF, //   Usage (0xFFFF)
    0x81, 0x00,       //   Input (Data,Array,Abs)
    0xC0,             // End Collection
};

static esp_hid_raw_report_map_t s_ble_report_maps[] = {
    { .data = rpt_map_keyboard, .len = sizeof(rpt_map_keyboard) },
    { .data = rpt_map_consumer, .len = sizeof(rpt_map_consumer) },
};

static const esp_hid_device_config_t s_ble_hid_config = {
    .vendor_id         = 0x1234,
    .product_id        = 0x5678,
    .version           = 0x0100,
    .device_name       = BLE_DEVICE_NAME,
    .manufacturer_name = BLE_MANUFACTURER_NAME,
    .serial_number     = BLE_SERIAL_NUMBER,
    .report_maps       = s_ble_report_maps,
    .report_maps_len   = sizeof(s_ble_report_maps) / sizeof(s_ble_report_maps[0]),
};

static esp_hidd_dev_t *s_hid_dev;
static volatile bool s_connected;

esp_err_t esp_hid_ble_gap_init(void);
esp_err_t esp_hid_ble_gap_adv_init(uint16_t appearance, const char *device_name);
esp_err_t esp_hid_ble_gap_adv_start(void);

static void ble_hidd_event_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    (void)handler_args;
    (void)base;
    esp_hidd_event_t event = (esp_hidd_event_t)id;
    esp_hidd_event_data_t *param = (esp_hidd_event_data_t *)event_data;

    switch (event) {
    case ESP_HIDD_START_EVENT:
        ESP_LOGI(TAG, "BLE HID started, advertising");
        esp_hid_ble_gap_adv_start();
        break;
    case ESP_HIDD_CONNECT_EVENT:
        s_connected = true;
        ESP_LOGI(TAG, "BLE HID connected");
        break;
    case ESP_HIDD_DISCONNECT_EVENT:
        s_connected = false;
        ESP_LOGI(TAG, "BLE HID disconnected, advertising again");
        esp_hid_ble_gap_adv_start();
        break;
    case ESP_HIDD_STOP_EVENT:
        ESP_LOGI(TAG, "BLE HID stopped");
        break;
    default:
        break;
    }
}

esp_err_t hid_ble_init(void)
{
    esp_err_t ret = esp_hid_ble_gap_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GAP init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_hid_ble_gap_adv_init(ESP_HID_APPEARANCE_KEYBOARD, BLE_DEVICE_NAME);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GAP adv init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_ble_gatts_register_callback(esp_hidd_gatts_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GATTS register callback failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_hidd_dev_init(&s_ble_hid_config, ESP_HID_TRANSPORT_BLE, ble_hidd_event_callback, &s_hid_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "HID dev init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    return ESP_OK;
}

bool hid_ble_is_connected(void)
{
    return s_connected;
}

esp_err_t hid_ble_keyboard_send(uint8_t modifier, const uint8_t keycodes[BLE_HID_MAX_KEYS])
{
    if (!s_connected) {
        return ESP_OK;
    }
    uint8_t report[8] = { 0 };
    report[0] = modifier;
    report[1] = 0;
    memcpy(&report[2], keycodes, BLE_HID_MAX_KEYS);
    return esp_hidd_dev_input_set(s_hid_dev, 0, RPT_ID_KEYBOARD, report, sizeof(report));
}

esp_err_t hid_ble_consumer_send(uint16_t usage)
{
    if (!s_connected) {
        return ESP_OK;
    }
    uint8_t report[2] = { (uint8_t)(usage & 0xFF), (uint8_t)(usage >> 8) };
    return esp_hidd_dev_input_set(s_hid_dev, 1, RPT_ID_CONSUMER, report, sizeof(report));
}
