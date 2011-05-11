/*
 * Copyright 20010, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */

#include <arch/x86/apic.h>
#include <arch/x86/arch_int.h>
#include <arch/x86/ioapic.h>
#include <arch/x86/msi.h>

#include <debug.h>
#include <lock.h>


static const uint32 kVectorCount = 256 - ARCH_INTERRUPT_BASE;
static const uint8 kNumISAVectors = 16;

static bool sMSISupported = false;
static bool sAllocatedVectors[kVectorCount];
static mutex sMSIAllocationLock = MUTEX_INITIALIZER("msi_allocation");


void
msi_init()
{
	if (!apic_available()) {
		dprintf("disabling msi due to missing apic\n");
		return;
	}

	// TODO: less hardcoding!

	// the first 16 vectors are legacy ISA in all cases
	for (uint16 i = 0; i < kNumISAVectors; i++)
		sAllocatedVectors[i] = true;

	for (uint16 i = kNumISAVectors; i < kVectorCount; i++) {
		// if ioapics aren't in use this will always return false, leaving
		// the vectors free for us; otherwise we'll avoid any vector that
		// can be addressed by an IO-APIC
		sAllocatedVectors[i] = ioapic_is_interrupt_available(i);
	}

	// performance testing and syscall interrupts
	sAllocatedVectors[98 - ARCH_INTERRUPT_BASE] = true;
	sAllocatedVectors[99 - ARCH_INTERRUPT_BASE] = true;

	// the upper range is used by apic local (timer) and smp interrupts (ipi)
	for (uint16 i = 251; i < 256; i++)
		sAllocatedVectors[i - ARCH_INTERRUPT_BASE] = true;

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

	mutex_lock(&sMSIAllocationLock);

	uint8 vector = 0;
	bool runFound = true;
	for (uint16 i = 0; i < kVectorCount - (count - 1); i++) {
		if (!sAllocatedVectors[i]) {
			vector = i;
			runFound = true;
			for (uint16 j = 1; j < count; j++) {
				if (sAllocatedVectors[i + j]) {
					runFound = false;
					i += j;
					break;
				}
			}

			if (runFound)
				break;
		}
	}

	if (!runFound) {
		mutex_unlock(&sMSIAllocationLock);
		dprintf("found no free vectors to allocate %u msi messages\n", count);
		return B_NO_MEMORY;
	}

	for (uint16 i = 0; i < count; i++)
		sAllocatedVectors[i + vector] = true;

	mutex_unlock(&sMSIAllocationLock);

	*startVector = vector;
	*address = MSI_ADDRESS_BASE | (0 << MSI_DESTINATION_ID_SHIFT)
		| MSI_NO_REDIRECTION | MSI_DESTINATION_MODE_PHYSICAL;
	*data = MSI_TRIGGER_MODE_EDGE | MSI_DELIVERY_MODE_FIXED
		| (vector + ARCH_INTERRUPT_BASE);

	dprintf("msi_allocate_vectors: allocated %u vectors starting from %u\n",
		count, vector);
	return B_OK;
}


void
msi_free_vectors(uint8 count, uint8 startVector)
{
	if (!sMSISupported) {
		panic("trying to free msi vectors but msi not supported\n");
		return;
	}

	if ((uint32)startVector + count > kVectorCount) {
		panic("invalid start vector %u or count %u supplied to "
			"msi_free_vectors\n", startVector, count);
	}

	dprintf("msi_free_vectors: freeing %u vectors starting from %u\n", count,
		startVector);

	mutex_lock(&sMSIAllocationLock);
	for (uint16 i = 0; i < count; i++) {
		if (!sAllocatedVectors[i + startVector])
			panic("msi vector %u was not allocated\n", i + startVector);

		sAllocatedVectors[i + startVector] = false;
	}

	mutex_unlock(&sMSIAllocationLock);
}
