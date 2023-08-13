#pragma once

#include "disp_proto.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @file */

/** OLED display I2C address */
#define BWD_OLED_I2C_ADDRESS   0x3C

#define BWD_CMD_DISPLAY_OFF                 0xAE
#define BWD_CMD_DISPLAY_ON                  0xAF

#define BWD_CMD_SET_DISPLAY_START_LINE      0x40

#define BWD_CMD_SET_SEGMENT_REMAP_INVERSE   0xA1
#define BWD_CMD_SET_SEGMENT_REMAP_NORMAL    0xA0

#define BWD_CMD_SET_COM_SCAN_MODE_REVERSE   0xC8    
#define BWD_CMD_SET_COM_SCAN_MODE_NORMAL    0xC0

#define BWD_CMD_SET_DISPLAY_OFFSET          0xD3

#define BWD_CMD_SET_CHARGE_PUMP_CTRL        0xAD
#define BWD_CMD_SET_CHARGE_PUMP_ON          0x0B
#define BWD_CMD_SET_CHARGE_PUMP_OFF         0x0A

#define BWD_CMD_SET_PAGE_ADDR               0xB0
#define BWD_CMD_SET_COL_ADDR_LO             0x00
#define BWD_CMD_SET_COL_ADDR_HI             0x10

typedef enum
{
    BWD_SH1106_128X64
} bw_disp_type_t;

typedef enum
{
    BWDC_BLACK,
    BWDC_WHITE
} bw_disp_clr_t; // color/colour

typedef enum
{
    BWDM_OVERRIDE,
    BWDM_ADD_WHITE,
    BWDM_ADD_BLACK
} bw_disp_img_draw_mode_t;

typedef struct 
{
    size_t sz;
    uint8_t* buf;    
} buf_sz_t;

typedef struct 
{
    uint16_t width;
    uint16_t height;
    uint8_t page_num;
    uint16_t first_col;
    uint8_t max_contrast;

    buf_sz_t init_commands;
    buf_sz_t close_commands;
    
    esp_err_t (*set_page_col)(disp_proto_handle_t conn_handle, uint8_t page, uint16_t col);
} bw_disp_if_t; ///< Display interface type

typedef struct
{
    uint16_t width;
    uint16_t height;
    uint8_t image[];
} bw_image_t; ///< Black & white image type


typedef uint16_t bw_disp_handle_t; ///< Handle to a display


bw_disp_handle_t bw_disp_init(disp_proto_handle_t conn_handle, bw_disp_type_t disp_type);

esp_err_t bw_disp_clear(bw_disp_handle_t handle);
esp_err_t bw_disp_fill(bw_disp_handle_t handle, bw_disp_clr_t c);
esp_err_t bw_disp_refresh(bw_disp_handle_t handle);
esp_err_t bw_disp_set_pixel(bw_disp_handle_t handle, uint16_t x, uint16_t y, bw_disp_clr_t c);
esp_err_t bw_disp_get_pixel(bw_disp_handle_t handle, uint16_t x, uint16_t y, bw_disp_clr_t *cptr);

esp_err_t bw_disp_vline(bw_disp_handle_t handle, uint16_t x, uint16_t y, uint16_t h, bw_disp_clr_t c);
esp_err_t bw_disp_hline(bw_disp_handle_t handle, uint16_t x, uint16_t y, uint16_t w, bw_disp_clr_t c);
esp_err_t bw_disp_rect(bw_disp_handle_t handle, uint16_t x, uint16_t y, uint16_t w, uint16_t h, bw_disp_clr_t c);
esp_err_t bw_disp_fill_rect(bw_disp_handle_t handle, uint16_t x, uint16_t y, uint16_t w, uint16_t h, bw_disp_clr_t c);

esp_err_t bw_disp_image(bw_disp_handle_t handle, uint16_t x, uint16_t y, bw_image_t *img);
esp_err_t bw_disp_image_sel(bw_disp_handle_t handle, uint16_t x, uint16_t y, uint16_t ix, uint16_t iy, uint16_t iw, uint16_t ih, bw_image_t *img);
esp_err_t bw_disp_image_sel_ex(bw_disp_handle_t handle, uint16_t x, uint16_t y, uint16_t ix, uint16_t iy, uint16_t iw, uint16_t ih, 
    bool inv_img, bw_disp_img_draw_mode_t mode, bw_image_t *img);

uint16_t bw_disp_get_width(bw_disp_handle_t handle);
uint16_t bw_disp_get_height(bw_disp_handle_t handle);

#ifdef __cplusplus
}
#endif
