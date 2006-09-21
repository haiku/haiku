/* General data region of fixed size data blocks
 *
 * Copyright 2006, Haiku, Inc.  All Rights Reserved
 * Distributed under the terms of the MIT liscence.
 *
 * Authors:
 *     Andrew Galante, haiku.galante@gmail.com
 */

struct datastore {
	void *store;		// ptr to data region
	size_t blocksize;	// size of the blocks in the datastore
	size_t blockcount;	// total number of blocks in the datastore
	int32 *refcounts;		// array containing refcounts for each block.  a refcount of 0 is a free block
	int lastfreed;		// index of last freed block
	int nextfree;		// index of next free block
};

status_t init_datastore(struct datastore *store, size_t blocksize, int blockcount);	// initializes the datastore
status_t uninit_datastore(struct datastore *store); // frees the memory used by the datastore
void *get_datablock(struct datastore *store);	// returns a ptr to the first found free block, and increments its refcount
void *get_datablock(struct datastore *store, void *block);	// increments the refcount of the specified block
status_t put_datablock(struct datastore *store, void *block);	// decrements the refcount of the specified block

// not thread safe - for amusement purposes only:
int is_empty_datastore(struct datastore *store);	// returns nonzero if all refcounts are zero
int is_full_datastore(struct datastore *store);		// returns nonzero if all refcounts are greater than zero
