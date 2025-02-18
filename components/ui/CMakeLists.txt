idf_component_register(
    SRCS	"ui.c" "pages/home_page.c"  "pages/setting_page.c"  "pages/Content_page.c"  "pages/filelist_page.c"
            "img/Sunny.c"   "img/rainy.c"   "img/cloudy.c"   "img/light_img.c"   "img/dis_wifi_32.c"
    INCLUDE_DIRS    "."
    REQUIRES    lvgl    bsp     com
)