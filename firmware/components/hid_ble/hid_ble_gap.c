#include <string.h>

#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_hid_common.h"

static const char *TAG = "hid_ble_gap";

static void ble_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        ESP_LOGV(TAG, "ADV_DATA_SET_COMPLETE");
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        ESP_LOGV(TAG, "ADV_START_COMPLETE");
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        if (!param->ble_security.auth_cmpl.success) {
            ESP_LOGE(TAG, "AUTH ERROR: 0x%x", param->ble_security.auth_cmpl.fail_reason);
        } else {
            ESP_LOGI(TAG, "AUTH SUCCESS");
        }
        break;
    case ESP_GAP_BLE_KEY_EVT:
        ESP_LOGV(TAG, "KEY type = %s", esp_ble_key_type_str(param->ble_security.ble_key.key_type));
        break;
    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:
        ESP_LOGI(TAG, "PASSKEY_NOTIF passkey:%06d", param->ble_security.key_notif.passkey);
        break;
    case ESP_GAP_BLE_NC_REQ_EVT:
        ESP_LOGI(TAG, "NC_REQ passkey:%06d, confirming", param->ble_security.key_notif.passkey);
        esp_ble_confirm_reply(param->ble_security.key_notif.bd_addr, true);
        break;
    case ESP_GAP_BLE_PASSKEY_REQ_EVT:
        ESP_LOGI(TAG, "PASSKEY_REQ");
        esp_ble_passkey_reply(param->ble_security.ble_req.bd_addr, true, 0);
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        ESP_LOGI(TAG, "SEC_REQ");
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    default:
        ESP_LOGV(TAG, "BLE GAP EVENT %d", event);
        break;
    }
}

static esp_err_t init_ble_gap(void)
{
    esp_err_t ret = esp_ble_gap_register_callback(ble_gap_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gap_register_callback failed: %d", ret);
        return ret;
    }
    return ret;
}

static esp_err_t init_low_level(void)
{
    esp_err_t ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_controller_mem_release failed: %d", ret);
        return ret;
    }

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_controller_init failed: %d", ret);
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_controller_enable failed: %d", ret);
        return ret;
    }

    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_bluedroid_init failed: %d", ret);
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_bluedroid_enable failed: %d", ret);
        return ret;
    }

    return init_ble_gap();
}

esp_err_t esp_hid_ble_gap_init(void)
{
    return init_low_level();
}

esp_err_t esp_hid_ble_gap_adv_init(uint16_t appearance, const char *device_name)
{
    esp_err_t ret;

    static const uint8_t hidd_service_uuid128[] = {
        0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x12, 0x18, 0x00, 0x00,
    };

    esp_ble_adv_data_t ble_adv_data = {
        .set_scan_rsp = false,
        .include_name = true,
        .include_txpower = true,
        .min_interval = 0x0006,
        .max_interval = 0x0010,
        .appearance = appearance,
        .manufacturer_len = 0,
        .p_manufacturer_data = NULL,
        .service_data_len = 0,
        .p_service_data = NULL,
        .service_uuid_len = sizeof(hidd_service_uuid128),
        .p_service_uuid = (uint8_t *)hidd_service_uuid128,
        .flag = 0x6,
    };

    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_IO;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t key_size = 16;

    if ((ret = esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, 1)) != ESP_OK) {
        ESP_LOGE(TAG, "set_security_param AUTHEN_REQ_MODE failed: %d", ret);
        return ret;
    }
    if ((ret = esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, 1)) != ESP_OK) {
        ESP_LOGE(TAG, "set_security_param IOCAP_MODE failed: %d", ret);
        return ret;
    }
    if ((ret = esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, 1)) != ESP_OK) {
        ESP_LOGE(TAG, "set_security_param SET_INIT_KEY failed: %d", ret);
        return ret;
    }
    if ((ret = esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, 1)) != ESP_OK) {
        ESP_LOGE(TAG, "set_security_param SET_RSP_KEY failed: %d", ret);
        return ret;
    }
    if ((ret = esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, 1)) != ESP_OK) {
        ESP_LOGE(TAG, "set_security_param MAX_KEY_SIZE failed: %d", ret);
        return ret;
    }

    if ((ret = esp_ble_gap_set_device_name(device_name)) != ESP_OK) {
        ESP_LOGE(TAG, "set_device_name failed: %d", ret);
        return ret;
    }
    if ((ret = esp_ble_gap_config_adv_data(&ble_adv_data)) != ESP_OK) {
        ESP_LOGE(TAG, "config_adv_data failed: %d", ret);
        return ret;
    }

    return ret;
}

esp_err_t esp_hid_ble_gap_adv_start(void)
{
    static esp_ble_adv_params_t hidd_adv_params = {
        .adv_int_min = 0x20,
        .adv_int_max = 0x30,
        .adv_type = ADV_TYPE_IND,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .channel_map = ADV_CHNL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };
    return esp_ble_gap_start_advertising(&hidd_adv_params);
}
