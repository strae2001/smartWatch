#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "incl/st7789_driver.h"
#include "incl/cst816t_driver.h"
#include "esp_log.h"
#include "lvgl/lvgl.h"

/*
  LVGL库移植
    1. 初始化和注册LVGL显示驱动
    2. 初始化和注册LVGL触摸驱动
    3. 初始化ST7789硬件接口
    4. 初始化CST816T硬件接口
    5. 提供一个定时器给LVGL使用
*/

#define TAG     "lv_port"

// 横屏
#define LCD_WIDTH       280
#define LCD_HEIGHT      240

static lv_disp_drv_t    disp_drv;

// 定时器回调
void esp_timer_expired_cb(void* arg)
{
    uint32_t tick_interval = *((uint32_t*)arg);
    lv_tick_inc(tick_interval);
}

// 刷新完成回调函数
void lv_port_flush_ready(void* param)
{
    lv_disp_flush_ready(&disp_drv);
}

void disp_flush(struct _lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    // 将内部缓冲区数据写入显示器
    st7789_flush(area->x1 + 20, area->x2 + 20 + 1, area->y1, area->y2 + 1, color_p);
}

// 1. 初始化和注册LVGL显示驱动
void lv_disp_init(void)
{
    // 定义显示缓冲区
    static lv_disp_draw_buf_t   disp_buf;
    // 定义单次刷新屏幕区域大小       
    const size_t disp_buf_size = LCD_WIDTH*(LCD_HEIGHT/7);

    // 向RAM申请两片缓存区，采用双缓冲结构刷新屏幕
    lv_color_t *disp_buf1 = heap_caps_malloc(disp_buf_size*sizeof(lv_color_t), MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA);
    lv_color_t *disp_buf2 = heap_caps_malloc(disp_buf_size*sizeof(lv_color_t), MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA);
    if(!disp_buf1 || !disp_buf2)
    {
        ESP_LOGI(TAG, "disp buff malloc failed!");
    }
    
    // 初始化显示缓冲区
    lv_disp_draw_buf_init(&disp_buf, disp_buf1, disp_buf2, disp_buf_size);

    // 初始化显示驱动
    lv_disp_drv_init(&disp_drv);

    // 设置显示区域水平宽度和垂直高度
    disp_drv.hor_res = LCD_WIDTH;       //水平宽度
    disp_drv.ver_res = LCD_HEIGHT;      //垂直宽度

    /* 设置刷新数据函数 */
    disp_drv.flush_cb = disp_flush;
    
    /*设置显示缓存*/
    disp_drv.draw_buf = &disp_buf;

    // 注册显示驱动
    lv_disp_drv_register(&disp_drv);
}


void IRAM_ATTR indev_read(struct _lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    int16_t x,y;
    int state;
    cst816t_read(&x, &y, &state);
    data->point.x = y;
    data->point.y = LCD_HEIGHT - x;        // 旋转后, x和y的坐标计算
    data->state = state;
}

// 2. 初始化和注册LVGL触摸驱动
void lv_touch_init(void)
{
    static lv_indev_drv_t indev_drv;

    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;     // 输入设备类型选择 触摸驱动
    indev_drv.read_cb = indev_read;

    // 注册输入驱动
    lv_indev_drv_register(&indev_drv);
}


// 3. 初始化ST7789硬件接口
void st7789_hw_init(void)
{
    st7789_cfg_t st7789_cfg = {
        .bl     = GPIO_NUM_26,
        .clk    = GPIO_NUM_18,
        .cs     = GPIO_NUM_5,
        .dc     = GPIO_NUM_17,
        .mosi   = GPIO_NUM_19,
        .rst    = GPIO_NUM_21,
        .spi_fre  = 40*1000*1000,        // spi时钟频率: 40MHZ
        .height   = LCD_HEIGHT,
        .width    = LCD_WIDTH,
        .spin     = 1,                   // 屏幕顺时针旋转90度
        .done_cb  = lv_port_flush_ready,
        .cb_param = &disp_drv
    };

    st7789_driver_hw_init(&st7789_cfg);
}


// 4. 初始化CST816T硬件接口
void cst816t_hw_init(void)
{
    cst816t_cfg_t cst816t_cfg = 
    {
        .scl = GPIO_NUM_22,
        .sda = GPIO_NUM_23,
        .fre = 300*1000,
        .x_limit = LCD_HEIGHT,          // 屏幕旋转后, xy需要调换
        .y_limit = LCD_WIDTH
    };

    cst816t_init(&cst816t_cfg);
}



// 5. 提供一个定时器给LVGL使用
void lv_tick_init(void)
{
    static uint32_t tick_interval = 5;

    // 配置定时器参数传递给esp_timer_create函数
    const esp_timer_create_args_t periodic_timer_args = 
    {
        .arg      = &tick_interval,         // 定时间隔
        .callback = esp_timer_expired_cb,   // 定时溢出回调函数
        .name     = "",
        .dispatch_method = ESP_TIMER_TASK,  // 在任务中调用回调函数
        .skip_unhandled_events = true       // 忽略周期定时中未处理的事件
    };

    esp_timer_handle_t timer_handle;
    esp_timer_create(&periodic_timer_args, &timer_handle);             // 创建定时器
    esp_timer_start_periodic(timer_handle, tick_interval*1000);        // 启动定时器
}

void lv_port_init(void)
{
    // lvgl库初始化
    lv_init();

    // 显示和触摸硬件接口初始化
    st7789_hw_init();
    cst816t_hw_init();

    // 显示以及触摸驱动初始化
    lv_disp_init();
    lv_touch_init();

    // lvgl定时器初始化
    lv_tick_init();
}