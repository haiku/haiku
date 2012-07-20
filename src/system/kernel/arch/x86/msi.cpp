/*
 * Copyright 2010-2011, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */

#include <arch/x86/apic.h>
#include <arch/x86/msi.h>

#include <debug.h>
#include <int.h>
#include <lock.h>


static bool sMSISupported = false;


void
msi_init()
{
#ifdef __x86_64__
	// TODO x86_64.
	return;
#endif
	if (!apic_available()) {
		dprintf("disabling msi due to missing apic\n");
		return;
	}

	dprintf("msi support enabled\n");
	sMSISupported = true;
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
	status_t result = allocate_io_interrupt_vectors(count, &vector);
	if (result != B_OK)
		return result;

	if (vector >= 256) {
		free_io_interrupt_vectors(count, vector);
		return B_NO_MEMORY;
	}

	*startVector = (uint8)vector;
	*address = MSI_ADDRESS_BASE | (0 << MSI_DESTINATION_ID_SHIFT)
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
