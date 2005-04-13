ifneq ($(_BOOT_MAKE),1)
_BOOT_MAKE = 1

# include targets we depend on
include lib/lib.mk
include kernel/kernel.mk
include apps/apps.mk

BOOT_DIR = boot/$(ARCH)
BOOT_OBJ_DIR = $(BOOT_DIR)/$(OBJ_DIR)

STAGE2_OBJS = \
	$(BOOT_OBJ_DIR)/stage2.o \
	$(BOOT_OBJ_DIR)/stage2_asm.o \
	$(BOOT_OBJ_DIR)/stage2_nextmon.o \
	$(BOOT_OBJ_DIR)/stage2_text.o \

DEPS += $(STAGE2_OBJS:.o=.d)

STAGE2 = $(BOOT_OBJ_DIR)/stage2

$(STAGE2): $(STAGE2_OBJS) $(KLIBS)
	$(LD) -dN --script=$(BOOT_DIR)/stage2.ld -L $(LIBGCC_PATH) $(STAGE2_OBJS) $(LINK_KLIBS) $(LIBGCC) -o $@

stage2: $(STAGE2)

stage2clean:
	rm -f $(STAGE2_OBJS) $(STAGE2) 

CLEAN += stage2clean

SEMIFINAL = $(BOOT_DIR)/final.bootdir

#$(SEMIFINAL): $(STAGE2) $(KERNEL) $(APPS) tools 
$(SEMIFINAL): $(STAGE2) tools 
	$(BOOTMAKER) --bigendian $(BOOT_DIR)/config.ini -o $(SEMIFINAL)

STAGE1_OBJS = \
	$(BOOT_OBJ_DIR)/stage1.o

DEPS += $(STAGE1_OBJS:.o=.d)

FINAL = $(BOOT_DIR)/final

AWKPROG='\
{ \
        printf "\0\207\01\07"; \
		printf "\0%c%c%c", $$1 / 65536, $$1 / 256, $$1; \
		printf "\0%c%c%c", $$2 / 65536, $$2 / 256, $$2; \
		printf "\0%c%c%c", $$3 / 65536, $$3 / 256, $$3; \
        printf "\0\0\0\0\04\070\0\0\0\0\0\0\0\0\0\0" \
}'

$(FINAL): $(STAGE1_OBJS)
	$(LD) -dN --script=$(BOOT_DIR)/stage1.ld $(STAGE1_OBJS) -o $@.elf
	@${SIZE} $@.elf
	@${OBJCOPY} -O binary $@.elf $@.raw
	@(${SIZE} $@.elf | tail +2 | ${AWK} ${AWKPROG} ; cat $@.raw) > $@
	
FINAL_ASMINCLUDE = $(BOOT_DIR)/final.asminclude

$(FINAL_ASMINCLUDE): $(SEMIFINAL) tools
	$(BIN2ASM) < $(SEMIFINAL) > $(FINAL_ASMINCLUDE)

finalclean:
	rm -f $(STAGE1_OBJS) $(FINAL) $(SEMIFINAL) $(FINAL_ASMINCLUDE)

CLEAN += finalclean

# 
$(BOOT_OBJ_DIR)/stage1.o: $(BOOT_DIR)/stage1.S
	@if [ ! -d $(BOOT_OBJ_DIR) ]; then mkdir -p $(BOOT_OBJ_DIR); fi
	$(CC) -c $< $(GLOBAL_CFLAGS) -I. -Iinclude -o $@

$(BOOT_OBJ_DIR)/%.o: $(BOOT_DIR)/%.c 
	@if [ ! -d $(BOOT_OBJ_DIR) ]; then mkdir -p $(BOOT_OBJ_DIR); fi
	$(CC) -c $< $(GLOBAL_CFLAGS) $(KERNEL_CFLAGS) -Iinclude -Iinclude/nulibc -o $@

$(BOOT_OBJ_DIR)/%.d: $(BOOT_DIR)/%.c
	@if [ ! -d $(BOOT_OBJ_DIR) ]; then mkdir -p $(BOOT_OBJ_DIR); fi
	@echo "making deps for $<..."
	@($(ECHO) -n $(dir $@);$(CC) $(KERNEL_CFLAGS) -Iinclude -Iinclude/nulibc -M -MG $<) > $@

$(BOOT_OBJ_DIR)/%.d: $(BOOT_DIR)/%.S
	@if [ ! -d $(BOOT_OBJ_DIR) ]; then mkdir -p $(BOOT_OBJ_DIR); fi
	@echo "making deps for $<..."
	@($(ECHO) -n $(dir $@);$(CC) $(KERNEL_CFLAGS) -Iinclude -Iinclude/nulibc -M -MG $<) > $@

$(BOOT_OBJ_DIR)/%.o: $(BOOT_DIR)/%.S
	@if [ ! -d $(BOOT_OBJ_DIR) ]; then mkdir -p $(BOOT_OBJ_DIR); fi
	$(CC) -c $< $(GLOBAL_CFLAGS) $(KERNEL_CFLAGS) -Iinclude -Iinclude/nulibc -o $@

endif
