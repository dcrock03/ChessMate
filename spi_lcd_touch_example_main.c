#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "menu_data.h"
#include "esp_lcd_ili9341.h"
#include "lvgl_demo_ui.h"

#define MAX_MENU_DEPTH 5
#define LCD_PIXEL_CLOCK_HZ     (20 * 1000 * 1000)
#define LCD_BK_LIGHT_ON_LEVEL  1
#define LCD_BK_LIGHT_OFF_LEVEL !LCD_BK_LIGHT_ON_LEVEL
#define PIN_NUM_SCLK           18
#define PIN_NUM_MOSI           23
#define PIN_NUM_MISO           19
#define PIN_NUM_LCD_DC         2
#define PIN_NUM_LCD_RST        4
#define PIN_NUM_LCD_CS         5
#define PIN_NUM_BK_LIGHT       16

#define PIN_BUTTON_UP          21
#define PIN_BUTTON_DOWN        22
#define PIN_BUTTON_SELECT      14
#define PIN_BUTTON_PLAYER1     12
#define PIN_BUTTON_PLAYER2     26

// Updated resolution for horizontal orientation
#define LCD_H_RES              320
#define LCD_V_RES              240
#define LCD_HOST              SPI2_HOST

static const char *TAG = "example";

// Global variables for menu state
static MenuItem* current_menu;
static int current_menu_size;
static int selected_index;

// Menu stack for navigation
static struct {
    MenuItem* menu;
    int size;
    int selected;
} menu_stack[MAX_MENU_DEPTH];
static int menu_stack_top = -1;

// Function prototypes
void example_lvgl_demo_ui(lv_disp_t *disp);
void update_menu(const char **items, int item_count, int selected_index);
void update_timers(int player1_time, int player2_time, int active_player);
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);
static void button_task(void *pvParameter);
static void timer_task(void *pvParameter);

// Menu navigation functions
void menu_up(void) {
    if (selected_index > 0) {
        selected_index--;
        const char* items[MAX_MENU_DEPTH * 10];
        for (int i = 0; i < current_menu_size; i++) {
            items[i] = current_menu[i].name;
        }
        update_menu(items, current_menu_size, selected_index);
        ESP_LOGI(TAG, "Menu UP: index now %d", selected_index);
    }
}

void menu_down(void) {
    if (selected_index < current_menu_size - 1) {
        selected_index++;
        const char* items[MAX_MENU_DEPTH * 10];
        for (int i = 0; i < current_menu_size; i++) {
            items[i] = current_menu[i].name;
        }
        update_menu(items, current_menu_size, selected_index);
        ESP_LOGI(TAG, "Menu DOWN: index now %d", selected_index);
    }
}

static bool pop_menu_state(void) {
    if (menu_stack_top >= 0) {
        current_menu = menu_stack[menu_stack_top].menu;
        current_menu_size = menu_stack[menu_stack_top].size;
        selected_index = menu_stack[menu_stack_top].selected;
        menu_stack_top--;
        return true;
    }
    return false;
}

static void push_menu_state(void) {
    if (menu_stack_top < MAX_MENU_DEPTH - 1) {
        menu_stack_top++;
        menu_stack[menu_stack_top].menu = current_menu;
        menu_stack[menu_stack_top].size = current_menu_size;
        menu_stack[menu_stack_top].selected = selected_index;
    }
}

void menu_select(void) {
    MenuItem* selected_item = &current_menu[selected_index];
    ESP_LOGI(TAG, "Selected item: %s", selected_item->name);

    if (strcmp(selected_item->name, "Back") == 0) {
        if (pop_menu_state()) {
            const char* items[MAX_MENU_DEPTH * 10];
            for (int i = 0; i < current_menu_size; i++) {
                items[i] = current_menu[i].name;
            }
            update_menu(items, current_menu_size, selected_index);
        }
        return;
    }

    if (selected_item->submenu != NULL) {
        push_menu_state();
        current_menu = selected_item->submenu;
        current_menu_size = selected_item->submenu_size;
        selected_index = 0;
        
        const char* items[MAX_MENU_DEPTH * 10];
        for (int i = 0; i < current_menu_size; i++) {
            items[i] = current_menu[i].name;
        }
        update_menu(items, current_menu_size, selected_index);
    }
    else if (selected_item->action != NULL) {
        selected_item->action();
    }
}

static void button_task(void *pvParameter) {
    bool last_up_state = false;
    bool last_down_state = false;
    bool last_select_state = false;
    bool last_player1_state = false;
    bool last_player2_state = false;
    bool button_pressed = false;
    
    TickType_t last_press_time = 0;
    const TickType_t debounce_delay = pdMS_TO_TICKS(50);

    while (1) {
        TickType_t now = xTaskGetTickCount();
        
        bool current_up_state = gpio_get_level(PIN_BUTTON_UP);
        bool current_down_state = gpio_get_level(PIN_BUTTON_DOWN);
        bool current_select_state = gpio_get_level(PIN_BUTTON_SELECT);
        bool current_player1_state = gpio_get_level(PIN_BUTTON_PLAYER1);
        bool current_player2_state = gpio_get_level(PIN_BUTTON_PLAYER2);

        if ((now - last_press_time) >= debounce_delay && !button_pressed) {
            if (current_up_state == 1 && last_up_state == 0) {
                ESP_LOGI(TAG, "UP button pressed");
                menu_up();
                button_pressed = true;
                last_press_time = now;
            }
            else if (current_down_state == 1 && last_down_state == 0) {
                ESP_LOGI(TAG, "DOWN button pressed");
                menu_down();
                button_pressed = true;
                last_press_time = now;
            }
            else if (current_select_state == 1 && last_select_state == 0) {
                ESP_LOGI(TAG, "SELECT button pressed");
                menu_select();
                button_pressed = true;
                last_press_time = now;
            }
            else if (timer_running) {
                if (current_player1_state == 1 && last_player1_state == 0 && active_player == 1) {
                    ESP_LOGI(TAG, "Player 1 button pressed");
                    active_player = 2;
                    display_message("Player 2's Turn");
                    button_pressed = true;
                    last_press_time = now;
                }
                else if (current_player2_state == 1 && last_player2_state == 0 && active_player == 2) {
                    ESP_LOGI(TAG, "Player 2 button pressed");
                    active_player = 1;
                    display_message("Player 1's Turn");
                    button_pressed = true;
                    last_press_time = now;
                }
            }
        }

        if (current_up_state == 0 && current_down_state == 0 && 
            current_select_state == 0 && current_player1_state == 0 && 
            current_player2_state == 0) {
            button_pressed = false;
        }

        last_up_state = current_up_state;
        last_down_state = current_down_state;
        last_select_state = current_select_state;
        last_player1_state = current_player1_state;
        last_player2_state = current_player2_state;

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void timer_task(void *pvParameter) {
    while (1) {
        if (timer_running) {
            if (active_player == 1 && player1_time > 0) {
                player1_time--;
            } 
            else if (active_player == 2 && player2_time > 0) {
                player2_time--;
            }

            update_timers(player1_time, player2_time, active_player);

            if (player1_time == 0) {
                timer_running = 0;
                active_player = 0;
                display_message("Game Over - Player 2 Wins!");
            }
            else if (player2_time == 0) {
                timer_running = 0;
                active_player = 0;
                display_message("Game Over - Player 1 Wins!");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
    lv_disp_flush_ready(drv);
}

void app_main(void) {
    static lv_disp_draw_buf_t disp_buf;
    static lv_disp_drv_t disp_drv;

    ESP_LOGI(TAG, "Initialize SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_SCLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_LCD_DC,
        .cs_gpio_num = PIN_NUM_LCD_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install ILI9341 panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    
    // Configure for horizontal orientation
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, true));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    ESP_LOGI(TAG, "Allocate separate LVGL buffer");
    // Allocate larger buffer for horizontal orientation
    void *buf1 = heap_caps_malloc(LCD_H_RES * 40 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
    void *buf2 = heap_caps_malloc(LCD_H_RES * 40 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2);
    
    ESP_LOGI(TAG, "Initialize LVGL draw buffers");
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LCD_H_RES * 40);

    ESP_LOGI(TAG, "Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    lv_disp_drv_register(&disp_drv);

    ESP_LOGI(TAG, "Initialize buttons");
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = ((1ULL << PIN_BUTTON_UP) | 
                        (1ULL << PIN_BUTTON_DOWN) | 
                        (1ULL << PIN_BUTTON_SELECT) |
                        (1ULL << PIN_BUTTON_PLAYER1) |
                        (1ULL << PIN_BUTTON_PLAYER2)),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE
    };
    gpio_config(&io_conf);

    // Initialize backlight
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level(PIN_NUM_BK_LIGHT, LCD_BK_LIGHT_ON_LEVEL);

    ESP_LOGI(TAG, "Initialize menu state");
    current_menu = main_menu;
    current_menu_size = main_menu_size;
    selected_index = 0;
    menu_stack_top = -1;

    ESP_LOGI(TAG, "Create GUI");
    lv_disp_t *disp = lv_disp_get_default();
    example_lvgl_demo_ui(disp);

    ESP_LOGI(TAG, "Create tasks");
    xTaskCreate(button_task, "button_task", 4096, NULL, 10, NULL);
    xTaskCreate(timer_task, "timer_task", 4096, NULL, 10, NULL);

    ESP_LOGI(TAG, "Display initial menu");
    const char* items[MAX_MENU_DEPTH * 10];
    for (int i = 0; i < current_menu_size; i++) {
        items[i] = current_menu[i].name;
        ESP_LOGI(TAG, "Menu item %d: %s", i, items[i]);
    }
    update_menu(items, current_menu_size, selected_index);

    ESP_LOGI(TAG, "Enter main loop");
    while (1) {
        // Run LVGL tick and task handler
        lv_timer_handler();
        
        // Update display periodically
        static uint32_t last_tick = 0;
        if (lv_tick_get() - last_tick > 100) {  // Every 100ms
            const char* current_items[MAX_MENU_DEPTH * 10];
            for (int i = 0; i < current_menu_size; i++) {
                current_items[i] = current_menu[i].name;
            }
            update_menu(current_items, current_menu_size, selected_index);
            last_tick = lv_tick_get();
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
