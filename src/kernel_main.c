#include "print.h"

int kernel_main(void) {
    for(int i = 0; i < 1000; i++) {
        kprint("Hello Wen\n");
        kprint("How are you?");
    }

    while (1) {};
    return 0;
}