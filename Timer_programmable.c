//////////////////////



#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "esp_log.h"

// GPIO pin definitions for player buttons
#define BUTTON_1_GPIO    0    // Button for Player 1
#define BUTTON_2_GPIO    35   // Button for Player 2
#define DEBOUNCE_TIME   50    // Debounce time in milliseconds to prevent multiple triggers

// Configuration structure for timer settings
typedef struct {
    int64_t initial_time_ms;  // Initial time allocation for each player in milliseconds
    int64_t increment_ms;     // Time increment after each move in milliseconds
} timer_config_t;

// Enum to track current game state
typedef enum {
    TIMER_STOPPED,      // Game not started or finished
    PLAYER_1_ACTIVE,    // Player 1's clock is running
    PLAYER_2_ACTIVE     // Player 2's clock is running
} timer_state_t;

// Global variables for game state and timing
static timer_state_t current_state = TIMER_STOPPED;
static int64_t player1_time_ms;        // Remaining time for Player 1
static int64_t player2_time_ms;        // Remaining time for Player 2
static int64_t last_update_time = 0;   // Timestamp of last time update

// Default timer configuration: 2 minutes with 1 second increment
static timer_config_t timer_config = {
    .initial_time_ms = 120000,  // 2 minutes in milliseconds
    .increment_ms = 1000        // 1 second increment
};

// LVGL UI elements
static lv_obj_t *player1_label;    // Display for Player 1's time
static lv_obj_t *player2_label;    // Display for Player 2's time
static lv_obj_t *status_label;     // Display for game status
static lv_obj_t *config_label;     // Display for current time control settings

// Queue for handling button press events
static QueueHandle_t button_evt_queue;

// Function declarations
static void IRAM_ATTR button_isr_handler(void* arg);
static void update_display(void);
static void init_buttons(void);
static void init_display(void);
static void reset_timers(void);

/**
 * @brief Configure the timer settings
 * @param initial_minutes Initial time in minutes for each player
 * @param increment_seconds Time increment in seconds after each move
 */
void set_timer_config(int initial_minutes, int increment_seconds) {
    timer_config.initial_time_ms = initial_minutes * 60 * 1000;
    timer_config.increment_ms = increment_seconds * 1000;
    reset_timers();  // Reset the game with new settings
}

/**
 * @brief Reset both players' timers to initial values
 */
static void reset_timers(void) {
    player1_time_ms = timer_config.initial_time_ms;
    player2_time_ms = timer_config.initial_time_ms;
    current_state = TIMER_STOPPED;
    update_display();
}

/**
 * @brief ISR handler for button presses
 * @param arg GPIO pin number that triggered the interrupt
 * 
 * This function runs in interrupt context - keep it minimal
 */
static void IRAM_ATTR button_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(button_evt_queue, &gpio_num, NULL);
}

/**
 * @brief Initialize GPIO buttons with interrupt handling
 */
static void init_buttons(void) {
    // Configure GPIO settings for both buttons
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,      // Trigger on falling edge
        .mode = GPIO_MODE_INPUT,             // Set as input
        .pin_bit_mask = (1ULL << BUTTON_1_GPIO) | (1ULL << BUTTON_2_GPIO),  // Select pins
        .pull_up_en = GPIO_PULLUP_ENABLE,    // Enable pull-up
    };
    gpio_config(&io_conf);

    // Create queue for button events
    button_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // Initialize interrupt service and attach handlers
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_1_GPIO, button_isr_handler, (void*)BUTTON_1_GPIO);
    gpio_isr_handler_add(BUTTON_2_GPIO, button_isr_handler, (void*)BUTTON_2_GPIO);
}

/**
 * @brief Initialize LVGL display elements
 */
static void init_display(void) {
    // Create main container for UI elements
    lv_obj_t *main_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(main_container, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(main_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(main_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Create and style all display labels
    player1_label = lv_label_create(main_container);
    lv_obj_set_style_text_font(player1_label, &lv_font_montserrat_48, 0);
    
    status_label = lv_label_create(main_container);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_24, 0);
    
    config_label = lv_label_create(main_container);
    lv_obj_set_style_text_font(config_label, &lv_font_montserrat_16, 0);
    
    player2_label = lv_label_create(main_container);
    lv_obj_set_style_text_font(player2_label, &lv_font_montserrat_48, 0);

    update_display();  // Initial display update
}

/**
 * @brief Format time for display
 * @param buf Output buffer for formatted time string
 * @param buf_size Size of output buffer
 * @param time_ms Time in milliseconds to format
 * 
 * Formats time differently based on remaining time:
 * - Under 1 minute: SS.T (seconds with tenths)
 * - Over 1 minute: MM:SS (minutes and seconds)
 */
static void format_time(char *buf, size_t buf_size, int64_t time_ms) {
    int minutes = time_ms / 60000;
    int seconds = (time_ms % 60000) / 1000;
    int tenths = (time_ms % 1000) / 100;
    
    if (time_ms <= 60000) { // Less than or equal to 1 minute
        snprintf(buf, buf_size, "%02d.%01d", seconds, tenths);
    } else {
        snprintf(buf, buf_size, "%02d:%02d", minutes, seconds);
    }
}

/**
 * @brief Update all display elements
 * 
 * Updates player times, game status, and configuration display
 */
static void update_display(void) {
    char buf[32];
    
    // Update Player 1's time display
    format_time(buf, sizeof(buf), player1_time_ms);
    lv_label_set_text(player1_label, buf);
    
    // Update Player 2's time display
    format_time(buf, sizeof(buf), player2_time_ms);
    lv_label_set_text(player2_label, buf);
    
    // Update status message based on game state
    const char *status_text;
    switch (current_state) {
        case TIMER_STOPPED:
            status_text = "Press any button to start";
            break;
        case PLAYER_1_ACTIVE:
            status_text = "Player 1's turn";
            break;
        case PLAYER_2_ACTIVE:
            status_text = "Player 2's turn";
            break;
    }
    lv_label_set_text(status_label, status_text);
    
    // Update configuration display
    snprintf(buf, sizeof(buf), "Time: %d min | Increment: %d sec", 
             (int)(timer_config.initial_time_ms / 60000),
             (int)(timer_config.increment_ms / 1000));
    lv_label_set_text(config_label, buf);
}

/**
 * @brief Main timer task
 * @param pvParameters Task parameters (unused)
 * 
 * Handles:
 * - Button press events
 * - Time updates
 * - Game state transitions
 * - Display updates
 */
void timer_task(void *pvParameters) {
    uint32_t gpio_num;
    int64_t current_time;
    
    while (1) {
        // Handle button press events
        if (xQueueReceive(button_evt_queue, &gpio_num, pdMS_TO_TICKS(10)) == pdTRUE) {
            current_time = esp_timer_get_time() / 1000;
            
            // Debounce check
            if (current_time - last_update_time < DEBOUNCE_TIME) {
                continue;
            }
            last_update_time = current_time;
            
            // State machine for game flow
            switch (current_state) {
                case TIMER_STOPPED:
                    // Start game based on which button was pressed
                    if (gpio_num == BUTTON_1_GPIO) {
                        current_state = PLAYER_2_ACTIVE;
                    } else {
                        current_state = PLAYER_1_ACTIVE;
                    }
                    break;
                    
                case PLAYER_1_ACTIVE:
                    // Player 1 completes move
                    if (gpio_num == BUTTON_1_GPIO) {
                        player1_time_ms += timer_config.increment_ms;  // Add increment
                        current_state = PLAYER_2_ACTIVE;              // Switch to Player 2
                    }
                    break;
                    
                case PLAYER_2_ACTIVE:
                    // Player 2 completes move
                    if (gpio_num == BUTTON_2_GPIO) {
                        player2_time_ms += timer_config.increment_ms;  // Add increment
                        current_state = PLAYER_1_ACTIVE;              // Switch to Player 1
                    }
                    break;
            }
            
            update_display();
        }
        
        // Update active player's time
        if (current_state != TIMER_STOPPED) {
            current_time = esp_timer_get_time() / 1000;
            // Update more frequently when under 1 minute for smooth tenths display
            int update_interval = (player1_time_ms <= 60000 || player2_time_ms <= 60000) ? 100 : 1000;
            
            if (current_time - last_update_time >= update_interval) {
                // Update Player 1's time
                if (current_state == PLAYER_1_ACTIVE) {
                    player1_time_ms -= (current_time - last_update_time);
                    if (player1_time_ms <= 0) {
                        player1_time_ms = 0;
                        current_state = TIMER_STOPPED;
                        lv_label_set_text(status_label, "Player 2 wins!");
                    }
                } 
                // Update Player 2's time
                else if (current_state == PLAYER_2_ACTIVE) {
                    player2_time_ms -= (current_time - last_update_time);
                    if (player2_time_ms <= 0) {
                        player2_time_ms = 0;
                        current_state = TIMER_STOPPED;
                        lv_label_set_text(status_label, "Player 1 wins!");
                    }
                }
                last_update_time = current_time;
                update_display();
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10)); // Prevent task from hogging CPU
    }
}

/**
 * @brief Application entry point
 */
void app_main(int x, int y) {
    // Initialize system components
    init_display();
    init_buttons();
    set_timer_config(x, y);  // x minutes with y second increment
    // Examples of different time control configurations:
    // set_timer_config(5, 3);  // 5 minutes with 3 second increment
    // set_timer_config(3, 2);  // 3 minutes with 2 second increment
    // set_timer_config(1, 1);  // 1 minute with 1 second increment
    
    // Create and start the timer task
    xTaskCreate(timer_task, "timer_task", 4096, NULL, 5, NULL);
}




// /* Include necessary headers */
// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/queue.h"
// #include "driver/gpio.h"
// #include "esp_timer.h"
// #include "lvgl.h"
// #include "esp_log.h"

// #define BUTTON_1_GPIO    0  // Adjust GPIO pins as needed
// #define BUTTON_2_GPIO    35
// #define DEBOUNCE_TIME   50  // milliseconds

// // Timer states
// typedef enum {
//     TIMER_STOPPED,
//     PLAYER_1_ACTIVE,
//     PLAYER_2_ACTIVE
// } timer_state_t;

// // Global variables
// static timer_state_t current_state = TIMER_STOPPED;
// static int64_t player1_time_ms = 120000; // 2 minutes in milliseconds
// static int64_t player2_time_ms = 120000;
// static int64_t last_update_time = 0;
// static const int INCREMENT_MS = 1000; // 1 second increment

// // LVGL objects
// static lv_obj_t *player1_label;
// static lv_obj_t *player2_label;
// static lv_obj_t *status_label;

// // Queue handle for button events
// static QueueHandle_t button_evt_queue;

// // Function declarations
// static void IRAM_ATTR button_isr_handler(void* arg);
// static void update_display(void);
// static void init_buttons(void);
// static void init_display(void);

// // Button ISR handler
// static void IRAM_ATTR button_isr_handler(void* arg) {
//     uint32_t gpio_num = (uint32_t)arg;
//     xQueueSendFromISR(button_evt_queue, &gpio_num, NULL);
// }

// // Initialize GPIO buttons with interrupts
// static void init_buttons(void) {
//     // Configure button GPIOs
//     gpio_config_t io_conf = {
//         .intr_type = GPIO_INTR_NEGEDGE,
//         .mode = GPIO_MODE_INPUT,
//         .pin_bit_mask = (1ULL << BUTTON_1_GPIO) | (1ULL << BUTTON_2_GPIO),
//         .pull_up_en = GPIO_PULLUP_ENABLE,
//     };
//     gpio_config(&io_conf);

//     // Create queue for button events
//     button_evt_queue = xQueueCreate(10, sizeof(uint32_t));

//     // Install GPIO ISR service
//     gpio_install_isr_service(0);

//     // Attach ISR handler to GPIOs
//     gpio_isr_handler_add(BUTTON_1_GPIO, button_isr_handler, (void*)BUTTON_1_GPIO);
//     gpio_isr_handler_add(BUTTON_2_GPIO, button_isr_handler, (void*)BUTTON_2_GPIO);
// }

// // Initialize LVGL display
// static void init_display(void) {
//     // Create main container
//     lv_obj_t *main_container = lv_obj_create(lv_scr_act());
//     lv_obj_set_size(main_container, LV_HOR_RES, LV_VER_RES);
//     lv_obj_set_flex_flow(main_container, LV_FLEX_FLOW_COLUMN);
//     lv_obj_set_flex_align(main_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

//     // Create labels
//     player1_label = lv_label_create(main_container);
//     lv_obj_set_style_text_font(player1_label, &lv_font_montserrat_48, 0);
    
//     status_label = lv_label_create(main_container);
//     lv_obj_set_style_text_font(status_label, &lv_font_montserrat_24, 0);
    
//     player2_label = lv_label_create(main_container);
//     lv_obj_set_style_text_font(player2_label, &lv_font_montserrat_48, 0);

//     // Initial display update
//     update_display();
// }

// // Update display with current times
// static void update_display(void) {
//     char buf[32];
    
//     // Format and display Player 1's time
//     int p1_minutes = player1_time_ms / 60000;
//     int p1_seconds = (player1_time_ms % 60000) / 1000;
//     snprintf(buf, sizeof(buf), "%02d:%02d", p1_minutes, p1_seconds);
//     lv_label_set_text(player1_label, buf);
    
//     // Format and display Player 2's time
//     int p2_minutes = player2_time_ms / 60000;
//     int p2_seconds = (player2_time_ms % 60000) / 1000;
//     snprintf(buf, sizeof(buf), "%02d:%02d", p2_minutes, p2_seconds);
//     lv_label_set_text(player2_label, buf);
    
//     // Update status label
//     const char *status_text;
//     switch (current_state) {
//         case TIMER_STOPPED:
//             status_text = "Press any button to start";
//             break;
//         case PLAYER_1_ACTIVE:
//             status_text = "Player 1's turn";
//             break;
//         case PLAYER_2_ACTIVE:
//             status_text = "Player 2's turn";
//             break;
//     }
//     lv_label_set_text(status_label, status_text);
// }

// // Main timer task
// void timer_task(void *pvParameters) {
//     uint32_t gpio_num;
//     int64_t current_time;
    
//     while (1) {
//         // Check for button press
//         if (xQueueReceive(button_evt_queue, &gpio_num, pdMS_TO_TICKS(10)) == pdTRUE) {
//             current_time = esp_timer_get_time() / 1000; // Convert to milliseconds
            
//             // Debounce check
//             if (current_time - last_update_time < DEBOUNCE_TIME) {
//                 continue;
//             }
//             last_update_time = current_time;
            
//             // Handle button press based on current state
//             switch (current_state) {
//                 case TIMER_STOPPED:
//                     if (gpio_num == BUTTON_1_GPIO) {
//                         current_state = PLAYER_2_ACTIVE;
//                     } else {
//                         current_state = PLAYER_1_ACTIVE;
//                     }
//                     break;
                    
//                 case PLAYER_1_ACTIVE:
//                     if (gpio_num == BUTTON_1_GPIO) {
//                         player1_time_ms += INCREMENT_MS;
//                         current_state = PLAYER_2_ACTIVE;
//                     }
//                     break;
                    
//                 case PLAYER_2_ACTIVE:
//                     if (gpio_num == BUTTON_2_GPIO) {
//                         player2_time_ms += INCREMENT_MS;
//                         current_state = PLAYER_1_ACTIVE;
//                     }
//                     break;
//             }
            
//             update_display();
//         }
        
//         // Update active player's time
//         if (current_state != TIMER_STOPPED) {
//             current_time = esp_timer_get_time() / 1000;
//             if (current_time - last_update_time >= 100) { // Update every 100ms
//                 if (current_state == PLAYER_1_ACTIVE) {
//                     player1_time_ms -= (current_time - last_update_time);
//                     if (player1_time_ms <= 0) {
//                         player1_time_ms = 0;
//                         current_state = TIMER_STOPPED;
//                         lv_label_set_text(status_label, "Player 2 wins!");
//                     }
//                 } else if (current_state == PLAYER_2_ACTIVE) {
//                     player2_time_ms -= (current_time - last_update_time);
//                     if (player2_time_ms <= 0) {
//                         player2_time_ms = 0;
//                         current_state = TIMER_STOPPED;
//                         lv_label_set_text(status_label, "Player 1 wins!");
//                     }
//                 }
//                 last_update_time = current_time;
//                 update_display();
//             }
//         }
        
//         vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to prevent task from hogging CPU
//     }
// }

// void app_main(void) {
//     // Initialize display first (assuming LVGL is properly initialized in your project)
//     init_display();
    
//     // Initialize buttons
//     init_buttons();
    
//     // Create timer task
//     xTaskCreate(timer_task, "timer_task", 4096, NULL, 5, NULL);
// }

