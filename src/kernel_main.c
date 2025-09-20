#include "kernel.h"
#include "screen.h"
#include "string.h"
#include "kprintf.h"
#include "keyboard.h"
#include "shell.h"
#include "panic.h"
#include "pmm.h"
#include "paging.h"
#include "kheap.h"

// External symbols from GDT
extern void *gdt;
extern void *gdt_end;
extern void gdt_setup_at_required_address(void);

// Function to parse multiboot information and return memory size
uint32_t parse_multiboot_info(uint32_t magic, uint32_t *multiboot_info)
{
    // Check if we have valid multiboot information
    if (magic != MULTIBOOT_MAGIC) {
        kprintf("Invalid multiboot magic: 0x%x (expected 0x%x)\n", magic, MULTIBOOT_MAGIC);
        kprintf("Falling back to 10MB default memory\n");
        return 10 * 1024 * 1024; // 10MB fallback
    }
    
    if (!multiboot_info) {
        kprintf("No multiboot info provided\n");
        kprintf("Falling back to 10MB default memory\n");
        return 10 * 1024 * 1024; // 10MB fallback
    }
    
    struct multiboot_info *mb_info = (struct multiboot_info *)multiboot_info;
    
    // Check if memory information is available
    if (!(mb_info->flags & 0x01)) {
        kprintf("No memory information available in multiboot info\n");
        kprintf("Falling back to 10MB default memory\n");
        return 10 * 1024 * 1024; // 10MB fallback
    }
    
    // Calculate total memory from mem_lower and mem_upper
    // mem_lower: KB of memory from 0 to 1MB
    // mem_upper: KB of memory from 1MB to 16MB
    uint32_t mem_lower_kb = mb_info->mem_lower;
    uint32_t mem_upper_kb = mb_info->mem_upper;
    
    // Convert to bytes
    uint32_t mem_lower_bytes = mem_lower_kb * 1024;
    uint32_t mem_upper_bytes = mem_upper_kb * 1024;
    uint32_t total_memory = mem_lower_bytes + mem_upper_bytes;
    
    kprintf("Multiboot memory info:\n");
    kprintf("  mem_lower: %d KB (0x%x bytes)\n", mem_lower_kb, mem_lower_bytes);
    kprintf("  mem_upper: %d KB (0x%x bytes)\n", mem_upper_kb, mem_upper_bytes);
    kprintf("  total: %d KB (0x%x bytes)\n", (total_memory / 1024), total_memory);
    
    // Cap memory at reasonable limit (1GB) to prevent issues
    if (total_memory > 1024 * 1024 * 1024) {
        kprintf("Memory size too large (%d MB), capping at 1GB\n", total_memory / (1024 * 1024));
        total_memory = 1024 * 1024 * 1024;
    }
    
    // Ensure minimum memory
    if (total_memory < 1024 * 1024) {
        kprintf("Memory size too small (%d KB), using 1MB minimum\n", total_memory / 1024);
        total_memory = 1024 * 1024;
    }
    
    return total_memory;
}

void kernel_main(uint32_t magic, uint32_t *multiboot_info) 
{
    // Copy GDT to required address 0x00000800
    gdt_setup_at_required_address();
    
    screen_init();
    keyboard_init();
    interrupt_init();
    
    // Parse multiboot information to get actual memory size
    uint32_t mem_size = parse_multiboot_info(magic, multiboot_info);
    
    // Initialize memory with detected size
    memory_init(mem_size);

    // Display GDT info
    // kprintf("GDT relocated to %x\n", 0x00000800);
    // kprintf("Kernel segments: Code=0x08, Data=0x10, Stack=0x18\n");
    // kprintf("User segments: Code=0x20, Data=0x28, Stack=0x30\n\n");
    
    // Initialize and start the shell
    shell_init();
    
    while (1) {
        // halt and wait for interrupts
        asm volatile("hlt");
    }
}
    