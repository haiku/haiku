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
	$(BOOT_OBJ_DIR)/stage2_asm.o \
	$(BOOT_OBJ_DIR)/stage2_mmu.o \
	$(BOOT_OBJ_DIR)/stage2_of.o \
	$(BOOT_OBJ_DIR)/stage2_text.o \
	$(BOOT_OBJ_DIR)/stage2_faults.o

DEPS += $(STAGE2_OBJS:.o=.d)

STAGE2 = $(BOOT_OBJ_DIR)/stage2

$(STAGE2): $(STAGE2_OBJS) $(LIBC)
	$(LD) $(GLOBAL_LDFLAGS) -dN --script=$(BOOT_DIR)/stage2.ld -L $(LIBGCC_PATH) $(STAGE2_OBJS) $(LIBC) $(LIBGCC) -o $@ 

stage2: $(STAGE2)

stage2clean:
	rm -f $(STAGE2_OBJS) $(STAGE2)

CLEAN += stage2clean

SEMIFINAL = $(BOOT_DIR)/final.bootdir

$(SEMIFINAL): $(STAGE2) $(KERNEL) $(APPS) tools
	$(BOOTMAKER) --bigendian $(BOOT_DIR)/config.ini -o $(SEMIFINAL)

STAGE1_OBJS = \
	$(BOOT_OBJ_DIR)/stage1.o

DEPS += $(STAGE1_OBJS:.o=.d)

FINAL = $(BOOT_DIR)/final

$(FINAL): $(STAGE1_OBJS)
	$(LD) $(GLOBAL_LDFLAGS) -dN --script=$(BOOT_DIR)/stage1.ld $(STAGE1_OBJS) -o $@

FINAL_ASMINCLUDE = $(BOOT_DIR)/final.asminclude

$(FINAL_ASMINCLUDE): $(SEMIFINAL) tools
	$(BIN2ASM) < $(SEMIFINAL) > $(FINAL_ASMINCLUDE)

finalclean:
	rm -f $(STAGE1_OBJS) $(FINAL) $(SEMIFINAL) $(FINAL_ASMINCLUDE)

CLEAN += finalclean

# 
$(BOOT_OBJ_DIR)/stage1.o: $(BOOT_DIR)/stage1.S
	@mkdir -p $(BOOT_OBJ_DIR)
	$(CC) $(GLOBAL_CFLAGS) -g -I. -Iinclude -I$(BOOT_DIR) -c $< -o $@

$(BOOT_OBJ_DIR)/%.o: $(BOOT_DIR)/%.c
	@mkdir -p $(BOOT_OBJ_DIR)
	$(CC) $(GLOBAL_CFLAGS) -g -Iinclude -I$(BOOT_DIR) -c $< -o $@

$(BOOT_OBJ_DIR)/%.d: $(BOOT_DIR)/%.c
	@mkdir -p $(BOOT_OBJ_DIR)
	@echo "making deps for $<..."
	@($(ECHO) -n $(dir $@);$(CC) $(GLOBAL_CFLAGS) -g -Iinclude -I$(BOOT_DIR) -M -MG $<) > $@

$(BOOT_OBJ_DIR)/%.d: $(BOOT_DIR)/%.S
	@mkdir -p $(BOOT_OBJ_DIR)
	@echo "making deps for $<..."
	@($(ECHO) -n $(dir $@);$(CC) $(GLOBAL_CFLAGS) -g -Iinclude -I$(BOOT_DIR) -M -MG $<) > $@

$(BOOT_OBJ_DIR)/%.o: $(BOOT_DIR)/%.S
	@mkdir -p $(BOOT_OBJ_DIR)
	$(CC) $(GLOBAL_CFLAGS) -g -Iinclude -I$(BOOT_DIR) -c $< -o $@
endif
