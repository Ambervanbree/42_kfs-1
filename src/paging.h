#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>
#include "pmm.h"

#define PAGE_PRESENT   0x001
#define PAGE_WRITE     0x002
#define PAGE_USER      0x004

typedef uint32_t page_entry_t;

void paging_init(void);
void paging_enable(void);

// Map/unmap single page
int  vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags);
void vmm_unmap_page(uint32_t virt);
uint32_t vmm_get_mapping(uint32_t virt);

// Internal paging functions
uint32_t *virt_to_pte(uint32_t virt, int create);

#endif
