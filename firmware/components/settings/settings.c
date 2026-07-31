#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"

#include "settings.h"

static const char *TAG = "settings";

#define NVS_NAMESPACE "numpad"
#define NVS_KEY_CONTRAST "contrast"
#define NVS_KEY_ENCODER "enc_inv"
#define NVS_KEY_SLEEP "sleep_to"

#define SETTINGS_DEFAULT_CONTRAST   0xCF
#define SETTINGS_DEFAULT_ENCODER    false
#define SETTINGS_DEFAULT_SLEEP      SETTINGS_SLEEP_OFF

settings_t g_settings;

static esp_err_t read_u8(const char *key, uint8_t *value, uint8_t fallback)
{
    nvs_handle_t handle = 0;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        *value = fallback;
        return ret;
    }
    ret = nvs_get_u8(handle, key, value);
    nvs_close(handle);
    if (ret != ESP_OK) {
        *value = fallback;
    }
    return ret;
}

void settings_load(void)
{
    uint8_t enc = 0;
    memset(&g_settings, 0, sizeof(g_settings));
    read_u8(NVS_KEY_CONTRAST, &g_settings.contrast, SETTINGS_DEFAULT_CONTRAST);
    read_u8(NVS_KEY_ENCODER, &enc, SETTINGS_DEFAULT_ENCODER);
    g_settings.invert_encoder = enc != 0;
    read_u8(NVS_KEY_SLEEP, &g_settings.sleep_timeout, SETTINGS_DEFAULT_SLEEP);
    ESP_LOGI(TAG, "loaded: contrast=%u invert=%d sleep=%u",
             (unsigned)g_settings.contrast, g_settings.invert_encoder ? 1 : 0,
             (unsigned)g_settings.sleep_timeout);
}

void settings_save(void)
{
    nvs_handle_t handle = 0;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(ret));
        return;
    }
    nvs_set_u8(handle, NVS_KEY_CONTRAST, g_settings.contrast);
    nvs_set_u8(handle, NVS_KEY_ENCODER, g_settings.invert_encoder ? 1 : 0);
    nvs_set_u8(handle, NVS_KEY_SLEEP, g_settings.sleep_timeout);
    ret = nvs_commit(handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nvs_commit failed: %s", esp_err_to_name(ret));
    }
    nvs_close(handle);
}
