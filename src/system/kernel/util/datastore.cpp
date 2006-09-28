/* General data region of fixed size data blocks
 *
 * Copyright 2006, Haiku, Inc.  All Rights Reserved
 * Distributed under the terms of the MIT liscence.
 *
 * Authors:
 *     Andrew Galante : haiku.galante@gmail.com
 */

#include <KernelExport.h>
#include <malloc.h>
#include <debug.h>
#include <Errors.h>
#include <string.h>
#include <util/datastore.h>

/*!
 * Creates a new datastore with \a blockcount datablocks of \a blocksize size
 * \return B_NO_MEMORY if there is not enough memory to allocate to the datastore
 * \return B_BAD_VALUE if blockcount is negative
 */
status_t
init_datastore(struct datastore *store, size_t blocksize, int blockcount)
{
	if (blockcount < 1)
		return B_BAD_VALUE;

	store->blocksize = blocksize;
	store->blockcount = blockcount;

	store->store = malloc(blocksize * blockcount);
	if (store->store == NULL)
		return B_NO_MEMORY;

	store->refcounts = (int32 *)calloc(blockcount, sizeof(uint32));
	if (store->refcounts == NULL)
		return B_NO_MEMORY;

	store->lastfreed = -1;
	store->nextfree = 0;

	return B_OK;
}


/*!
 * Frees all memory occupied by the datastore, except for the datastore structure itself
 */
status_t
uninit_datastore(struct datastore *store)
{
	store->blocksize = 0;
	store->blockcount = 0;

	free(store->store);
	store->store = NULL;
	free(store->refcounts);
	store->refcounts = NULL;

	store->lastfreed = -1;
	store->nextfree = 0;

	return B_OK;
}


/*!
 * Searches the datastore for a free datablock.  If none are free,
 * continues search until one is freed.
 * \return a pointer to the datablock
 * \return NULL if the datastore has not been properly initialized
 */
void *
get_datablock(struct datastore *store)
{
	void *block = NULL;
	if (store->store == NULL)
		return NULL;

	int index = store->lastfreed;
	// first check lastfreed index, as that's most likely to be free
	if (index >= 0) {
		if (atomic_add(&store->refcounts[index], 1) == 0) {
			block = (void *)(index * store->blocksize + (uint8 *)store->store);
			store->lastfreed = -1;
			return block;
		} else
			atomic_add(&store->refcounts[index], -1);
	}
	// otherwise start at the "next free" block and search
	index = store->nextfree;
	while (1) {
		if(atomic_add(&store->refcounts[index], 1) == 0) {
			// found a free block
			block = (void *)(index * store->blocksize + (uint8 *)store->store);
			store->nextfree = (index + 1) % store->blockcount;
			break;
		} else
			atomic_add(&store->refcounts[index], -1);
		index = (index + 1) % store->blockcount;
	}

	return block;
}


/*!
 * Increments the refcount of the specified block
 * \return a reference to the block on success
 * \return NULL if there was an error
 */
void *
get_datablock(struct datastore *store, void *block)
{
	if (block == NULL
		|| block < store->store
		|| block > (uint8 *)store->store + (store->blocksize * store->blockcount))
		return NULL;
	size_t i = ((uint8 *)block - (uint8 *)store->store) / store->blocksize;

	if (atomic_add(&store->refcounts[i], 1) < 0) {
		atomic_add(&store->refcounts[i], -1);
		return NULL;
	}
	return block;
}


/*!
 * Decrements the refcount of the specified block
 * \return B_OK on success
 * \return B_BAD_VALUE if \a block is not in the datastore \a store
 */
status_t
put_datablock(struct datastore *store, void *block)
{
	if (block == NULL
		|| block < store->store
		|| block > (uint8 *)store->store + (store->blocksize * store->blockcount))
		return B_BAD_VALUE;
	size_t i = ((uint8 *)block - (uint8 *)store->store) / store->blocksize;

	if (store->refcounts[i] == 0)
		return B_OK;
	if (atomic_add(&store->refcounts[i], -1) == 1)
		if ((store->nextfree - 1) % store->blockcount == i)
			store->nextfree = i;
		else
			store->lastfreed = i;
	return B_OK;
}


/*!
 * Checks if the datastore is empty.
 * Not guaranteed to be thread safe!
 * \return nonzero if all blocks are free
 * \return zero otherwise
 */
int
is_empty_datastore(struct datastore *store)
{
	size_t i;
	for (i = 0; i < store->blockcount; i++)
		if (store->refcounts[i] != 0)
			return 0;
	return 1;
}


/*!
 * Checks if the datastore is occupied
 * Not guaranteed to be thread safe!
 * \return nonzero if no blocks are free
 * \return zero otherwise
 */
int is_full_datastore(struct datastore *store)
{
	size_t i;
	for (i = 0; i < store->blockcount; i++)
		if (store->refcounts[i] == 0)
			return 0;
	return 1;
}
