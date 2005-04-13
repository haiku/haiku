CFLAGS = -O3 -DBOOTXX -D_STANDALONE -DSTANDALONE -DRELOC=0x380000 -DSUN4M -DSUN_BOOTPARAMS -DDEBUG -DDEBUG_PROM -Iboot/$(ARCH)/include
LIBSACFLAGS = -O3 -D_STANDALONE -D__INTERNAL_LIBSA_CREAD -Iboot/$(ARCH)/include
LIBKERNCFLAGS = -O3 -D_KERNEL -Iboot/$(ARCH)/include

STAGE2_OBJS =  boot/$(ARCH)/srt0.o \
	boot/$(ARCH)/promdev.o \
	boot/$(ARCH)/closeall.o \
	boot/$(ARCH)/dvma.o \
	boot/$(ARCH)/stage2.o \
	boot/$(ARCH)/libsa/alloc.o \
	boot/$(ARCH)/libsa/exit.o \
	boot/$(ARCH)/libsa/printf.o \
	boot/$(ARCH)/libsa/memset.o \
	boot/$(ARCH)/libsa/memcpy.o \
	boot/$(ARCH)/libsa/memcmp.o \
	boot/$(ARCH)/libsa/strcmp.o \
	boot/$(ARCH)/libkern/bzero.o \
	boot/$(ARCH)/libkern/udiv.o \
	boot/$(ARCH)/libkern/urem.o \
	boot/$(ARCH)/libkern/__main.o

DEPS += $(STAGE2_OBJS:.o=.d)

boot/$(ARCH)/stage2: $(STAGE2_OBJS)
	ld -dN -Ttext 0x381278 -e start $(STAGE2_OBJS) -o $@

boot/$(ARCH)/libsa/%.o: boot/$(ARCH)/libsa/%.c
	$(CC) $(LIBSACFLAGS) -c $< -o $@

boot/$(ARCH)/libsa/%.d: boot/$(ARCH)/libsa/%.c
	@($(ECHO) -n $(dir $@);$(CC) $(LIBSACFLAGS) -M -MG $<) > $@

boot/$(ARCH)/libkern/%.o: boot/$(ARCH)/libkern/%.c
	$(CC) $(LIBKERNCFLAGS) -c $< -o $@

boot/$(ARCH)/libkern/%.d: boot/$(ARCH)/libkern/%.c
	@($(ECHO) -n $(dir $@);$(CC) $(LIBKERNCFLAGS) -M -MG $<) > $@

boot/$(ARCH)/libkern/%.o: boot/$(ARCH)/libkern/%.S
	$(CC) $(LIBKERNCFLAGS) -c $< -o $@

boot/$(ARCH)/libkern/%.d: boot/$(ARCH)/libkern/%.S
	@($(ECHO) -n $(dir $@);$(CC) $(LIBKERNCFLAGS) -M -MG $<) > $@

boot/$(ARCH)/%.o: boot/$(ARCH)/%.S
	$(CC) $(CFLAGS) -D_LOCORE -c $< -o $@

boot/$(ARCH)/%.d: boot/$(ARCH)/%.S
	@($(ECHO) -n $(dir $@);$(CC) $(CFLAGS) -D_LOCORE -M -MG $<) > $@

boot/$(ARCH)/%.o: boot/$(ARCH)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

boot/$(ARCH)/%.d: boot/$(ARCH)/%.c
	@($(ECHO) -n $(dir $@);$(CC) $(CFLAGS) -M -MG $<) > $@

stage2clean:
	rm -f $(STAGE2_OBJS) boot/$(ARCH)/stage2 boot/$(ARCH)/a.out
