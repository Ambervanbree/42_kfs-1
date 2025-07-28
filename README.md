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
