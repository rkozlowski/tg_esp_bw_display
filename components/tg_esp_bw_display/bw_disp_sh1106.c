// bw_disp_sh1106.c

#include "bw_disp.h"

uint8_t bw_disp_sh1106_init_commands[] =
{
    BWD_CMD_SET_CHARGE_PUMP_CTRL,
    BWD_CMD_SET_CHARGE_PUMP_ON,

    BWD_CMD_SET_SEGMENT_REMAP_INVERSE,
    BWD_CMD_SET_COM_SCAN_MODE_REVERSE,

    BWD_CMD_DISPLAY_ON,

    BWD_CMD_SET_COL_ADDR_LO,
    BWD_CMD_SET_COL_ADDR_HI,
    BWD_CMD_SET_PAGE_ADDR,
    BWD_CMD_SET_DISPLAY_START_LINE + 0,
    BWD_CMD_SET_DISPLAY_OFFSET,
    0x00
};

uint8_t bw_disp_sh1106_close_commands[] =
{
    BWD_CMD_DISPLAY_OFF, 
    BWD_CMD_SET_CHARGE_PUMP_CTRL, 
    BWD_CMD_SET_CHARGE_PUMP_OFF
};

esp_err_t bw_disp_sh1106_set_page_col(disp_proto_handle_t conn_handle, uint8_t page, uint16_t col);


bw_disp_if_t bw_disp_sh1106_128x64_if = 
{
    .width = 128,
    .height = 64,
    .page_num = 8,
    .first_col = 2,
    .max_contrast = 0xFF,
    .init_commands.sz = sizeof(bw_disp_sh1106_init_commands),
    .init_commands.buf = bw_disp_sh1106_init_commands,
    .close_commands.sz = sizeof(bw_disp_sh1106_close_commands),
    .close_commands.buf = bw_disp_sh1106_close_commands,
    .set_page_col = &bw_disp_sh1106_set_page_col
};

esp_err_t bw_disp_sh1106_set_page_col(disp_proto_handle_t conn_handle, uint8_t page, uint16_t col)
{
    uint8_t commands[] = { BWD_CMD_SET_PAGE_ADDR + page, BWD_CMD_SET_COL_ADDR_LO + (col & 0x0F), BWD_CMD_SET_COL_ADDR_HI + ((col >> 4) & 0x0F) };
    esp_err_t ret = disp_proto_write_commands(conn_handle, commands, sizeof(commands));
    return ret;
}
