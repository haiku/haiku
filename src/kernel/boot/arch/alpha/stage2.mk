
STAGE2_DIR = boot/$(ARCH)
STAGE2_OBJ_DIR = $(STAGE2_DIR)/$(OBJ_DIR)
STAGE2_OBJS = $(STAGE2_OBJ_DIR)/stage2.o
DEPS += $(STAGE2_OBJS:.o=.d)

STAGE2 = $(STAGE2_OBJ_DIR)/stage2

$(STAGE2): $(STAGE2_OBJS) $(LIBC)
	$(LD) -dN --script=$(STAGE2_DIR)/stage2.ld -L $(LIBGCC_PATH) $(LIBGCC) $(STAGE2_OBJS) $(LIBC) -o $@

stage2clean:
	rm -f $(STAGE2_OBJS) $(STAGE2_ARCH)

$(STAGE2_OBJ_DIR)/%.o: $(STAGE2_DIR)/%.c 
	@mkdir -p $(STAGE2_OBJ_DIR)
	$(CC) $(GLOBAL_CFLAGS) -Iinclude -I$(STAGE2_DIR) -c $< -o $@

$(STAGE2_OBJ_DIR)/%.d: $(STAGE2_DIR)/%.c
	@mkdir -p $(STAGE2_OBJ_DIR)
	@echo "making deps for $<..."
	@($(ECHO) -n $(dir $@);$(CC) $(GLOBAL_CFLAGS) -Iinclude -I$(STAGE2_DIR) -M -MG $<) > $@

$(STAGE2_OBJ_DIR)/%.d: $(STAGE2_DIR)/%.S
	@mkdir -p $(STAGE2_OBJ_DIR)
	@echo "making deps for $<..."
	@($(ECHO) -n $(dir $@);$(CC) $(GLOBAL_CFLAGS) -Iinclude -I$(STAGE2_DIR) -M -MG $<) > $@

$(STAGE2_OBJ_DIR)/%.o: $(STAGE2_DIR)/%.S
	@mkdir -p $(STAGE2_OBJ_DIR)
	$(CC) $(GLOBAL_CFLAGS) -Iinclude -I$(STAGE2_DIR) -c $< -o $@
