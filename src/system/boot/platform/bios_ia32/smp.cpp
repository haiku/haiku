/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
*/


#include "smp.h"
#include "mmu.h"

#include <KernelExport.h>

#include <kernel.h>
#include <safemode.h>
#include <boot/stage2.h>
#include <boot/menu.h>
#include <arch/x86/smp_apic.h>
#include <arch/x86/arch_system_info.h>

#include <string.h>

#define NO_SMP 0

#define TRACE_SMP
#ifdef TRACE_SMP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

struct gdt_idt_descr {
	uint16 a;
	uint32 *b;
} _PACKED;

struct smp_scan_spots_struct {
	uint32 start;
	uint32 stop;
	uint32 length;
};

static struct smp_scan_spots_struct smp_scan_spots[] = {
	{ 0x9fc00, 0xa0000, 0xa0000 - 0x9fc00 },
	{ 0xf0000, 0x100000, 0x100000 - 0xf0000 },
	{ 0, 0, 0 }
};


extern "C" void execute_n_instructions(int count);

extern "C" void smp_trampoline(void);
extern "C" void smp_trampoline_end(void);


static struct mp_floating_struct *sFloatingStruct = NULL;

static int smp_get_current_cpu(void);


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


static bool
supports_hyper_threading(void)
{
	cpuid_info info;
	if (get_current_cpuid(&info, 0) == B_OK
		&& !strncmp(info.eax_0.vendor_id, "GenuineIntel", 12)
		&& info.eax_0.max_eax > 0) {
		if (get_current_cpuid(&info, 1) == B_OK)
			return (info.eax_1.features & (1 << 28)) != 0;
	}

	return false;
}


static int
smp_get_current_cpu(void)
{
	if (gKernelArgs.arch_args.apic == NULL)
		return 0;

	return gKernelArgs.arch_args.cpu_os_id[(apic_read(APIC_ID) & 0xffffffff) >> 24];
}


static uint32 *
smp_probe(uint32 base, uint32 limit)
{
	uint32 *ptr;

	TRACE(("smp_probe: entry base 0x%lx, limit 0x%lx\n", base, limit));

	for (ptr = (uint32 *) base; (uint32)ptr < limit; ptr++) {
		if (*ptr == MP_FLOATING_SIGNATURE) {
			TRACE(("smp_probe: found floating pointer structure at %p\n", ptr));
			return ptr;
		}
	}
	return NULL;
}


static void
smp_do_config(void)
{
	struct mp_config_table *config;
	char *ptr;
	int i;
#ifdef TRACE_SMP
	const char *cpu_family[] = { "", "", "", "", "Intel 486",
		"Intel Pentium", "Intel Pentium Pro", "Intel Pentium II" };
#endif

	/*
	 * we are not running in standard configuration, so we have to look through
	 * all of the mp configuration table crap to figure out how many processors
	 * we have, where our apics are, etc.
	 */
	gKernelArgs.num_cpus = 0;

	config = sFloatingStruct->config_table;

	/* print out our new found configuration. */

	ptr = (char *)&(config->oem[0]);
	TRACE(("smp: oem id: %c%c%c%c%c%c%c%c product id: "
		"%c%c%c%c%c%c%c%c%c%c%c%c\n", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4],
		ptr[5], ptr[6], ptr[7], ptr[8], ptr[9], ptr[10], ptr[11], ptr[12],
		ptr[13], ptr[14], ptr[15], ptr[16], ptr[17], ptr[18], ptr[19]));
	TRACE(("smp: base table has %d entries, extended section %d bytes\n",
		config->num_base_entries, config->ext_length));

	gKernelArgs.arch_args.apic_phys = (uint32)config->apic;

	ptr = (char *)((uint32)config + sizeof(struct mp_config_table));
	for (i = 0; i < config->num_base_entries; i++) {
		switch (*ptr) {
			case MP_BASE_PROCESSOR:
			{
				struct mp_base_processor *processor = (struct mp_base_processor *)ptr;

				gKernelArgs.arch_args.cpu_apic_id[gKernelArgs.num_cpus] = processor->apic_id;
				gKernelArgs.arch_args.cpu_os_id[processor->apic_id] = gKernelArgs.num_cpus;
				gKernelArgs.arch_args.cpu_apic_version[gKernelArgs.num_cpus] = processor->apic_version;

				TRACE(("smp: cpu#%ld: %s, apic id %d, version %d%s\n",
					gKernelArgs.num_cpus, cpu_family[(processor->signature & 0xf00) >> 8],
					processor->apic_id, processor->apic_version, (processor->cpu_flags & 0x2) ?
					", BSP" : ""));

				gKernelArgs.num_cpus++;
				ptr += sizeof(struct mp_base_processor);
				break;
			}
			case MP_BASE_BUS:
			{
				struct mp_base_bus *bus = (struct mp_base_bus *)ptr;

				TRACE(("smp: bus %d: %c%c%c%c%c%c\n", bus->bus_id,
					bus->name[0], bus->name[1], bus->name[2], bus->name[3],
					bus->name[4], bus->name[5]));

				ptr += sizeof(struct mp_base_bus);
				break;
			}
			case MP_BASE_IO_APIC:
			{
				struct mp_base_ioapic *io = (struct mp_base_ioapic *) ptr;
				gKernelArgs.arch_args.ioapic_phys = (uint32)io->addr;

				TRACE(("smp: found io apic with apic id %d, version %d\n",
					io->ioapic_id, io->ioapic_version));

				ptr += sizeof(struct mp_base_ioapic);
				break;
			}
			case MP_BASE_IO_INTR:
			case MP_BASE_LOCAL_INTR:
			{
				struct mp_base_interrupt *interrupt = (struct mp_base_interrupt *)ptr;

				dprintf("smp: %s int: type %d, source bus %d, irq %d, dest apic %d, int %d, polarity %d, trigger mode %d\n",
					interrupt->type == MP_BASE_IO_INTR ? "I/O" : "local",
					interrupt->interrupt_type, interrupt->source_bus_id,
					interrupt->source_bus_irq, interrupt->dest_apic_id,
					interrupt->dest_apic_int, interrupt->polarity,
					interrupt->trigger_mode);
				ptr += sizeof(struct mp_base_interrupt);
				break;
			}
		}
	}

	dprintf("smp: apic @ %p, i/o apic @ %p, total %ld processors detected\n",
		(void *)gKernelArgs.arch_args.apic_phys,
		(void *)gKernelArgs.arch_args.ioapic_phys,
		gKernelArgs.num_cpus);

	// Try to detect single CPU hyper threading setup
	// ToDo: this should be done using the ACPI APIC table
	// ToDo: this only works with a single HT enabled CPU anyway

	if (gKernelArgs.num_cpus == 1 && supports_hyper_threading()) {
		cpuid_info info;
		get_current_cpuid(&info, 1);

		dprintf("CPU supports Hyper-Threading, %ld processors in package\n",
			info.eax_1.logical_cpus);

		// enable the second logical processor
/*
		gKernelArgs.num_cpus = 2;
		gKernelArgs.arch_args.cpu_apic_id[1] = gKernelArgs.arch_args.cpu_apic_id[0] + 1;
		gKernelArgs.arch_args.cpu_os_id[gKernelArgs.arch_args.cpu_apic_id[1]] = 1;
		gKernelArgs.arch_args.cpu_apic_version[1] = gKernelArgs.arch_args.cpu_apic_version[0];;
*/
	}

	// this BIOS looks broken, because it didn't report any cpus (VMWare)
	if (gKernelArgs.num_cpus == 0)
		gKernelArgs.num_cpus = 1;
}


static int
smp_find_mp_config(void)
{
	int i;

#if NO_SMP
	if (1)
		return gKernelArgs.num_cpus = 1;
#endif

	for (i = 0; smp_scan_spots[i].length > 0; i++) {
		sFloatingStruct = (struct mp_floating_struct *)smp_probe(smp_scan_spots[i].start,
			smp_scan_spots[i].stop);
		if (sFloatingStruct != NULL)
			break;
	}

	if (sFloatingStruct != NULL) {
		TRACE(("smp_boot: intel mp version %s, %s",
			(sFloatingStruct->spec_revision == 1) ? "1.1" : "1.4",
			(sFloatingStruct->mp_feature_2 & 0x80)
				? "imcr and pic compatibility mode.\n"
				: "virtual wire compatibility mode.\n"));

		if (sFloatingStruct->config_table == NULL) {
			// XXX need to implement
#if 1
			gKernelArgs.num_cpus = 1;
			return 1;
#else
			/* this system conforms to one of the default configurations */
//			mp_num_def_config = sFloatingStruct->mp_feature_1;
			TRACE(("smp: standard configuration %d\n", sFloatingStruct->mp_feature_1));
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
	}

	return gKernelArgs.num_cpus = 1;
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

	//TRACE(("smp_cpu_ready: entry cpu %ld\n", curr_cpu));

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
		: : "g" (curr_cpu), "g" (&gKernelArgs), "g" (gKernelArgs.kernel_image.elf_header.e_entry));

	// no where to return to
	return 0;
}


static void
calculate_apic_timer_conversion_factor(void)
{
	int64 t1, t2;
	uint32 config;
	uint32 count;

	// setup the timer
	config = apic_read(APIC_LVT_TIMER);
	config = (config & APIC_LVT_TIMER_MASK) + APIC_LVT_MASKED; // timer masked, vector 0
	apic_write(APIC_LVT_TIMER, config);

	config = (apic_read(APIC_TIMER_DIVIDE_CONFIG) & ~0x0000000f);
	apic_write(APIC_TIMER_DIVIDE_CONFIG, config | APIC_TIMER_DIVIDE_CONFIG_1);
		// divide clock by one

	t1 = system_time();
	apic_write(APIC_INITIAL_TIMER_COUNT, 0xffffffff); // start the counter

	execute_n_instructions(128*20000);

	count = apic_read(APIC_CURRENT_TIMER_COUNT);
	t2 = system_time();

	count = 0xffffffff - count;

	gKernelArgs.arch_args.apic_time_cv_factor = (uint32)((1000000.0/(t2 - t1)) * count);

	TRACE(("APIC ticks/sec = %ld\n", gKernelArgs.arch_args.apic_time_cv_factor));
}


//	#pragma mark -


void
smp_boot_other_cpus(void)
{
	uint32 trampolineCode;
	uint32 trampolineStack;
	uint32 i;

	void *handle = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);
	if (handle != NULL) {
		if (get_driver_boolean_parameter(handle, B_SAFEMODE_DISABLE_SMP, false, false)) {
			// SMP has been disabled!
			gKernelArgs.num_cpus = 1;
		}
		unload_driver_settings(handle);
	}

	if (gKernelArgs.num_cpus < 2)
		return;

	TRACE(("trampolining other cpus\n"));

	// The first 8 MB are identity mapped, 0x9e000-0x9ffff is reserved for this

	// allocate a stack and a code area for the smp trampoline
	// (these have to be < 1M physical, 0xa0000-0xfffff is reserved by the BIOS)
	trampolineCode = 0x9f000;
	trampolineStack = 0x9e000;

	// copy the trampoline code over
	memcpy((char *)trampolineCode, (const void*)&smp_trampoline,
		(uint32)&smp_trampoline_end - (uint32)&smp_trampoline);

	// boot the cpus
	for (i = 1; i < gKernelArgs.num_cpus; i++) {
		uint32 *finalStack;
		uint32 *tempStack;
		uint32 config;
		uint32 numStartups;
		uint32 j;

		// set this stack up
		finalStack = (uint32 *)gKernelArgs.cpu_kstack[i].start;
		memset(finalStack, 0, KERNEL_STACK_SIZE);
		tempStack = (finalStack + KERNEL_STACK_SIZE / sizeof(uint32)) - 1;
		*tempStack = (uint32)&smp_cpu_ready;

		// set the trampoline stack up
		tempStack = (uint32 *)(trampolineStack + B_PAGE_SIZE - 4);
		// final location of the stack
		*tempStack = ((uint32)finalStack) + KERNEL_STACK_SIZE - sizeof(uint32);
		tempStack--;
		// page dir
		*tempStack = gKernelArgs.arch_args.phys_pgdir;

		// put a gdt descriptor at the bottom of the stack
		*((uint16 *)trampolineStack) = 0x18 - 1; // LIMIT
		*((uint32 *)(trampolineStack + 2)) = trampolineStack + 8;

		// put the gdt at the bottom
		memcpy(&((uint32 *)trampolineStack)[2], (void *)gKernelArgs.arch_args.vir_gdt, 6*4);

		/* clear apic errors */
		if (gKernelArgs.arch_args.cpu_apic_version[i] & 0xf0) {
			apic_write(APIC_ERROR_STATUS, 0);
			apic_read(APIC_ERROR_STATUS);
		}

//dprintf("assert INIT\n");
		/* send (aka assert) INIT IPI */
		config = (apic_read(APIC_INTR_COMMAND_2) & APIC_INTR_COMMAND_2_MASK)
			| (gKernelArgs.arch_args.cpu_apic_id[i] << 24);
		apic_write(APIC_INTR_COMMAND_2, config); /* set target pe */
		config = (apic_read(APIC_INTR_COMMAND_1) & 0xfff00000)
			| APIC_TRIGGER_MODE_LEVEL | APIC_INTR_COMMAND_1_ASSERT | APIC_DELIVERY_MODE_INIT;
		apic_write(APIC_INTR_COMMAND_1, config);

dprintf("wait for delivery\n");
		// wait for pending to end
		while ((apic_read(APIC_INTR_COMMAND_1) & APIC_DELIVERY_STATUS) != 0)
			;

dprintf("deassert INIT\n");
		/* deassert INIT */
		config = (apic_read(APIC_INTR_COMMAND_2) & APIC_INTR_COMMAND_2_MASK)
			| (gKernelArgs.arch_args.cpu_apic_id[i] << 24);
		apic_write(APIC_INTR_COMMAND_2, config);
		config = (apic_read(APIC_INTR_COMMAND_1) & 0xfff00000)
			| APIC_TRIGGER_MODE_LEVEL | APIC_DELIVERY_MODE_INIT;
		apic_write(APIC_INTR_COMMAND_1, config);

dprintf("wait for delivery\n");
		// wait for pending to end
		while ((apic_read(APIC_INTR_COMMAND_1) & APIC_DELIVERY_STATUS) != 0)
			;

		/* wait 10ms */
		spin(10000);

		/* is this a local apic or an 82489dx ? */
		numStartups = (gKernelArgs.arch_args.cpu_apic_version[i] & 0xf0) ? 2 : 0;
dprintf("num startups = %ld\n", numStartups);
		for (j = 0; j < numStartups; j++) {
			/* it's a local apic, so send STARTUP IPIs */
dprintf("send STARTUP\n");
			apic_write(APIC_ERROR_STATUS, 0);

			/* set target pe */
			config = (apic_read(APIC_INTR_COMMAND_2) & APIC_INTR_COMMAND_2_MASK)
				| (gKernelArgs.arch_args.cpu_apic_id[i] << 24);
			apic_write(APIC_INTR_COMMAND_2, config);

			/* send the IPI */
			config = (apic_read(APIC_INTR_COMMAND_1) & 0xfff0f800)
				| APIC_DELIVERY_MODE_STARTUP | (trampolineCode >> 12);
			apic_write(APIC_INTR_COMMAND_1, config);

			/* wait */
			spin(200);

dprintf("wait for delivery\n");
			while ((apic_read(APIC_INTR_COMMAND_1) & APIC_DELIVERY_STATUS) != 0)
				;
		}
	}

	TRACE(("done trampolining\n"));
}


void
smp_add_safemode_menus(Menu *menu)
{
	MenuItem *item;

	if (gKernelArgs.num_cpus < 2)
		return;

	// ToDo: this should work with dual CPUs with HT as well!
	if (gKernelArgs.num_cpus > 2 || !supports_hyper_threading()) {
		menu->AddItem(item = new(nothrow) MenuItem("Disable SMP"));
		item->SetData(B_SAFEMODE_DISABLE_SMP);
		item->SetType(MENU_ITEM_MARKABLE);
	}

	if (supports_hyper_threading()) {
		menu->AddItem(item = new(nothrow) MenuItem("Disable Hyper-Threading"));
		item->SetData(B_SAFEMODE_DISABLE_HYPER_THREADING);
		item->SetType(MENU_ITEM_MARKABLE);
	}
}


void
smp_init(void)
{
	if (smp_find_mp_config() > 1) {
		uint32 i;

		TRACE(("smp_boot: had found > 1 cpus\n"));
		TRACE(("post config:\n"));
		TRACE(("num_cpus = %ld\n", gKernelArgs.num_cpus));
		TRACE(("apic_phys = %p\n", (void *)gKernelArgs.arch_args.apic_phys));
		TRACE(("ioapic_phys = %p\n", (void *)gKernelArgs.arch_args.ioapic_phys));

		// map in the apic & ioapic
		gKernelArgs.arch_args.apic = (uint32 *)mmu_map_physical_memory(
			gKernelArgs.arch_args.apic_phys, B_PAGE_SIZE, kDefaultPageFlags);
		gKernelArgs.arch_args.ioapic = (uint32 *)mmu_map_physical_memory(
			gKernelArgs.arch_args.ioapic_phys, B_PAGE_SIZE, kDefaultPageFlags);

		TRACE(("apic = %p\n", gKernelArgs.arch_args.apic));
		TRACE(("ioapic = %p\n", gKernelArgs.arch_args.ioapic));

		// calculate how fast the apic timer is
		calculate_apic_timer_conversion_factor();

		for (i = 1; i < gKernelArgs.num_cpus; i++) {
			// create a final stack the trampoline code will put the ap processor on
			gKernelArgs.cpu_kstack[i].start = (addr_t)mmu_allocate(NULL, KERNEL_STACK_SIZE);
			gKernelArgs.cpu_kstack[i].size = KERNEL_STACK_SIZE;
		}
	}
}


