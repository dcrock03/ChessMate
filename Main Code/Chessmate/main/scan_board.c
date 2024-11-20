#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include <string.h>
#include <unistd.h>
#include "esp_timer.h"
#include "esp_log.h"

// GPIO Input Pin Definitions
#define GPIO_HALL_EFFECT 25         // master signal in from the selected hall effect sensor
#define INPUT_BIT_MASK (1ULL<<GPIO_HALL_EFFECT)

// GPIO Output Pin Definitions
#define GPIO_MUX_SEL_1_1 17         // bit one to select signal on row selecting mux
#define GPIO_MUX_SEL_1_2 25         // bit two to select signal on row selecting mux
#define GPIO_MUX_SEL_1_3 5         // bit three to select signal on row selecting mux
#define GPIO_MUX_SEL_2_1 22         // bit one to select signal on column selecting mux
#define GPIO_MUX_SEL_2_2 14         // bit two to select signal on column selecting mux
#define GPIO_MUX_SEL_2_3 19         // bit three to select signal on column selecting mux
#define OUTPUT_BIT_MASK ((1ULL<<GPIO_MUX_SEL_1_1) | (1ULL<<GPIO_MUX_SEL_1_2) | (1ULL<<GPIO_MUX_SEL_1_3) | (1ULL<<GPIO_MUX_SEL_2_1) | (1ULL<<GPIO_MUX_SEL_2_2) | (1ULL<<GPIO_MUX_SEL_2_3))

#define scan_frequency 5
static int clock_frequency_input = 1000000 / (scan_frequency * 64);

// create variables to iterate through the mux
uint8_t row = 0;
uint8_t col = 0;

void configure_input_GPIO() {
    gpio_config_t input_io_config;
    input_io_config.intr_type = GPIO_INTR_DISABLE;
    input_io_config.mode = GPIO_MODE_INPUT;
    input_io_config.pin_bit_mask = INPUT_BIT_MASK;
    input_io_config.pull_down_en = 0;
    input_io_config.pull_up_en = 1;
    gpio_config(&input_io_config);
}

void configure_output_GPIO() {
    gpio_config_t output_io_config;
    output_io_config.intr_type = GPIO_INTR_DISABLE;
    output_io_config.mode = GPIO_MODE_OUTPUT;
    output_io_config.pin_bit_mask = OUTPUT_BIT_MASK;
    output_io_config.pull_down_en = 0;
    output_io_config.pull_up_en = 0;
    gpio_config(&output_io_config);
}

void update_mux(uint8_t row, uint8_t col) {
    // set the row and column mux
    gpio_set_level(GPIO_MUX_SEL_1_1, (row & 0x01) >> 0);
    gpio_set_level(GPIO_MUX_SEL_1_2, (row & 0x02) >> 1);
    gpio_set_level(GPIO_MUX_SEL_1_3, (row & 0x04) >> 2);
    gpio_set_level(GPIO_MUX_SEL_2_1, (col & 0x01) >> 0);
    gpio_set_level(GPIO_MUX_SEL_2_2, (col & 0x02) >> 1);
    gpio_set_level(GPIO_MUX_SEL_2_3, (col & 0x04) >> 2);
}

static void periodic_timer_callback(void* arg);

void app_main(void)
{
    configure_output_GPIO();

    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            .name = "periodic"
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));

    // // temp code to test the mux
    // gpio_set_level(GPIO_MUX_SEL_1_1, 0);
    // gpio_set_level(GPIO_MUX_SEL_1_2, 0);
    // gpio_set_level(GPIO_MUX_SEL_1_3, 0);
    // gpio_set_level(GPIO_MUX_SEL_2_1, 0);
    // gpio_set_level(GPIO_MUX_SEL_2_2, 0);
    // gpio_set_level(GPIO_MUX_SEL_2_3, 0);

    // start clock
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, clock_frequency_input));

    while (1) {
        // 100ms delay
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

static void periodic_timer_callback(void* arg) {
    // update the mux
    update_mux(row, col);

    // read the value of the hall effect sensor
    int hall_effect_value = gpio_get_level(GPIO_HALL_EFFECT);
    if (hall_effect_value == 1) {
        printf("Hall Effect Value: %d\n", hall_effect_value);
        printf("Row: %d\n", row);
        printf("Col: %d\n", col);
    }

    update the row and column
    if (col == 7) {
        if (row == 7) {
            row = 0;
            col = 0;
        } else {
            row++;
            col = 0;
        }
    } else {
        col++;
    }
}