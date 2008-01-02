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
#include <arch/x86/smp_acpi.h>
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

static struct smp_scan_spots_struct acpi_scan_spots[] = {
	{ 0x0, 0x400, 0x400 - 0x0 },
	{ 0xe0000, 0x100000, 0x100000 - 0xe0000 },
	{ 0, 0, 0 }
};

extern "C" void execute_n_instructions(int count);

extern "C" void smp_trampoline(void);
extern "C" void smp_trampoline_end(void);


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


static int
smp_get_current_cpu(void)
{
	if (gKernelArgs.arch_args.apic == NULL)
		return 0;

	return gKernelArgs.arch_args.cpu_os_id[(apic_read(APIC_ID) & 0xffffffff) >> 24];
}


static mp_floating_struct *
smp_mp_probe(uint32 base, uint32 limit)
{
	TRACE(("smp_mp_probe: entry base 0x%lx, limit 0x%lx\n", base, limit));
	for (uint32 *pointer = (uint32 *)base; (uint32)pointer < limit; pointer++) {
		if (*pointer == MP_FLOATING_SIGNATURE) {
			TRACE(("smp_mp_probe: found floating pointer structure at %p\n", pointer));
			return (mp_floating_struct *)pointer;
		}
	}

	return NULL;
}


static acpi_rsdp *
smp_acpi_probe(uint32 base, uint32 limit)
{
	TRACE(("smp_acpi_probe: entry base 0x%lx, limit 0x%lx\n", base, limit));
	for (char *pointer = (char *)base; (uint32)pointer < limit; pointer += 16) {
		if (strncmp(pointer, ACPI_RSDP_SIGNATURE, 8) == 0) {
			TRACE(("smp_acpi_probe: found ACPI RSDP signature at %p\n", pointer));
			return (acpi_rsdp *)pointer;
		}
	}

	return NULL;
}


static status_t
smp_do_mp_config(mp_floating_struct *floatingStruct)
{
	TRACE(("smp: intel mp version %s, %s",
		(floatingStruct->spec_revision == 1) ? "1.1" : "1.4",
		(floatingStruct->mp_feature_2 & 0x80)
			? "imcr and pic compatibility mode.\n"
			: "virtual wire compatibility mode.\n"));

	if (floatingStruct->config_table == NULL) {
#if 1
		// XXX need to implement
		TRACE(("smp: standard configuration %d unimplemented\n", floatingStruct->mp_feature_1));
		gKernelArgs.num_cpus = 1;
		return B_OK;
#else
		/* this system conforms to one of the default configurations */
		TRACE(("smp: standard configuration %d\n", floatingStruct->mp_feature_1));
		gKernelArgs.num_cpus = 2;
		gKernelArgs.cpu_apic_id[0] = 0;
		gKernelArgs.cpu_apic_id[1] = 1;
		apic_phys = (unsigned int *) 0xfee00000;
		ioapic_phys = (unsigned int *) 0xfec00000;
		dprintf("smp: WARNING: standard configuration code is untested");
		return B_OK;
#endif
	}

	/*
	 * we are not running in standard configuration, so we have to look through
	 * all of the mp configuration table crap to figure out how many processors
	 * we have, where our apics are, etc.
	 */
	mp_config_table *config = floatingStruct->config_table;
	gKernelArgs.num_cpus = 0;

	/* print out our new found configuration. */
	TRACE(("smp: oem id: %.8s product id: %.12s\n", config->oem,
		config->product));
	TRACE(("smp: base table has %d entries, extended section %d bytes\n",
		config->num_base_entries, config->ext_length));

	gKernelArgs.arch_args.apic_phys = (uint32)config->apic;

	char *pointer = (char *)((uint32)config + sizeof(struct mp_config_table));
	for (int32 i = 0; i < config->num_base_entries; i++) {
		switch (*pointer) {
			case MP_BASE_PROCESSOR:
			{
				if (gKernelArgs.num_cpus == MAX_BOOT_CPUS) {
					TRACE(("smp: already reached maximum boot CPUs (%d)\n", MAX_BOOT_CPUS));
					pointer += sizeof(struct mp_base_processor);
					break;
				}

				struct mp_base_processor *processor = (struct mp_base_processor *)pointer;

				gKernelArgs.arch_args.cpu_apic_id[gKernelArgs.num_cpus] = processor->apic_id;
				gKernelArgs.arch_args.cpu_os_id[processor->apic_id] = gKernelArgs.num_cpus;
				gKernelArgs.arch_args.cpu_apic_version[gKernelArgs.num_cpus] = processor->apic_version;

#ifdef TRACE_SMP
				const char *cpuFamily[] = { "", "", "", "", "Intel 486",
					"Intel Pentium", "Intel Pentium Pro", "Intel Pentium II" };
#endif
				TRACE(("smp: cpu#%ld: %s, apic id %d, version %d%s\n",
					gKernelArgs.num_cpus, cpuFamily[(processor->signature & 0xf00) >> 8],
					processor->apic_id, processor->apic_version, (processor->cpu_flags & 0x2) ?
					", BSP" : ""));

				gKernelArgs.num_cpus++;
				pointer += sizeof(struct mp_base_processor);
				break;
			}
			case MP_BASE_BUS:
			{
				struct mp_base_bus *bus = (struct mp_base_bus *)pointer;

				TRACE(("smp: bus %d: %c%c%c%c%c%c\n", bus->bus_id,
					bus->name[0], bus->name[1], bus->name[2], bus->name[3],
					bus->name[4], bus->name[5]));

				pointer += sizeof(struct mp_base_bus);
				break;
			}
			case MP_BASE_IO_APIC:
			{
				struct mp_base_ioapic *io = (struct mp_base_ioapic *)pointer;
				gKernelArgs.arch_args.ioapic_phys = (uint32)io->addr;

				TRACE(("smp: found io apic with apic id %d, version %d\n",
					io->ioapic_id, io->ioapic_version));

				pointer += sizeof(struct mp_base_ioapic);
				break;
			}
			case MP_BASE_IO_INTR:
			case MP_BASE_LOCAL_INTR:
			{
				struct mp_base_interrupt *interrupt = (struct mp_base_interrupt *)pointer;

				dprintf("smp: %s int: type %d, source bus %d, irq %3d, dest apic %d, int %3d, polarity %d, trigger mode %d\n",
					interrupt->type == MP_BASE_IO_INTR ? "I/O" : "local",
					interrupt->interrupt_type, interrupt->source_bus_id,
					interrupt->source_bus_irq, interrupt->dest_apic_id,
					interrupt->dest_apic_int, interrupt->polarity,
					interrupt->trigger_mode);
				pointer += sizeof(struct mp_base_interrupt);
				break;
			}
		}
	}

	dprintf("smp: apic @ %p, i/o apic @ %p, total %ld processors detected\n",
		(void *)gKernelArgs.arch_args.apic_phys,
		(void *)gKernelArgs.arch_args.ioapic_phys,
		gKernelArgs.num_cpus);

	return gKernelArgs.num_cpus > 0 ? B_OK : B_ERROR;
}


static status_t
smp_do_acpi_config(acpi_rsdp *rsdp)
{
	TRACE(("smp: using ACPI to detect MP configuration\n"));
	TRACE(("smp: found rsdp at %p oem id: %.6s\n", rsdp, rsdp->oem_id));
	TRACE(("smp: rsdp points to rsdt at 0x%lx\n", rsdp->rsdt_address));

	// reset CPU count
	gKernelArgs.num_cpus = 0;

	// map and validate the root system description table
	acpi_descriptor_header *rsdt
		= (acpi_descriptor_header *)mmu_map_physical_memory(
		rsdp->rsdt_address, B_PAGE_SIZE, kDefaultPageFlags);
	if (!rsdt || strncmp(rsdt->signature, ACPI_RSDT_SIGNATURE, 4) != 0) {
		TRACE(("smp: invalid root system description table\n"));
		return B_ERROR;
	}

	int32 numEntries = (rsdt->length - sizeof(acpi_descriptor_header)) / 4;
	if (numEntries <= 0) {
		TRACE(("smp: root system description table is empty\n"));
		return B_ERROR;
	}

	TRACE(("smp: searching %ld entries for APIC information\n", numEntries));
	uint32 *pointer = (uint32 *)((uint8 *)rsdt + sizeof(acpi_descriptor_header));
	for (int32 j = 0; j < numEntries; j++, pointer++) {
		acpi_descriptor_header *header = (acpi_descriptor_header *)
			mmu_map_physical_memory(*pointer, B_PAGE_SIZE, kDefaultPageFlags);
		if (!header || strncmp(header->signature, ACPI_MADT_SIGNATURE, 4) != 0) {
			// not interesting for us
			TRACE(("smp: skipping uninteresting header '%.4s'\n", header->signature));
			continue;
		}

		acpi_madt *madt = (acpi_madt *)header;
		gKernelArgs.arch_args.apic_phys = madt->local_apic_address;
		TRACE(("smp: local apic address is 0x%lx\n", madt->local_apic_address));

		acpi_apic *apic = (acpi_apic *)((uint8 *)madt + sizeof(acpi_madt));
		acpi_apic *end = (acpi_apic *)((uint8 *)madt + header->length);
		while (apic < end) {
			switch (apic->type) {
				case ACPI_MADT_LOCAL_APIC:
				{
					if (gKernelArgs.num_cpus == MAX_BOOT_CPUS) {
						TRACE(("smp: already reached maximum boot CPUs (%d)\n", MAX_BOOT_CPUS));
						break;
					}

					acpi_local_apic *localApic = (acpi_local_apic *)apic;
					TRACE(("smp: found local APIC with id %u\n", localApic->apic_id));
					if ((localApic->flags & ACPI_LOCAL_APIC_ENABLED) == 0) {
						TRACE(("smp: APIC is disabled and will not be used\n"));
						break;
					}

					gKernelArgs.arch_args.cpu_apic_id[gKernelArgs.num_cpus] = localApic->apic_id;
					gKernelArgs.arch_args.cpu_os_id[localApic->apic_id] = gKernelArgs.num_cpus;
					// ToDo: how to find out? putting 0x10 in to indicate a local apic
					gKernelArgs.arch_args.cpu_apic_version[gKernelArgs.num_cpus] = 0x10;
					gKernelArgs.num_cpus++;
					break;
				}

				case ACPI_MADT_IO_APIC: {
					acpi_io_apic *ioApic = (acpi_io_apic *)apic;
					TRACE(("smp: found io APIC with id %u and address 0x%lx\n",
						ioApic->io_apic_id, ioApic->io_apic_address));
					gKernelArgs.arch_args.ioapic_phys = ioApic->io_apic_address;
					break;
				}
			}

			apic = (acpi_apic *)((uint8 *)apic + apic->length);
		}

		if (gKernelArgs.num_cpus > 0)
			break;
	}

	return gKernelArgs.num_cpus > 0 ? B_OK : B_ERROR;
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

	execute_n_instructions(128 * 20000);

	count = apic_read(APIC_CURRENT_TIMER_COUNT);
	t2 = system_time();

	count = 0xffffffff - count;

	gKernelArgs.arch_args.apic_time_cv_factor = (uint32)((1000000.0/(t2 - t1)) * count);

	TRACE(("APIC ticks/sec = %ld\n", gKernelArgs.arch_args.apic_time_cv_factor));
}


//	#pragma mark -


void
smp_init_other_cpus(void)
{
	void *handle = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);
	if (handle != NULL) {
		if (get_driver_boolean_parameter(handle, B_SAFEMODE_DISABLE_SMP, false, false)) {
			// SMP has been disabled!
			TRACE(("smp disabled per safemode setting\n"));
			gKernelArgs.num_cpus = 1;
		}
		unload_driver_settings(handle);
	}

	if (gKernelArgs.num_cpus < 2)
		return;

	TRACE(("smp: found %ld cpus\n", gKernelArgs.num_cpus));
	TRACE(("smp: apic_phys = %p\n", (void *)gKernelArgs.arch_args.apic_phys));
	TRACE(("smp: ioapic_phys = %p\n", (void *)gKernelArgs.arch_args.ioapic_phys));

	// map in the apic & ioapic
	gKernelArgs.arch_args.apic = (uint32 *)mmu_map_physical_memory(
		gKernelArgs.arch_args.apic_phys, B_PAGE_SIZE, kDefaultPageFlags);
	gKernelArgs.arch_args.ioapic = (uint32 *)mmu_map_physical_memory(
		gKernelArgs.arch_args.ioapic_phys, B_PAGE_SIZE, kDefaultPageFlags);

	TRACE(("smp: apic = %p\n", gKernelArgs.arch_args.apic));
	TRACE(("smp: ioapic = %p\n", gKernelArgs.arch_args.ioapic));

	// calculate how fast the apic timer is
	calculate_apic_timer_conversion_factor();

	for (uint32 i = 1; i < gKernelArgs.num_cpus; i++) {
		// create a final stack the trampoline code will put the ap processor on
		gKernelArgs.cpu_kstack[i].start = (addr_t)mmu_allocate(NULL, KERNEL_STACK_SIZE);
		gKernelArgs.cpu_kstack[i].size = KERNEL_STACK_SIZE;
	}
}


void
smp_boot_other_cpus(void)
{
	if (gKernelArgs.num_cpus < 2)
		return;

	TRACE(("trampolining other cpus\n"));

	// The first 8 MB are identity mapped, either 0x9e000-0x9ffff is reserved for
	// this, or when PXE services are used 0x8b000-0x8cfff.

	// allocate a stack and a code area for the smp trampoline
	// (these have to be < 1M physical, 0xa0000-0xfffff is reserved by the BIOS,
	// and when PXE services are used, the 0x8d000-0x9ffff is also reserved)
#ifdef _PXE_ENV
	uint32 trampolineCode = 0x8b000;
	uint32 trampolineStack = 0x8c000;
#else
	uint32 trampolineCode = 0x9f000;
	uint32 trampolineStack = 0x9e000;
#endif

	// copy the trampoline code over
	memcpy((char *)trampolineCode, (const void*)&smp_trampoline,
		(uint32)&smp_trampoline_end - (uint32)&smp_trampoline);

	// boot the cpus
	for (uint32 i = 1; i < gKernelArgs.num_cpus; i++) {
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
			asm volatile ("pause;");

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
			asm volatile ("pause;");

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
				asm volatile ("pause;");
		}
	}

	TRACE(("done trampolining\n"));
}


void
smp_add_safemode_menus(Menu *menu)
{

	if (gKernelArgs.num_cpus < 2)
		return;

	MenuItem *item = new(nothrow) MenuItem("Disable SMP");
	menu->AddItem(item);
	item->SetData(B_SAFEMODE_DISABLE_SMP);
	item->SetType(MENU_ITEM_MARKABLE);
}


void
smp_init(void)
{
#if NO_SMP
	gKernelArgs.num_cpus = 1;
	return;
#endif

	// first try to find ACPI tables to get MP configuration as it handles
	// physical as well as logical MP configurations as in multiple cpus,
	// multiple cores or hyper threading.
	for (int32 i = 0; acpi_scan_spots[i].length > 0; i++) {
		acpi_rsdp *rsdp = smp_acpi_probe(smp_scan_spots[i].start,
			smp_scan_spots[i].stop);
		if (rsdp != NULL && smp_do_acpi_config(rsdp) == B_OK)
			return;
	}

	// then try to find MPS tables and do configuration based on them
	for (int32 i = 0; smp_scan_spots[i].length > 0; i++) {
		mp_floating_struct *floatingStruct = smp_mp_probe(
			smp_scan_spots[i].start, smp_scan_spots[i].stop);
		if (floatingStruct != NULL && smp_do_mp_config(floatingStruct) == B_OK)
			return;
	}

	// everything failed or we are not running an SMP system
	gKernelArgs.num_cpus = 1;
}
