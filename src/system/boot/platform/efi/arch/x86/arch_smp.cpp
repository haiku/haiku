/*
 * Copyright 2021-2022 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 *
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Copyright 2004-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "arch_smp.h"

#include <kernel.h>
#include <safemode.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/menu.h>
#include <arch/x86/apic.h>
#include <arch/x86/arch_cpu.h>
#include <arch/x86/arch_system_info.h>

#include "mmu.h"
#include "acpi.h"


#define NO_SMP 0

//#define TRACE_SMP
#ifdef TRACE_SMP
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif

void copy_trampoline_code(uint64 trampolineCode, uint64 trampolineStack);
void prepare_trampoline_args(uint64 trampolineCode, uint64 trampolineStack,
	uint32 pagedir, uint64 kernelEntry, addr_t virtKernelArgs,
	uint32 currentCpu);
uint32 get_sentinel(uint64 trampolineStack);

static bool sX2APIC = false;


static uint32
apic_read(uint32 offset)
{
	return *(volatile uint32 *)((addr_t)gKernelArgs.arch_args.apic_phys + offset);
}


static void
apic_write(uint32 offset, uint32 data)
{
	*(volatile uint32 *)((addr_t)gKernelArgs.arch_args.apic_phys + offset) = data;
}

static uint32
apic_error_status()
{
	if (sX2APIC)
		return x86_read_msr(IA32_MSR_APIC_ERROR_STATUS);
	else
		return apic_read(APIC_ERROR_STATUS);
}


static void
apic_set_error_status(uint32 config)
{
	if (sX2APIC)
		x86_write_msr(IA32_MSR_APIC_ERROR_STATUS, config);
	else
		apic_write(APIC_ERROR_STATUS, config);
}



static bool
apic_interrupt_delivered(void)
{
	if (sX2APIC)
		return true;
	else
		return (apic_read(APIC_INTR_COMMAND_1) & APIC_DELIVERY_STATUS) == 0;
}


static void
apic_set_interrupt_command(uint32 destination, uint32 mode)
{
	if (sX2APIC) {
		uint64 command = 0;
		command |= (uint64)destination << 32;
		command |= mode;
		x86_write_msr(IA32_MSR_APIC_INTR_COMMAND, command);
	} else {
		uint32 command2 = 0;
		command2 |= destination << 24;
		apic_write(APIC_INTR_COMMAND_2, command2);

		uint32 command1 = mode;
		apic_write(APIC_INTR_COMMAND_1, command1);
	}
}


static uint32
apic_local_id()
{
	if (sX2APIC)
		return x86_read_msr(IA32_MSR_APIC_ID);
	else
		return (apic_read(APIC_ID) & 0xffffffff) >> 24;
}


static status_t
acpi_do_smp_config(void)
{
	TRACE("smp: using ACPI to detect MP configuration\n");

	sX2APIC = ((x86_read_msr(IA32_MSR_APIC_BASE) & IA32_MSR_APIC_BASE_X2APIC) != 0);
	TRACE("smp: X2APIC is %s\n", sX2APIC ? "enabled" : "disabled");

	// reset CPU count
	gKernelArgs.num_cpus = 0;

	acpi_madt *madt = (acpi_madt *)acpi_find_table(ACPI_MADT_SIGNATURE);

	if (madt == NULL) {
		TRACE("smp: Failed to find MADT!\n");
		return B_ERROR;
	}

	gKernelArgs.arch_args.apic_phys = madt->local_apic_address;
	TRACE("smp: local apic address is 0x%" B_PRIx32 "\n", madt->local_apic_address);

	acpi_apic *apic = (acpi_apic *)((uint8 *)madt + sizeof(acpi_madt));
	acpi_apic *end = (acpi_apic *)((uint8 *)madt + madt->header.length);
	while (apic < end) {
		switch (apic->type) {
			case ACPI_MADT_LOCAL_APIC:
			{
				if (gKernelArgs.num_cpus == SMP_MAX_CPUS) {
					TRACE("smp: already reached maximum CPUs (%d)\n",
						SMP_MAX_CPUS);
					break;
				}

				acpi_local_apic *localApic = (acpi_local_apic *)apic;
				TRACE("smp: found local APIC with id %u\n",
					localApic->apic_id);
				if ((localApic->flags & ACPI_LOCAL_APIC_ENABLED) == 0) {
					TRACE("smp: APIC is disabled and will not be used\n");
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
				TRACE("smp: found io APIC with id %" B_PRIu32 " and address 0x%" B_PRIx32 "\n",
					ioApic->io_apic_id, ioApic->io_apic_address);
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


//	#pragma mark -


int
arch_smp_get_current_cpu(void)
{
	if (gKernelArgs.arch_args.apic == NULL)
		return 0;

	uint8 apicID = apic_local_id();
	for (uint32 i = 0; i < gKernelArgs.num_cpus; i++) {
		if (gKernelArgs.arch_args.cpu_apic_id[i] == apicID)
			return i;
	}

	return 0;
}


void
arch_smp_init_other_cpus(void)
{
	if (get_safemode_boolean(B_SAFEMODE_DISABLE_SMP, false)) {
		// SMP has been disabled!
		TRACE("smp disabled per safemode setting\n");
		gKernelArgs.num_cpus = 1;
	}

	if (get_safemode_boolean(B_SAFEMODE_DISABLE_APIC, false)) {
		TRACE("local apic disabled per safemode setting, disabling smp\n");
		gKernelArgs.arch_args.apic_phys = 0;
		gKernelArgs.num_cpus = 1;
	}

	if (gKernelArgs.arch_args.apic_phys == 0)
		return;

	TRACE("smp: found %" B_PRId32 " cpu%s\n", gKernelArgs.num_cpus,
		gKernelArgs.num_cpus != 1 ? "s" : "");
	TRACE("smp: apic_phys = %lx\n", (addr_t)gKernelArgs.arch_args.apic_phys);
	TRACE("smp: ioapic_phys = %lx\n",
		(addr_t)gKernelArgs.arch_args.ioapic_phys);

	// map in the apic
	gKernelArgs.arch_args.apic = (void *)mmu_map_physical_memory(
		gKernelArgs.arch_args.apic_phys, B_PAGE_SIZE, kDefaultPageFlags);

	TRACE("smp: apic (mapped) = %lx\n", (addr_t)gKernelArgs.arch_args.apic.Pointer());

	if (gKernelArgs.num_cpus < 2)
		return;

	for (uint32 i = 1; i < gKernelArgs.num_cpus; i++) {
		// create a final stack the trampoline code will put the ap processor on
		void * stack = NULL;
		const size_t size = KERNEL_STACK_SIZE + KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;
		if (platform_allocate_region(&stack, size, 0, false) != B_OK) {
			panic("Unable to allocate AP stack");
		}
		memset(stack, 0, size);
		gKernelArgs.cpu_kstack[i].start = fix_address((uint64_t)stack);
		gKernelArgs.cpu_kstack[i].size = size;
	}
}


void
arch_smp_boot_other_cpus(uint32 pagedir, uint64 kernelEntry, addr_t virtKernelArgs)
{
	TRACE("trampolining other cpus\n");

	// allocate a stack and a code area for the smp trampoline
	// (these have to be < 1M physical, 0xa0000-0xfffff is reserved by the BIOS)
	uint64 trampolineCode = 0x9000;
	uint64 trampolineStack = 0x8000;

	// copy the trampoline code over
	copy_trampoline_code(trampolineCode, trampolineStack);

	// boot the cpus
	TRACE("we have %" B_PRId32 " CPUs to boot...\n", gKernelArgs.num_cpus - 1);
	for (uint32 i = 1; i < gKernelArgs.num_cpus; i++) {
		TRACE("trampolining CPU %" B_PRId32 " (apicid=0x%x)\n", i,
		      gKernelArgs.arch_args.cpu_apic_id[i]);
		uint64 numStartups;
		uint32 j;

		prepare_trampoline_args(trampolineCode, trampolineStack,
			pagedir, kernelEntry, virtKernelArgs, i);

		/* clear apic errors */
		if (gKernelArgs.arch_args.cpu_apic_version[i] & 0xf0) {
			apic_set_error_status(0);
			apic_error_status();
		}

		/* send (aka assert) INIT IPI */
		apic_set_interrupt_command(gKernelArgs.arch_args.cpu_apic_id[i],
			APIC_TRIGGER_MODE_LEVEL | APIC_INTR_COMMAND_1_ASSERT
				| APIC_DELIVERY_MODE_INIT);

		// wait for pending to end
		while (!apic_interrupt_delivered())
			asm volatile ("pause;");

		spin(200);

		/* deassert INIT */
		apic_set_interrupt_command(gKernelArgs.arch_args.cpu_apic_id[i],
			APIC_TRIGGER_MODE_LEVEL | APIC_DELIVERY_MODE_INIT);

		// wait for pending to end
		while (!apic_interrupt_delivered())
			asm volatile ("pause;");

		/* wait 10ms */
		spin(10000);
		/* is this a local apic or an 82489dx ? */
		numStartups = (gKernelArgs.arch_args.cpu_apic_version[i] & 0xf0)
			? 2 : 0;
		for (j = 0; j < numStartups; j++) {
			/* it's a local apic, so send STARTUP IPIs */
			apic_set_error_status(0);

			/* send the IPI */
			apic_set_interrupt_command(gKernelArgs.arch_args.cpu_apic_id[i],
				APIC_DELIVERY_MODE_STARTUP | (trampolineCode >> 12));

			/* wait */
			spin(200);

			while (!apic_interrupt_delivered())
				asm volatile ("pause;");
		}

		// Wait for the trampoline code to clear the final stack location.
		// This serves as a notification for us that it has loaded the address
		// and it is safe for us to overwrite it to trampoline the next CPU.
		while (get_sentinel(trampolineStack) != 0)
			spin(1000);
	}

	TRACE("done trampolining\n");
}


void
arch_smp_add_safemode_menus(Menu *menu)
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

		cpuid_info info;
		if (get_current_cpuid(&info, 1, 0) == B_OK
				&& (info.regs.ecx & IA32_FEATURE_EXT_X2APIC) != 0) {
			menu->AddItem(item = new(nothrow) MenuItem("Disable X2APIC"));
			item->SetType(MENU_ITEM_MARKABLE);
			item->SetData(B_SAFEMODE_DISABLE_X2APIC);
			item->SetHelpText("Disables using X2APIC.");
		}

		if (get_current_cpuid(&info, 7, 0) == B_OK
				&& ((info.regs.ebx & (IA32_FEATURE_SMEP
					| IA32_FEATURE_SMAP)) != 0)) {
			menu->AddItem(item = new(nothrow) MenuItem(
				"Disable SMEP and SMAP"));
			item->SetType(MENU_ITEM_MARKABLE);
			item->SetData(B_SAFEMODE_DISABLE_SMEP_SMAP);
			item->SetHelpText("Disables using SMEP and SMAP.");
		}
	}

	cpuid_info info;
	if (get_current_cpuid(&info, 1, 0) == B_OK
		&& (info.regs.edx & IA32_FEATURE_PAT) != 0) {
		menu->AddItem(item = new(nothrow) MenuItem("Disable PAT"));
		item->SetType(MENU_ITEM_MARKABLE);
		item->SetData(B_SAFEMODE_DISABLE_PAT);
		item->SetHelpText("Disables using page attribute tables for memory "
			"type setting, falling back to MTRRs.");
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
arch_smp_init(void)
{
	cpuid_info info;
	if (get_current_cpuid(&info, 1, 0) != B_OK)
		return;

	if ((info.eax_1.features & IA32_FEATURE_APIC) == 0) {
		// Local APICs aren't present; As they form the basis for all inter CPU
		// communication and therefore SMP, we don't need to go any further.
		TRACE("no local APIC present, not attempting SMP init\n");
		return;
	}

	// first try to find ACPI tables to get MP configuration as it handles
	// physical as well as logical MP configurations as in multiple cpus,
	// multiple cores or hyper threading.
	if (acpi_do_smp_config() == B_OK) {
		TRACE("smp init success\n");
		return;
	}

	// Everything failed or we are not running an SMP system, reset anything
	// that might have been set through an incomplete configuration attempt.
	gKernelArgs.arch_args.apic_phys = 0;
	gKernelArgs.arch_args.ioapic_phys = 0;
	gKernelArgs.num_cpus = 1;
}
