ifneq ($(_BOOT_MAKE),1)
_BOOT_MAKE = 1

# include targets we depend on
include lib/lib.mk
include kernel/kernel.mk
include apps/apps.mk

# sh4 stage2 makefile
BOOT_DIR = boot/$(ARCH)
BOOT_OBJ_DIR = $(BOOT_DIR)/$(OBJ_DIR)
STAGE2_OBJS = $(BOOT_OBJ_DIR)/stage2.o \
	$(BOOT_OBJ_DIR)/serial.o \
	$(BOOT_OBJ_DIR)/mmu.o \
	$(BOOT_OBJ_DIR)/vcpu.o \
	$(BOOT_OBJ_DIR)/vcpu_c.o
DEPS += $(STAGE2_OBJS:.o=.d)

STAGE2 = $(BOOT_OBJ_DIR)/stage2

$(STAGE2): $(STAGE2_OBJS) $(KLIBS)
	$(LD) $(GLOBAL_LDFLAGS) -dN --script=$(BOOT_DIR)/stage2.ld -L $(LIBGCC_PATH) $(STAGE2_OBJS) $(LINK_KLIBS) $(LIBGCC) -o $@ 

stage2: $(STAGE2)

stage2clean:
	rm -f $(STAGE2_OBJS) $(STAGE2)

CLEAN += stage2clean

SEMIFINAL = $(BOOT_DIR)/final.bootdir

$(SEMIFINAL): $(STAGE2) $(KERNEL) $(KERNEL_ADDONS) $(APPS) tools
	$(BOOTMAKER) $(BOOT_DIR)/config.ini -o $(SEMIFINAL)

STAGE1_OBJS = \
	$(BOOT_OBJ_DIR)/stage1.o

DEPS += $(STAGE1_OBJS:.o=.d)

STAGE1 = $(BOOT_OBJ_DIR)/stage1

$(STAGE1): $(STAGE1_OBJS)
	$(LD) $(GLOBAL_LDFLAGS) -N -Ttext 0x8c000000 $(STAGE1_OBJS) -o $(STAGE1)

$(STAGE1).bin: $(STAGE1)
	$(OBJCOPY) -O binary $(STAGE1) $@1
	dd if=/dev/zero of=$(STAGE1).bin bs=4096 count=1 2> /dev/null
	dd if=$(STAGE1).bin1 of=$(STAGE1).bin conv=notrunc 2> /dev/null
	rm $(STAGE1).bin1

stage1clean:
	rm -f $(STAGE1_OBJS) $(STAGE1) $(STAGE1).bin $(STAGE1).bin1

CLEAN += stage1clean

FINAL = $(BOOT_DIR)/final

$(FINAL): $(SEMIFINAL) $(STAGE1).bin
	cat $(STAGE1).bin $(SEMIFINAL) > $(FINAL)

# 
$(BOOT_OBJ_DIR)/%.o: $(BOOT_DIR)/%.c
	@mkdir -p $(BOOT_OBJ_DIR)
	$(CC) $(GLOBAL_CFLAGS) -Iinclude -Iinclude/nulibc -I$(BOOT_DIR) -c $< -o $@

$(BOOT_OBJ_DIR)/%.d: $(BOOT_DIR)/%.c
	@mkdir -p $(BOOT_OBJ_DIR)
	@echo "making deps for $<..."
	@($(ECHO) -n $(dir $@);$(CC) $(GLOBAL_CFLAGS) -Iinclude -Iinclude/nulibc -I$(BOOT_DIR) -M -MG $<) > $@

$(BOOT_OBJ_DIR)/%.d: $(BOOT_DIR)/%.S
	@mkdir -p $(BOOT_OBJ_DIR)
	@echo "making deps for $<..."
	@($(ECHO) -n $(dir $@);$(CC) $(GLOBAL_CFLAGS) -Iinclude -Iinclude/nulibc -I$(BOOT_DIR) -M -MG $<) > $@

$(BOOT_OBJ_DIR)/%.o: $(BOOT_DIR)/%.S
	@mkdir -p $(BOOT_OBJ_DIR)
	$(CC) $(GLOBAL_CFLAGS) -Iinclude -Iinclude/nulibc -I$(BOOT_DIR) -c $< -o $@
endif
