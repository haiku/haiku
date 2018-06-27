/*
 * Copyright 2009 Colin Günther, coling@gmx.de
 * All Rights Reserved. Distributed under the terms of the MIT License.
 *
 */


/*! Implementation of a number allocator.*/


#include "unit.h"

#include <compat/sys/mutex.h>

#include <stdlib.h>
#include <util/RadixBitmap.h>


extern struct mtx gIdStoreLock;


struct unrhdr*
new_unrhdr(int low, int high, struct mtx* mutex)
{
	struct unrhdr* idStore;
	uint32 maxIdCount = high - low + 1;

	KASSERT(low <= high,
		("ID-Store: use error: %s(%u, %u)", __func__, low, high));

	idStore = malloc(sizeof *idStore);
	if (idStore == NULL)
		return NULL;

	if (_new_unrhdr_buffer(idStore, maxIdCount) != B_OK) {
		free(idStore);
		return NULL;
	}

	if (mutex)
		idStore->storeMutex = mutex;
	else
		idStore->storeMutex = &gIdStoreLock;

	idStore->idBias = low;

	return idStore;
}


void
delete_unrhdr(struct unrhdr* idStore)
{
	KASSERT(idStore != NULL,
		("ID-Store: %s: NULL pointer as argument.", __func__));

	mtx_lock(idStore->storeMutex);

	KASSERT(idStore->idBuffer->root_size == 0,
		("ID-Store: %s: some ids are still in use..", __func__));

	_delete_unrhdr_buffer_locked(idStore);
	mtx_unlock(idStore->storeMutex);

	free(idStore);
	idStore = NULL;
}


int
alloc_unr(struct unrhdr* idStore)
{
	int id;

	KASSERT(idStore != NULL,
		("ID-Store: %s: NULL pointer as argument.", __func__));

	mtx_lock(idStore->storeMutex);
	id = _alloc_unr_locked(idStore);
	mtx_unlock(idStore->storeMutex);

	return id;
}


void
free_unr(struct unrhdr* idStore, u_int identity)
{
	KASSERT(idStore != NULL,
		("ID-Store: %s: NULL pointer as argument.", __func__));

	mtx_lock(idStore->storeMutex);

	KASSERT((int32)identity - idStore->idBias >= 0, ("ID-Store: %s(%p, %u): second "
		"parameter is not in interval.", __func__, idStore, identity));

	_free_unr_locked(idStore, identity);

	mtx_unlock(idStore->storeMutex);
}
