#ifndef SHELL_H
#define SHELL_H

#include "kernel.h"

// I/O functions
extern void outb(uint16_t port, uint8_t val);

#define SHELL_BUFFER_SIZE 256
#define SHELL_MAX_ARGS 16
#define SHELL_PROMPT "kfs> "

// Shell command structure
struct shell_command {
    const char *name;
    const char *description;
    void (*function)(int argc, char **argv);
};

// Shell functions
void shell_init(void);
void shell_run(void);
void shell_process_input(const char *input);
void shell_execute_command(const char *command_line);
void shell_parse_args(const char *input, char **argv, int *argc);
void shell_print_prompt(void);

// Built-in commands
void cmd_help(int argc, char **argv);
void cmd_clear(int argc, char **argv);
void cmd_echo(int argc, char **argv);
void cmd_reboot(int argc, char **argv);
void cmd_halt(int argc, char **argv);
void cmd_gdt_info(int argc, char **argv);


// Forward declarations for new commands
void cmd_version(int argc, char **argv);
void cmd_shutdown(int argc, char **argv);
void cmd_meminfo(int argc, char **argv);
void cmd_kmalloc(int argc, char **argv);
void cmd_kfree(int argc, char **argv);
void cmd_ksize(int argc, char **argv);
void cmd_vget(int argc, char **argv);
void cmd_kbrk(int argc, char **argv);
void cmd_vmalloc(int argc, char **argv);
void cmd_vfree(int argc, char **argv);
void cmd_vsize(int argc, char **argv);
void cmd_vbrk(int argc, char **argv);
void cmd_umalloc(int argc, char **argv);
void cmd_ufree(int argc, char **argv);
void cmd_usize(int argc, char **argv);
void cmd_ps(int argc, char **argv);
void cmd_fork(int argc, char **argv);
void cmd_kill(int argc, char **argv);
void cmd_page_ops(int argc, char **argv);
void cmd_alloc_functions(int argc, char **argv);
void cmd_virtual_physical(int argc, char **argv);
void cmd_panic_test(int argc, char **argv);
void cmd_ktest(int argc, char **argv);
void cmd_write(int argc, char **argv);
void cmd_read(int argc, char **argv);
void cmd_present(int argc, char **argv);
void cmd_rotest(int argc, char **argv);
void cmd_pftest(int argc, char **argv);
void cmd_biostest(int argc, char **argv);
#endif