idf_component_register(
    SRCS 
        "spi_lcd_touch_example_main.c"
        "lvgl_demo_ui.c"
        "menu_data.c"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        driver
        esp_lcd
        esp_timer
        lvgl__lvgl
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_CONF_INCLUDE_SIMPLE=1)
