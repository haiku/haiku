LIBC_ARCH_DIR = $(LIBC_DIR)/arch/$(ARCH)
LIBC_ARCH_OBJ_DIR = $(LIBC_ARCH_DIR)/$(OBJ_DIR)

LIBC_OBJS +=

# build prototypes
$(LIBC_ARCH_OBJ_DIR)/%.o: $(LIBC_ARCH_DIR)/%.c 
	@if [ ! -d $(LIBC_ARCH_OBJ_DIR) ]; then mkdir -p $(LIBC_ARCH_OBJ_DIR); fi
	$(CC) -c $< $(GLOBAL_CFLAGS) -Iinclude -o $@

$(LIBC_ARCH_OBJ_DIR)/%.d: $(LIBC_ARCH_DIR)/%.c
	@if [ ! -d $(LIBC_ARCH_OBJ_DIR) ]; then mkdir -p $(LIBC_ARCH_OBJ_DIR); fi
	@echo "making deps for $<..."
	@($(ECHO) -n $(dir $@); $(CC) $(GLOBAL_CFLAGS) -Iinclude -M -MG $<) > $@

$(LIBC_ARCH_OBJ_DIR)/%.d: $(LIBC_ARCH_DIR)/%.S
	@if [ ! -d $(LIBC_ARCH_OBJ_DIR) ]; then mkdir -p $(LIBC_ARCH_OBJ_DIR); fi
	@echo "making deps for $<..."
	@($(ECHO) -n $(dir $@);$(CC) $(GLOBAL_CFLAGS) -Iinclude -M -MG $<) > $@

$(LIBC_ARCH_OBJ_DIR)/%.o: $(LIBC_ARCH_DIR)/%.S
	@if [ ! -d $(LIBC_ARCH_OBJ_DIR) ]; then mkdir -p $(LIBC_ARCH_OBJ_DIR); fi
	$(CC) -c $< $(GLOBAL_CFLAGS) -Iinclude -o $@

