# ppc kernel makefile
# included from kernel.mk
KERNEL_ARCH_OBJ_DIR = $(KERNEL_ARCH_DIR)/$(OBJ_DIR)
KERNEL_OBJS += \
	$(KERNEL_ARCH_OBJ_DIR)/arch_asm.o \
	$(KERNEL_ARCH_OBJ_DIR)/arch_cpu.o \
	$(KERNEL_ARCH_OBJ_DIR)/arch_dbg_console.o \
	$(KERNEL_ARCH_OBJ_DIR)/arch_debug.o \
	$(KERNEL_ARCH_OBJ_DIR)/arch_faults.o \
	$(KERNEL_ARCH_OBJ_DIR)/arch_int.o \
	$(KERNEL_ARCH_OBJ_DIR)/arch_smp.o \
	$(KERNEL_ARCH_OBJ_DIR)/arch_thread.o \
	$(KERNEL_ARCH_OBJ_DIR)/arch_timer.o \
	$(KERNEL_ARCH_OBJ_DIR)/arch_vm.o \
	$(KERNEL_ARCH_OBJ_DIR)/arch_vm_translation_map.o

KERNEL_ARCH_INCLUDES = $(KERNEL_INCLUDES)

$(KERNEL_ARCH_OBJ_DIR)/%.o: $(KERNEL_ARCH_DIR)/%.c
	@mkdir -p $(KERNEL_ARCH_OBJ_DIR)
	$(CC) -c $< $(GLOBAL_CFLAGS) $(KERNEL_ARCH_INCLUDES) -o $@

$(KERNEL_ARCH_OBJ_DIR)/%.d: $(KERNEL_ARCH_DIR)/%.c
	@mkdir -p $(KERNEL_ARCH_OBJ_DIR)
	@echo "making deps for $<..."
	@($(ECHO) -n $(dir $@); $(CC) $(GLOBAL_CFLAGS) $(KERNEL_ARCH_INCLUDES) -M -MG $<) > $@

$(KERNEL_ARCH_OBJ_DIR)/%.d: $(KERNEL_ARCH_DIR)/%.S
	@mkdir -p $(KERNEL_ARCH_OBJ_DIR)
	@echo "making deps for $<..."
	@($(ECHO) -n $(dir $@);$(CC) $(GLOBAL_CFLAGS) $(KERNEL_ARCH_INCLUDES) -M -MG $<) > $@

$(KERNEL_ARCH_OBJ_DIR)/%.o: $(KERNEL_ARCH_DIR)/%.S
	@mkdir -p $(KERNEL_ARCH_OBJ_DIR)
	$(CC) -c $< $(GLOBAL_CFLAGS) $(KERNEL_ARCH_INCLUDES) -o $@
