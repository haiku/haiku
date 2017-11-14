/*
 * Copyright 2009 Colin Günther, coling@gmx.de
 * All Rights Reserved. Distributed under the terms of the MIT License.
 *
 */


/*! Wrapper functions for accessing the number buffer.*/


#include "unit.h"

#include <util/RadixBitmap.h>


#define ID_STORE_FULL -1


status_t
_new_unrhdr_buffer(struct unrhdr* idStore, uint32 maxIdCount)
{
	status_t status = B_OK;

	idStore->idBuffer = radix_bitmap_create(maxIdCount);
	if (idStore->idBuffer == NULL)
		status = B_NO_MEMORY;

	return status;
}


void
_delete_unrhdr_buffer_locked(struct unrhdr* idStore)
{
	radix_bitmap_destroy(idStore->idBuffer);
}


int
_alloc_unr_locked(struct unrhdr* idStore)
{
	radix_slot_t slotIndex;
	int id = ID_STORE_FULL;

	slotIndex = radix_bitmap_alloc(idStore->idBuffer, 1);
	if (slotIndex != RADIX_SLOT_NONE)
		id = slotIndex + idStore->idBias;

	return id;
}


void
_free_unr_locked(struct unrhdr* idStore, u_int identity)
{
	uint32 slotIndex = (int32)identity - idStore->idBias;

	radix_bitmap_dealloc(idStore->idBuffer, slotIndex, 1);
}
