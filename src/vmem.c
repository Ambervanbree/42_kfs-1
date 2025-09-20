#include "vmem.h"
#include "paging.h"
#include "pmm.h"
#include "panic.h"
#include "kprintf.h"

// Virtual memory region for vmalloc
static uint32_t vmem_current = VMEM_START;
static uint32_t vmem_size = 0;

typedef struct vmem_block {
	size_t size;           // Requested size (what user asked for)
	size_t capacity;       // Actual capacity (page-aligned)
	int free;
	uint32_t magic;  // Magic number for double free detection
	struct vmem_block *next;
} vmem_block_t;

#define VMEM_MAGIC_ALLOCATED 0xDEADBEEF
#define VMEM_MAGIC_FREED     0xFEEED000

static vmem_block_t *vmem_list = 0;

void *vmalloc(size_t size)
{
	if (size == 0) return 0;
	
    // Align to 8 bytes
	if (size & 7) size = (size + 7) & ~7u;
	
	// Find free block or expand virtual region
	vmem_block_t *prev = 0;
	vmem_block_t *cur = vmem_list;
	
	while (cur) {
		if (cur->free && cur->size >= size) {
			// Split if large enough
			if (cur->size >= size + sizeof(vmem_block_t) + 16) {
				vmem_block_t *n = (vmem_block_t*)((uint8_t*)cur + sizeof(vmem_block_t) + size);
				n->size = cur->size - size - sizeof(vmem_block_t);
				n->free = 1;
				n->next = cur->next;
				cur->size = size;
				cur->next = n;
			}
			cur->free = 0;
			cur->magic = VMEM_MAGIC_ALLOCATED;
			return (uint8_t*)cur + sizeof(vmem_block_t);
		}
		prev = cur;
		cur = cur->next;
	}
	
	// Need to expand virtual region
	uint32_t needed_pages = (size + sizeof(vmem_block_t) + PAGE_SIZE - 1) / PAGE_SIZE;
	uint32_t new_vmem_end = vmem_current + needed_pages * PAGE_SIZE;
	
	// Check if expansion would exceed vmalloc region
	if (new_vmem_end > VMEM_END) {
		kpanic_fatal("vmalloc: would exceed vmalloc region\n");
	}
	
    // Map new pages
	for (uint32_t va = vmem_current; va < new_vmem_end; va += PAGE_SIZE) {
        void *phys = pmm_alloc_page();
        if (!phys) {
            kpanic_fatal("vmalloc: failed to allocate physical page\n");
        }
		if (vmm_map_page(va, (uint32_t)phys, PAGE_WRITE) != 0) {
			kpanic_fatal("vmalloc: failed to map page\n");
		}
	}
	
	// Create new block
	vmem_block_t *new_block = (vmem_block_t*)vmem_current;
	new_block->size = size;  // Store requested size
	new_block->capacity = needed_pages * PAGE_SIZE - sizeof(vmem_block_t);  // Store actual capacity
	new_block->free = 0;
	new_block->magic = VMEM_MAGIC_ALLOCATED;
	new_block->next = 0;
	
	if (prev) prev->next = new_block;
	else vmem_list = new_block;
	
	vmem_current = new_vmem_end;
	vmem_size += needed_pages * PAGE_SIZE;
	
	return (uint8_t*)new_block + sizeof(vmem_block_t);
}

void vfree(void *ptr)
{
	if (!ptr) return;
	
	vmem_block_t *blk = (vmem_block_t*)((uint8_t*)ptr - sizeof(vmem_block_t));
	
	// Check for double free
    if (blk->magic == VMEM_MAGIC_FREED) {
        kpanic_fatal("vfree: double free detected at %x\n", (uint32_t)ptr);
        return;
    }
	
	// Check for invalid magic number
    if (blk->magic != VMEM_MAGIC_ALLOCATED) {
        kpanic_fatal("vfree: invalid memory block at %x (magic: %x)\n", (uint32_t)ptr, blk->magic);
        return;
    }
	
	// Mark as freed
	blk->free = 1;
	blk->magic = VMEM_MAGIC_FREED;
	
	// Merge with adjacent free blocks if possible, to reduce fragmentation
	vmem_block_t *cur = vmem_list;
	while (cur && cur->next) {
		uint8_t *end_cur = (uint8_t*)cur + sizeof(vmem_block_t) + cur->size;
		if (cur->free && cur->next->free && end_cur == (uint8_t*)cur->next) {
			cur->size += sizeof(vmem_block_t) + cur->next->size;
			cur->next = cur->next->next;
		} else {
			cur = cur->next;
		}
	}
}

// Helper function to validate if a block is properly allocated
static int is_valid_allocated_block(vmem_block_t *blk)
{
	// Check if block is within virtual memory region
	uint32_t block_addr = (uint32_t)blk;
	if (block_addr < VMEM_START || block_addr >= vmem_current) {
		return 0; // Block is outside virtual memory region
	}
	
	// Check if block header alignment is at least 8 bytes (allocator guarantees 8-byte alignment)
	if (block_addr & 7) {
		return 0; // Block header is not 8-byte aligned
	}
	
	// Check if block is linked in the allocation list
	vmem_block_t *cur = vmem_list;
	while (cur) {
		if (cur == blk) {
			return 1; // Found in allocation list
		}
		cur = cur->next;
	}
	
	return 0; // Not found in allocation list
}

size_t vsize(void *ptr)
{
	if (!ptr) return 0;
	
	// Check if pointer is within virtual memory region
	uint32_t ptr_addr = (uint32_t)ptr;
	if (ptr_addr < VMEM_START || ptr_addr >= vmem_current) {
		kprintf("[ERROR] vsize: invalid pointer %x (outside vmalloc allocated region)\n", ptr_addr);
		return 0; // Pointer is outside virtual memory region
	}
	
	vmem_block_t *blk = (vmem_block_t*)((uint8_t*)ptr - sizeof(vmem_block_t));
	
	// Check if block is still allocated
	if (blk->magic != VMEM_MAGIC_ALLOCATED) {
		kprintf("[ERROR] vsize: pointer %x refers to non-allocated block (magic=%x)\n", ptr_addr, blk->magic);
		return 0; // Block is freed or invalid
	}
	
	// Additional validation: check if block is properly allocated
	if (!is_valid_allocated_block(blk)) {
		kprintf("[ERROR] vsize: pointer %x fails allocation validation\n", ptr_addr);
		return 0; // Block is not properly allocated
	}
	
	return blk->size;
}

void *vbrk(void *new_brk)
{
	if (new_brk == 0) {
		return (void*)vmem_current;
	}
	
	uint32_t new_addr = (uint32_t)new_brk;
	if (new_addr < VMEM_START || new_addr < vmem_current) {
		return (void*)-1; // Invalid break
	}
	
	// Expand virtual region if needed
	while (vmem_current < new_addr) {
		uint32_t va = vmem_current;
		void *phys = pmm_alloc_page();
		if (vmm_map_page(va, (uint32_t)phys, PAGE_WRITE) != 0) {
			kpanic_fatal("vbrk: failed to map page\n");
		}
		vmem_current += PAGE_SIZE;
		vmem_size += PAGE_SIZE;
	}
	
	return (void*)vmem_current;
}