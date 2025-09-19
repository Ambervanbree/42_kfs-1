#ifndef PANIC_H
#define PANIC_H

#include "kprintf.h"

// Fatal panic: print message and halt the CPU forever
void kpanic_fatal(const char *fmt, ...);
#endif
