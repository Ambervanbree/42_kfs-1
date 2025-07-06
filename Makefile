# === Compiler and Flags ===
CC       := gcc
CFLAGS   := -Wall -Wextra -Werror -Iinclude -m32 -ffreestanding \
             -fno-builtin -fno-exceptions -fno-stack-protector -nostdlib -nodefaultlibs
LD       := ld
LDFLAGS  := -T link.ld -m elf_i386
AS       := nasm
ASFLAGS  := -f elf32

# === Debug Flags ===
DEBUG_FLAGS := -g

# === Project Name ===
NAME     := kfs
ISO_NAME := kfs.iso

# === Directories ===
ASM_DIR := asm
SRC_DIR := src
OBJ_DIR := obj
ISO_DIR := iso

# === Source and Object Files ===
C_FILES  := kernel_main.c print.c
C_SRCS   := $(addprefix $(SRC_DIR)/, $(C_FILES))
C_OBJS   := $(C_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

ASM_SRCS := $(ASM_DIR)/boot.s
ASM_OBJS := $(ASM_SRCS:$(ASM_DIR)/%.s=$(OBJ_DIR)/%.o)

OBJS     := $(C_OBJS) $(ASM_OBJS) 

# === Default Rule ===
all: build

# === Create Object Directory If Not Exists ===
$(OBJ_DIR):
	mkdir -p $@

# === Build Rules ===
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(ASM_DIR)/%.s | $(OBJ_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(NAME): $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

$(ISO_NAME): $(NAME)
	mkdir -p $(ISO_DIR)/boot/grub
	cp grub.cfg $(ISO_DIR)/boot/grub/
	cp $(NAME) $(ISO_DIR)/boot/
	grub-mkrescue -o $(ISO_NAME) $(ISO_DIR)

build: $(ISO_NAME)

run: build
	qemu-system-i386 -cdrom $(ISO_NAME) -boot d -serial mon:stdio

# === Debug Build ===
debug: CFLAGS += $(DEBUG_FLAGS)
debug: fclean
	$(MAKE) build

# === Clean Object Files ===
clean:
	rm -rf $(OBJ_DIR) $(ISO_DIR)

# === Clean Everything ===
fclean: clean
	rm -f $(NAME) $(ISO_NAME)

# === Rebuild Everything ===
re: fclean all
.PHONY: all build run clean fclean re debug
