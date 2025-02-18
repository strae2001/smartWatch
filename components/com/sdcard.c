#include "esp_err.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"
#include "driver/sdmmc_host.h"
#include "dirent.h"
#include "incl/sdcard.h"

#define SD_SPI_DRIVER
#define MOUNT_POINT     "/sdcard"   //挂载点名称

static char* TAG = "sdcard";

#ifdef SD_SPI_DRIVER
#define PIN_NUM_MISO  GPIO_NUM_4
#define PIN_NUM_MOSI  GPIO_NUM_15
#define PIN_NUM_CLK   GPIO_NUM_14
#define PIN_NUM_CS    GPIO_NUM_13
#endif

esp_err_t sdcard_init(void)
{
    esp_err_t ret;
    esp_vfs_fat_mount_config_t mount_conf = 
    {
        .allocation_unit_size = 16*1024,
        .format_if_mount_failed = false,
        .max_files = 5,
    };
    sdmmc_card_t *card;

#ifndef SD_SPI_DRIVER
    ESP_LOGI(TAG, "Using SDMMC peripheral");
    // sdmmc主机配置
    sdmmc_host_t host_cfg = SDMMC_HOST_DEFAULT();
    host_cfg.max_freq_khz = SDMMC_FREQ_DEFAULT;
    
    // sd卡插槽配置
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;          // 数据总线宽度

    // 获得SD卡注册在VFS的FAT文件系统 (挂载文件系统)
    ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host_cfg, &slot_config, &mount_conf, &card);
#else
    ESP_LOGI(TAG, "Using SPI peripheral");
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI3_HOST;
    host.max_freq_khz = 20000;
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH2);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus. erro_num:%s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_conf, &card);
#endif

    if(ret != ESP_OK)
    {
        ESP_LOGI(TAG, "SDcard mount failed!");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "SDcard mount success!");

    return ESP_OK;
}

int get_sdcard_filelist(const char (**file)[256])
{
    DIR* dir;
    struct dirent* entry;       // 定义文件入口

    // 定义20个文件名数组, 每个文件名最大长度256
    static char filename[20][256] = {0};
    dir = opendir(MOUNT_POINT);         // 打开挂载路径目录
    if(!dir)
        return ESP_FAIL;
    
    int file_cnt = 0;
    while((entry = readdir(dir)) != NULL)          // 遍历文件列表, readdir函数依次读取目录下所有文件后返回NULL
    {
        printf("%s\n", entry->d_name);
        snprintf(&filename[file_cnt++][0], 256, "%s", entry->d_name);
        if(file_cnt >= 20)
            break;
    }
    closedir(dir);
    *file = filename;

    return file_cnt;
}