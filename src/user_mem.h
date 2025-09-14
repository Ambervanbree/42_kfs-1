#ifndef USER_MEM_H
#define USER_MEM_H

#include <stddef.h>
#include <stdint.h>

// User space memory management
void *umalloc(size_t size);
void ufree(void *ptr);
size_t usize(void *ptr);

// User space page management
int vmm_map_user_page(uint32_t virt, uint32_t phys, uint32_t flags);
void vmm_unmap_user_page(uint32_t virt);
uint32_t vmm_get_user_mapping(uint32_t virt);

// User space memory region management
void user_mem_init(void);
void *user_mem_alloc_page(void);
void user_mem_free_page(void *page);

// User space memory limits
#define USER_SPACE_START  0x00000000
#define USER_SPACE_END    0xBFFFFFFF
#define USER_HEAP_START   0x08000000  // 128MB virtual start for user heap
#define USER_STACK_START  0xB0000000  // 3GB virtual start for user stack
#define USER_STACK_SIZE   0x10000000  // 256MB stack size

#endif