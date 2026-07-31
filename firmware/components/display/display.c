#include <string.h>
#include <stdint.h>

#include "esp_log.h"
#include "driver/i2c_master.h"
#include "apps.h"
#include "display.h"

static const char *TAG = "display";

#define DISPLAY_COL_OFFSET 2

#define FONT_W 5
#define FONT_H 7

static i2c_master_dev_handle_t s_dev;

static uint8_t s_fb[DISPLAY_HEIGHT / 8][DISPLAY_WIDTH];

static const uint8_t font5x7[256][5] = {
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00},
    ['0'] = {0x3E, 0x51, 0x49, 0x45, 0x3E},
    ['1'] = {0x00, 0x42, 0x7F, 0x40, 0x00},
    ['2'] = {0x42, 0x61, 0x51, 0x49, 0x46},
    ['3'] = {0x21, 0x41, 0x45, 0x4B, 0x31},
    ['4'] = {0x18, 0x14, 0x12, 0x7F, 0x10},
    ['5'] = {0x27, 0x45, 0x45, 0x45, 0x39},
    ['6'] = {0x3C, 0x4A, 0x49, 0x49, 0x30},
    ['7'] = {0x01, 0x71, 0x09, 0x05, 0x03},
    ['8'] = {0x36, 0x49, 0x49, 0x49, 0x36},
    ['9'] = {0x06, 0x49, 0x49, 0x29, 0x1E},
    ['A'] = {0x7E, 0x11, 0x11, 0x11, 0x7E},
    ['B'] = {0x7F, 0x49, 0x49, 0x49, 0x36},
    ['C'] = {0x3E, 0x41, 0x41, 0x41, 0x22},
    ['D'] = {0x7F, 0x41, 0x41, 0x22, 0x1C},
    ['E'] = {0x7F, 0x49, 0x49, 0x49, 0x41},
    ['F'] = {0x7F, 0x09, 0x09, 0x09, 0x01},
    ['G'] = {0x3E, 0x41, 0x49, 0x49, 0x7A},
    ['H'] = {0x7F, 0x08, 0x08, 0x08, 0x7F},
    ['I'] = {0x00, 0x41, 0x7F, 0x41, 0x00},
    ['J'] = {0x20, 0x40, 0x41, 0x3F, 0x01},
    ['K'] = {0x7F, 0x08, 0x14, 0x22, 0x41},
    ['L'] = {0x7F, 0x40, 0x40, 0x40, 0x40},
    ['M'] = {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    ['N'] = {0x7F, 0x04, 0x08, 0x10, 0x7F},
    ['O'] = {0x3E, 0x41, 0x41, 0x41, 0x3E},
    ['P'] = {0x7F, 0x09, 0x09, 0x09, 0x06},
    ['Q'] = {0x3E, 0x41, 0x51, 0x21, 0x5E},
    ['R'] = {0x7F, 0x09, 0x19, 0x29, 0x46},
    ['S'] = {0x46, 0x49, 0x49, 0x49, 0x31},
    ['T'] = {0x01, 0x01, 0x7F, 0x01, 0x01},
    ['U'] = {0x3F, 0x40, 0x40, 0x40, 0x3F},
    ['V'] = {0x1F, 0x20, 0x40, 0x20, 0x1F},
    ['W'] = {0x3F, 0x40, 0x38, 0x40, 0x3F},
    ['X'] = {0x63, 0x14, 0x08, 0x14, 0x63},
    ['Y'] = {0x07, 0x08, 0x70, 0x08, 0x07},
    ['Z'] = {0x61, 0x51, 0x49, 0x45, 0x43},
    ['+'] = {0x08, 0x08, 0x3E, 0x08, 0x08},
    ['-'] = {0x08, 0x08, 0x08, 0x08, 0x08},
    ['*'] = {0x00, 0x24, 0x18, 0x24, 0x00},
    ['/'] = {0x20, 0x10, 0x08, 0x04, 0x02},
    ['='] = {0x14, 0x14, 0x14, 0x14, 0x14},
    ['.'] = {0x00, 0x60, 0x60, 0x00, 0x00},
    [':'] = {0x00, 0x36, 0x36, 0x00, 0x00},
    ['%'] = {0x27, 0x12, 0x08, 0x24, 0x12},
};

static esp_err_t display_cmd(uint8_t cmd)
{
    uint8_t buf[2] = { 0x00, cmd };
    return i2c_master_transmit(s_dev, buf, sizeof(buf), 100);
}

static esp_err_t display_cmd2(uint8_t cmd, uint8_t arg)
{
    uint8_t buf[3] = { 0x00, cmd, arg };
    return i2c_master_transmit(s_dev, buf, sizeof(buf), 100);
}

static void fb_clear(void)
{
    memset(s_fb, 0, sizeof(s_fb));
}

static void fb_pixel(int x, int y, int on)
{
    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT) {
        return;
    }
    uint8_t mask = (uint8_t)(1u << (y & 7));
    if (on) {
        s_fb[y >> 3][x] |= mask;
    } else {
        s_fb[y >> 3][x] &= (uint8_t)~mask;
    }
}

static void fb_char(int x, int y, char c)
{
    const uint8_t *g = font5x7[(uint8_t)c];
    for (int cx = 0; cx < FONT_W; cx++) {
        for (int cy = 0; cy < FONT_H; cy++) {
            if (g[cx] & (1u << cy)) {
                fb_pixel(x + cx, y + cy, 1);
            }
        }
    }
}

static void fb_text(int x, int y, const char *s)
{
    for (int i = 0; s[i] != '\0' && i < (DISPLAY_WIDTH - x) / (FONT_W + 1); i++) {
        fb_char(x + i * (FONT_W + 1), y, s[i]);
    }
}

static void fb_flush(void)
{
    for (int page = 0; page < DISPLAY_HEIGHT / 8; page++) {
        display_cmd((uint8_t)(0xB0 | page));
        display_cmd((uint8_t)(DISPLAY_COL_OFFSET & 0x0F));
        display_cmd(0x10);
        uint8_t buf[1 + DISPLAY_WIDTH];
        buf[0] = 0x40;
        memcpy(&buf[1], s_fb[page], DISPLAY_WIDTH);
        i2c_master_transmit(s_dev, buf, sizeof(buf), 200);
    }
}

esp_err_t display_init(const display_config_t *cfg)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = cfg->sda,
        .scl_io_num = cfg->scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus = NULL;
    esp_err_t ret = i2c_new_master_bus(&bus_cfg, &bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(ret));
        return ret;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = cfg->addr,
        .scl_speed_hz = 400000,
    };
    ret = i2c_master_bus_add_device(bus, &dev_cfg, &s_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_bus_add_device failed: %s", esp_err_to_name(ret));
        return ret;
    }

    static const uint8_t init_cmds[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
        0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6,
    };
    for (size_t i = 0; i < sizeof(init_cmds); i++) {
        ret = display_cmd(init_cmds[i]);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "init cmd %02X failed: %s", init_cmds[i], esp_err_to_name(ret));
            return ret;
        }
    }
    return display_cmd(0xAF);
}

void display_off(void)
{
    display_cmd(0xAE);
}

void display_on(void)
{
    display_cmd(0xAF);
}

void display_set_brightness(uint8_t level)
{
    display_cmd2(0x81, level);
}

void display_update(void)
{
    fb_clear();
    fb_text(0, 0, apps_status_line());
    fb_text(0, 16, apps_calc_line());
    fb_flush();
}
