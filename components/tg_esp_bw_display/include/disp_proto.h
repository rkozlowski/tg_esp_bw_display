// display protocol interface (SPI/I2C)

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver/i2c.h"


#ifdef __cplusplus
extern "C" {
#endif


#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })


#define MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

/** @file */

/** @brief Display communication protocol */
typedef enum
{
    DP_I2C, ///< I2C communication protocol
    DP_SPI  ///< SPI communication protocol
} disp_proto_type_t;

typedef uint16_t disp_proto_handle_t; ///< handle to a display communication protocol

/** Invalid handle */
#define INVALID_HANDLE 0x0000

/** @brief Initializes display communication protocol
 *  @param proto_type        Communication protocol type
 *  @param write_command     Pointer to the write command function
 *  @param write_commands    Pointer to the write commands function
 *  @param write_data_byte   Pointer to the write data byte function
 *  @param write_data        Pointer to the write data function
 *  @param close             Pointer to the close function
 *  @return
 *          - Non-zero handle if successful
 *          - INVALID_HANDLE in case of error
 */
disp_proto_handle_t disp_proto_init(disp_proto_type_t proto_type, 
        esp_err_t (*write_command)(disp_proto_handle_t handle, void* dp_data, uint8_t cmd),
        esp_err_t (*write_commands)(disp_proto_handle_t handle, void* dp_data, uint8_t commands[], int len),
        esp_err_t (*write_data_byte)(disp_proto_handle_t handle, void* dp_data, uint8_t data),
        esp_err_t (*write_data)(disp_proto_handle_t handle, void* dp_data, uint8_t data[], int len),
        esp_err_t (*close)(disp_proto_handle_t handle, void* dp_data), 
        void *dp_data, int len);

/** @brief Initializes I2C display communication protocol
 *  @param port         Communication port (e.g. I2C_NUM_0)
 *  @param clock_speed  I2C clock speed (in Hz) 
 *  @param address      Display address
 *  @param sda          SDA GPIO pin number
 *  @param scl          SCL GPIO pin number
 *  @return
 *          - Non-zero handle if successful
 *          - INVALID_HANDLE in case of error
 */
disp_proto_handle_t disp_proto_init_i2c(i2c_port_t port, int clock_speed, uint8_t address, gpio_num_t sda, gpio_num_t scl);

/** @brief Writes single command
 *  @param handle   Communication protocol handle
 *  @param cmd      Command
 *  @return ESP_OK in case of success or any other value indicating an error
 */
esp_err_t disp_proto_write_command(disp_proto_handle_t handle, uint8_t cmd);

/** @brief Writes multiple commands
 *  @param handle   Communication protocol handle
 *  @param commands Command
 *  @param len      Number of commands
 *  @return ESP_OK in case of success or any other value indicating an error
 */
esp_err_t disp_proto_write_commands(disp_proto_handle_t handle, uint8_t commands[], int len);

/** @brief Writes single data byte
 *  @param handle   Communication protocol handle
 *  @param data     Data byte
 *  @return ESP_OK in case of success or any other value indicating an error
 */
esp_err_t disp_proto_write_data_byte(disp_proto_handle_t handle, uint8_t data);

/** @brief Writes data bytes
 *  @param handle   Communication protocol handle
 *  @param data     Data bytes
 *  @param len      Number of data bytes
 *  @return ESP_OK in case of success or any other value indicating an error
 */
esp_err_t disp_proto_write_data(disp_proto_handle_t handle, uint8_t data[], int len);

/** @brief Closes communication link
 *  @param handle   Communication protocol handle 
 *  @return ESP_OK in case of success or any other value indicating an error
 */
esp_err_t disp_proto_close(disp_proto_handle_t handle);

#ifdef __cplusplus
}
#endif
