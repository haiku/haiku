/*
 * Copyright 2010-2011, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */

#include <arch/x86/apic.h>
#include <arch/x86/msi.h>
#include <arch/x86/arch_smp.h>

#include <debug.h>
#include <int.h>
#include <lock.h>


struct MSIConfiguration {
	uint64*	fAddress;
	uint16* fData;
};

static MSIConfiguration sMSIConfigurations[NUM_IO_VECTORS];

static bool sMSISupported = false;
static uint32 sBootCPUAPICId = 0;


void
msi_init(kernel_args* args)
{
	if (!apic_available()) {
		dprintf("disabling msi due to missing apic\n");
		return;
	}

	dprintf("msi support enabled\n");
	sMSISupported = true;
	sBootCPUAPICId = args->arch_args.cpu_apic_id[0];
}


bool
msi_supported()
{
	return sMSISupported;
}


status_t
msi_allocate_vectors(uint8 count, uint8 *startVector, uint64 *address,
	uint16 *data)
{
	if (!sMSISupported)
		return B_UNSUPPORTED;

	long vector;
	status_t result = allocate_io_interrupt_vectors(count, &vector,
		INTERRUPT_TYPE_IRQ);
	if (result != B_OK)
		return result;

	if (vector >= NUM_IO_VECTORS) {
		free_io_interrupt_vectors(count, vector);
		return B_NO_MEMORY;
	}

	sMSIConfigurations[vector].fAddress = address;
	sMSIConfigurations[vector].fData = data;
	x86_set_irq_source(vector, IRQ_SOURCE_MSI);

	*startVector = (uint8)vector;
	*address = MSI_ADDRESS_BASE | (sBootCPUAPICId << MSI_DESTINATION_ID_SHIFT)
		| MSI_NO_REDIRECTION | MSI_DESTINATION_MODE_PHYSICAL;
	*data = MSI_TRIGGER_MODE_EDGE | MSI_DELIVERY_MODE_FIXED
		| ((uint16)vector + ARCH_INTERRUPT_BASE);

	dprintf("msi_allocate_vectors: allocated %u vectors starting from %u\n",
		count, *startVector);
	return B_OK;
}


void
msi_free_vectors(uint8 count, uint8 startVector)
{
	if (!sMSISupported) {
		panic("trying to free msi vectors but msi not supported\n");
		return;
	}

	dprintf("msi_free_vectors: freeing %u vectors starting from %u\n", count,
		startVector);

	free_io_interrupt_vectors(count, startVector);
}


void
msi_assign_interrupt_to_cpu(uint8 irq, int32 cpu)
{
	uint32 apic_id = x86_get_cpu_apic_id(cpu);

	uint64* address = sMSIConfigurations[irq].fAddress;
	*address = MSI_ADDRESS_BASE | (apic_id << MSI_DESTINATION_ID_SHIFT)
		| MSI_NO_REDIRECTION | MSI_DESTINATION_MODE_PHYSICAL;
}

