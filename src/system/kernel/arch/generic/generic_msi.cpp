/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <arch/generic/msi.h>


MSIInterface* sMSIInterface;


void
msi_set_interface(MSIInterface* interface)
{
	sMSIInterface = interface;
}


bool
msi_supported()
{
	return sMSIInterface != NULL;
}


status_t
msi_allocate_vectors(uint8 count, uint8 *startVector, uint64 *address, uint16 *data)
{
	return sMSIInterface->AllocateVectors(count, *startVector, *address, *data);
}


void
msi_free_vectors(uint8 count, uint8 startVector)
{
	sMSIInterface->FreeVectors(count, startVector);
}

