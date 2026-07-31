#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "esp_bit_defs.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"

#include "board_config.h"
#include "encoder.h"

#define ENC_POLL_MS     5
#define ENC_SW_DEBOUNCE 20
#define ENC_FAST_MS     25

static encoder_cb_t s_cb;
static void *s_cb_arg;
static int s_last;
static bool s_sw_state;
static uint8_t s_sw_count;
static int64_t s_last_step_us;

static int encoder_read_gray(void)
{
    int a = gpio_get_level(ENCODER_PIN_A);
    int b = gpio_get_level(ENCODER_PIN_B);
    return (a << 1) | b;
}

static void encoder_update_state(void)
{
    int cur = encoder_read_gray();
    if (cur == s_last) {
        return;
    }
    encoder_dir_t dir;
    switch ((s_last << 2) | cur) {
    case 0b0010:
    case 0b1011:
    case 0b1101:
    case 0b0100:
        dir = ENC_CW;
        break;
    case 0b0001:
    case 0b0111:
    case 0b1110:
    case 0b1000:
        dir = ENC_CCW;
        break;
    default:
        dir = ENC_NONE;
        break;
    }
    s_last = cur;
    if (dir != ENC_NONE && s_cb) {
        int64_t now = esp_timer_get_time();
        bool fast = (s_last_step_us != 0 && now - s_last_step_us < ENC_FAST_MS * 1000);
        s_last_step_us = now;
        s_cb(dir, s_cb_arg);
        if (fast) {
            s_cb(dir, s_cb_arg);
        }
    }
}

static void encoder_poll_task(void *arg)
{
    while (1) {
        encoder_update_state();
        bool sw = gpio_get_level(ENCODER_PIN_SW) == 0;
        if (sw != s_sw_state) {
            if (++s_sw_count >= ENC_SW_DEBOUNCE) {
                s_sw_state = sw;
                s_sw_count = 0;
            }
        } else {
            s_sw_count = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(ENC_POLL_MS));
    }
}

void encoder_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = BIT64(ENCODER_PIN_A) | BIT64(ENCODER_PIN_B) | BIT64(ENCODER_PIN_SW),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = true,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);

    s_last = encoder_read_gray();
    s_sw_state = gpio_get_level(ENCODER_PIN_SW) == 0;
    s_sw_count = 0;
    s_last_step_us = 0;

    xTaskCreate(encoder_poll_task, "enc", 2048, NULL, 5, NULL);
}

void encoder_set_callback(encoder_cb_t cb, void *arg)
{
    s_cb = cb;
    s_cb_arg = arg;
}

encoder_dir_t encoder_get_direction(void)
{
    return ENC_NONE;
}

bool encoder_poll_switch(void)
{
    bool prev = s_sw_state;
    s_sw_state = false;
    return prev;
}
