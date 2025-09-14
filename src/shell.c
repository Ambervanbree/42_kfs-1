#include "shell.h"
#include "screen.h"
#include "string.h"
#include "kprintf.h"
#include "pmm.h"
#include "kheap.h"
#include "paging.h"
#include "vmem.h"
#include "user_mem.h"
#include "process.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

// Shell state
static char shell_buffer[SHELL_BUFFER_SIZE];
static size_t shell_buffer_pos = 0;
static uint32_t boot_time = 0;

// Forward declarations for new commands
void cmd_version(int argc, char **argv);
void cmd_shutdown(int argc, char **argv);
void cmd_meminfo(int argc, char **argv);
void cmd_kalloc(int argc, char **argv);
void cmd_kfree(int argc, char **argv);
void cmd_ksize(int argc, char **argv);
void cmd_vget(int argc, char **argv);
void cmd_kbrk(int argc, char **argv);
void cmd_vmalloc(int argc, char **argv);
void cmd_vfree(int argc, char **argv);
void cmd_vsize(int argc, char **argv);
void cmd_vbrk(int argc, char **argv);
void cmd_ualloc(int argc, char **argv);
void cmd_ufree(int argc, char **argv);
void cmd_usize(int argc, char **argv);
void cmd_ps(int argc, char **argv);
void cmd_fork(int argc, char **argv);
void cmd_kill(int argc, char **argv);

// Command table
static struct shell_command commands[] = {
    {"help", "Display this help message", cmd_help},
    {"clear", "Clear the screen", cmd_clear},
    {"echo", "Echo arguments to screen", cmd_echo},
    {"reboot", "Restart the system", cmd_reboot},
    {"halt", "Stop CPU (requires manual restart)", cmd_halt},
    {"gdt", "Display GDT information", cmd_gdt_info},
    {"version", "Display kernel version", cmd_version},
    {"shutdown", "Shutdown system gracefully", cmd_shutdown},
    {"meminfo", "Show memory stats", cmd_meminfo},
    {"kalloc", "Allocate kernel memory: kalloc <bytes>", cmd_kalloc},
    {"kfree", "Free kernel memory: kfree <addr>", cmd_kfree},
    {"ksize", "Get allocated block size: ksize <addr>", cmd_ksize},
    {"kbrk", "Physical memory break: kbrk [new_addr]", cmd_kbrk},
    {"vmalloc", "Allocate virtual memory: vmalloc <bytes>", cmd_vmalloc},
    {"vfree", "Free virtual memory: vfree <addr>", cmd_vfree},
    {"vsize", "Get virtual block size: vsize <addr>", cmd_vsize},
    {"vbrk", "Virtual memory break: vbrk [new_addr]", cmd_vbrk},
    {"vget", "Show mapping of a virtual addr: vget <virt>", cmd_vget},
    {"ualloc", "Allocate user memory: ualloc <bytes>", cmd_ualloc},
    {"ufree", "Free user memory: ufree <addr>", cmd_ufree},
    {"usize", "Get user block size: usize <addr>", cmd_usize},
    {"ps", "List processes", cmd_ps},
    {"fork", "Create new process: fork <name>", cmd_fork},
    {"kill", "Kill process: kill <pid>", cmd_kill},
    {NULL, NULL, NULL} // Sentinel
};

void shell_init(void)
{
    boot_time = 0; // We'll implement a timer later
    shell_buffer_pos = 0;
    memset(shell_buffer, 0, SHELL_BUFFER_SIZE);
    
    kprintf("KFS Debug Shell v1.0\n");
    kprintf("Type 'help' for available commands.\n\n");
    shell_print_prompt();
}

void shell_print_prompt(void)
{
    kprintf(SHELL_PROMPT);
    // Set input start position to current cursor position (after prompt)
    input_set_start_position();
}

void shell_process_input(const char *input)
{
    size_t len = strlen(input);
    
    if (len == 0) {
        shell_print_prompt();
        return;
    }
    
    // Copy input to buffer for processing
    if (len < SHELL_BUFFER_SIZE - 1) {
        strcpy(shell_buffer, input);
        shell_execute_command(shell_buffer);
    } else {
        kprintf("Command too long!\n");
    }
    
    shell_print_prompt();
}

void shell_execute_command(const char *command_line)
{
    char *argv[SHELL_MAX_ARGS];
    int argc;
    
    shell_parse_args(command_line, argv, &argc);
    
    if (argc == 0) {
        return;
    }
    
    // Find and execute command
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].function(argc, argv);
            return;
        }
    }
    
    kprintf("Unknown command: %s\n", argv[0]);
    kprintf("Type 'help' for available commands.\n");
}

void shell_parse_args(const char *input, char **argv, int *argc)
{
    static char args_buffer[SHELL_BUFFER_SIZE];
    strcpy(args_buffer, input);
    
    *argc = 0;
    char *token = args_buffer;
    char *end = args_buffer + strlen(args_buffer);
    
    while (token < end && *argc < SHELL_MAX_ARGS - 1) {
        // Skip leading spaces
        while (*token == ' ' || *token == '\t') {
            token++;
        }
        
        if (*token == '\0') {
            break;
        }
        
        argv[*argc] = token;
        (*argc)++;
        
        // Find end of current argument
        while (*token != ' ' && *token != '\t' && *token != '\0') {
            token++;
        }
        
        if (*token != '\0') {
            *token = '\0';
            token++;
        }
    }
    
    argv[*argc] = NULL;
}

// Built-in commands implementation

void cmd_help(int argc, char **argv)
{
    (void)argc; // Suppress unused parameter warning
    (void)argv;
    
    kprintf("Available commands:\n");
    for (int i = 0; commands[i].name != NULL; i++) {
        kprintf("  %-10s - %s\n", commands[i].name, commands[i].description);
    }
}

void cmd_clear(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    screen_clear();
}

void cmd_echo(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        kprintf("%s", argv[i]);
        if (i < argc - 1) {
            kprintf(" ");
        }
    }
    kprintf("\n");
}

void cmd_reboot(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    kprintf("Rebooting system...\n");
    kprintf("Trying multiple reboot methods...\n");
    
    // Method 1: Use keyboard controller to reboot (most common)
    kprintf("Method 1: Keyboard controller reset...\n");
    outb(0x64, 0xFE);
    
    // Small delay
    for (volatile int i = 0; i < 1000000; i++);
    
    // Method 2: Try ACPI reset (if available)
    kprintf("Method 2: ACPI reset...\n");
    outb(0xCF9, 0x06);
    
    // Small delay
    for (volatile int i = 0; i < 1000000; i++);
    
    // Method 3: Triple fault (force CPU reset)
    kprintf("Method 3: Triple fault...\n");
    asm volatile("cli");           // Disable interrupts
    asm volatile("lidt %0" : : "m"((struct {uint16_t limit; uint32_t base;}){0, 0})); // Load invalid IDT
    asm volatile("int $0x00");     // Trigger interrupt with invalid IDT
    
    // If we get here, nothing worked
    kprintf("All reboot methods failed. Please use:\n");
    kprintf("- Physical reset button on computer case\n");
    kprintf("- VM reset function (Ctrl+R in QEMU, or VM menu)\n");
    kprintf("- Power cycle the machine\n");
}

void cmd_halt(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    kprintf("System halted. To restart:\n");
    kprintf("- Physical computer: Press reset button on case\n");
    kprintf("- QEMU: Ctrl+A then X, or close window\n");
    kprintf("- VirtualBox: Machine menu -> Reset\n");
    kprintf("- VMware: VM menu -> Power -> Reset\n");
    kprintf("\nSystem is now halted...\n");
    
    // Disable interrupts and halt
    asm volatile("cli; hlt");
    
    // In case we somehow continue, infinite loop
    while (1) {
        asm volatile("hlt");
    }
}

void cmd_gdt_info(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    kprintf("Global Descriptor Table Information:\n");
    kprintf("  GDT Base Address: 0x%x\n", 0x00000800);
    kprintf("  Segment Layout:\n");
    kprintf("    0x00: Null Segment\n");
    kprintf("    0x08: Kernel Code Segment (Ring 0)\n");
    kprintf("    0x10: Kernel Data Segment (Ring 0)\n");
    kprintf("    0x18: Kernel Stack Segment (Ring 0)\n");
    kprintf("    0x20: User Code Segment (Ring 3)\n");
    kprintf("    0x28: User Data Segment (Ring 3)\n");
    kprintf("    0x30: User Stack Segment (Ring 3)\n");
    
    // Display current segment registers
    uint16_t cs, ds, es, fs, gs, ss;
    asm volatile("mov %%cs, %0" : "=r"(cs));
    asm volatile("mov %%ds, %0" : "=r"(ds));
    asm volatile("mov %%es, %0" : "=r"(es));
    asm volatile("mov %%fs, %0" : "=r"(fs));
    asm volatile("mov %%gs, %0" : "=r"(gs));
    asm volatile("mov %%ss, %0" : "=r"(ss));
    
    kprintf("  Current Segment Registers:\n");
    kprintf("    CS: 0x%x, DS: 0x%x, ES: 0x%x\n", cs, ds, es);
    kprintf("    FS: 0x%x, GS: 0x%x, SS: 0x%x\n", fs, gs, ss);
}

void cmd_version(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    kprintf("KFS (Kernel From Scratch) v1.0\n");
    kprintf("Built: %s %s\n", __DATE__, __TIME__);
    kprintf("Architecture: i386 (32-bit)\n");
    kprintf("Features: GDT, Interrupts, Keyboard, VGA Text Mode, Shell\n");
}



void cmd_shutdown(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    kprintf("Shutting down system gracefully...\n");
    kprintf("This will try to power off the system.\n\n");
    
    // Method 1: ACPI shutdown (works in most VMs)
    kprintf("Attempting ACPI shutdown...\n");
    // QEMU ACPI shutdown - write 16-bit value to port
    asm volatile("outw %0, %1" : : "a"((uint16_t)0x2000), "Nd"((uint16_t)0x604));
    
    // Small delay
    for (volatile int i = 0; i < 1000000; i++);
    
    // Method 2: APM shutdown (older systems)
    kprintf("Attempting APM shutdown...\n");
    asm volatile(
        "mov $0x5307, %%ax\n\t"  // APM function: set power state
        "mov $0x0001, %%bx\n\t"  // Device: all devices  
        "mov $0x0003, %%cx\n\t"  // Power state: off
        "int $0x15"              // APM BIOS interrupt
        :
        :
        : "ax", "bx", "cx"
    );
    
    // Small delay
    for (volatile int i = 0; i < 1000000; i++);
    
    // Method 3: Try alternative ACPI methods
    kprintf("Trying alternative shutdown methods...\n");
    asm volatile("outw %0, %1" : : "a"((uint16_t)0x2000), "Nd"((uint16_t)0xB004));
    asm volatile("outw %0, %1" : : "a"((uint16_t)0x3400), "Nd"((uint16_t)0x4004));
    
    // If we get here, shutdown failed
    kprintf("\nShutdown failed. The system is still running.\n");
    kprintf("You can:\n");
    kprintf("- Use 'halt' to stop the CPU (requires restart)\n");
    kprintf("- Use 'reboot' to restart the system\n");
    kprintf("- Close the VM window manually\n");
    kprintf("- Power off physical machine manually\n");
}

static uint32_t parse_hex_or_dec(const char *s)
{
    uint32_t val = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
        while (*s) {
            char c = *s++;
            uint32_t n;
            if (c >= '0' && c <= '9') n = c - '0';
            else if (c >= 'a' && c <= 'f') n = 10 + (c - 'a');
            else if (c >= 'A' && c <= 'F') n = 10 + (c - 'A');
            else break;
            val = (val << 4) | n;
        }
        return val;
    }
    while (*s && *s >= '0' && *s <= '9') {
        val = val * 10 + (*s - '0');
        s++;
    }
    return val;
}

void cmd_meminfo(int argc, char **argv)
{
    (void)argc; (void)argv;
    kprintf("PMM total pages: %d, free pages: %d\n", (int)pmm_total_pages(), (int)pmm_free_pages());
}

void cmd_kalloc(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: kalloc <bytes>\n"); return; }
    uint32_t n = parse_hex_or_dec(argv[1]);
    void *p = kmalloc(n);
    kprintf("kalloc(%d) -> %x\n", (int)n, (uint32_t)p);
}

void cmd_kfree(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: kfree <addr>\n"); return; }
    uint32_t a = parse_hex_or_dec(argv[1]);
    kfree((void*)a);
    kprintf("kfree(%x)\n", a);
}

void cmd_ksize(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: ksize <addr>\n"); return; }
    uint32_t a = parse_hex_or_dec(argv[1]);
    size_t s = ksize((void*)a);
    kprintf("ksize(%x) -> %d\n", a, (int)s);
}

void cmd_kbrk(int argc, char **argv)
{
    if (argc < 2) {
        void *brk = kbrk(0);
        kprintf("kbrk: %x\n", (uint32_t)brk);
        return;
    }
    uint32_t new_brk = parse_hex_or_dec(argv[1]);
    void *result = kbrk((void*)new_brk);
    if (result == (void*)-1) {
        kprintf("kbrk: invalid address %x\n", new_brk);
    } else {
        kprintf("kbrk: %x\n", (uint32_t)result);
    }
}

void cmd_vmalloc(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: vmalloc <bytes>\n"); return; }
    uint32_t n = parse_hex_or_dec(argv[1]);
    void *p = vmalloc(n);
    kprintf("vmalloc(%d) -> %x\n", (int)n, (uint32_t)p);
}

void cmd_vfree(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: vfree <addr>\n"); return; }
    uint32_t a = parse_hex_or_dec(argv[1]);
    vfree((void*)a);
    kprintf("vfree(%x)\n", a);
}

void cmd_vsize(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: vsize <addr>\n"); return; }
    uint32_t a = parse_hex_or_dec(argv[1]);
    size_t s = vsize((void*)a);
    kprintf("vsize(%x) -> %d\n", a, (int)s);
}

void cmd_vbrk(int argc, char **argv)
{
    if (argc < 2) {
        void *brk = vbrk(0);
        kprintf("vbrk: %x\n", (uint32_t)brk);
        return;
    }
    uint32_t new_brk = parse_hex_or_dec(argv[1]);
    void *result = vbrk((void*)new_brk);
    if (result == (void*)-1) {
        kprintf("vbrk: invalid address %x\n", new_brk);
    } else {
        kprintf("vbrk: %x\n", (uint32_t)result);
    }
}

void cmd_vget(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: vget <virt>\n"); return; }
    uint32_t virt = parse_hex_or_dec(argv[1]);
    uint32_t pte = vmm_get_mapping(virt);
    uint32_t pd_idx = (virt >> 22) & 0x3FF;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;
    uint32_t offset = virt & 0xFFF;
    kprintf("logical: %x (flat seg)\n", virt);
    kprintf("virtual: %x  pd=%d pt=%d off=%d\n", virt, (int)pd_idx, (int)pt_idx, (int)offset);
    if (pte == 0) {
        kprintf("mapping: (not present)\n");
        return;
    }
    uint32_t phys = (pte & 0xFFFFF000) | offset;
    uint32_t flags = pte & 0xFFF;
    kprintf("physical: %x  flags: %x\n", phys, flags);
}

// User space memory commands
void cmd_ualloc(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: ualloc <bytes>\n"); return; }
    uint32_t n = parse_hex_or_dec(argv[1]);
    void *p = umalloc(n);
    kprintf("ualloc(%d) -> %x\n", (int)n, (uint32_t)p);
}

void cmd_ufree(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: ufree <addr>\n"); return; }
    uint32_t a = parse_hex_or_dec(argv[1]);
    ufree((void*)a);
    kprintf("ufree(%x)\n", a);
}

void cmd_usize(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: usize <addr>\n"); return; }
    uint32_t a = parse_hex_or_dec(argv[1]);
    size_t s = usize((void*)a);
    kprintf("usize(%x) -> %d\n", a, (int)s);
}

// Process management commands
void cmd_ps(int argc, char **argv)
{
    (void)argc; (void)argv;
    process_list();
}

void cmd_fork(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: fork <name>\n"); return; }
    process_t *proc = process_create(argv[1]);
    if (proc) {
        kprintf("Created process %d: %s\n", proc->pid, proc->name);
    } else {
        kprintf("Failed to create process\n");
    }
}

void cmd_kill(int argc, char **argv)
{
    if (argc < 2) { kprintf("Usage: kill <pid>\n"); return; }
    uint32_t pid = parse_hex_or_dec(argv[1]);
    process_t *proc = process_find_by_pid(pid);
    if (proc) {
        process_destroy(proc);
        kprintf("Killed process %d\n", pid);
    } else {
        kprintf("Process %d not found\n", pid);
    }
}