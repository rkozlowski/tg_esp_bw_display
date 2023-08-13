#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"



#include "disp_proto.h"
#include "bw_disp.h"


#define SDA_PIN GPIO_NUM_21
#define SCL_PIN GPIO_NUM_22

#define TAG "BW_DISPLAY_TEST"


bw_image_t img_tennis_ball =
{
    .width = 32,
    .height = 32,
    .image =
    {
        0x00, 0x00, 0x80, 0xC0, 0xE0, 0xB0, 0x98, 0x9C, 0x8C, 0x86, 0x86, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x03, 0x03, 0x03, 0x03, 0x07, 0x06, 0x0E, 0x3E, 0x7C, 0xF8, 0xB8, 0x70, 0xE0, 0xC0, 0x00, 0x00,
        0xF8, 0xFE, 0x0F, 0x07, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x07, 0x0F, 0x3E, 0xFC, 0xF0, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFC, 0x03, 0xFF, 0xFC,
        0x1F, 0x7F, 0xE0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x3F, 0x78,
        0xE0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xE0, 0x70, 0x38, 0x1F, 0x87, 0xE0, 0xFF, 0x1F,
        0x00, 0x00, 0x01, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x70, 0x60, 0x60, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0,
        0xC0, 0xC0, 0xC0, 0xC1, 0xC1, 0x61, 0x60, 0x70, 0x30, 0x18, 0x1C, 0x0E, 0x07, 0x01, 0x00, 0x00
    }
};


void app_main(void)
{
    disp_proto_handle_t disp_i2c = disp_proto_init_i2c(I2C_NUM_0, 1000000, BWD_OLED_I2C_ADDRESS, SDA_PIN, SCL_PIN);
    bw_disp_handle_t disp = INVALID_HANDLE;
    if (disp_i2c == INVALID_HANDLE)
    {
        ESP_LOGE(TAG, "I2C initialization failed!");
    }
    else
    {
        disp = bw_disp_init(disp_i2c, BWD_SH1106_128X64);
        if (disp == INVALID_HANDLE)
        {
            ESP_LOGE(TAG, "Display initialization failed!");
        }
    }
    
    bw_disp_fill(disp, BWDC_WHITE);
    bw_disp_refresh(disp);    
    vTaskDelay(1000/portTICK_PERIOD_MS);
    bw_disp_fill(disp, BWDC_BLACK);
    bw_disp_refresh(disp);
    
    vTaskDelay(1000/portTICK_PERIOD_MS);
    uint16_t x = 0;
    uint16_t y = 16;
    uint16_t width = bw_disp_get_width(disp);
    uint16_t height = bw_disp_get_height(disp);
    bw_disp_image(disp, x, y, &img_tennis_ball);
    uint16_t iw = img_tennis_ball.width;
    uint16_t ih = img_tennis_ball.height;

    int dx = 1;
    int dy = 1;
    bw_disp_refresh(disp);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    while (1)
    {
        bw_disp_fill_rect(disp, x, y, iw, ih, BWDC_BLACK);
        x += dx;
        if (x == 0 || (x == (width - iw)))
        {
            dx = -dx;
        }
        y += dy;
        if (y == 0 || (y == (height - ih)))
        {
            dy = -dy;
        }
        bw_disp_image(disp, x, y, &img_tennis_ball);
        bw_disp_refresh(disp);
        vTaskDelay(10/portTICK_PERIOD_MS);
    }    
}
