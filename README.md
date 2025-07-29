# KFS_1

This is the first project in the "Kernel From Scratch"-branch of 42 Paris.

Our kernel currently includes:
- an ASM base that is bootable with GRUB
- a basic kernel library
- keyboard interrupt support that prints to the screen
- a kprintf function
- scroll and cursor support
- color support
- different screens, with shortcuts to switch between them

## Commands:

```
# compile all source files
make kfs

# compile all source files and build iso
make build

# compile all source files, build iso, and run the kernel
make run

# remove object folder and iso folder
make clean

# remove object folder, iso folder, executable, and iso image
make fclean

```

# Project Notes

## Tools

- **QEMU**
QEMU is a machine emulator and virtualizer. It allows you to run operating systems and programs for one machine on a different machine. In the context of kernel development, it's used to emulate an x86 PC so you can test your kernel without needing real hardware.
Emulation means simulating a system (hardware or software) so that code written for that system can run in a controlled environment. QEMU emulates an x86 computer to run your kernel.

- **GRUB**
GRUB is a bootloader: a small program that runs right after the BIOS/UEFI when a computer starts. Its job is to load an operating system kernel into memory and start executing it. GRUB supports various filesystems and OS kernels. It's especially useful in kernel development because it supports the Multiboot Specification, which allows it to load kernels in a standard way.
GRUB looks for a multiboot header in your kernel binary. Once found, it uses it to know how and where to load your kernel into memory, then passes control to it.

## Concepts

- **Virtual Image**
A virtual image is a file that simulates a hard drive. GRUB and your kernel are installed on this image. QEMU can boot from this image as if it were a real disk.

- **Multiboot**
The Multiboot Specification is a standard for bootloaders and kernels. It defines:
	-   How the kernel should be structured
	-   How the bootloader passes information (like memory map, boot device info) to the kernel
	-   A specific header (placed in your ASM file) that the bootloader searches for

	To be compatible, your kernel must:
	-   Include a multiboot header in the first 8 KB of the file
    -   Align the header properly
    -   Declare things like the entry point (where execution starts)

- **ASM Boot Code**
Initial boot code written in assembly. This code sets up the CPU and passes control to your kernel's entry point (usually a function like  `main`). It must contain the multiboot header.

- **Boot**
Booting is the process of starting a computer and loading the operating system. The steps generally are:
	1.  **BIOS/UEFI**: Runs firmware-level code from hardware.
  2.  **Bootloader (like GRUB)**: Loaded by the firmware, GRUB reads its configuration, finds your kernel's multiboot header, loads the kernel binary into memory, and then transfers execution control to your kernel's entry point.
  3.  **Kernel**: Your code takes over once GRUB loads it, initializes the system (memory, drivers, etc.), and starts execution from your entry point (e.g.,  `_start`  or  `main`).
    
- **Linker**
The linker is a program that combines multiple object files (.o) into a single binary. For kernel development, it also ensures that code is loaded into the correct memory addresses.

- **Linker Script**
A file (usually an `.ld`-file) that tells the linker how to place code and data in memory. Required because kernels must be loaded at specific physical addresses.

- **Host vs Kernel**
Your host machine runs the compiler and linker, but the resulting binary must not use host libraries or linker scripts. It must be self-contained and multiboot-compliant.

- **Memory and the VGA-buffer**
The kernel will initially run in a very primitive environment, without any sophisticated graphics drivers. To display characters on the screen at this stage, it interacts directly with the VGA (Video Graphics Array) text mode buffer.
VGA Text Mode is a legacy display mode where the screen is treated as a simple grid of characters, typically 80 columns by 25 rows. Each individual character position on this grid holds both the character's code and its associated display attributes, such as foreground and background color. The VGA buffer address is located directly in memory at a specific physical address, which is commonly 0xB8000 (hexadecimal).
To display a character, you write directly to the memory location within this VGA buffer that corresponds to the desired screen position. Each character cell in the buffer is represented by two bytes. The first byte, the character byte, contains the ASCII value of the character you wish to display. The second byte, the attribute byte, defines the character's color. Within this attribute byte, the higher four bits specify the background color, and the lower four bits specify the foreground color.
