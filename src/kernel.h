#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stddef.h>

struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint32_t vbe_mode;
    uint32_t vbe_interface_seg;
    uint32_t vbe_interface_off;
    uint32_t vbe_interface_len;
};

#define MULTIBOOT_MAGIC 0x2BADB002
#define BOOTLOADER "GRUB"
#define ARCHITECTURE "i386 (x86)"
#define KERNEL_NAME "KrnL"

// Memory zone boundaries (Fitted for 10MB total memory: 0x00000000 - 0x00A00000)
#define KERNEL_ZONE_START 0x00000000
#define KERNEL_ZONE_END   0x005FFFFF  // Kernel zone: 0x00000000 - 0x005FFFFF (6MB)
#define USER_ZONE_START   0x00600000  // User zone: 0x00600000 - 0x00A00000 (4MB)
#define USER_ZONE_END     0x00A00000

// Kernel zone allocator regions (fitted for 10MB total memory: 0x00000000 - 0x00A00000)
#define KHEAP_START       0x00200000  // kmalloc: 1MB
#define KHEAP_END         0x002FFFFF
#define KVMEM_START       0x00300000  // Kernel virtual memory: 3MB (expanded)
#define KVMEM_END         0x005FFFFF

// User zone allocator regions (fitted for 10MB total memory)
#define VMEM_START        0x00600000  // vmalloc: 1MB
#define VMEM_END          0x006FFFFF
#define USER_PROCESS_START 0x00700000  // User processes: 3MB (up to 0x00A00000)

void kernel_main(); 

extern void outb(uint16_t port, uint8_t val);

// Memory subsystem initialization
void memory_init(uint32_t mem_bytes);

// User space support - removed umalloc, using vmalloc only

#endif 