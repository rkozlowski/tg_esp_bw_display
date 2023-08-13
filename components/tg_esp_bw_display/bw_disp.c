// bw_disp.c

#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bw_disp.h"

/** @file */

/** Maximum number of display instances */
#define MAX_DISP_INST_NUM 128 
/** Maximum number of pages */
#define MAX_PAGE_NUM 8


#define TAG "BW_DISP"

/** @brief rectangle */
typedef struct 
{
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} bwd_rect_t;


/** @brief Black and white display type.
 * 
 */
typedef struct
{
    bw_disp_type_t type;                ///< Display type
    bw_disp_handle_t handle;            ///< Display handle
    disp_proto_handle_t comm_handle;    ///< Communication protocol handle
    
    bw_disp_if_t* disp_if;              ///< Display interface
    uint8_t page_num;                   ///< Number of pages
    uint8_t* pages[MAX_PAGE_NUM];       ///< Array of pointers to the beginnings of pages in a display buffer
    bwd_rect_t dirty_rect;              ///< "Dirty" part of the display, that needs to be refreshed

    uint16_t buffer_size;               ///< Display buffer size
    
    uint8_t buffer[];                   ///< Display buffer
} bw_disp_t;

extern bw_disp_if_t bw_disp_sh1106_128x64_if;   ///< Display interface definition for 128x64 display using SH1106 driver

static int s_bw_disp_inst_num = 0;              ///< Number of display instances
static int s_bw_disp_inst_free_num = 0;         ///< Number of free (dealocated) display instances

static bw_disp_t **s_bw_disp_instances = NULL;  ///< Array of display instances


static void bw_disp_clear_dirty_rect(bw_disp_t *inst)
{
    assert(inst != NULL);
    inst->dirty_rect.x = inst->dirty_rect.y = 0;
    inst->dirty_rect.width = inst->dirty_rect.height = 0;    
}

static bool bw_disp_is_dirty(bw_disp_t *inst)
{
    assert(inst != NULL);
    return (inst->dirty_rect.width > 0) && (inst->dirty_rect.height > 0);
}

static void bw_disp_set_dirty_rect(bw_disp_t *inst, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    assert(inst != NULL);
    assert(width > 0 && height > 0);
    if (!bw_disp_is_dirty(inst))
    {        
        inst->dirty_rect.x = x;
        inst->dirty_rect.y = y;
        inst->dirty_rect.width = width;
        inst->dirty_rect.height = height;        
        return;
    }
    uint16_t x0 = MIN(x, inst->dirty_rect.x);
    uint16_t y0 = MIN(y, inst->dirty_rect.y);
    uint16_t x1 = MAX(x + width - 1, inst->dirty_rect.x + inst->dirty_rect.width - 1);
    uint16_t y1 = MAX(y + height - 1, inst->dirty_rect.y + inst->dirty_rect.height - 1);
    inst->dirty_rect.x = x0;
    inst->dirty_rect.y = y0;
    inst->dirty_rect.width = x1 - x0 + 1;
    inst->dirty_rect.height = y1 - y0 + 1;
}

bw_disp_handle_t bw_disp_init(disp_proto_handle_t comm_handle, bw_disp_type_t disp_type)
{
    if (comm_handle == INVALID_HANDLE)
    {
        ESP_LOGE(TAG, "Invalid communication protocol handle!");
        return INVALID_HANDLE;
    }
    bw_disp_if_t* disp_if;    
    switch (disp_type)
    {
    case BWD_SH1106_128X64:
        disp_if = &bw_disp_sh1106_128x64_if;        
        break;
    default:
        ESP_LOGE(TAG, "Invalid display type: %d", disp_type);
        return INVALID_HANDLE;        
    }
    uint16_t width = disp_if->width;
    uint16_t height = disp_if->height;    
    uint8_t page_num = disp_if->page_num != 0 ? disp_if->page_num : height / 8;    
    assert(page_num <= MAX_PAGE_NUM);
    assert((int) page_num * width <= 0xFFFF);
    uint16_t buffer_size = page_num * width;
    int inst_no = -1;
    if (s_bw_disp_inst_free_num == 0)
    {
        if (s_bw_disp_inst_num >= MAX_DISP_INST_NUM)
        {
            ESP_LOGE(TAG, "Too many instances!");
            return INVALID_HANDLE;
        }
        inst_no = s_bw_disp_inst_num;
        bw_disp_t **instPtr = (bw_disp_t **) realloc(s_bw_disp_instances, (s_bw_disp_inst_num + 1) * sizeof(bw_disp_t *));
        if (instPtr == NULL)
        {
            ESP_LOGE(TAG, "Failed to allocate memory for instances");
            return INVALID_HANDLE;
        }
        s_bw_disp_instances = instPtr;
        s_bw_disp_instances[s_bw_disp_inst_num] = NULL;
        s_bw_disp_inst_num++;
    }
    else
    {
        for (int i = 0; i < s_bw_disp_inst_num; i++)
        {
            if (s_bw_disp_instances[i] == NULL)
            {
                inst_no = i;
                break;
            }
        }
    }
    assert((inst_no >= 0) && (inst_no < s_bw_disp_inst_num));
    s_bw_disp_instances[inst_no] = (bw_disp_t *) calloc(1, sizeof(bw_disp_t) + buffer_size);
    if (s_bw_disp_instances[inst_no] == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate instance memory");
        return INVALID_HANDLE;
    }
    bw_disp_t *inst = s_bw_disp_instances[inst_no];
    inst->type = disp_type;
    inst->handle = inst_no + 1;
    inst->comm_handle = comm_handle;
    inst->disp_if = disp_if;
    inst->page_num = page_num;    
    inst->buffer_size = buffer_size;
    for (int i = 0; i < page_num; i++)
    {
        inst->pages[i] = &(inst->buffer[i * width]);
    }
    esp_err_t ret = disp_proto_write_commands(inst->comm_handle, inst->disp_if->init_commands.buf, inst->disp_if->init_commands.sz);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Display initialization failed");
        free(s_bw_disp_instances[inst_no]);
        s_bw_disp_instances[inst_no] = NULL;
        s_bw_disp_inst_free_num++;
        return INVALID_HANDLE;
    }    
    bw_disp_set_dirty_rect(inst, 0, 0, inst->disp_if->width, inst->disp_if->height);
    ESP_LOGI(TAG, "Display initialized. Handle: #%d; Type: %d; W: %d; H: %d", inst->handle, disp_type, inst->disp_if->width, inst->disp_if->height);
    return inst->handle;
}

static bw_disp_t* bw_disp_get_instance(bw_disp_handle_t handle)
{
    if ((handle == 0) || (handle > s_bw_disp_inst_num) || (s_bw_disp_instances[handle - 1] == NULL))
    {
        ESP_LOGE(TAG, "Invalid handle: #%d", handle);
        return NULL;
    }
    return s_bw_disp_instances[handle - 1];
}


esp_err_t bw_disp_close(bw_disp_handle_t handle)
{
    bw_disp_t *inst = bw_disp_get_instance(handle);
    if (inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = disp_proto_write_commands(inst->comm_handle, inst->disp_if->close_commands.buf, inst->disp_if->close_commands.sz);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to close/shutdown the display. Handle: #%d.", handle);
    }
    esp_err_t ret2 = disp_proto_close(inst->comm_handle);
    if (ret2 != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to close display connection. Handle: #%d. Comm handle: #%d", handle, inst->comm_handle);
    }
    free(inst);
    s_bw_disp_instances[handle - 1] = NULL;
    s_bw_disp_inst_free_num++;
    return ret != ESP_OK ? ret : ret2;
}

esp_err_t bw_disp_clear(bw_disp_handle_t handle)
{
    return bw_disp_fill(handle, BWDC_BLACK);
}

esp_err_t bw_disp_fill(bw_disp_handle_t handle, bw_disp_clr_t c)
{
    bw_disp_t *inst = bw_disp_get_instance(handle);
    if (inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(inst->buffer, c == BWDC_BLACK ? 0x00 : 0xFF, inst->buffer_size);
    bw_disp_set_dirty_rect(inst, 0, 0, inst->disp_if->width, inst->disp_if->height);
    return ESP_OK;
}

esp_err_t bw_disp_refresh(bw_disp_handle_t handle)
{
    bw_disp_t *inst = bw_disp_get_instance(handle);
    if (inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!bw_disp_is_dirty(inst))
    {
        return ESP_OK;
    }
    esp_err_t ret;    
    uint8_t first_page = (inst->dirty_rect.y >> 3);
    uint8_t last_page = (inst->dirty_rect.y + inst->dirty_rect.height - 1) >> 3;    
    for (int page = first_page; page <= last_page; page++)
    {
        ret = inst->disp_if->set_page_col(inst->comm_handle, page, inst->disp_if->first_col + inst->dirty_rect.x);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set page/column address. Handle: #%d. Page: %d", handle, page);
            return ret;
        }
        ret = disp_proto_write_data(inst->comm_handle, &(inst->buffer[page * inst->disp_if->width + inst->dirty_rect.x]), inst->dirty_rect.width);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to write page data. Handle: #%d. Page: %d", handle, page);
            return ret;
        }        
    }
    bw_disp_clear_dirty_rect(inst);
    return ESP_OK;
}

esp_err_t bw_disp_set_pixel(bw_disp_handle_t handle, uint16_t x, uint16_t y, bw_disp_clr_t c)
{
    bw_disp_t *inst = bw_disp_get_instance(handle);
    if (inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (x >= inst->disp_if->width || y >= inst->disp_if->height)
    {
        return ESP_ERR_INVALID_ARG;
    }
    int page = y >> 3;
    int y_bit = 1 << (y & 0x07);
    if (c == BWDC_BLACK)
    {
        inst->pages[page][x] &= !y_bit;
    }
    else
    {
        inst->pages[page][x] |= y_bit;
    }
    bw_disp_set_dirty_rect(inst, x, y, 1, 1);
    return ESP_OK;
}

esp_err_t bw_disp_get_pixel(bw_disp_handle_t handle, uint16_t x, uint16_t y, bw_disp_clr_t *c)
{
    bw_disp_t *inst = bw_disp_get_instance(handle);
    if (inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (x >= inst->disp_if->width || y >= inst->disp_if->height)
    {
        return ESP_ERR_INVALID_ARG;
    }
    int page = y >> 3;
    int y_bit = 1 << (y & 0x07);
    *c = (inst->pages[page][x] & y_bit) ? BWDC_WHITE : BWDC_BLACK;
    return ESP_OK;
}

esp_err_t bw_disp_vline(bw_disp_handle_t handle, uint16_t x, uint16_t y, uint16_t h, bw_disp_clr_t c)
{
    bw_disp_t *inst = bw_disp_get_instance(handle);
    if (inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (x >= inst->disp_if->width || y >= inst->disp_if->height || (y + h) > inst->disp_if->height)
    {
        return ESP_ERR_INVALID_ARG;
    }
    int first_page = y >> 3;
    uint16_t y_e = y + h - 1;
    int last_page = y_e >> 3;
    int page = first_page;
    uint8_t pix_mask = 0;
    int y_bit_s = 1 << (y & 0x07);
    int y_bit_e = 1 << (y_e & 0x07);
    int y_bit_e_f = (first_page < last_page) ? (1 << 7) : y_bit_e;
    for (int i = y_bit_s; i <=  y_bit_e_f; i <<= 1) 
    {
        pix_mask |= i;
    }
    while (page <= last_page)
    {
        if (c == BWDC_WHITE)
        {
            // set pixels
            inst->pages[page][x] |= pix_mask;
        }
        else // c == bwdc_black
        {
            // clear pixels
            inst->pages[page][x] &= ~pix_mask;
        }
        page++;
        if (page < last_page)
        {
            pix_mask = 0xFF;
        }
        else
        {
            pix_mask = 0;
            for (int i = 1; i <= y_bit_e; i <<= 1) 
            {
                pix_mask |= i;
            }
        }
    }
    bw_disp_set_dirty_rect(inst, x, y, 1, h);
    return ESP_OK;
}

esp_err_t bw_disp_hline(bw_disp_handle_t handle, uint16_t x, uint16_t y, uint16_t w, bw_disp_clr_t c)
{
    bw_disp_t *inst = bw_disp_get_instance(handle);
    if (inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (x >= inst->disp_if->width || y >= inst->disp_if->height || (x + w) > inst->disp_if->width)
    {
        return ESP_ERR_INVALID_ARG;
    }
    int page = y >> 3;
    int y_bit = 1 << (y & 0x07);
    if (c == BWDC_BLACK)
    {
        for (int i = 0; i < w; i++, x++)
        {
            inst->pages[page][x] &= ~y_bit;
        }
    }
    else
    {
        for (int i = 0; i < w; i++, x++)
        {
            inst->pages[page][x] |= y_bit;
        }
    }
    bw_disp_set_dirty_rect(inst, x, y, w, 1);
    return ESP_OK;
}

esp_err_t bw_disp_rect(bw_disp_handle_t handle, uint16_t x, uint16_t y, uint16_t w, uint16_t h, bw_disp_clr_t c)
{
    esp_err_t ret;
    ret = bw_disp_hline(handle, x, y, w, c);
    if (ret != ESP_OK)
    {
        return ret;
    }
    ret = bw_disp_vline(handle, x, y, h, c);
    if (ret != ESP_OK)
    {
        return ret;
    }
    ret = bw_disp_hline(handle, x, y + h - 1, w, c);
    if (ret != ESP_OK)
    {
        return ret;
    }
    ret = bw_disp_vline(handle, x + w - 1, y, h, c);
    return ret;
}

esp_err_t bw_disp_fill_rect(bw_disp_handle_t handle, uint16_t x, uint16_t y, uint16_t w, uint16_t h, bw_disp_clr_t c)
{
    esp_err_t ret;
    for (int i = 0; i < w; i++, x++)
    {
        ret = bw_disp_vline(handle, x, y, h, c);
        if (ret != ESP_OK)
        {
            return ret;
        }
    }
    return ESP_OK;
}

static esp_err_t bw_disp_image_sel_priv(bw_disp_t *inst, uint16_t x, uint16_t y, uint16_t ix, uint16_t iy, uint16_t iw, uint16_t ih, 
    bool inv_img, bw_disp_img_draw_mode_t mode, bw_image_t *img)
{
    if (iw > (img->width - ix))
    {
        iw = img->width - ix;
    }
    if (ih > (img->height - iy))
    {
        ih = img->height - iy;
    }
    if (iw > (inst->disp_if->width - x))
    {
        iw = inst->disp_if->width - x;
    }
    if (ih > (inst->disp_if->height - y))
    {
        ih = inst->disp_if->height - y;
    }
    int first_page = y >> 3;    // first page in the display buffer
    uint16_t ly = y + ih - 1;  // last y in the display buffer
    int last_page = ly >> 3;   // last page in the display buffer
    
    int first_img_page =  iy >> 3;    // first page in the image buffer
    uint16_t ily = iy + ih - 1;  // last y in the image buffer
    int last_img_page =  ily >> 3;    // first page in the image buffer

    uint16_t imgw = img->width;
    
    uint16_t yo = y & 0x07; // display y offset;
    uint16_t iyo = iy & 0x07; // image y offset;

    uint16_t lyo = ly & 0x07; // display lasy y offset;
    //uint16_t ilyo = ily & 0x07; // image last y offset;

    int page = first_page;
    int img_page = first_img_page;

    if (iyo > yo)
    {
        img_page++;
    }

    uint8_t page_mask = 0;          // display page mask

    for (int i = 0; i < yo; i++)
    {
        page_mask |= 1 << i;   
    }

    uint8_t last_page_mask = 0;     // last display page mask

    if (lyo < 7)
    {
        for (int i = lyo + 1; i <= 7; i++)
        {
            last_page_mask |= 1 << i;   
        }
        if (first_page == last_page)
        {
            page_mask |= last_page_mask;
        }
    }

    int shrp = (iyo >= yo) ? (iyo - yo) : (iyo + 8 - yo); // shift right previous image page
    int shlc = shrp > 0 ? 8 - shrp : 0; // shift left current image page
    
    while (page <= last_page)
    {
        uint8_t img_mask = ~page_mask;
        for (int i = 0; i < iw; i++)
        {
            //uint16_t pixels = inst->pages[page][x + i] & page_mask;
            uint16_t pixels = 0;
            if (img_page > first_img_page && shrp > 0)
            {
                uint8_t ppxs = (img->image[(imgw * (img_page - 1)) + ix + i] >> shrp);
                if (inv_img)
                {
                    ppxs = ~ppxs;
                }
                pixels |= ppxs & img_mask;
            }            
            if (img_page <= last_img_page)
            {
                uint8_t pxs = (img->image[(imgw * img_page) + ix + i] << shlc);
                if (inv_img)
                {
                    pxs = ~pxs;
                }
                pixels |= pxs & img_mask;
            }
            switch (mode)
            {                
            case BWDM_ADD_WHITE:
                pixels |= inst->pages[page][x + i];
                break;
            case BWDM_ADD_BLACK:
                pixels &= inst->pages[page][x + i];
                break;
            case BWDM_OVERRIDE:
            default:
                pixels |= inst->pages[page][x + i] & page_mask;
                break;
            }
            inst->pages[page][x + i] = pixels;
        }
        img_page++;
        page++;        
        page_mask = (page < last_page) ? 0 : last_page_mask;
    }
    bw_disp_set_dirty_rect(inst, x, y, iw, ih);
    return ESP_OK;
}

esp_err_t bw_disp_image_sel_ex(bw_disp_handle_t handle, uint16_t x, uint16_t y, uint16_t ix, uint16_t iy, uint16_t iw, uint16_t ih, 
    bool inv_img, bw_disp_img_draw_mode_t mode, bw_image_t *img)
{
    bw_disp_t *inst = bw_disp_get_instance(handle);
    if (inst == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (x >= inst->disp_if->width || y >= inst->disp_if->height)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (ix >= img->width || iy >= img->height)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (iw > (img->width - ix))
    {
        iw = img->width - ix;
    }
    if (ih > (img->height - iy))
    {
        ih = img->height - iy;
    }
    return bw_disp_image_sel_priv(inst, x, y, ix, iy, iw, ih, inv_img, mode, img);
}

esp_err_t bw_disp_image_sel(bw_disp_handle_t handle, uint16_t x, uint16_t y, uint16_t ix, uint16_t iy, uint16_t iw, uint16_t ih, bw_image_t *img)
{
    return bw_disp_image_sel_ex(handle, x, y, ix, iy, iw, ih, false, BWDM_OVERRIDE, img);
}

esp_err_t bw_disp_image(bw_disp_handle_t handle, uint16_t x, uint16_t y, bw_image_t *img)
{
    return bw_disp_image_sel(handle, x, y, 0, 0, 0xFFFF, 0xFFFF, img);
}

uint16_t bw_disp_get_height(bw_disp_handle_t handle)
{
    bw_disp_t *inst = bw_disp_get_instance(handle);
    if (inst == NULL)
    {
        return 0;
    }
    return inst->disp_if->height;
}

uint16_t bw_disp_get_width(bw_disp_handle_t handle)
{
    bw_disp_t *inst = bw_disp_get_instance(handle);
    if (inst == NULL)
    {
        return 0;
    }
    return inst->disp_if->width;
}
