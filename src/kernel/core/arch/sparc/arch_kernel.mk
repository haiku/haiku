# sparc kernel makefile
# included from kernel.mk
KERNEL_ARCH_OBJ_DIR = $(KERNEL_ARCH_DIR)/$(OBJ_DIR)
KERNEL_OBJS += \

KERNEL_ARCH_INCLUDES = $(KERNEL_INCLUDES)

$(KERNEL_ARCH_OBJ_DIR)/%.o: $(KERNEL_ARCH_DIR)/%.c
	@mkdir -p $(KERNEL_ARCH_OBJ_DIR)
	$(CC) -c $< $(GLOBAL_CFLAGS) $(KERNEL_ARCH_INCLUDES) -o $@

$(KERNEL_ARCH_OBJ_DIR)/%.d: $(KERNEL_ARCH_DIR)/%.c
	@mkdir -p $(KERNEL_ARCH_OBJ_DIR)
	@echo "making deps for $<..."
	@(echo -n $(dir $@); $(CC) $(GLOBAL_CFLAGS) $(KERNEL_ARCH_INCLUDES) -M -MG $<) > $@

$(KERNEL_ARCH_OBJ_DIR)/%.d: $(KERNEL_ARCH_DIR)/%.S
	@mkdir -p $(KERNEL_ARCH_OBJ_DIR)
	@echo "making deps for $<..."
	@(echo -n $(dir $@);$(CC) $(GLOBAL_CFLAGS) $(KERNEL_ARCH_INCLUDES) -M -MG $<) > $@

$(KERNEL_ARCH_OBJ_DIR)/%.o: $(KERNEL_ARCH_DIR)/%.S
	@mkdir -p $(KERNEL_ARCH_OBJ_DIR)
	$(CC) -c $< $(GLOBAL_CFLAGS) $(KERNEL_ARCH_INCLUDES) -o $@
