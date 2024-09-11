#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "esp_adc/adc_oneshot.h"

// Constants for easier configuration
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000

#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

#define JOY_X_CHANNEL ADC_CHANNEL_4  // GPIO32 for X
#define JOY_Y_CHANNEL ADC_CHANNEL_5  // GPIO33 for Y
#define JOY_SEL_PIN 25

#define JOY_CENTER 2048
#define JOY_DEADZONE 500

static const char *TAG = "MENU_SYSTEM";

// Extended MenuItem structure to include timer settings
typedef struct MenuItem {
    const char* name;
    void (*action)(void);
    struct MenuItem* submenu;
    int submenuSize;
    int initialMinutes;
    int incrementSeconds;
} MenuItem;

// Function prototypes
void lcd_init(void);
void lcd_clear(void);
void lcd_write_string(const char* str);
void lcd_set_cursor(uint8_t col, uint8_t row);
void display_menu(MenuItem* menu, int size, int selectedIndex);
void handle_joystick(void);
void execute_menu_item(MenuItem* item);
void set_timer(int minutes, int increment);

// Sample menu actions
void start_game(void) { 
    lcd_clear();
    lcd_write_string("Starting game...");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void show_options(void) {
    lcd_clear();
    lcd_write_string("Showing options...");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void exit_menu(void) {
    lcd_clear();
    lcd_write_string("Exiting menu...");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void set_timer_5min(void) {
    set_timer(5, 0);
}

void set_timer_10min(void) {
    set_timer(10, 0);
}

void set_timer_15min(void) {
    set_timer(15, 0);
}

void set_timer_30min(void) {
    set_timer(30, 0);
}

void set_timer_60min(void) {
    set_timer(60, 0);
}

void set_timer_3_2(void) {
    set_timer(3, 2);
}

// Submenu items
MenuItem options_submenu[] = {
    {"Sound", NULL, NULL, 0, 0, 0},
    {"Difficulty", NULL, NULL, 0, 0, 0},
    {"Back", NULL, NULL, 0, 0, 0}
};

// Timer submenu
MenuItem timer_submenu[] = {
    {"5 minutes", set_timer_5min, NULL, 0, 5, 0},
    {"10 minutes", set_timer_10min, NULL, 0, 10, 0},
    {"15 minutes", set_timer_15min, NULL, 0, 15, 0},
    {"30 minutes", set_timer_30min, NULL, 0, 30, 0},
    {"60 minutes", set_timer_60min, NULL, 0, 60, 0},
    {"3|2", set_timer_3_2, NULL, 0, 3, 2},
    {"Back", NULL, NULL, 0, 0, 0}
};

// Main menu
MenuItem main_menu[] = {
    {"Start Game", start_game, NULL, 0, 0, 0},
    {"Set Timer", NULL, timer_submenu, 7, 0, 0},
    {"Options", show_options, options_submenu, 3, 0, 0},
    {"Exit", exit_menu, NULL, 0, 0, 0}
};

int current_menu_size = sizeof(main_menu) / sizeof(MenuItem);
MenuItem* current_menu = main_menu;
int selected_index = 0;

// Global variables for timer settings
int current_timer_minutes = 10;  // Default to 10 minutes
int current_timer_increment = 0; // Default to no increment

// ADC handle
adc_oneshot_unit_handle_t adc1_handle;

void lcd_init(void) {
    // Initialize I2C for LCD
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));

    // TODO: Implement actual LCD initialization
    ESP_LOGI(TAG, "LCD initialized");
}

void lcd_clear(void) {
    // TODO: Implement actual LCD clear function
    ESP_LOGI(TAG, "LCD cleared");
}

void lcd_write_string(const char* str) {
    // TODO: Implement actual LCD write function
    ESP_LOGI(TAG, "LCD write: %s", str);
}

void lcd_set_cursor(uint8_t col, uint8_t row) {
    // TODO: Implement actual LCD cursor set function
    ESP_LOGI(TAG, "LCD cursor set to (%d, %d)", col, row);
}

void display_menu(MenuItem* menu, int size, int selectedIndex) {
    lcd_clear();
    int startIdx = selectedIndex - (selectedIndex % LCD_ROWS);
    for (int i = 0; i < LCD_ROWS && (startIdx + i) < size; i++) {
        lcd_set_cursor(0, i);
        char line[LCD_COLS + 1];
        snprintf(line, sizeof(line), "%c%s", (startIdx + i == selectedIndex) ? '>' : ' ', menu[startIdx + i].name);
        lcd_write_string(line);
    }
}

void handle_joystick(void) {
    int joy_x, joy_y;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY_X_CHANNEL, &joy_x));
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY_Y_CHANNEL, &joy_y));
    int select_state = gpio_get_level(JOY_SEL_PIN);

    // Y-axis for menu navigation
    if (abs(joy_y - JOY_CENTER) > JOY_DEADZONE) {
        if (joy_y < JOY_CENTER) {
            selected_index = (selected_index > 0) ? selected_index - 1 : current_menu_size - 1;
        } else {
            selected_index = (selected_index < current_menu_size - 1) ? selected_index + 1 : 0;
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);  // Delay to prevent too fast scrolling
    }

    // Select button press
    static int last_select_state = 1;
    static TickType_t last_debounce_time = 0;
    TickType_t current_time = xTaskGetTickCount();

    if (select_state != last_select_state) {
        last_debounce_time = current_time;
    }

    if ((current_time - last_debounce_time) > pdMS_TO_TICKS(50)) {
        if (select_state == 0) {
            execute_menu_item(&current_menu[selected_index]);
        }
    }

    last_select_state = select_state;
}

void execute_menu_item(MenuItem* item) {
    if (item->action != NULL) {
        item->action();
    } else if (item->submenu != NULL) {
        current_menu = item->submenu;
        current_menu_size = item->submenuSize;
        selected_index = 0;
    } else if (strcmp(item->name, "Back") == 0) {
        current_menu = main_menu;
        current_menu_size = sizeof(main_menu) / sizeof(MenuItem);
        selected_index = 0;
    }
    
    // Set timer if applicable
    if (item->initialMinutes > 0) {
        set_timer(item->initialMinutes, item->incrementSeconds);
    }
}

void set_timer(int minutes, int increment) {
    current_timer_minutes = minutes;
    current_timer_increment = increment;
    lcd_clear();
    char timer_str[32];
    if (increment > 0) {
        snprintf(timer_str, sizeof(timer_str), "Timer: %d|%d", minutes, increment);
    } else {
        snprintf(timer_str, sizeof(timer_str), "Timer: %d min", minutes);
    }
    lcd_write_string(timer_str);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void menu_task(void *pvParameter) {
    while (1) {
        handle_joystick();
        display_menu(current_menu, current_menu_size, selected_index);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    // Initialize ADC for joystick
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOY_X_CHANNEL, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOY_Y_CHANNEL, &config));

    // Initialize GPIO for joystick select button
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << JOY_SEL_PIN),
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);

    // Initialize LCD
    lcd_init();

    // Create menu task
    xTaskCreate(menu_task, "menu_task", 2048, NULL, 10, NULL);
}
