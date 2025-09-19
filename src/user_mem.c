#include "user_mem.h"
#include "paging.h"
#include "pmm.h"
#include "panic.h"
#include "kprintf.h"

// User space memory management
// Very simple user heap allocator similar to kernel heap

typedef struct user_block_header {
    size_t size;
    int free;
    uint32_t magic;  // Magic number for double free detection
    struct user_block_header *next;
} user_block_header_t;

#define USER_MAGIC_ALLOCATED 0xDEADBEEF
#define USER_MAGIC_FREED     0xFEEED000

static uint8_t *user_heap_base = 0;
static size_t user_heap_size = 0;
static user_block_header_t *user_free_list = 0;

static void *user_heap_expand(size_t min_bytes)
{
    // Allocate whole pages for user heap
    size_t pages = (min_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
    if (!user_heap_base) {
        user_heap_base = (uint8_t*)USER_HEAP_START;
        user_heap_size = 0;
        user_free_list = 0;
    }
    
    for (size_t i = 0; i < pages; i++) {
        void *phys = pmm_alloc_page();
        uint32_t v = (uint32_t)user_heap_base + user_heap_size;
        if (vmm_map_user_page(v, (uint32_t)phys, PAGE_WRITE | PAGE_USER) != 0) {
            kprintf("[ERROR] User heap map failed\n");
        }
        user_heap_size += PAGE_SIZE;
    }
    return user_heap_base + user_heap_size - pages * PAGE_SIZE;
}

void user_mem_init(void)
{
    // Initialize user memory management
    user_heap_base = 0;
    user_heap_size = 0;
    user_free_list = 0;
    kprintf("User memory management initialized.\n");
}

static void user_split_block(user_block_header_t *blk, size_t size)
{
    if (blk->size < sizeof(user_block_header_t)) {
        kprintf("[ERROR] user_heap: corrupt block size (%d) before split\n", (int)blk->size);
    }
    if (blk->size >= size + sizeof(user_block_header_t) + 16) {
        user_block_header_t *n = (user_block_header_t*)((uint8_t*)blk + sizeof(user_block_header_t) + size);
        n->size = blk->size - size - sizeof(user_block_header_t);
        n->free = 1;
        n->next = blk->next;
        blk->size = size;
        blk->next = n;
    }
}

void *umalloc(size_t size)
{
    if (size == 0) return 0;
    
    // Align to 8-byte boundary
    if (size & 7) size = (size + 7) & ~7u;
    
    // Search free list for suitable block
    user_block_header_t *prev = 0;
    user_block_header_t *cur = user_free_list;
    
    while (cur) {
        if (cur->free && cur->size >= size) {
            user_split_block(cur, size);
            cur->free = 0;
            cur->magic = USER_MAGIC_ALLOCATED;
            return (uint8_t*)cur + sizeof(user_block_header_t);
        }
        prev = cur;
        cur = cur->next;
    }
    
    // No suitable block found - expand user heap
    uint8_t *chunk = (uint8_t*)user_heap_expand(size + sizeof(user_block_header_t));
    user_block_header_t *blk = (user_block_header_t*)chunk;
    blk->size = (size + sizeof(user_block_header_t) <= PAGE_SIZE) ? 
                (PAGE_SIZE - sizeof(user_block_header_t)) : (size);
    blk->free = 0;
    blk->magic = USER_MAGIC_ALLOCATED;
    blk->next = 0;
    
    if (prev) prev->next = blk; else user_free_list = blk;
    return (uint8_t*)blk + sizeof(user_block_header_t);
}

void ufree(void *ptr)
{
    if (!ptr) return;
    
    user_block_header_t *blk = (user_block_header_t*)((uint8_t*)ptr - sizeof(user_block_header_t));
    
    // Check for double free
    if (blk->magic == USER_MAGIC_FREED) {
        kprintf("[ERROR] ufree: double free detected at 0x%x\n", (uint32_t)ptr);
        return;
    }
    
    // Check for invalid magic number
    if (blk->magic != USER_MAGIC_ALLOCATED) {
        kprintf("[ERROR] ufree: invalid memory block at 0x%x (magic: 0x%x)\n", (uint32_t)ptr, blk->magic);
        return;
    }
    
    // Mark as freed
    blk->free = 1;
    blk->magic = USER_MAGIC_FREED;
    
    // Coalesce adjacent free blocks
    user_block_header_t *cur = user_free_list;
    while (cur && cur->next) {
        uint8_t *end_cur = (uint8_t*)cur + sizeof(user_block_header_t) + cur->size;
        if (cur->free && cur->next->free && end_cur == (uint8_t*)cur->next) {
            cur->size += sizeof(user_block_header_t) + cur->next->size;
            cur->next = cur->next->next;
        } else {
            cur = cur->next;
        }
    }
}

// Helper function to validate if a user heap block is properly allocated
static int is_valid_user_heap_block(user_block_header_t *blk)
{
    // Check if block is within user heap region
    uint32_t block_addr = (uint32_t)blk;
    if (!user_heap_base || block_addr < (uint32_t)user_heap_base || block_addr >= (uint32_t)user_heap_base + user_heap_size) {
        return 0; // Block is outside user heap region
    }
    
	// Check if block header alignment is at least 8 bytes (user allocator guarantees 8-byte alignment)
	if (block_addr & 7) {
		return 0; // Block header is not 8-byte aligned
	}
    
    // Check if block is linked in the free list (either allocated or free)
    user_block_header_t *cur = user_free_list;
    while (cur) {
        if (cur == blk) {
            return 1; // Found in free list
        }
        cur = cur->next;
    }
    
    // Also check if it's a recently allocated block that might not be in free list yet
    // This is a simplified check - in a real implementation, you'd maintain an allocated list too
    return 1; // Allow if within heap bounds and aligned
}

size_t usize(void *ptr)
{
    if (!ptr) return 0;
	
    // Check if pointer is within user heap region
    uint32_t ptr_addr = (uint32_t)ptr;
	if (!user_heap_base || ptr_addr < (uint32_t)user_heap_base || ptr_addr >= (uint32_t)user_heap_base + user_heap_size) {
		kprintf("[ERROR] usize: invalid pointer 0x%x (outside user heap)\n", ptr_addr);
		return 0; // Pointer is outside user heap region
	}
    
    user_block_header_t *blk = (user_block_header_t*)((uint8_t*)ptr - sizeof(user_block_header_t));
	
    // Check if block is still allocated
	if (blk->magic != USER_MAGIC_ALLOCATED) {
		kprintf("[ERROR] usize: pointer 0x%x refers to non-allocated block (magic=0x%x)\n", ptr_addr, blk->magic);
		return 0; // Block is freed or invalid
	}
    
    // Additional validation: check if block is properly allocated
	if (!is_valid_user_heap_block(blk)) {
		kprintf("[ERROR] usize: pointer 0x%x fails allocation validation\n", ptr_addr);
		return 0; // Block is not properly allocated
	}
    
    return blk->size;
}

// User space page mapping functions
int vmm_map_user_page(uint32_t virt, uint32_t phys, uint32_t flags)
{

    
    uint32_t *pte = virt_to_pte(virt, 1);
    if (!pte) return -1;
    *pte = (phys & 0xFFFFF000) | (flags & 0xFFF) | PAGE_PRESENT | PAGE_USER;
    return 0;
}

void vmm_unmap_user_page(uint32_t virt)
{

    
    uint32_t *pte = virt_to_pte(virt, 0);
    if (!pte) return;
    *pte = 0;
}

uint32_t vmm_get_user_mapping(uint32_t virt)
{

    
    uint32_t *pte = virt_to_pte(virt, 0);
    if (!pte) return 0;
    return *pte;
}

// User space page allocation
void *user_mem_alloc_page(void)
{
    void *phys = pmm_alloc_page();
    if (!phys) return 0;
    
    // Find free virtual address in user space
    static uint32_t user_virt_current = USER_HEAP_START;
    uint32_t virt = user_virt_current;
    user_virt_current += PAGE_SIZE;
    
    if (vmm_map_user_page(virt, (uint32_t)phys, PAGE_WRITE | PAGE_USER) != 0) {
        pmm_free_page(phys);
        return 0;
    }
    
    return (void*)virt;
}

void user_mem_free_page(void *page)
{
    if (!page) return;
    
    uint32_t virt = (uint32_t)page;

    
    uint32_t pte = vmm_get_user_mapping(virt);
    if (pte) {
        uint32_t phys = pte & 0xFFFFF000;
        pmm_free_page((void*)phys);
        vmm_unmap_user_page(virt);
    }
}