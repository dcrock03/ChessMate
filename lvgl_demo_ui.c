#include "lvgl.h"

static lv_obj_t *main_screen;
static lv_obj_t *menu_list;
static lv_obj_t *timer_panel;
static lv_obj_t *timer1_label;
static lv_obj_t *timer2_label;
static lv_obj_t *active_player_indicator;

// Function prototypes
void create_main_screen(lv_disp_t *disp);
void update_menu(const char **items, int item_count, int selected_index);
void update_timers(int player1_time, int player2_time, int active_player);

void example_lvgl_demo_ui(lv_disp_t *disp)
{
    create_main_screen(disp);
}

void create_main_screen(lv_disp_t *disp)
{
    main_screen = lv_obj_create(NULL);
    lv_disp_load_scr(main_screen);

    // Create a nice background
    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x2c3e50), 0);
    lv_obj_set_style_bg_opa(main_screen, LV_OPA_COVER, 0);

    // Create a container for the menu
    lv_obj_t *menu_cont = lv_obj_create(main_screen);
    lv_obj_set_size(menu_cont, lv_pct(100), lv_pct(60));
    lv_obj_align(menu_cont, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_bg_color(menu_cont, lv_color_hex(0x34495e), 0);
    lv_obj_set_style_bg_opa(menu_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(menu_cont, 0, 0);
    lv_obj_set_style_radius(menu_cont, 10, 0);

    // Create the menu list
    menu_list = lv_list_create(menu_cont);
    lv_obj_set_size(menu_list, lv_pct(90), lv_pct(90));
    lv_obj_center(menu_list);
    lv_obj_set_style_bg_color(menu_list, lv_color_hex(0x34495e), 0);
    lv_obj_set_style_bg_opa(menu_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(menu_list, 0, 0);

    // Create a container for the timers
    timer_panel = lv_obj_create(main_screen);
    lv_obj_set_size(timer_panel, lv_pct(100), lv_pct(35));
    lv_obj_align(timer_panel, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(timer_panel, lv_color_hex(0x2980b9), 0);
    lv_obj_set_style_bg_opa(timer_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(timer_panel, 0, 0);
    lv_obj_set_style_radius(timer_panel, 10, 0);

    // Create labels for player timers
    timer1_label = lv_label_create(timer_panel);
    lv_obj_align(timer1_label, LV_ALIGN_LEFT_MID, 20, 0);
    lv_obj_set_style_text_color(timer1_label, lv_color_hex(0xecf0f1), 0);
    lv_obj_set_style_text_font(timer1_label, &lv_font_montserrat_28, 0);

    timer2_label = lv_label_create(timer_panel);
    lv_obj_align(timer2_label, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_set_style_text_color(timer2_label, lv_color_hex(0xecf0f1), 0);
    lv_obj_set_style_text_font(timer2_label, &lv_font_montserrat_28, 0);

    // Create an active player indicator
    active_player_indicator = lv_obj_create(timer_panel);
    lv_obj_set_size(active_player_indicator, 20, 20);
    lv_obj_align(active_player_indicator, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_bg_color(active_player_indicator, lv_color_hex(0x27ae60), 0);
    lv_obj_set_style_radius(active_player_indicator, LV_RADIUS_CIRCLE, 0);
}

void update_menu(const char **items, int item_count, int selected_index)
{
    lv_obj_clean(menu_list);

    for (int i = 0; i < item_count; i++) {
        lv_obj_t *btn = lv_list_add_btn(menu_list, NULL, items[i]);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x34495e), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(btn, lv_color_hex(0xecf0f1), 0);
        
        if (i == selected_index) {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x3498db), 0);
        }
    }
}

void update_timers(int player1_time, int player2_time, int active_player)
{
    char timer1_text[32], timer2_text[32];
    snprintf(timer1_text, sizeof(timer1_text), "P1: %02d:%02d", player1_time / 60, player1_time % 60);
    snprintf(timer2_text, sizeof(timer2_text), "P2: %02d:%02d", player2_time / 60, player2_time % 60);
    
    lv_label_set_text(timer1_label, timer1_text);
    lv_label_set_text(timer2_label, timer2_text);

    if (active_player == 1) {
        lv_obj_align(active_player_indicator, LV_ALIGN_LEFT_MID, 10, 0);
    } else if (active_player == 2) {
        lv_obj_align(active_player_indicator, LV_ALIGN_RIGHT_MID, -10, 0);
    } else {
        lv_obj_align(active_player_indicator, LV_ALIGN_TOP_MID, 0, 10);
    }
}

// Add these function declarations to your main file (spi_lcd_touch_example_main.c)
extern void update_menu(const char **items, int item_count, int selected_index);
extern void update_timers(int player1_time, int player2_time, int active_player);
