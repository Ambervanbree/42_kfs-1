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
NAME    := kfs

# === Directories ===
SRC_DIR := src
OBJ_DIR := obj

# === Source and Object Files ===
SRCS    := $(wildcard $(SRC_DIR)/*.c)
OBJS    := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# === Default Rule ===
all: $(NAME)

# === Create Object Directory If Not Exists ===
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# === Build Rules ===
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

obj/boot.o: asm/boot.s | $(OBJ_DIR)
	$(AS) $(ASFLAGS) $< -o $@

obj/kernel_main.o: src/kernel_main.c
	$(CC) -m32 -ffreestanding -c $< -o $@

$(NAME): obj/boot.o obj/kernel_main.o
	$(LD) $(LDFLAGS) $^ -o $@

# === Debug Build ===
debug: CFLAGS += $(DEBUG_FLAGS)
debug: fclean
	$(MAKE) all CFLAGS="$(CFLAGS)"

# === Run the Kernel (Assuming QEMU) ===
run: $(NAME)
	qemu-system-i386 -kernel $(NAME) -serial mon:stdio

# === Clean Object Files ===
clean:
	rm -rf $(OBJ_DIR)

# === Clean Everything ===
fclean: clean
	rm -f $(NAME)

# === Rebuild Everything ===
re: fclean all
.PHONY: all clean fclean re debug
