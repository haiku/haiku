/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
**
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


// ToDo: this should be integrated better with the rest of the loader!

#include <KernelExport.h>

#include <boot/stage2.h>
#include <arch/x86/smp_apic.h>

#include <string.h>

// ToDo: SMP is temporarily disabled!
#define NO_SMP 1

//#define TRACE_SMP
#ifdef TRACE_SMP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define ADDR_MASK 0xfffff000
#define DEFAULT_PAGE_FLAGS (1 | 2) // present/rw
#define STACK_SIZE 2

struct gdt_idt_descr {
	uint16 a;
	uint32 *b;
} _PACKED;


extern void execute_n_instructions(int count);

extern void smp_boot(void);
extern void smp_trampoline(void);
extern void smp_trampoline_end(void);


static uint32 mp_mem_phys = 0;
static uint32 mp_mem_virt = 0;
static struct mp_flt_struct *mp_flt_ptr = NULL;

static int smp_get_current_cpu(void);


static void
sleep(uint64 time)
{
	uint64 start = system_time();

	while(system_time() - start <= time)
		;
}


static uint32
map_page(uint32 paddr, uint32 vaddr)
{
	uint32 *pentry;
	uint32 *pgdir = (uint32 *)(gKernelArgs.arch_args.page_hole + (4*1024*1024-B_PAGE_SIZE));

	// check to see if a page table exists for this range
	if (pgdir[vaddr / B_PAGE_SIZE / 1024] == 0) {
		unsigned int pgtable;
		// we need to allocate a pgtable
		pgtable = gKernelArgs.physical_allocated_range[0].start + gKernelArgs.physical_allocated_range[0].size;
		gKernelArgs.physical_allocated_range[0].size += B_PAGE_SIZE;
		gKernelArgs.arch_args.pgtables[gKernelArgs.arch_args.num_pgtables++] = pgtable;

		// put it in the pgdir
		pgdir[vaddr / B_PAGE_SIZE / 1024] = (pgtable & ADDR_MASK) | DEFAULT_PAGE_FLAGS;

		// zero it out in it's new mapping
		memset((uint32 *)((uint32 *)gKernelArgs.arch_args.page_hole + (vaddr / B_PAGE_SIZE / 1024) * B_PAGE_SIZE), 0, B_PAGE_SIZE);
	}
	// now, fill in the pentry
	pentry = (uint32 *)((uint32 *)gKernelArgs.arch_args.page_hole + vaddr / B_PAGE_SIZE);

	*pentry = (paddr & ADDR_MASK) | DEFAULT_PAGE_FLAGS;

	asm volatile("invlpg (%0)" : : "r" (vaddr));

	return 0;
}


static uint32
apic_read(uint32 offset)
{
	return *(uint32 *)((uint32)gKernelArgs.arch_args.apic + offset);
}


static void
apic_write(uint32 offset, uint32 data)
{
	uint32 *addr = (uint32 *)((uint32)gKernelArgs.arch_args.apic + offset);
	*addr = data;
}

/*
static void *
mp_virt_to_phys(void *ptr)
{
	return ((void *)(((unsigned int)ptr - mp_mem_virt) + mp_mem_phys));
}
*/

static void *
mp_phys_to_virt(void *ptr)
{
	return ((void *)(((uint32)ptr - mp_mem_phys) + mp_mem_virt));
}


static uint32 *
smp_probe(uint32 base, uint32 limit)
{
	uint32 *ptr;

	TRACE(("smp_probe: entry base 0x%lx, limit 0x%lx\n", base, limit));

	for (ptr = (uint32 *) base; (uint32)ptr < limit; ptr++) {
		if (*ptr == MP_FLT_SIGNATURE) {
			TRACE(("smp_probe: found floating pointer structure at %p\n", ptr));
			return ptr;
		}
	}
	return NULL;
}


static void
smp_do_config(void)
{
	char *ptr;
	int i;
	struct mp_config_table *mpc;
	struct mp_ext_pe *pe;
	struct mp_ext_ioapic *io;
	struct mp_ext_bus *bus;
#if TRACE_SMP
	const char *cpu_family[] = { "", "", "", "", "Intel 486",
		"Intel Pentium", "Intel Pentium Pro", "Intel Pentium II" };
#endif

	/*
	 * we are not running in standard configuration, so we have to look through
	 * all of the mp configuration table crap to figure out how many processors
	 * we have, where our apics are, etc.
	 */
	gKernelArgs.num_cpus = 0;

	mpc = mp_phys_to_virt(mp_flt_ptr->mpc);

	/* print out our new found configuration. */
	ptr = (char *) &(mpc->oem[0]);

	TRACE(("smp: oem id: %c%c%c%c%c%c%c%c product id: "
		"%c%c%c%c%c%c%c%c%c%c%c%c\n", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4],
		ptr[5], ptr[6], ptr[7], ptr[8], ptr[9], ptr[10], ptr[11], ptr[12],
		ptr[13], ptr[14], ptr[15], ptr[16], ptr[17], ptr[18], ptr[19]));
	TRACE(("smp: base table has %d entries, extended section %d bytes\n",
		mpc->num_entries, mpc->ext_len));

	gKernelArgs.arch_args.apic_phys = (uint32)mpc->apic;

	ptr = (char *)((uint32)mpc + sizeof(struct mp_config_table));
	for (i = 0; i < mpc->num_entries; i++) {
		switch (*ptr) {
			case MP_EXT_PE:
				pe = (struct mp_ext_pe *) ptr;
				gKernelArgs.arch_args.cpu_apic_id[gKernelArgs.num_cpus] = pe->apic_id;
				gKernelArgs.arch_args.cpu_os_id[pe->apic_id] = gKernelArgs.num_cpus;
				gKernelArgs.arch_args.cpu_apic_version[gKernelArgs.num_cpus] = pe->apic_version;

				TRACE(("smp: cpu#%ld: %s, apic id %d, version %d%s\n",
					gKernelArgs.num_cpus, cpu_family[(pe->signature & 0xf00) >> 8],
					pe->apic_id, pe->apic_version, (pe->cpu_flags & 0x2) ?
					", BSP" : ""));

				ptr += 20;
				gKernelArgs.num_cpus++;
				break;
			case MP_EXT_BUS:
				bus = (struct mp_ext_bus *)ptr;

				TRACE(("smp: bus%d: %c%c%c%c%c%c\n", bus->bus_id,
					bus->name[0], bus->name[1], bus->name[2], bus->name[3],
					bus->name[4], bus->name[5]));

				ptr += 8;
				break;
			case MP_EXT_IO_APIC:
				io = (struct mp_ext_ioapic *) ptr;
				gKernelArgs.arch_args.ioapic_phys = (uint32)io->addr;

				TRACE(("smp: found io apic with apic id %d, version %d\n",
					io->ioapic_id, io->ioapic_version));

				ptr += 8;
				break;
			case MP_EXT_IO_INT:
				ptr += 8;
				break;
			case MP_EXT_LOCAL_INT:
				ptr += 8;
				break;
		}
	}
	dprintf("smp: apic @ %p, i/o apic @ %p, total %ld processors detected\n",
		(void *)gKernelArgs.arch_args.apic_phys, (void *)gKernelArgs.arch_args.ioapic_phys, gKernelArgs.num_cpus);

	// this BIOS looks broken, because it didn't report any cpus (VMWare)
	if (gKernelArgs.num_cpus == 0)
		gKernelArgs.num_cpus = 1;
}


struct smp_scan_spots_struct {
	uint32 start;
	uint32 stop;
	uint32 len;
};

static struct smp_scan_spots_struct smp_scan_spots[] = {
	{ 0x9fc00, 0xa0000, 0xa0000 - 0x9fc00 },
	{ 0xf0000, 0x100000, 0x100000 - 0xf0000 },
	{ 0, 0, 0 }
};


static int
smp_find_mp_config(void)
{
	int i;

	// XXX for now, assume the memory is identity mapped by the 1st stage
	for (i = 0; smp_scan_spots[i].len > 0; i++) {
		mp_flt_ptr = (struct mp_flt_struct *)smp_probe(smp_scan_spots[i].start,
			smp_scan_spots[i].stop);
		if (mp_flt_ptr != NULL)
			break;
	}
#if NO_SMP
	if (0) {
#else
	if (mp_flt_ptr != NULL) {
#endif
		mp_mem_phys = smp_scan_spots[i].start;
		mp_mem_virt = smp_scan_spots[i].start;

		TRACE(("smp_boot: intel mp version %s, %s", (mp_flt_ptr->mp_rev == 1) ? "1.1" :
			"1.4", (mp_flt_ptr->mp_feature_2 & 0x80) ?
			"imcr and pic compatibility mode.\n" : "virtual wire compatibility mode.\n"));

		if (mp_flt_ptr->mpc == 0) {
			// XXX need to implement
#if 1
			gKernelArgs.num_cpus = 1;
			return 1;
#else
			/* this system conforms to one of the default configurations */
//			mp_num_def_config = mp_flt_ptr->mp_feature_1;
			TRACE(("smp: standard configuration %d\n", mp_flt_ptr->mp_feature_1));
/*			num_cpus = 2;
			gKernelArgs.cpu_apic_id[0] = 0;
			gKernelArgs.cpu_apic_id[1] = 1;
			apic_phys = (unsigned int *) 0xfee00000;
			ioapic_phys = (unsigned int *) 0xfec00000;
			kprintf ("smp: WARNING: standard configuration code is untested");
*/
#endif
		} else {
			smp_do_config();
		}
		return gKernelArgs.num_cpus;
	} else {
		gKernelArgs.num_cpus = 1;
		return 1;
	}
}


/** Target function of the trampoline code.
 *	The trampoline code should have the pgdir and a gdt set up for us,
 *	along with us being on the final stack for this processor. We need
 *	to set up the local APIC and load the global idt and gdt. When we're
 *	done, we'll jump into the kernel with the cpu number as an argument.
 */

static int
smp_cpu_ready(void)
{
	uint32 curr_cpu = smp_get_current_cpu();
	struct gdt_idt_descr idt_descr;
	struct gdt_idt_descr gdt_descr;

	TRACE(("smp_cpu_ready: entry cpu %ld\n", curr_cpu));

	// Important.  Make sure supervisor threads can fault on read only pages...
	asm("movl %%eax, %%cr0" : : "a" ((1 << 31) | (1 << 16) | (1 << 5) | 1));
	asm("cld");
	asm("fninit");

	// Set up the final idt
	idt_descr.a = IDT_LIMIT - 1;
	idt_descr.b = (uint32 *)gKernelArgs.arch_args.vir_idt;

	asm("lidt	%0;"
		: : "m" (idt_descr));

	// Set up the final gdt
	gdt_descr.a = GDT_LIMIT - 1;
	gdt_descr.b = (uint32 *)gKernelArgs.arch_args.vir_gdt;

	asm("lgdt	%0;"
		: : "m" (gdt_descr));

	asm("pushl  %0; "					// push the cpu number
		"pushl 	%1;	"					// kernel args
		"pushl 	$0x0;"					// dummy retval for call to main
		"pushl 	%2;	"					// this is the start address
		"ret;		"					// jump.
		: : "r" (curr_cpu), "m" (&gKernelArgs), "g" (gKernelArgs.kernel_image.elf_header.e_entry));

	// no where to return to
	return 0;
}


static int
smp_boot_all_cpus(void)
{
	uint32 trampoline_code;
	uint32 trampoline_stack;
	uint32 i;

	// XXX assume low 1 meg is identity mapped by the 1st stage bootloader
	// and nothing important is in 0x9e000 & 0x9f000

	// allocate a stack and a code area for the smp trampoline
	// (these have to be < 1M physical)
	trampoline_code = 0x9f000; 	// 640kB - 4096 == 0x9f000
	trampoline_stack = 0x9e000; // 640kB - 8192 == 0x9e000
	map_page(0x9f000, 0x9f000);
	map_page(0x9e000, 0x9e000);

	// copy the trampoline code over
	memcpy((char *)trampoline_code, &smp_trampoline,
		(uint32)&smp_trampoline_end - (uint32)&smp_trampoline);

	// boot the cpus
	for (i = 1; i < gKernelArgs.num_cpus; i++) {
		uint32 *final_stack;
		uint32 *final_stack_ptr;
		uint32 *tramp_stack_ptr;
		uint32 config;
		uint32 num_startups;
		uint32 j;

		// create a final stack the trampoline code will put the ap processor on
		gKernelArgs.cpu_kstack[i].start = gKernelArgs.virtual_allocated_range[0].start + gKernelArgs.virtual_allocated_range[0].size;
		gKernelArgs.cpu_kstack[i].size = STACK_SIZE * B_PAGE_SIZE;
		for (j = 0; j < gKernelArgs.cpu_kstack[i].size / B_PAGE_SIZE; j++) {
			// map the pages in
			map_page(gKernelArgs.physical_allocated_range[0].start + gKernelArgs.physical_allocated_range[0].size,
				gKernelArgs.virtual_allocated_range[0].start + gKernelArgs.virtual_allocated_range[0].size);
			gKernelArgs.physical_allocated_range[0].size += B_PAGE_SIZE;
			gKernelArgs.virtual_allocated_range[0].size += B_PAGE_SIZE;
		}

		// set this stack up
		final_stack = (uint32 *)gKernelArgs.cpu_kstack[i].start;
		memset(final_stack, 0, STACK_SIZE * B_PAGE_SIZE);
		final_stack_ptr = (final_stack + (STACK_SIZE * B_PAGE_SIZE) / sizeof(uint32)) - 1;
		*final_stack_ptr = (uint32)&smp_cpu_ready;
		final_stack_ptr--;

		// set the trampoline stack up
		tramp_stack_ptr = (uint32 *)(trampoline_stack + B_PAGE_SIZE - 4);
		// final location of the stack
		*tramp_stack_ptr = ((uint32)final_stack) + STACK_SIZE * B_PAGE_SIZE - sizeof(uint32);
		tramp_stack_ptr--;
		// page dir
		*tramp_stack_ptr = gKernelArgs.arch_args.phys_pgdir;
		tramp_stack_ptr--;

		// put a gdt descriptor at the bottom of the stack
		*((uint16 *)trampoline_stack) = 0x18 - 1; // LIMIT
		*((uint32 *)(trampoline_stack + 2)) = trampoline_stack + 8;

		// put the gdt at the bottom
		memcpy(&((uint32 *)trampoline_stack)[2], (void *)gKernelArgs.arch_args.vir_gdt, 6*4);

		/* clear apic errors */
		if (gKernelArgs.arch_args.cpu_apic_version[i] & 0xf0) {
			apic_write(APIC_ESR, 0);
			apic_read(APIC_ESR);
		}

		/* send (aka assert) INIT IPI */
		config = (apic_read(APIC_ICR2) & 0x00ffffff) | (gKernelArgs.arch_args.cpu_apic_id[i] << 24);
		apic_write(APIC_ICR2, config); /* set target pe */
		config = (apic_read(APIC_ICR1) & 0xfff00000) | 0x0000c500;
		apic_write(APIC_ICR1, config);

		// wait for pending to end
		while ((apic_read(APIC_ICR1) & 0x00001000) == 0x00001000)
			;

		/* deassert INIT */
		config = (apic_read(APIC_ICR2) & 0x00ffffff) | (gKernelArgs.arch_args.cpu_apic_id[i] << 24);
		apic_write(APIC_ICR2, config);
		config = (apic_read(APIC_ICR1) & 0xfff00000) | 0x00008500;
		apic_write(APIC_ICR1, config);

		// wait for pending to end
		while ((apic_read(APIC_ICR1) & 0x00001000) == 0x00001000)
			;

		/* wait 10ms */
		sleep(10000);

		/* is this a local apic or an 82489dx ? */
		num_startups = (gKernelArgs.arch_args.cpu_apic_version[i] & 0xf0) ? 2 : 0;
		for (j = 0; j < num_startups; j++) {
			/* it's a local apic, so send STARTUP IPIs */
			apic_write(APIC_ESR, 0);

			/* set target pe */
			config = (apic_read(APIC_ICR2) & 0xf0ffffff) | (gKernelArgs.arch_args.cpu_apic_id[i] << 24);
			apic_write(APIC_ICR2, config);

			/* send the IPI */
			config = (apic_read(APIC_ICR1) & 0xfff0f800) | APIC_DM_STARTUP |
				(0x9f000 >> 12);
			apic_write(APIC_ICR1, config);

			/* wait */
			sleep(200);

			while ((apic_read(APIC_ICR1)& 0x00001000) == 0x00001000)
				;
		}
	}

	return 0;
}


static void
calculate_apic_timer_conversion_factor(void)
{
	int64 t1, t2;
	uint32 config;
	uint32 count;

	// setup the timer
	config = apic_read(APIC_LVTT);
	config = (config & ~APIC_LVTT_MASK) + APIC_LVTT_M; // timer masked, vector 0
	apic_write(APIC_LVTT, config);

	config = (apic_read(APIC_TDCR) & ~0x0000000f) + 0xb; // divide clock by one
	apic_write(APIC_TDCR, config);

	t1 = system_time();
	apic_write(APIC_ICRT, 0xffffffff); // start the counter

	execute_n_instructions(128*20000);

	count = apic_read(APIC_CCRT);
	t2 = system_time();

	count = 0xffffffff - count;

	gKernelArgs.arch_args.apic_time_cv_factor = (uint32)((1000000.0/(t2 - t1)) * count);

	TRACE(("APIC ticks/sec = %ld\n", gKernelArgs.arch_args.apic_time_cv_factor));
}


void
smp_boot(void)
{
//	dprintf("smp_boot: entry\n");

	if (smp_find_mp_config() > 1) {
		TRACE(("smp_boot: had found > 1 cpus\n"));
		TRACE(("post config:\n"));
		TRACE(("num_cpus = %ld\n", gKernelArgs.num_cpus));
		TRACE(("apic_phys = %p\n", (void *)gKernelArgs.arch_args.apic_phys));
		TRACE(("ioapic_phys = %p\n", (void *)gKernelArgs.arch_args.ioapic_phys));

		// map in the apic & ioapic
		map_page(gKernelArgs.arch_args.apic_phys, gKernelArgs.virtual_allocated_range[0].start + gKernelArgs.virtual_allocated_range[0].size);
		gKernelArgs.arch_args.apic = (uint32 *)(gKernelArgs.virtual_allocated_range[0].start + gKernelArgs.virtual_allocated_range[0].size);
		gKernelArgs.virtual_allocated_range[0].size += B_PAGE_SIZE;

		map_page(gKernelArgs.arch_args.ioapic_phys, gKernelArgs.virtual_allocated_range[0].start + gKernelArgs.virtual_allocated_range[0].size);
		gKernelArgs.arch_args.ioapic = (uint32 *)(gKernelArgs.virtual_allocated_range[0].start + gKernelArgs.virtual_allocated_range[0].size);
		gKernelArgs.virtual_allocated_range[0].size += B_PAGE_SIZE;

		TRACE(("apic = %p\n", gKernelArgs.arch_args.apic));
		TRACE(("ioapic = %p\n", gKernelArgs.arch_args.ioapic));

		// calculate how fast the apic timer is
		calculate_apic_timer_conversion_factor();

		TRACE(("trampolining other cpus\n"));
		smp_boot_all_cpus();
		TRACE(("done trampolining\n"));
	}

	TRACE(("smp_boot: exit\n"));
}


static int
smp_get_current_cpu(void)
{
	if (gKernelArgs.arch_args.apic == NULL)
		return 0;

	return gKernelArgs.arch_args.cpu_os_id[(apic_read(APIC_ID) & 0xffffffff) >> 24];
}
