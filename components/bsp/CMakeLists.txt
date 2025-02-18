idf_component_register(
    SRCS	"weather.c" "cst816t_driver.c" "st7789_driver.c" "screen_ledc.c"
    INCLUDE_DIRS    "."
    REQUIRES  json  esp_partition   driver  esp_lcd
    PRIV_REQUIRES   nvs_flash
)