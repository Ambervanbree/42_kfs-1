#ifndef SCREEN_H
#define SCREEN_H

#include "kernel.h"


#define SCREEN_WIDTH  80
#define SCREEN_HEIGHT 25
#define SCREEN_SIZE (SCREEN_WIDTH*SCREEN_HEIGHT*2) //Byte 1: The ASCII character, Byte 2: The attribute byte (color information)
#define MAX_SCREENS 3
#define TAB_WIDTH 4

struct screen_state {
    uint8_t buffer[SCREEN_SIZE];
    size_t cursor_x;
    size_t cursor_y;
    uint8_t color;
};

enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};


void screen_init(void);
void init_screen_if_needed(int n);
void load_home_screen();
void screen_clear(void);
void screen_putchar(char c);
void screen_putstring(const char* str);
void screen_set_color(enum vga_color fg, enum vga_color bg);
void screen_set_cursor(size_t x, size_t y);
void screen_get_cursor(size_t* x, size_t* y);
void screen_scroll(void);
void switch_screen(int n);

#endif 