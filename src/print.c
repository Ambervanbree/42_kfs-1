#include "print.h"

#define VGA_ADDRESS 0xB8000
#define VGA_WIDTH 80

static int cursor = 0;

void kprint(const char* str) {
    volatile char* vga = (volatile char*) VGA_ADDRESS;
    
    for (size_t i = 0; str[i] != '\0'; ++i){
        vga[cursor * 2] = str[i];
        vga[cursor * 2 + 1] = 0x07;
        cursor++;
        if (cursor >= VGA_WIDTH * 25){
            cursor = 0;
        }
    }
}