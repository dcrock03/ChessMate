// menu_data.c
#include "menu_data.h"
#include "esp_log.h"

static const char *TAG = "menu_data";

// Global variables
int timer_running = 0;
int active_player = 0;
int player1_time = 600;  // 10 minutes default
int player2_time = 600;  // 10 minutes default

// Forward declarations of submenus with static keyword
static MenuItem bullet_submenu[] = {
    {"1 minute", NULL, 0, set_bullet_1min},
    {"1 | 1", NULL, 0, set_bullet_1_1},
    {"2 | 1", NULL, 0, set_bullet_2_1},
    {"Back", NULL, 0, NULL}
};

static MenuItem blitz_submenu[] = {
    {"3 minute", NULL, 0, set_blitz_3min},
    {"3 | 2", NULL, 0, set_blitz_3_2},
    {"5 minute", NULL, 0, set_blitz_5min},
    {"Back", NULL, 0, NULL}
};

static MenuItem rapid_submenu[] = {
    {"10 minute", NULL, 0, set_rapid_10min},
    {"15 | 10", NULL, 0, set_rapid_15_10},
    {"30 minute", NULL, 0, set_rapid_30min},
    {"Back", NULL, 0, NULL}
};

static MenuItem timer_submenu[] = {
    {"Bullet", bullet_submenu, 4, NULL},
    {"Blitz", blitz_submenu, 4, NULL},
    {"Rapid", rapid_submenu, 4, NULL},
    {"Back", NULL, 0, NULL}
};

static MenuItem assist_submenu[] = {
    {"Low", NULL, 0, set_assist_low},
    {"High", NULL, 0, set_assist_high},
    {"Back", NULL, 0, NULL}
};

static MenuItem brightness_submenu[] = {
    {"Low", NULL, 0, set_brightness_low},
    {"Med", NULL, 0, set_brightness_med},
    {"High", NULL, 0, set_brightness_high},
    {"Back", NULL, 0, NULL}
};

static MenuItem options_submenu[] = {
    {"Level of assistance", assist_submenu, 3, NULL},
    {"LED brightness", brightness_submenu, 4, NULL},
    {"Back", NULL, 0, NULL}
};

static MenuItem game_config_submenu[] = {
    {"Player Select", NULL, 0, show_player_select},
    {"Timer", timer_submenu, 4, NULL},
    {"Back", NULL, 0, NULL}
};

// Main menu (non-static since it needs to be accessed from other files)
MenuItem main_menu[] = {
    {"Game Start", NULL, 0, start_game},
    {"Options", options_submenu, 3, NULL},
    {"Game Config", game_config_submenu, 3, NULL},
    {"Game Stop", NULL, 0, stop_game}
};
const int main_menu_size = sizeof(main_menu) / sizeof(MenuItem);

// Function implementations
void start_game(void) {
    ESP_LOGI(TAG, "Game Started");
    timer_running = 1;
    active_player = 1;  // Start with Player 1
    // Reset both timers to their initial values when starting a new game
    player1_time = 600;  // 10 minutes default, or whatever time was set in the menu
    player2_time = 600;  // 10 minutes default, or whatever time was set in the menu
    update_timers(player1_time, player2_time, active_player);
    display_message("Game Started - Player 1's Turn!");
}

void stop_game(void) {
    ESP_LOGI(TAG, "Game Stopped");
    timer_running = 0;
    active_player = 0;
    player1_time = 600;
    player2_time = 600;
    update_timers(player1_time, player2_time, active_player);
    display_message("Game Stopped!");
}

void set_assist_low(void) {
    ESP_LOGI(TAG, "Assist Level: Low");
    display_message("Assist Level: Low");
}

void set_assist_high(void) {
    ESP_LOGI(TAG, "Assist Level: High");
    display_message("Assist Level: High");
}

void set_brightness_low(void) {
    ESP_LOGI(TAG, "Brightness: Low");
    display_message("Brightness: Low");
}

void set_brightness_med(void) {
    ESP_LOGI(TAG, "Brightness: Medium");
    display_message("Brightness: Medium");
}

void set_brightness_high(void) {
    ESP_LOGI(TAG, "Brightness: High");
    display_message("Brightness: High");
}

void show_player_select(void) {
    ESP_LOGI(TAG, "Player Select Screen");
    display_message("Select Players");
}

void set_bullet_1min(void) {
    ESP_LOGI(TAG, "Timer: 1 minute bullet");
    player1_time = 60;
    player2_time = 60;
    update_timers(player1_time, player2_time, active_player);
    display_message("1 min bullet selected");
}

void set_bullet_1_1(void) {
    ESP_LOGI(TAG, "Timer: 1|1 bullet");
    player1_time = 60;
    player2_time = 60;
    update_timers(player1_time, player2_time, active_player);
    display_message("1|1 bullet selected");
}

void set_bullet_2_1(void) {
    ESP_LOGI(TAG, "Timer: 2|1 bullet");
    player1_time = 120;
    player2_time = 120;
    update_timers(player1_time, player2_time, active_player);
    display_message("2|1 bullet selected");
}

void set_blitz_3min(void) {
    ESP_LOGI(TAG, "Timer: 3 minute blitz");
    player1_time = 180;
    player2_time = 180;
    update_timers(player1_time, player2_time, active_player);
    display_message("3 min blitz selected");
}

void set_blitz_3_2(void) {
    ESP_LOGI(TAG, "Timer: 3|2 blitz");
    player1_time = 180;
    player2_time = 180;
    update_timers(player1_time, player2_time, active_player);
    display_message("3|2 blitz selected");
}

void set_blitz_5min(void) {
    ESP_LOGI(TAG, "Timer: 5 minute blitz");
    player1_time = 300;
    player2_time = 300;
    update_timers(player1_time, player2_time, active_player);
    display_message("5 min blitz selected");
}

void set_rapid_10min(void) {
    ESP_LOGI(TAG, "Timer: 10 minute rapid");
    player1_time = 600;
    player2_time = 600;
    update_timers(player1_time, player2_time, active_player);
    display_message("10 min rapid selected");
}

void set_rapid_15_10(void) {
    ESP_LOGI(TAG, "Timer: 15|10 rapid");
    player1_time = 900;
    player2_time = 900;
    update_timers(player1_time, player2_time, active_player);
    display_message("15|10 rapid selected");
}

void set_rapid_30min(void) {
    ESP_LOGI(TAG, "Timer: 30 minute rapid");
    player1_time = 1800;
    player2_time = 1800;
    update_timers(player1_time, player2_time, active_player);
    display_message("30 min rapid selected");
}