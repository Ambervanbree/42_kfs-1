#ifndef KHEAP_H
#define KHEAP_H

#include <stddef.h>
#include <stdint.h>

void kheap_init(void);
void *kmalloc(size_t size);
void kfree(void *ptr);
size_t ksize(void *ptr);

#endif
