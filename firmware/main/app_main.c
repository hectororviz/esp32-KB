#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "board_config.h"
#include "matrix.h"
#include "keymap.h"
#include "encoder.h"
#include "hid_route.h"
#include "keycodes.h"
#include "apps.h"
#include "display.h"
#include "power.h"
#include "hid_usb.h"
#include "hid_ble.h"

static const char *TAG = "numpad";

static void encoder_event_cb(encoder_dir_t dir, void *arg)
{
    (void)arg;
    switch (dir) {
    case ENC_CW:
        hid_route_consumer_event(CC_VOLUME_UP);
        break;
    case ENC_CCW:
        hid_route_consumer_event(CC_VOLUME_DOWN);
        break;
    default:
        break;
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    hid_route_init();
    apps_init();

    ESP_ERROR_CHECK(hid_ble_init());
    ESP_ERROR_CHECK(hid_usb_init());

    const power_config_t power_cfg = {
        .battery_gpio = BATTERY_ADC_GPIO,
        .battery_channel = BATTERY_ADC_CH,
        .charge_pin = CHARGE_PIN,
        .charge_done_pin = CHARGE_DONE_PIN,
        .vbus_pin = VBUS_PIN,
    };
    ESP_ERROR_CHECK(power_init(&power_cfg));

    const display_config_t disp_cfg = {
        .sda = OLED_SDA,
        .scl = OLED_SCL,
        .addr = OLED_I2C_ADDR,
    };
    ESP_ERROR_CHECK(display_init(&disp_cfg));

    matrix_init();
    encoder_init();
    encoder_set_callback(encoder_event_cb, NULL);

    uint32_t prev_pressed = 0;
    bool prev_fn_active = false;
    int last_battery = -1;
    TickType_t last_display = 0;

    ESP_LOGI(TAG, "Numpad-20 ready");

    while (1) {
        matrix_scan();
        uint32_t pressed = matrix_get_state();
        bool fn_active = (pressed & (1u << KEY_FN_INDEX)) != 0;

        uint32_t diff = pressed ^ prev_pressed;
        if (diff) {
            for (int i = 0; i < MATRIX_KEYS; i++) {
                uint32_t mask = (1u << i);
                if ((diff & mask) == 0 || (pressed & mask) == 0) {
                    continue;
                }
                uint16_t kc = keymap_resolve(i, fn_active);
                if (kc == KC_CALC) {
                    apps_toggle_calc();
                } else if (kc != KC_FN && kc != KC_NO) {
                    apps_on_key(kc);
                }
            }
        }
        prev_pressed = pressed;

        if (fn_active != prev_fn_active) {
            apps_set_fn(fn_active);
            prev_fn_active = fn_active;
        }

        hid_report_t kbd;
        uint16_t consumer = 0;
        keymap_build_report(pressed, fn_active, &kbd, &consumer);
        hid_route_update(fn_active, &kbd, consumer);

        if (encoder_poll_switch()) {
            apps_calc_mode_cycle();
        }

        if (xTaskGetTickCount() - last_display >= pdMS_TO_TICKS(250)) {
            int pct = power_battery_percent();
            if (pct != last_battery) {
                apps_set_battery_pct(pct);
                last_battery = pct;
            }
            apps_set_charging(power_is_charging());
            apps_set_ble_connected(hid_ble_is_connected());
            apps_set_usb_mounted(hid_usb_is_mounted());
            display_update();
            last_display = xTaskGetTickCount();
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
