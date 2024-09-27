#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"

#include "esp_lcd_ili9341.h"

static const char *TAG = "example";

// LCD Pin Configuration
#define LCD_HOST  SPI2_HOST
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

// Button Pin Configuration
#define PIN_BUTTON_UP          21
#define PIN_BUTTON_DOWN        22
#define PIN_BUTTON_SELECT      14
#define PIN_BUTTON_BACK        12
#define PIN_BUTTON_PLAYER1     13
#define PIN_BUTTON_PLAYER2     26

#define LCD_H_RES              240
#define LCD_V_RES              320

// LVGL specific configuration
#define LVGL_TICK_PERIOD_MS    2

static lv_disp_draw_buf_t disp_buf;
static lv_color_t buf1[LCD_H_RES * 10];
static lv_color_t buf2[LCD_H_RES * 10];
static lv_disp_drv_t disp_drv;

// Menu System
typedef struct MenuItem {
    const char* name;
    void (*action)(void);
    struct MenuItem* submenu;
    int submenuSize;
    int initialMinutes;
    int incrementSeconds;
} MenuItem;

// Function prototypes
static void button_task(void *pvParameter);
static void timer_task(void *pvParameter);
static void lv_tick_task(void *arg);
static void create_application_gui(void);
static void display_menu(void);
static void update_timers(void);

// Timer variables
static int player1_time = 600; // 10 minutes in seconds
static int player2_time = 600;
static int active_player = 0; // 0: no active player, 1: player1, 2: player2
static int timer_running = 0;

// Menu variables
static MenuItem* current_menu;
static int current_menu_size;
static int selected_index = 0;

// LVGL objects
static lv_obj_t *menu_label;
static lv_obj_t *timer1_label;
static lv_obj_t *timer2_label;

// Function to start the game
void start_game(void) {
    timer_running = 1;
    active_player = 1; // Start with player 1
}

// Function to stop the game
void stop_game(void) {
    timer_running = 0;
    active_player = 0;
}

// Function to set the timer
void set_timer(int minutes, int increment) {
    player1_time = minutes * 60;
    player2_time = minutes * 60;
    // TODO: Handle increment
}

// Timer submenu
MenuItem timer_submenu[] = {
    {"5 minutes", NULL, NULL, 0, 5, 0},
    {"10 minutes", NULL, NULL, 0, 10, 0},
    {"15 minutes", NULL, NULL, 0, 15, 0},
    {"Back", NULL, NULL, 0, 0, 0}
};

// Main menu
MenuItem main_menu[] = {
    {"Start Game", start_game, NULL, 0, 0, 0},
    {"Set Timer", NULL, timer_submenu, 4, 0, 0},
    {"Stop Game", stop_game, NULL, 0, 0, 0},
    {"Exit", NULL, NULL, 0, 0, 0}
};

void app_main(void)
{
    static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
    static lv_disp_drv_t disp_drv;      // contains callback functions

    ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

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

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_LOGI(TAG, "Install ILI9341 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));

    // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level(PIN_NUM_BK_LIGHT, LCD_BK_LIGHT_ON_LEVEL);

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();
    // alloc draw buffers used by LVGL
    // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
    lv_color_t *buf1 = heap_caps_malloc(LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
    lv_color_t *buf2 = heap_caps_malloc(LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2);
    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LCD_H_RES * 20);

    ESP_LOGI(TAG, "Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = esp_lcd_panel_io_tx_param;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    ESP_LOGI(TAG, "Install LVGL tick timer");
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lv_tick_task,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    // Initialize button GPIO
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << PIN_BUTTON_UP) | (1ULL << PIN_BUTTON_DOWN) | 
                        (1ULL << PIN_BUTTON_SELECT) | (1ULL << PIN_BUTTON_BACK) |
                        (1ULL << PIN_BUTTON_PLAYER1) | (1ULL << PIN_BUTTON_PLAYER2),
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);

    // Create GUI
    create_application_gui();

    // Create tasks
    xTaskCreate(button_task, "button_task", 4096, NULL, 10, NULL);
    xTaskCreate(timer_task, "timer_task", 4096, NULL, 10, NULL);

    // Set initial menu
    current_menu = main_menu;
    current_menu_size = sizeof(main_menu) / sizeof(MenuItem);

    while (1) {
        // Run LVGL tasks
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void lv_tick_task(void *arg)
{
    (void) arg;
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static void create_application_gui(void)
{
    // Create a screen
    lv_obj_t *scr = lv_disp_get_scr_act(NULL);

    // Create labels
    menu_label = lv_label_create(scr);
    lv_obj_align(menu_label, LV_ALIGN_TOP_LEFT, 10, 10);

    timer1_label = lv_label_create(scr);
    lv_obj_align(timer1_label, LV_ALIGN_TOP_LEFT, 10, 200);

    timer2_label = lv_label_create(scr);
    lv_obj_align(timer2_label, LV_ALIGN_TOP_RIGHT, -10, 200);

    // Initial display
    display_menu();
    update_timers();
}

static void display_menu(void)
{
    char menu_text[256] = "";
    for (int i = 0; i < current_menu_size; i++) {
        char line[64];
        snprintf(line, sizeof(line), "%s%s\n", (i == selected_index) ? "> " : "  ", current_menu[i].name);
        strcat(menu_text, line);
    }
    lv_label_set_text(menu_label, menu_text);
}

static void update_timers(void)
{
    char timer1_text[32], timer2_text[32];
    snprintf(timer1_text, sizeof(timer1_text), "P1: %02d:%02d", player1_time / 60, player1_time % 60);
    snprintf(timer2_text, sizeof(timer2_text), "P2: %02d:%02d", player2_time / 60, player2_time % 60);
    lv_label_set_text(timer1_label, timer1_text);
    lv_label_set_text(timer2_label, timer2_text);
}

static void button_task(void *pvParameter)
{
    while (1) {
        if (gpio_get_level(PIN_BUTTON_UP) == 0) {
            if (selected_index > 0) selected_index--;
            display_menu();
        } else if (gpio_get_level(PIN_BUTTON_DOWN) == 0) {
            if (selected_index < current_menu_size - 1) selected_index++;
            display_menu();
        } else if (gpio_get_level(PIN_BUTTON_SELECT) == 0) {
            MenuItem *item = &current_menu[selected_index];
            if (item->action) {
                item->action();
            } else if (item->submenu) {
                current_menu = item->submenu;
                current_menu_size = item->submenuSize;
                selected_index = 0;
            }
            display_menu();
        } else if (gpio_get_level(PIN_BUTTON_BACK) == 0) {
            if (current_menu != main_menu) {
                current_menu = main_menu;
                current_menu_size = sizeof(main_menu) / sizeof(MenuItem);
                selected_index = 0;
                display_menu();
            }
        } else if (gpio_get_level(PIN_BUTTON_PLAYER1) == 0) {
            if (timer_running && active_player == 1) {
                active_player = 2;
            }
        } else if (gpio_get_level(PIN_BUTTON_PLAYER2) == 0) {
            if (timer_running && active_player == 2) {
                active_player = 1;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Debounce delay
    }
}

static void timer_task(void *pvParameter)
{
    while (1) {
        if (timer_running) {
            if (active_player == 1 && player1_time > 0) {
                player1_time--;
            } else if (active_player == 2 && player2_time > 0) {
                player2_time--;
            }

            if (player1_time == 0 || player2_time == 0) {
                timer_running = 0;
                active_player = 0;
                // TODO: Handle game end
            }

            update_timers();
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Update every second
    }
}

// Helper function to execute menu item
static void execute_menu_item(MenuItem* item)
{
    if (item->action) {
        item->action();
    } else if (item->submenu) {
        current_menu = item->submenu;
        current_menu_size = item->submenuSize;
        selected_index = 0;
    } else if (item->initialMinutes > 0) {
        set_timer(item->initialMinutes, item->incrementSeconds);
    }
    display_menu();
    update_timers();
}

// Helper function to go back to the main menu
static void go_to_main_menu(void)
{
    current_menu = main_menu;
    current_menu_size = sizeof(main_menu) / sizeof(MenuItem);
    selected_index = 0;
    display_menu();
}

// Modify the button_task to use execute_menu_item and go_to_main_menu
static void button_task(void *pvParameter)
{
    while (1) {
        if (gpio_get_level(PIN_BUTTON_UP) == 0) {
            if (selected_index > 0) selected_index--;
            display_menu();
        } else if (gpio_get_level(PIN_BUTTON_DOWN) == 0) {
            if (selected_index < current_menu_size - 1) selected_index++;
            display_menu();
        } else if (gpio_get_level(PIN_BUTTON_SELECT) == 0) {
            execute_menu_item(&current_menu[selected_index]);
        } else if (gpio_get_level(PIN_BUTTON_BACK) == 0) {
            if (current_menu != main_menu) {
                go_to_main_menu();
            }
        } else if (gpio_get_level(PIN_BUTTON_PLAYER1) == 0) {
            if (timer_running && active_player == 1) {
                active_player = 2;
            }
        } else if (gpio_get_level(PIN_BUTTON_PLAYER2) == 0) {
            if (timer_running && active_player == 2) {
                active_player = 1;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Debounce delay
    }
}
