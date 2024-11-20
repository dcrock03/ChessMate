#include "lvgl_demo_ui.h"
#include <stdio.h>
#include "esp_log.h"

static const char *TAG = "lvgl_demo_ui";

static lv_obj_t *main_screen;
static lv_obj_t *menu_cont;
static lv_obj_t *timer1_label;
static lv_obj_t *timer2_label;
static lv_obj_t *msg_label;
static lv_timer_t *msg_timer = NULL;
static lv_style_t style_msg_bg;

// Timer callback for message hiding
static void msg_timer_cb(lv_timer_t *timer)
{
    lv_obj_t *msg_bg = timer->user_data;
    
    // Hide both the background and the message label
    lv_obj_add_flag(msg_bg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lv_obj_get_child(msg_bg, 0), LV_OBJ_FLAG_HIDDEN);
    
    // Delete the timer
    if (msg_timer) {
        lv_timer_del(msg_timer);
        msg_timer = NULL;
    }
}

void example_lvgl_demo_ui(lv_disp_t *disp)
{
    // Get the current screen
    main_screen = lv_disp_get_scr_act(disp);
    
    // Set screen background to black
    lv_obj_set_style_bg_color(main_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(main_screen, LV_OPA_COVER, 0);

    // Create left side menu container (65% of screen width)
    menu_cont = lv_obj_create(main_screen);
    lv_obj_set_size(menu_cont, lv_pct(65), lv_pct(100));
    lv_obj_align(menu_cont, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(menu_cont, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(menu_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(menu_cont, 0, 0);
    lv_obj_set_style_pad_all(menu_cont, 15, 0);
    lv_obj_clear_flag(menu_cont, LV_OBJ_FLAG_SCROLLABLE);

    // Create right side container (35% of screen width)
    lv_obj_t *right_cont = lv_obj_create(main_screen);
    lv_obj_set_size(right_cont, lv_pct(35), lv_pct(100));
    lv_obj_align(right_cont, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(right_cont, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(right_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(right_cont, 0, 0);
    lv_obj_set_style_pad_all(right_cont, 10, 0);

    // Create timer labels in the right container
    timer1_label = lv_label_create(right_cont);
    lv_obj_align(timer1_label, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_text_color(timer1_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(timer1_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(timer1_label, "P1: 10:00");

    timer2_label = lv_label_create(right_cont);
    lv_obj_align(timer2_label, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_text_color(timer2_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(timer2_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(timer2_label, "P2: 10:00");

    // Initialize message background style
    lv_style_init(&style_msg_bg);
    lv_style_set_bg_color(&style_msg_bg, lv_color_make(40, 40, 40));
    lv_style_set_bg_opa(&style_msg_bg, LV_OPA_70);
    lv_style_set_pad_all(&style_msg_bg, 4);
    lv_style_set_radius(&style_msg_bg, 2);
    lv_style_set_max_width(&style_msg_bg, 100);
    lv_style_set_text_align(&style_msg_bg, LV_TEXT_ALIGN_CENTER);

    // Create message background container
    lv_obj_t *msg_bg = lv_obj_create(menu_cont);
    lv_obj_add_style(msg_bg, &style_msg_bg, 0);
    lv_obj_align(msg_bg, LV_ALIGN_TOP_LEFT, 10, 5);
    lv_obj_set_size(msg_bg, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_max_width(msg_bg, 100, 0);
    lv_obj_add_flag(msg_bg, LV_OBJ_FLAG_HIDDEN);

    // Create message label
    msg_label = lv_label_create(msg_bg);
    lv_obj_set_style_text_color(msg_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(msg_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(msg_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(msg_label, LV_SIZE_CONTENT);
    lv_obj_set_style_max_width(msg_label, 90, 0);
    lv_label_set_long_mode(msg_label, LV_LABEL_LONG_WRAP);
    lv_obj_center(msg_label);
    lv_obj_add_flag(msg_label, LV_OBJ_FLAG_HIDDEN);
}

void update_menu(const char **items, int item_count, int selected_index)
{
    ESP_LOGI(TAG, "Updating menu with %d items, selected: %d", item_count, selected_index);
    
    // Clear existing menu items
    lv_obj_clean(menu_cont);
    
    // Create new menu items
    for (int i = 0; i < item_count; i++) {
        lv_obj_t *item = lv_label_create(menu_cont);
        
        // Configure label
        lv_label_set_text(item, items[i]);
        lv_obj_set_width(item, lv_pct(90));  // Set width to 90% of container
        
        // Position label - increased vertical spacing for better readability
        lv_obj_align(item, LV_ALIGN_TOP_LEFT, 20, 20 + i * 40);
        
        // Set text style
        lv_obj_set_style_text_font(item, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_opa(item, LV_OPA_COVER, 0);
        
        // Set color based on selection
        if (i == selected_index) {
            lv_obj_set_style_text_color(item, lv_color_make(255, 0, 0), 0);  // Red for selected
        } else {
            lv_obj_set_style_text_color(item, lv_color_make(255, 255, 255), 0);  // White for others
        }
        
        ESP_LOGI(TAG, "Created menu item: %s", items[i]);
    }
    
    // Force an immediate refresh
    lv_refr_now(NULL);
}

void update_timers(int player1_time, int player2_time, int active_player)
{
    char timer1_text[32], timer2_text[32];
    snprintf(timer1_text, sizeof(timer1_text), "P1: %02d:%02d", player1_time / 60, player1_time % 60);
    snprintf(timer2_text, sizeof(timer2_text), "P2: %02d:%02d", player2_time / 60, player2_time % 60);
    
    lv_label_set_text(timer1_label, timer1_text);
    lv_label_set_text(timer2_label, timer2_text);

    // Update colors based on active player
    lv_obj_set_style_text_color(timer1_label, 
        active_player == 1 ? lv_color_make(255, 0, 0) : lv_color_make(255, 255, 255), 0);
    lv_obj_set_style_text_color(timer2_label, 
        active_player == 2 ? lv_color_make(255, 0, 0) : lv_color_make(255, 255, 255), 0);
}

void display_message(const char *message)
{
    // Get message background (parent of msg_label)
    lv_obj_t *msg_bg = lv_obj_get_parent(msg_label);
    
    // Update message text
    lv_label_set_text(msg_label, message);
    
    // Ensure message and background are properly sized
    lv_obj_set_size(msg_bg, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_center(msg_label);
    
    // Show message and background
    lv_obj_clear_flag(msg_bg, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(msg_label, LV_OBJ_FLAG_HIDDEN);
    
    // Delete any existing timer
    if (msg_timer) {
        lv_timer_del(msg_timer);
        msg_timer = NULL;
    }
    
    // Create new timer to hide the message after 3 seconds
    msg_timer = lv_timer_create(msg_timer_cb, 3000, msg_bg);
}
