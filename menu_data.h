// menu_data.h
#ifndef MENU_DATA_H
#define MENU_DATA_H

#include <stddef.h>

// Forward declaration of display_message function
void display_message(const char* message);
void update_timers(int player1_time, int player2_time, int active_player);

// Global variables declarations
extern int timer_running;
extern int active_player;
extern int player1_time;
extern int player2_time;

typedef struct MenuItem {
    const char* name;
    struct MenuItem* submenu;
    int submenu_size;
    void (*action)(void);
} MenuItem;

// Function declarations for menu actions
void start_game(void);
void stop_game(void);
void set_assist_low(void);
void set_assist_high(void);
void set_brightness_low(void);
void set_brightness_med(void);
void set_brightness_high(void);
void show_player_select(void);
void set_bullet_1min(void);
void set_bullet_1_1(void);
void set_bullet_2_1(void);
void set_blitz_3min(void);
void set_blitz_3_2(void);
void set_blitz_5min(void);
void set_rapid_10min(void);
void set_rapid_15_10(void);
void set_rapid_30min(void);

// Only export main menu
extern MenuItem main_menu[];
extern const int main_menu_size;

#endif // MENU_DATA_H