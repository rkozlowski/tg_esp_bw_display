// disp_proto.c

#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "disp_proto.h"


#define MAX_INST_NUM 1024

#define TAG "DISP_PROTO"

typedef struct
{
    disp_proto_type_t type;
    disp_proto_handle_t handle;    
    esp_err_t (*write_command)(disp_proto_handle_t handle, void* dp_data, uint8_t cmd);
    esp_err_t (*write_commands)(disp_proto_handle_t handle, void* dp_data, uint8_t commands[], int len);
    esp_err_t (*write_data_byte)(disp_proto_handle_t handle, void* dp_data, uint8_t data);
    esp_err_t (*write_data)(disp_proto_handle_t handle, void* dp_data, uint8_t data[], int len);
    esp_err_t (*close)(disp_proto_handle_t handle, void* dp_data);

    uint8_t data[];
} disp_proto_t;

static int s_disp_proto_inst_num = 0; // number of protocol instances;
static int s_disp_proto_inst_free_num = 0; // number of free (dealocated) protocol instances;

static disp_proto_t **s_disp_proto_instances = NULL; 

disp_proto_handle_t disp_proto_init(disp_proto_type_t proto_type, 
        esp_err_t (*write_command)(disp_proto_handle_t handle, void* dp_data, uint8_t cmd),
        esp_err_t (*write_commands)(disp_proto_handle_t handle, void* dp_data, uint8_t commands[], int len),
        esp_err_t (*write_data_byte)(disp_proto_handle_t handle, void* dp_data, uint8_t data),
        esp_err_t (*write_data)(disp_proto_handle_t handle, void* dp_data, uint8_t data[], int len),
        esp_err_t (*close)(disp_proto_handle_t handle, void* dp_data), 
        void *dp_data, int len)
{
    int inst_no = -1;
    if (s_disp_proto_inst_free_num == 0)
    {
        if (s_disp_proto_inst_num >= MAX_INST_NUM)
        {
            ESP_LOGE(TAG, "Too many instances!");
            return INVALID_HANDLE;
        }
        inst_no = s_disp_proto_inst_num;
        disp_proto_t **instances = (disp_proto_t **) realloc(s_disp_proto_instances, (s_disp_proto_inst_num + 1) * sizeof(disp_proto_t *));
        if (instances == NULL)
        {
            ESP_LOGE(TAG, "Failed to allocate memory for instances");
            return INVALID_HANDLE;
        }
        s_disp_proto_instances = instances;
        s_disp_proto_instances[s_disp_proto_inst_num] = NULL;
        s_disp_proto_inst_num++;
    }
    else
    {
        for (int i = 0; i < s_disp_proto_inst_num; i++)
        {
            if (s_disp_proto_instances[i] == NULL)
            {
                inst_no = i;
                break;
            }
        }
    }
    assert((inst_no >= 0) && (inst_no < s_disp_proto_inst_num));
    s_disp_proto_instances[inst_no] = (disp_proto_t *) calloc(1, sizeof(disp_proto_t) + len);
    if (s_disp_proto_instances[inst_no] == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate instance memory");
        return INVALID_HANDLE;
    }
    s_disp_proto_instances[inst_no]->type = proto_type;
    s_disp_proto_instances[inst_no]->handle = inst_no + 1;
    s_disp_proto_instances[inst_no]->write_command = write_command;
    s_disp_proto_instances[inst_no]->write_commands = write_commands;
    s_disp_proto_instances[inst_no]->write_data_byte = write_data_byte;
    s_disp_proto_instances[inst_no]->write_data = write_data;    
    s_disp_proto_instances[inst_no]->close = close;
    memcpy(s_disp_proto_instances[inst_no]->data, dp_data, len);
    return s_disp_proto_instances[inst_no]->handle;
}

static disp_proto_t* s_disp_proto_get_instance(disp_proto_handle_t handle)
{
    if ((handle == 0) || (handle > s_disp_proto_inst_num) || (s_disp_proto_instances[handle - 1] == NULL))
    {
        ESP_LOGE(TAG, "Invalid handle: #%d", handle);
        return NULL;
    }
    return s_disp_proto_instances[handle - 1];
}

esp_err_t disp_proto_write_command(disp_proto_handle_t handle, uint8_t cmd)
{
    disp_proto_t *inst = s_disp_proto_get_instance(handle);
    if (inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = inst->write_command(handle, inst->data, cmd);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Write command operation failed. Handle: #%d. Code: 0x%.2X", handle, ret);
    }
    return ret;
}

esp_err_t disp_proto_write_commands(disp_proto_handle_t handle, uint8_t commands[], int len)
{
    disp_proto_t *inst = s_disp_proto_get_instance(handle);
    if (inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = inst->write_commands(handle, inst->data, commands, len);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Write commands operation failed. Handle: #%d. Code: 0x%.2X", handle, ret);
    }
    return ret;
}

esp_err_t disp_proto_write_data_byte(disp_proto_handle_t handle, uint8_t data)
{
    disp_proto_t *inst = s_disp_proto_get_instance(handle);
    if (inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = inst->write_data_byte(handle, inst->data, data);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Write data byte operation failed. Handle: #%d. Code: 0x%.2X", handle, ret);
    }
    return ret;
}

esp_err_t disp_proto_write_data(disp_proto_handle_t handle, uint8_t data[], int len)
{
    disp_proto_t *inst = s_disp_proto_get_instance(handle);
    if (inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = inst->write_data(handle, inst->data, data, len);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Write data operation failed. Handle: #%d. Code: 0x%.2X", handle, ret);
    }
    return ret;
}

esp_err_t disp_proto_close(disp_proto_handle_t handle)
{
    disp_proto_t *inst = s_disp_proto_get_instance(handle);
    if (inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = inst->close(handle, inst->data);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Protocol close operation failed for handle #%d. Code: 0x%.2X. Continuing with memory deallocation.", handle, ret);
    }
    free(inst);
    s_disp_proto_instances[handle - 1] = NULL;
    s_disp_proto_inst_free_num++;
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Protocol close operation finished for handle #%d.", handle);
    }
    return ret;
}
