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

#endif