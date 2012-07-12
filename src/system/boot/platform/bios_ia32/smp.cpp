/*
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Copyright 2004-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
*/


#include "smp.h"

#include <string.h>

#include <KernelExport.h>

#include <kernel.h>
#include <safemode.h>
#include <boot/stage2.h>
#include <boot/menu.h>
#include <arch/x86/apic.h>
#include <arch/x86/arch_acpi.h>
#include <arch/x86/arch_cpu.h>
#include <arch/x86/arch_smp.h>
#include <arch/x86/arch_system_info.h>
#include <arch/x86/descriptors.h>

#include "mmu.h"
#include "acpi.h"
#include "hpet.h"


#define NO_SMP 0

#define TRACE_SMP
#ifdef TRACE_SMP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static struct scan_spots_struct smp_scan_spots[] = {
	{ 0x9fc00, 0xa0000, 0xa0000 - 0x9fc00 },
	{ 0xf0000, 0x100000, 0x100000 - 0xf0000 },
	{ 0, 0, 0 }
};

extern "C" void execute_n_instructions(int count);

extern "C" void smp_trampoline(void);
extern "C" void smp_trampoline_end(void);


static uint32
apic_read(uint32 offset)
{
	return *(volatile uint32 *)((addr_t)(void *)gKernelArgs.arch_args.apic + offset);
}


static void
apic_write(uint32 offset, uint32 data)
{
	*(volatile uint32 *)((addr_t)(void *)gKernelArgs.arch_args.apic + offset) = data;
}


static mp_floating_struct *
smp_mp_probe(uint32 base, uint32 limit)
{
	TRACE(("smp_mp_probe: entry base 0x%lx, limit 0x%lx\n", base, limit));
	for (uint32 *pointer = (uint32 *)base; (uint32)pointer < limit; pointer++) {
		if (*pointer == MP_FLOATING_SIGNATURE) {
			TRACE(("smp_mp_probe: found floating pointer structure at %p\n",
				pointer));
			return (mp_floating_struct *)pointer;
		}
	}

	return NULL;
}


static status_t
smp_do_mp_config(mp_floating_struct *floatingStruct)
{
	if (floatingStruct->config_length != 1) {
		TRACE(("smp: unsupported structure length of %" B_PRIu8 " units\n",
			floatingStruct->config_length));
		return B_UNSUPPORTED;
	}

	TRACE(("smp: intel mp version %s, %s",
		(floatingStruct->spec_revision == 1) ? "1.1" : "1.4",
		(floatingStruct->mp_feature_2 & 0x80)
			? "imcr and pic compatibility mode.\n"
			: "virtual wire compatibility mode.\n"));

	if (floatingStruct->config_table == NULL) {
#if 1
		// TODO: need to implement
		TRACE(("smp: standard configuration %d unimplemented\n",
			floatingStruct->mp_feature_1));
		gKernelArgs.num_cpus = 1;
		return B_OK;
#else
		// this system conforms to one of the default configurations
		TRACE(("smp: standard configuration %d\n", floatingStruct->mp_feature_1));
		gKernelArgs.num_cpus = 2;
		gKernelArgs.cpu_apic_id[0] = 0;
		gKernelArgs.cpu_apic_id[1] = 1;
		apic_phys = (unsigned int *)0xfee00000;
		ioapic_phys = (unsigned int *)0xfec00000;
		dprintf("smp: WARNING: standard configuration code is untested");
		return B_OK;
#endif
	}

	// We are not running in standard configuration, so we have to look through
	// all of the mp configuration table crap to figure out how many processors
	// we have, where our apics are, etc.

	mp_config_table *config = floatingStruct->config_table;
	gKernelArgs.num_cpus = 0;

	if (config->signature != MP_CONFIG_TABLE_SIGNATURE) {
		TRACE(("smp: invalid config table signature, aborting\n"));
		return B_ERROR;
	}

	if (config->base_table_length < sizeof(mp_config_table)) {
		TRACE(("smp: config table length %" B_PRIu16
			" too short for structure, aborting\n",
			config->base_table_length));
		return B_ERROR;
	}

	// print our new found configuration.
	TRACE(("smp: oem id: %.8s product id: %.12s\n", config->oem,
		config->product));
	TRACE(("smp: base table has %d entries, extended section %d bytes\n",
		config->num_base_entries, config->ext_length));

	gKernelArgs.arch_args.apic_phys = (uint32)config->apic;
	if ((gKernelArgs.arch_args.apic_phys % 4096) != 0) {
		// MP specs mandate a 4K alignment for the local APIC(s)
		TRACE(("smp: local apic %p has bad alignment, aborting\n",
			(void *)gKernelArgs.arch_args.apic_phys));
		return B_ERROR;
	}

	char *pointer = (char *)((uint32)config + sizeof(struct mp_config_table));
	for (int32 i = 0; i < config->num_base_entries; i++) {
		switch (*pointer) {
			case MP_BASE_PROCESSOR:
			{
				struct mp_base_processor *processor
					= (struct mp_base_processor *)pointer;
				pointer += sizeof(struct mp_base_processor);

				if (gKernelArgs.num_cpus == MAX_BOOT_CPUS) {
					TRACE(("smp: already reached maximum boot CPUs (%d)\n",
						MAX_BOOT_CPUS));
					continue;
				}

				// skip if the processor is not enabled.
				if (!(processor->cpu_flags & 0x1)) {
					TRACE(("smp: skip apic id %d: disabled\n",
						processor->apic_id));
					continue;
				}

				gKernelArgs.arch_args.cpu_apic_id[gKernelArgs.num_cpus]
					= processor->apic_id;
				gKernelArgs.arch_args.cpu_apic_version[gKernelArgs.num_cpus]
					= processor->apic_version;

#ifdef TRACE_SMP
				const char *cpuFamily[] = { "", "", "", "", "Intel 486",
					"Intel Pentium", "Intel Pentium Pro", "Intel Pentium II" };
#endif
				TRACE(("smp: cpu#%ld: %s, apic id %d, version %d%s\n",
					gKernelArgs.num_cpus,
					cpuFamily[(processor->signature & 0xf00) >> 8],
					processor->apic_id, processor->apic_version,
					(processor->cpu_flags & 0x2) ? ", BSP" : ""));

				gKernelArgs.num_cpus++;
				break;
			}
			case MP_BASE_BUS:
			{
				struct mp_base_bus *bus = (struct mp_base_bus *)pointer;
				pointer += sizeof(struct mp_base_bus);

				TRACE(("smp: bus %d: %c%c%c%c%c%c\n", bus->bus_id,
					bus->name[0], bus->name[1], bus->name[2], bus->name[3],
					bus->name[4], bus->name[5]));

				break;
			}
			case MP_BASE_IO_APIC:
			{
				struct mp_base_ioapic *io = (struct mp_base_ioapic *)pointer;
				pointer += sizeof(struct mp_base_ioapic);

				if (gKernelArgs.arch_args.ioapic_phys == 0) {
					gKernelArgs.arch_args.ioapic_phys = (uint32)io->addr;
					if (gKernelArgs.arch_args.ioapic_phys % 1024) {
						// MP specs mandate a 1K alignment for the IO-APICs
						TRACE(("smp: io apic %p has bad alignment, aborting\n",
							(void *)gKernelArgs.arch_args.ioapic_phys));
						return B_ERROR;
					}
				}

				TRACE(("smp: found io apic with apic id %d, version %d\n",
					io->ioapic_id, io->ioapic_version));

				break;
			}
			case MP_BASE_IO_INTR:
			case MP_BASE_LOCAL_INTR:
			{
				struct mp_base_interrupt *interrupt
					= (struct mp_base_interrupt *)pointer;
				pointer += sizeof(struct mp_base_interrupt);

				dprintf("smp: %s int: type %d, source bus %d, irq %3d, dest "
					"apic %d, int %3d, polarity %d, trigger mode %d\n",
					interrupt->type == MP_BASE_IO_INTR ? "I/O" : "local",
					interrupt->interrupt_type, interrupt->source_bus_id,
					interrupt->source_bus_irq, interrupt->dest_apic_id,
					interrupt->dest_apic_int, interrupt->polarity,
					interrupt->trigger_mode);
				break;
			}
		}
	}

	if (gKernelArgs.num_cpus == 0) {
		TRACE(("smp: didn't find any processors, aborting\n"));
		return B_ERROR;
	}

	dprintf("smp: apic @ %p, i/o apic @ %p, total %ld processors detected\n",
		(void *)gKernelArgs.arch_args.apic_phys,
		(void *)gKernelArgs.arch_args.ioapic_phys,
		gKernelArgs.num_cpus);

	return B_OK;
}


static status_t
smp_do_acpi_config(void)
{
	TRACE(("smp: using ACPI to detect MP configuration\n"));

	// reset CPU count
	gKernelArgs.num_cpus = 0;

	acpi_madt *madt = (acpi_madt *)acpi_find_table(ACPI_MADT_SIGNATURE);

	if (madt == NULL) {
		TRACE(("smp: Failed to find MADT!\n"));
		return B_ERROR;
	}

	gKernelArgs.arch_args.apic_phys = madt->local_apic_address;
	TRACE(("smp: local apic address is 0x%lx\n", madt->local_apic_address));

	acpi_apic *apic = (acpi_apic *)((uint8 *)madt + sizeof(acpi_madt));
	acpi_apic *end = (acpi_apic *)((uint8 *)madt + madt->header.length);
	while (apic < end) {
		switch (apic->type) {
			case ACPI_MADT_LOCAL_APIC:
			{
				if (gKernelArgs.num_cpus == MAX_BOOT_CPUS) {
					TRACE(("smp: already reached maximum boot CPUs (%d)\n",
						MAX_BOOT_CPUS));
					break;
				}

				acpi_local_apic *localApic = (acpi_local_apic *)apic;
				TRACE(("smp: found local APIC with id %u\n",
					localApic->apic_id));
				if ((localApic->flags & ACPI_LOCAL_APIC_ENABLED) == 0) {
					TRACE(("smp: APIC is disabled and will not be used\n"));
					break;
				}

				gKernelArgs.arch_args.cpu_apic_id[gKernelArgs.num_cpus]
					= localApic->apic_id;
				// TODO: how to find out? putting 0x10 in to indicate a local apic
				gKernelArgs.arch_args.cpu_apic_version[gKernelArgs.num_cpus]
					= 0x10;
				gKernelArgs.num_cpus++;
				break;
			}

			case ACPI_MADT_IO_APIC: {
				acpi_io_apic *ioApic = (acpi_io_apic *)apic;
				TRACE(("smp: found io APIC with id %u and address 0x%lx\n",
					ioApic->io_apic_id, ioApic->io_apic_address));
				if (gKernelArgs.arch_args.ioapic_phys == 0)
					gKernelArgs.arch_args.ioapic_phys = ioApic->io_apic_address;
				break;
			}
			default:
				break;
		}

		apic = (acpi_apic *)((uint8 *)apic + apic->length);
	}

	return gKernelArgs.num_cpus > 0 ? B_OK : B_ERROR;
}


static void
calculate_apic_timer_conversion_factor(void)
{
	int64 t1, t2;
	uint32 config;
	uint32 count;

	// setup the timer
	config = apic_read(APIC_LVT_TIMER);
	config = (config & APIC_LVT_TIMER_MASK) + APIC_LVT_MASKED;
		// timer masked, vector 0
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

	gKernelArgs.arch_args.apic_time_cv_factor
		= (uint32)((1000000.0/(t2 - t1)) * count);

	TRACE(("APIC ticks/sec = %ld\n",
		gKernelArgs.arch_args.apic_time_cv_factor));
}


//	#pragma mark -


int
smp_get_current_cpu(void)
{
	if (gKernelArgs.arch_args.apic == NULL)
		return 0;

	uint8 apicID = apic_read(APIC_ID) >> 24;
	for (uint32 i = 0; i < gKernelArgs.num_cpus; i++) {
		if (gKernelArgs.arch_args.cpu_apic_id[i] == apicID)
			return i;
	}

	return 0;
}


void
smp_init_other_cpus(void)
{
	if (get_safemode_boolean(B_SAFEMODE_DISABLE_SMP, false)) {
		// SMP has been disabled!
		TRACE(("smp disabled per safemode setting\n"));
		gKernelArgs.num_cpus = 1;
	}

	if (get_safemode_boolean(B_SAFEMODE_DISABLE_APIC, false)) {
		TRACE(("local apic disabled per safemode setting, disabling smp\n"));
		gKernelArgs.arch_args.apic_phys = 0;
		gKernelArgs.num_cpus = 1;
	}

	if (gKernelArgs.arch_args.apic_phys == 0)
		return;

	TRACE(("smp: found %ld cpu%s\n", gKernelArgs.num_cpus,
		gKernelArgs.num_cpus != 1 ? "s" : ""));
	TRACE(("smp: apic_phys = %p\n", (void *)gKernelArgs.arch_args.apic_phys));
	TRACE(("smp: ioapic_phys = %p\n",
		(void *)gKernelArgs.arch_args.ioapic_phys));

	// map in the apic
	gKernelArgs.arch_args.apic = (void *)mmu_map_physical_memory(
		gKernelArgs.arch_args.apic_phys, B_PAGE_SIZE, kDefaultPageFlags);

	TRACE(("smp: apic (mapped) = %p\n", (void *)gKernelArgs.arch_args.apic));

	// calculate how fast the apic timer is
	calculate_apic_timer_conversion_factor();

	if (gKernelArgs.num_cpus < 2)
		return;

	for (uint32 i = 1; i < gKernelArgs.num_cpus; i++) {
		// create a final stack the trampoline code will put the ap processor on
		gKernelArgs.cpu_kstack[i].start = (addr_t)mmu_allocate(NULL,
			KERNEL_STACK_SIZE + KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE);
		gKernelArgs.cpu_kstack[i].size = KERNEL_STACK_SIZE
			+ KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;
	}
}


void
smp_boot_other_cpus(void (*entryFunc)(void))
{
	if (gKernelArgs.num_cpus < 2)
		return;

	TRACE(("trampolining other cpus\n"));

	// The first 8 MB are identity mapped, either 0x9e000-0x9ffff is reserved
	// for this, or when PXE services are used 0x8b000-0x8cfff.

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
		memset((uint8*)finalStack + KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE, 0,
			KERNEL_STACK_SIZE);
		tempStack = (finalStack
			+ (KERNEL_STACK_SIZE + KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE)
				/ sizeof(uint32)) - 1;
		*tempStack = (uint32)entryFunc;

		// set the trampoline stack up
		tempStack = (uint32 *)(trampolineStack + B_PAGE_SIZE - 4);
		// final location of the stack
		*tempStack = ((uint32)finalStack) + KERNEL_STACK_SIZE
			+ KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE - sizeof(uint32);
		tempStack--;
		// page dir
		*tempStack = x86_read_cr3() & 0xfffff000;

		// put a gdt descriptor at the bottom of the stack
		*((uint16 *)trampolineStack) = 0x18 - 1; // LIMIT
		*((uint32 *)(trampolineStack + 2)) = trampolineStack + 8;

		// construct a temporary gdt at the bottom
		segment_descriptor* tempGDT
			= (segment_descriptor*)&((uint32 *)trampolineStack)[2];
		clear_segment_descriptor(&tempGDT[0]);
		set_segment_descriptor(&tempGDT[1], 0, 0xffffffff, DT_CODE_READABLE,
			DPL_KERNEL);
		set_segment_descriptor(&tempGDT[2], 0, 0xffffffff, DT_DATA_WRITEABLE,
			DPL_KERNEL);

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
			| APIC_TRIGGER_MODE_LEVEL | APIC_INTR_COMMAND_1_ASSERT
			| APIC_DELIVERY_MODE_INIT;
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
		numStartups = (gKernelArgs.arch_args.cpu_apic_version[i] & 0xf0)
			? 2 : 0;
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

		// Wait for the trampoline code to clear the final stack location.
		// This serves as a notification for us that it has loaded the address
		// and it is safe for us to overwrite it to trampoline the next CPU.
		tempStack++;
		while (*tempStack != 0)
			spin(1000);
	}

	TRACE(("done trampolining\n"));
}


void
smp_add_safemode_menus(Menu *menu)
{
	MenuItem *item;

	if (gKernelArgs.arch_args.ioapic_phys != 0) {
		menu->AddItem(item = new(nothrow) MenuItem("Disable IO-APIC"));
		item->SetType(MENU_ITEM_MARKABLE);
		item->SetData(B_SAFEMODE_DISABLE_IOAPIC);
		item->SetHelpText("Disables using the IO APIC for interrupt routing, "
			"forcing the use of the legacy PIC instead.");
	}

	if (gKernelArgs.arch_args.apic_phys != 0) {
		menu->AddItem(item = new(nothrow) MenuItem("Disable local APIC"));
		item->SetType(MENU_ITEM_MARKABLE);
		item->SetData(B_SAFEMODE_DISABLE_APIC);
		item->SetHelpText("Disables using the local APIC, also disables SMP.");
	}

	if (gKernelArgs.num_cpus < 2)
		return;

	item = new(nothrow) MenuItem("Disable SMP");
	menu->AddItem(item);
	item->SetData(B_SAFEMODE_DISABLE_SMP);
	item->SetType(MENU_ITEM_MARKABLE);
	item->SetHelpText("Disables all but one CPU core.");
}


void
smp_init(void)
{
#if NO_SMP
	gKernelArgs.num_cpus = 1;
	return;
#endif

	cpuid_info info;
	if (get_current_cpuid(&info, 1) != B_OK)
		return;

	if ((info.eax_1.features & IA32_FEATURE_APIC) == 0) {
		// Local APICs aren't present; As they form the basis for all inter CPU
		// communication and therefore SMP, we don't need to go any further.
		dprintf("no local APIC present, not attempting SMP init\n");
		return;
	}

	// first try to find ACPI tables to get MP configuration as it handles
	// physical as well as logical MP configurations as in multiple cpus,
	// multiple cores or hyper threading.
	if (smp_do_acpi_config() == B_OK)
		return;

	// then try to find MPS tables and do configuration based on them
	for (int32 i = 0; smp_scan_spots[i].length > 0; i++) {
		mp_floating_struct *floatingStruct = smp_mp_probe(
			smp_scan_spots[i].start, smp_scan_spots[i].stop);
		if (floatingStruct != NULL && smp_do_mp_config(floatingStruct) == B_OK)
			return;
	}

	// Everything failed or we are not running an SMP system, reset anything
	// that might have been set through an incomplete configuration attempt.
	gKernelArgs.arch_args.apic_phys = 0;
	gKernelArgs.arch_args.ioapic_phys = 0;
	gKernelArgs.num_cpus = 1;
}
