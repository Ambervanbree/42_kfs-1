# KFS_1

This is the first project in the "Kernel From Scratch"-branch of 42 Paris. 

## Commands:

```
# compile all source files
make kfs

# compile all source files and build iso
make build

# compile all source files, build iso, and run the kernel
make run

# run kernel code in debug mode
make debug
make run

# remove object folder and iso folder
make clean

# remove object folder, iso folder, executable, and iso image
make fclean

```


# Compile C files
gcc -Wall -Wextra -Werror -Iinclude -m32 -ffreestanding -fno-builtin -fno-exceptions -fno-stack-protector -nostdlib -nodefaultlibs -c src/kernel_main.c -o obj/kernel_main.o
gcc -Wall -Wextra -Werror -Iinclude -m32 -ffreestanding -fno-builtin -fno-exceptions -fno-stack-protector -nostdlib -nodefaultlibs -c src/screen.c -o obj/screen.o
gcc -Wall -Wextra -Werror -Iinclude -m32 -ffreestanding -fno-builtin -fno-exceptions -fno-stack-protector -nostdlib -nodefaultlibs -c src/string.c -o obj/string.o
gcc -Wall -Wextra -Werror -Iinclude -m32 -ffreestanding -fno-builtin -fno-exceptions -fno-stack-protector -nostdlib -nodefaultlibs -c src/interrupt.c -o obj/interrupt_c.o

# Compile assembly files
nasm -f elf32 asm/boot.s -o obj/boot.o
nasm -f elf32 asm/interrupt.s -o obj/interrupt.o

# Link
ld -T link.ld -m elf_i386 obj/kernel_main.o obj/screen.o obj/string.o obj/interrupt_c.o obj/boot.o obj/interrupt.o -o kfs

# Create ISO
mkdir -p iso/boot/grub && cp grub.cfg iso/boot/grub/ && cp kfs iso/boot/ && grub-mkrescue -o kfs.iso iso