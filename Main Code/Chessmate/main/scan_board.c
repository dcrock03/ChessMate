//Scans the chess board hall effect sensors
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "scan_board.h"

// GPIO Input Pin Definitions
#define GPIO_HALL_EFFECT 21         // master signal in from the selected hall effect sensor
#define INPUT_BIT_MASK (1ULL<<GPIO_HALL_EFFECT)

// GPIO Output Pin Definitions
#define GPIO_MUX_SEL_1_1 17         // bit one to select signal on row selecting mux
#define GPIO_MUX_SEL_1_2 25         // bit two to select signal on row selecting mux
#define GPIO_MUX_SEL_1_3 5         // bit three to select signal on row selecting mux
#define GPIO_MUX_SEL_2_1 22         // bit one to select signal on column selecting mux
#define GPIO_MUX_SEL_2_2 14         // bit two to select signal on column selecting mux
#define GPIO_MUX_SEL_2_3 19         // bit three to select signal on column selecting mux
#define OUTPUT_BIT_MASK ((1ULL<<GPIO_MUX_SEL_1_1) | (1ULL<<GPIO_MUX_SEL_1_2) | (1ULL<<GPIO_MUX_SEL_1_3) | (1ULL<<GPIO_MUX_SEL_2_1) | (1ULL<<GPIO_MUX_SEL_2_2) | (1ULL<<GPIO_MUX_SEL_2_3))

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

void app_main(void)
{
    configure_output_GPIO();
    
}
