idf_component_register(
    SRCS	"lv_port.c"  "sdcard.c" "https_reqst.c" "Ntp_time.c" "myWIFI.c"  "text_parse.c"
    INCLUDE_DIRS    "."
    REQUIRES    bsp  lvgl  driver   sdmmc   fatfs   nvs_flash   esp_wifi    esp-tls
)