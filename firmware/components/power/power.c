#include <stdint.h>

#include "esp_bit_defs.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gpio.h"

#include "power.h"

static const char *TAG = "power";

#define BATTERY_V_MAX_MV 4200
#define BATTERY_V_MIN_MV 3300
#define BATTERY_DIVIDER_RATIO 2.0f

static power_config_t s_cfg;
static adc_oneshot_unit_handle_t s_adc;
static adc_cali_handle_t s_cali;

static bool adc_init(const power_config_t *cfg)
{
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    esp_err_t ret = adc_oneshot_new_unit(&init_cfg, &s_adc);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed: %s", esp_err_to_name(ret));
        return false;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_oneshot_config_channel(s_adc, cfg->battery_channel, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_config_channel failed: %s", esp_err_to_name(ret));
        return false;
    }

    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .chan = cfg->battery_channel,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_cali);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_cali_create_scheme_curve_fitting failed: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

esp_err_t power_init(const power_config_t *cfg)
{
    s_cfg = *cfg;

    gpio_config_t gpio_cfg = {
        .pin_bit_mask = BIT64(cfg->charge_pin) | BIT64(cfg->charge_done_pin) | BIT64(cfg->vbus_pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = true,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&gpio_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    if (!adc_init(cfg)) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

int power_battery_percent(void)
{
    int raw = 0;
    int mv = 0;
    if (adc_oneshot_read(s_adc, s_cfg.battery_channel, &raw) != ESP_OK) {
        return -1;
    }
    if (adc_cali_raw_to_voltage(s_cali, raw, &mv) != ESP_OK) {
        return -1;
    }
    float v = (float)mv * BATTERY_DIVIDER_RATIO;
    int pct = (int)((v - BATTERY_V_MIN_MV) * 100.0f / (BATTERY_V_MAX_MV - BATTERY_V_MIN_MV));
    if (pct < 0) {
        pct = 0;
    }
    if (pct > 100) {
        pct = 100;
    }
    return pct;
}

bool power_is_charging(void)
{
    return gpio_get_level(s_cfg.charge_pin) == 0;
}

bool power_is_charging_done(void)
{
    return gpio_get_level(s_cfg.charge_done_pin) == 0;
}

bool power_is_usb_powered(void)
{
    return gpio_get_level(s_cfg.vbus_pin) == 1;
}
