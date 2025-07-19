#ifndef KERNEL_H
#define KERNEL_H

/* Basic types */
typedef unsigned int   uint32_t;
typedef int            int32_t;
typedef unsigned short uint16_t;
typedef short          int16_t;
typedef unsigned char  uint8_t;
typedef char           int8_t;
typedef uint32_t       size_t;

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

void kernel_main(); //uint32_t magic, struct multiboot_info* mbi

extern void outb(uint16_t port, uint8_t val);

#endif 