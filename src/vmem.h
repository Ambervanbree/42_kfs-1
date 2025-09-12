#ifndef VMEM_H
#define VMEM_H

#include <stddef.h>
#include <stdint.h>

// Virtual memory helpers
void *vmalloc(size_t size);
void vfree(void *ptr);
size_t vsize(void *ptr);
void *vbrk(void *new_brk);

#endif