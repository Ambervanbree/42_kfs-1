#ifndef PMM_H
#define PMM_H

#include "kernel.h"
#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096
#define PMM_START 0x00100000u
#define PMM_MAX_BYTES (10u * 1024u * 1024u)  // 10MB maximum

void pmm_init(uint32_t mem_size_bytes);
void *pmm_alloc_page(void);
void pmm_free_page(void *page);
uint32_t pmm_free_pages(void);
uint32_t pmm_total_pages(void);
void *pmm_brk(void *new_brk);

#endif
