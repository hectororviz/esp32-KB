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
#include "settings.h"
#include "hid_usb.h"
#include "hid_ble.h"

static const char *TAG = "numpad";

static void encoder_event_cb(encoder_dir_t dir, void *arg)
{
    (void)arg;
    bool cw = (dir == ENC_CW);
    if (g_settings.invert_encoder) {
        cw = !cw;
    }
    if (!apps_encoder_turn(cw)) {
        hid_route_consumer_event(cw ? CC_VOLUME_UP : CC_VOLUME_DOWN);
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

    settings_load();
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
    display_set_brightness(g_settings.contrast);

    matrix_init();
    encoder_init();
    encoder_set_callback(encoder_event_cb, NULL);

    uint32_t prev_pressed = 0;
    int last_battery = -1;
    TickType_t last_display = 0;

    ESP_LOGI(TAG, "Numpad-20 ready");

    while (1) {
        matrix_scan();
        uint32_t pressed = matrix_get_state();

        uint32_t diff = pressed ^ prev_pressed;
        if (diff) {
            for (int i = 0; i < MATRIX_KEYS; i++) {
                uint32_t mask = (1u << i);
                if ((diff & mask) == 0 || (pressed & mask) == 0) {
                    continue;
                }
                uint16_t kc = keymap_resolve(i);
                if (apps_mode() == APP_MENU) {
                    continue;
                }
                if (kc == KC_CALC) {
                    apps_toggle_calc();
                } else if (kc != KC_NO) {
                    apps_on_key(kc);
                }
            }
        }
        prev_pressed = pressed;

        hid_report_t kbd;
        keymap_build_report(apps_mode() == APP_KEYBOARD ? pressed : 0, &kbd);
        hid_route_update(&kbd);

        if (encoder_poll_switch()) {
            apps_encoder_press();
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
