// disp_proto_i2c.c

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"

#include "disp_proto.h"

#define TAG "DISP_PROTO_I2C"

#define I2C_CMD_SINGLE    0x80
#define I2C_CMD_STREAM    0x00
#define I2C_DATA_STREAM   0x40

typedef struct 
{
    i2c_port_t port;
    uint8_t address;
} disp_proto_i2c_t;

esp_err_t disp_proto_i2c_write_command(disp_proto_handle_t handle, void* dp_data_ptr, uint8_t cmd);
esp_err_t disp_proto_i2c_write_commands(disp_proto_handle_t handle, void* dp_data_ptr, uint8_t commands[], int len);
esp_err_t disp_proto_i2c_write_data_byte(disp_proto_handle_t handle, void* dp_data_ptr, uint8_t data);
esp_err_t disp_proto_i2c_write_data(disp_proto_handle_t handle, void* dp_data_ptr, uint8_t data[], int len);
esp_err_t disp_proto_i2c_close(disp_proto_handle_t handle, void* dp_data_ptr);


disp_proto_handle_t disp_proto_init_i2c(i2c_port_t port, int clock_speed, uint8_t address, gpio_num_t sda, gpio_num_t scl)
{
    disp_proto_handle_t handle = INVALID_HANDLE;

    i2c_config_t i2c_config = 
    {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = sda,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_io_num = scl,		
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = clock_speed,
        .clk_flags = 0
	};
    esp_err_t ret;
	ret = i2c_param_config(port, &i2c_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set I2C configuration. Code: 0x%.2X", ret);
        return INVALID_HANDLE;
    }
	ret = i2c_driver_install(port, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to install I2C driver. Code: 0x%.2X", ret);
        return INVALID_HANDLE;
    }
    disp_proto_i2c_t i2cData = 
    {
        .port = port,
        .address = address
    };
    handle = disp_proto_init(DP_I2C, 
        &disp_proto_i2c_write_command, &disp_proto_i2c_write_commands, &disp_proto_i2c_write_data_byte, &disp_proto_i2c_write_data, &disp_proto_i2c_close, 
        &i2cData, sizeof(i2cData));
    if (handle != INVALID_HANDLE)
    {
        ESP_LOGI(TAG, "Initialized I2C connection on port #%d. SDA: %d; SCL: %d; Address: 0x%02X; Handle: #%d", port, sda, scl, address, handle);
    }
    return handle;
}

esp_err_t disp_proto_i2c_write_command(disp_proto_handle_t handle, void* dp_data_ptr, uint8_t cmd)
{    
    disp_proto_i2c_t *i2cDataPtr = (disp_proto_i2c_t *)dp_data_ptr;    
    assert(i2cDataPtr != NULL);
    esp_err_t ret = ESP_FAIL;
    i2c_cmd_handle_t cmdh = i2c_cmd_link_create();
    i2c_master_start(cmdh);
	i2c_master_write_byte(cmdh, (i2cDataPtr->address << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmdh, I2C_CMD_SINGLE, true);
    i2c_master_write_byte(cmdh, cmd, true);
    i2c_master_stop(cmdh);
    ret = i2c_master_cmd_begin(i2cDataPtr->port, cmdh, 10/portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmdh);
    return ret;
}

esp_err_t disp_proto_i2c_write_commands(disp_proto_handle_t handle, void* dp_data_ptr, uint8_t commands[], int len)
{
    disp_proto_i2c_t *i2cDataPtr = (disp_proto_i2c_t *)dp_data_ptr;    
    assert(i2cDataPtr != NULL);
    esp_err_t ret = ESP_FAIL;
    i2c_cmd_handle_t cmdh = i2c_cmd_link_create();
    i2c_master_start(cmdh);
	i2c_master_write_byte(cmdh, (i2cDataPtr->address << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmdh, I2C_CMD_STREAM, true);
    i2c_master_write(cmdh, commands, len, true);
    i2c_master_stop(cmdh);
    ret = i2c_master_cmd_begin(i2cDataPtr->port, cmdh, 10/portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmdh);
    return ret;
}

esp_err_t disp_proto_i2c_write_data_byte(disp_proto_handle_t handle, void* dp_data_ptr, uint8_t data)
{
    disp_proto_i2c_t *i2cDataPtr = (disp_proto_i2c_t *)dp_data_ptr;    
    assert(i2cDataPtr != NULL);
    esp_err_t ret = ESP_FAIL;
    i2c_cmd_handle_t cmdh = i2c_cmd_link_create();
    i2c_master_start(cmdh);
	i2c_master_write_byte(cmdh, (i2cDataPtr->address << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmdh, I2C_DATA_STREAM, true);
    i2c_master_write_byte(cmdh, data, true);
    i2c_master_stop(cmdh);
    ret = i2c_master_cmd_begin(i2cDataPtr->port, cmdh, 10/portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmdh);
    return ret;
}

esp_err_t disp_proto_i2c_write_data(disp_proto_handle_t handle, void* dp_data_ptr, uint8_t data[], int len)
{
    disp_proto_i2c_t *i2cDataPtr = (disp_proto_i2c_t *)dp_data_ptr;    
    assert(i2cDataPtr != NULL);
    esp_err_t ret = ESP_FAIL;
    i2c_cmd_handle_t cmdh = i2c_cmd_link_create();
    i2c_master_start(cmdh);
	i2c_master_write_byte(cmdh, (i2cDataPtr->address << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmdh, I2C_DATA_STREAM, true);
    i2c_master_write(cmdh, data, len, true);
    i2c_master_stop(cmdh);
    ret = i2c_master_cmd_begin(i2cDataPtr->port, cmdh, 10/portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmdh);
    return ret;
}

esp_err_t disp_proto_i2c_close(disp_proto_handle_t handle, void* dp_data_ptr)
{
    disp_proto_i2c_t *i2cDataPtr = (disp_proto_i2c_t *)dp_data_ptr;    
    assert(i2cDataPtr != NULL);
    return i2c_driver_delete(i2cDataPtr->port);
}
