/*
 * Copyright 2022, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_GENERIC_MSI_H
#define _KERNEL_ARCH_GENERIC_MSI_H

#include <SupportDefs.h>


#ifdef __cplusplus

class MSIInterface {
public:
	virtual status_t AllocateVectors(
		uint32 count, uint32& startVector, uint64& address, uint32& data) = 0;
	virtual void FreeVectors(uint32 count, uint32 startVector) = 0;
};


extern "C" {
void msi_set_interface(MSIInterface* interface);
#endif

bool		msi_supported();
status_t	msi_allocate_vectors(uint32 count, uint32 *startVector,
				uint64 *address, uint32 *data);
void		msi_free_vectors(uint32 count, uint32 startVector);

#ifdef __cplusplus
}
#endif


#endif	// _KERNEL_ARCH_GENERIC_MSI_H
