/*
 * Copyright 2008-2020, François Revol, revol@free.fr. All rights reserved.
 * Copyright 2003-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>
#include <boot/platform.h>
#include <boot/heap.h>
#include <boot/stage2.h>
#include <arch/cpu.h>

#include <string.h>

#include "console.h"
#include "cpu.h"
#include "mmu.h"
#include "keyboard.h"
#include "nextrom.h"

#define HEAP_SIZE 65536

// GCC defined globals
extern void (*__ctor_list)(void);
extern void (*__ctor_end)(void);
extern uint8 __bss_start;
extern uint8 _end;

extern "C" int main(stage2_args *args);
extern "C" void _start(void);

// the boot rom monitor entry point
struct mon_global *mg = NULL;

static uint32 sBootOptions;


static void
clear_bss(void)
{
	memset(&__bss_start, 0, &_end - &__bss_start);
}


static void
call_ctors(void)
{
	void (**f)(void);

	for (f = &__ctor_list; f < &__ctor_end; f++) {
		(**f)();
	}
}


extern "C" uint32
platform_boot_options(void)
{
#if 0
	if (!gKernelArgs.fb.enabled)
		sBootOptions |= check_for_boot_keys();
#endif
	return sBootOptions;
}


extern "C" void
platform_start_kernel(void)
{
#if 0
	static struct kernel_args *args = &gKernelArgs;
		// something goes wrong when we pass &gKernelArgs directly
		// to the assembler inline below - might be a bug in GCC
		// or I don't see something important...
	addr_t stackTop
		= gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size;

	preloaded_elf32_image *image = static_cast<preloaded_elf32_image *>(
		gKernelArgs.kernel_image.Pointer());

	//smp_init_other_cpus();
	//serial_cleanup();
	mmu_init_for_kernel();
	//smp_boot_other_cpus();

#warning M68K: stop ints

	dprintf("kernel entry at %lx\n", image->elf_header.e_entry);

	asm volatile (
		"move.l	%0,%%sp;	"			// move stack out of way
		: : "m" (stackTop));

	asm volatile (
		"or	#0x0700,%%sr; " : : );		// disable interrupts

	asm volatile (
		"move.l  #0x0,-(%%sp); "		// we're the BSP cpu (0)
		"move.l 	%0,-(%%sp);	"		// kernel args
		"move.l 	#0x0,-(%%sp);"		// dummy retval for call to main
		"move.l 	%1,-(%%sp);	"		// this is the start address
		"rts;		"					// jump.
		: : "g" (args), "g" (image->elf_header.e_entry));

	// Huston, we have a problem!

	asm volatile (
		"and	#0xf8ff,%%sr; " : : );		// reenable interrupts

	panic("kernel returned!\n");
#endif

}


extern "C" void
platform_exit(void)
{
	// TODO
	while (true);
}

inline void dump_mg(struct mon_global *mg)
{
	int i;
	dprintf("mg@ %p\n", (void*)mg);

	dprintf("mg_flags\t%x\n", (unsigned char)mg->mg_flags);
	dprintf("mg_sid\t%x\n", mg->mg_sid);
	dprintf("mg_pagesize\t%x\n", mg->mg_pagesize);
	dprintf("mg_mon_stack\t%x\n", mg->mg_mon_stack);
	dprintf("mg_vbr\t%x\n", mg->mg_vbr);
	dprintf("mg_console_i\t%x\n", mg->mg_console_i);
	dprintf("mg_console_o\t%x\n", mg->mg_console_o);

	for (i = 0; i < N_SIMM; i++) {
		dprintf("mg_region[%d] = {%08lx, %08lx}\n", i,
				mg->mg_region[i].first_phys_addr,
				mg->mg_region[i].last_phys_addr);
	}

	dprintf("mg_boot_dev\t%s\n", mg->mg_boot_dev);
	dprintf("mg_boot_arg\t%s\n", mg->mg_boot_arg);
	dprintf("mg_boot_info\t%s\n", mg->mg_boot_info);
	dprintf("mg_boot_file\t%s\n", mg->mg_boot_file);
	dprintf("mg_boot_dev\t%s\n", mg->mg_boot_dev);
	dprintf("mg_boot_how\t%d\n", mg->mg_boot_how);

	dprintf("mg_sddp\t%p\n", mg->mg_sddp);
	dprintf("mg_dgp\t%p\n", mg->mg_dgp);
		// "dlV3" (disk label signature ?) SCSI boot gives "@dlV3"
	dprintf("mg_fdgp\t%p\n", mg->mg_fdgp);	// "A" ?? SCSI: "to become ready"
	dprintf("mg_s5cp\t%p\n", mg->mg_s5cp);
	dprintf("mg_odc\t%p\n", mg->mg_odc);
	dprintf("mg_odd\t%p\n", mg->mg_odd);

#if 0
	for (int i = 0; i < sizeof(struct mon_global); i++) {
		uint8 *p = ((uint8 *)mg)+i;
		if (i % 32 == 0)
			dprintf("%04x", i);
		if (i % 4 == 0)
			dprintf(" ");
		if (i % 8 == 0)
			dprintf(" ");
		dprintf("%02x", *p);
		if (i % 32 == 31 || i == sizeof(struct mon_global) - 1)
			dprintf("\n");
	}
	//while(true);
#endif


	dprintf("mg_si\t%p\n", mg->mg_si);
	/* XXX:the pointer seems completely random, sadly.
	 * Possibly the kernel is supposed to set it?
	 */
#if 0
	for (int i = 0; i < sizeof(struct sio); i++) {
		uint8 *p = ((uint8 *)mg->mg_si)+i;
		if (i % 32 == 0)
			dprintf("%04x", i);
		if (i % 4 == 0)
			dprintf(" ");
		if (i % 8 == 0)
			dprintf(" ");
		dprintf("%02x", *p);
		if (i % 32 == 31 || i == sizeof(struct sio) - 1)
			dprintf("\n");
	}
	for (i = 0; i < 5; i++) {
		struct sio *s = &mg->mg_si[i];
		dprintf("sio[%d] = {%08x, %u, %u, %u, %p, %u, %u, %p, %p, %p}\n", i,
				s->si_args, s->si_ctrl, s->si_unit, s->si_part,
				s->si_dev, s->si_blklen, s->si_lastlba,
				s->si_sadmem, s->si_protomem, s->si_devmem);
		s++;
	}
#endif

	//dprintf("test_msg\t%p\n", mg->test_msg);
	/* Framebuffer info */
#if 1
	dprintf("km_coni = {%d, %d, %d, %d, %d, ... %d, %d, %d, ...%d}\n",
			mg->km_coni.pixels_per_word,
			mg->km_coni.bytes_per_scanline,
			mg->km_coni.dspy_w,
			mg->km_coni.dspy_max_w,
			mg->km_coni.dspy_h,
			//...
			mg->km_coni.slot_num,
			mg->km_coni.fb_num,
			mg->km_coni.byte_lane_id,
			mg->km_coni.access_stack);
	for (i = 0; i < KM_CON_MAP_ENTRIES; i++) {
		dprintf("km_coni.map_addr[%d] = {%08lx, %08lx, %08lx}\n", i,
				mg->km_coni.map_addr[i].phys_addr,
				mg->km_coni.map_addr[i].virt_addr,
				mg->km_coni.map_addr[i].size);
	}
#endif

	//XXX: this one crashes on older ROMs
	//dprintf("mg_cpu_clk\t%d\n", mg->mg_cpu_clk);
}


extern "C" void *
start_next(const char *boot_args, struct mon_global *monitor)
{
	stage2_args args;

	monitor->mg_putc('H');

	//asm("cld");			// Ain't nothing but a GCC thang.
	//asm("fninit");		// initialize floating point unit

	//clear_bss();
	/* save monitor ROM entry */
	mg = monitor;
	// DEBUG
	mg->mg_putc('A');

	call_ctors();
		// call C++ constructors before doing anything else
	mg->mg_putc('I');

	args.heap_size = HEAP_SIZE;
	args.arguments = NULL;

	//serial_init();
	mg->mg_putc('K');
	console_init();
	mg->mg_putc('U');
	mg->mg_putc('\n');

	dump_mg(mg);
	//while(1);
	//return NULL;
	cpu_init();
#if 0
	// TODO
	mmu_init();

	// wait a bit to give the user the opportunity to press a key
	spin(750000);

	// reading the keyboard doesn't seem to work in graphics mode (maybe a bochs problem)
	sBootOptions = check_for_boot_keys();
	//if (sBootOptions & BOOT_OPTION_DEBUG_OUTPUT)
		//serial_enable();

	//apm_init();
	//smp_init();
#endif
	main(&args);
	return NULL;
}
