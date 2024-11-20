#ifndef LVGL_DEMO_UI_H
#define LVGL_DEMO_UI_H

#include "lvgl.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

// Function declarations
void example_lvgl_demo_ui(lv_disp_t *disp);
void update_menu(const char **items, int item_count, int selected_index);
void update_timers(int player1_time, int player2_time, int active_player);
void display_message(const char *message);

#ifdef __cplusplus
}
#endif

#endif // LVGL_DEMO_UI_H
