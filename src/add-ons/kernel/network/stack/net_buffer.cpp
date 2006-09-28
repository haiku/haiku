/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Andrew Galante, haiku.galante@gmail.com
 */


#include "utility.h"

#include <net_buffer.h>
#include <util/list.h>

#include <ByteOrder.h>
#include <KernelExport.h>

#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>


#define TRACE_BUFFER
#ifdef TRACE_BUFFER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define BUFFER_SIZE 2048
#define MAX_BUFFERS 2048


struct datastore {
	void *store;		// ptr to data region
	size_t blocksize;	// size of the blocks in the datastore
	size_t blockcount;	// total number of blocks in the datastore
	int32 *refcounts;		// array containing refcounts for each block.  a refcount of 0 is a free block
	int lastfreed;		// index of last freed block
	int nextfree;		// index of next free block
};

struct data_node {
	struct data_node *next;
	struct data_node *previous;
	uint8		*datablock;
	uint8		*start;
	size_t		header_space;
	size_t		trailer_space;
	size_t		size;
};

typedef struct data_node data_node;


struct net_buffer_private : net_buffer {
	struct data_node *first_node;
};

typedef struct net_buffer_private net_buffer_private;


static struct datastore sDatastore;
	// The structure that manages all the storage space


//	#pragma mark -


/*!
	Creates a new datastore with \a blockcount datablocks of \a blocksize size
	\return B_NO_MEMORY if there is not enough memory to allocate to the datastore
	\return B_BAD_VALUE if blockcount is negative
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


//	#pragma mark -


static net_buffer *
net_buffer_create(size_t headerSpace)
{
	net_buffer_private *buffer = (net_buffer_private *)malloc(sizeof(struct net_buffer_private));
	if (buffer == NULL)
		return NULL;

	TRACE(("create buffer %p\n", buffer));

	data_node *node = buffer->first_node = (data_node *)malloc(sizeof(data_node));
	if (node == NULL) {
		free(buffer);
		return NULL;
	}

	node->datablock = (uint8 *)get_datablock(&sDatastore);
	if (node->datablock == NULL) {
		free(node);
		free(buffer);
		return NULL;
	}

	if (headerSpace > BUFFER_SIZE)
		headerSpace = 0;
	node->start = node->datablock + headerSpace;
	node->header_space = headerSpace;
	node->trailer_space = BUFFER_SIZE - headerSpace;

	node->next = NULL;
	node->previous = NULL;
	node->size = 0;

	buffer->source.ss_len = 0;
	buffer->destination.ss_len = 0;
	buffer->interface = NULL;
	buffer->flags = 0;
	buffer->size = node->size;

	return buffer;
}


static void
data_node_free(data_node *node)
{
	if (node != NULL) {


		put_datablock(&sDatastore, (void *)node->datablock);
		if (node->next != NULL)
			data_node_free(node->next);

		free(node);
	}
}


static void
net_buffer_free(net_buffer *_buffer)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	TRACE(("free buffer %p\n", buffer));

	if (buffer->first_node != NULL)
		data_node_free(buffer->first_node);

	free(buffer);
}


static data_node *
data_node_duplicate(data_node *node)
{
	data_node *duplicate = (data_node *)malloc(sizeof(data_node));

	// copy the data from the source buffer
	duplicate->datablock = (uint8 *)get_datablock(&sDatastore);
	if (duplicate->datablock == NULL) {
		free(duplicate);
		return NULL;
	}
	
	duplicate->header_space = node->start - node->datablock;
	duplicate->start = duplicate->datablock + duplicate->header_space;
	duplicate->size = node->size;
	duplicate->trailer_space = BUFFER_SIZE - duplicate->size - duplicate->header_space;
	memcpy(&duplicate->start, &node->start, duplicate->size);

	if (node->next != NULL) {
		duplicate->next = data_node_duplicate(node->next);
		if (duplicate->next == NULL) {
			put_datablock(&sDatastore, (void *)duplicate->datablock);
			free(duplicate);
			return NULL;
		}
		duplicate->next->previous = duplicate;
	} else
		duplicate->next = NULL;

	duplicate->previous = NULL;

	return duplicate;
}


/*!
 *	Creates a duplicate of the \a buffer. The new buffer does not share internal
 *	storage; they are completely independent from each other.
 */
static net_buffer *
net_buffer_duplicate(net_buffer *_buffer)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	net_buffer_private *duplicate =
			(net_buffer_private *)malloc(sizeof(struct net_buffer_private));
	if (duplicate == NULL)
		return NULL;

	duplicate->first_node = data_node_duplicate(buffer->first_node);
	if (duplicate->first_node == NULL) {
		free(duplicate);
		return NULL;
	}

	// copy meta data from source buffer
	memcpy(&duplicate->source, &buffer->source, buffer->source.ss_len);
	memcpy(&duplicate->destination, &buffer->destination, buffer->destination.ss_len);

	duplicate->flags = buffer->flags;
	duplicate->interface = buffer->interface;
	duplicate->protocol = buffer->protocol;
	duplicate->size = buffer->size;

	return duplicate;
}


static data_node *
data_node_clone(data_node *node, bool shareFreeSpace)
{
	data_node *clone = (data_node *)malloc(sizeof(data_node));

	clone->datablock = (uint8 *)get_datablock(&sDatastore, (void *)node->datablock);
	if (clone->datablock == NULL) {
		free(clone);
		return NULL;
	}

	// clone the data from the source buffer
	clone->start = node->start;
	if (shareFreeSpace) {
		clone->header_space = node->header_space;
		clone->trailer_space = node->trailer_space;
	} else {
		clone->header_space = 0;
		clone->trailer_space = 0;
	}

	clone->size = node->size;

	if (node->next != NULL) {
		clone->next = data_node_clone(node->next, shareFreeSpace);
		if (clone->next == NULL) {
			put_datablock(&sDatastore, (void *)clone->datablock);
			free(clone);
			return NULL;
		}
		clone->next->previous = clone;
	} else
		clone->next = NULL;

	clone->previous = NULL;

	return clone;
}


/*!	Clones the buffer by grabbing another reference to the underlying data.
	If that data changes, it will be changed in the clone as well.

	If \a shareFreeSpace is \c true, the cloned buffer may claim the free
	space in the original buffer as the original buffer can still do. If you
	are using this, it's your responsibility that only one of the buffers
	will do this.
*/
static net_buffer *
net_buffer_clone(net_buffer *_buffer, bool shareFreeSpace)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	net_buffer_private *clone =
			(net_buffer_private *)malloc(sizeof(struct net_buffer_private));
	if (clone == NULL)
		return NULL;

	if (buffer->first_node != NULL) {
		clone->first_node = data_node_clone(buffer->first_node, shareFreeSpace);
		if (clone->first_node == NULL) {
			free(clone);
			return NULL;
		}
	}

	// copy meta data from source buffer
	memcpy(&clone->source, &buffer->source, buffer->source.ss_len);
	memcpy(&clone->destination, &buffer->destination, buffer->destination.ss_len);

	clone->flags = buffer->flags;
	clone->interface = buffer->interface;
	clone->protocol = buffer->protocol;
	clone->size = buffer->size;

	return clone;
}


static data_node *
data_node_split(data_node *node, uint32 offset)
{
	if (node == NULL)
		return NULL;

	if (offset > node->size)
		return data_node_split(node->next, offset - node->size);

	data_node *new_node;
	if (offset == node->size) {
		new_node = node->next;
		node->next = NULL;
		if (new_node != NULL)
			new_node->previous = NULL;
	} else {
		// offset < node->size
		new_node = (data_node *)malloc(sizeof(data_node));
		new_node->datablock = (uint8 *)get_datablock(&sDatastore, (void *)node->datablock);
		if (new_node->datablock == NULL) {
			free(new_node);
			return NULL;
		}
		new_node->size = node->size - offset;
		node->size = offset;
		new_node->start = node->start + offset;
		new_node->header_space = 0;
		new_node->trailer_space = node->trailer_space;
		node->trailer_space = 0;
		new_node->next = node->next;
		if (new_node->next != NULL)
			new_node->next->previous = new_node;
		node->next = NULL;
	}

	return new_node;
}

/*!
 * Splits \a buffer at byte \a offset.
 * \return a buffer containing the lower half of the split
 * \return NULL if offset is larger than the buffer size, 0, or there was an error
 */
static net_buffer *
net_buffer_split(net_buffer *_buffer, uint32 offset)
{
	if (offset >= _buffer->size || offset == 0)
		return NULL;
	net_buffer_private *buffer = (net_buffer_private *)_buffer;
	net_buffer_private *new_buf =
			(net_buffer_private *)malloc(sizeof(net_buffer_private));

	new_buf->first_node = data_node_split(buffer->first_node, offset);
	if (new_buf->first_node == NULL) {
		free(new_buf);
		return NULL;
	}

	// copy meta data from source buffer
	memcpy(&new_buf->source, &buffer->source, buffer->source.ss_len);
	memcpy(&new_buf->destination, &buffer->destination, buffer->destination.ss_len);

	new_buf->flags = buffer->flags;
	new_buf->interface = buffer->interface;
	new_buf->protocol = buffer->protocol;
	new_buf->size = buffer->size;

	return new_buf;
}


static data_node *
data_node_merge(data_node *before, data_node *after)
{
	if (before == NULL)
		return NULL;

	data_node *node = before;

	while (node->next != NULL)
		node = node->next;

	node->next = after;
	if (after != NULL)
		after->previous = node;

	return before;
}


/*!
	Merges the second buffer with the first.
	if \a after is true, \a _with will be placed after \a _buffer
	else \a _with will be placed before \a _buffer
	\return a pointer to the merged buffer
*/
static status_t
net_buffer_merge(net_buffer *_buffer, net_buffer *_with, bool after)
{
	net_buffer_private *buffer = (net_buffer_private *) _buffer;
	net_buffer_private *with = (net_buffer_private *) _with;

	TRACE(("merge buffer %p %s %p\n", _with, after ? "after" : "before", _buffer));

	if (with == NULL || buffer == NULL)
		return B_ERROR;
	if (buffer->first_node == NULL || with->first_node == NULL)
		return B_ERROR;

	if (after)
		buffer->first_node = data_node_merge(buffer->first_node, with->first_node);
	else
		buffer->first_node = data_node_merge(with->first_node, buffer->first_node);

	buffer->size += with->size;

	free(with);

	return B_OK;
}


static status_t
data_node_write_data(data_node *node, size_t offset, const void *data, size_t size)
{
	if (node == NULL)
		return ENOBUFS;

	if (offset > node->size)
		return data_node_write_data(node->next, offset - node->size, data, size);
	else {
		if (node->size - offset < size) {
			memcpy(node->start + offset, data, node->size - offset);
			return data_node_write_data(node->next, 0, (uint8 *)data + (node->size - offset), size - (node->size - offset));
		} else {
			memcpy(node->start + offset, data, size);
			return B_OK;
		}
	}

	return B_OK;
}


/*!	Writes into existing allocated memory.
 *	\return B_BAD_VALUE if you write outside of the buffers current bounds.
 *	\return ENOBUFS if there was an internal buffer error
 */
static status_t
net_buffer_write_data(net_buffer *_buffer, size_t offset, const void *data, size_t size)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	if (buffer == NULL)
		return ENOBUFS;
	if (offset + size > buffer->size)
		return B_BAD_VALUE;
	if (size == 0)
		return B_OK;

	return data_node_write_data(buffer->first_node, offset, data, size);
}


static status_t
data_node_read_data(data_node *node, size_t offset, void *data, size_t size)
{
	if (node == NULL)
		return ENOBUFS;

	if (offset > node->size)
		return data_node_read_data(node->next, offset - node->size, data, size);
	else {
		if (node->size - offset < size) {
			memcpy(data, node->start + offset, node->size - offset);
			return data_node_read_data(node->next, 0, (uint8 *)data + (node->size - offset), size - (node->size - offset));
		} else {
			memcpy(data, node->start + offset, size);
			return B_OK;
		}
	}

	return B_OK;
}


/*!	Reads into \a data \a size bytes from \a buffer starting at \a offset
 *	\return B_BAD_VALUE if you write outside of the buffers current bounds.
 *	\return ENOBUFS if there was an internal buffer error
 */
static status_t
net_buffer_read_data(net_buffer *_buffer, size_t offset, void *data, size_t size)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	if (buffer == NULL)
		return ENOBUFS;
	if (offset + size > buffer->size)
		return B_BAD_VALUE;
	if (size == 0)
		return B_OK;

	return data_node_read_data(buffer->first_node, offset, data, size);
}


static data_node *
data_node_prepend_size(data_node *node, size_t size, void **_contiguousBuffer)
{

	if (node->header_space < size) {
		data_node *new_node;
		size_t available_space;
		if (node->size == 0) {
			// current node is empty; use it before making new ones
			available_space = node->header_space + node->trailer_space;
			node->start += node->trailer_space;
			new_node = node;
		} else {
			// need one or more new nodes
			new_node = (data_node *)malloc(sizeof(data_node));
			new_node->datablock = (uint8 *)get_datablock(&sDatastore);
			if (new_node->datablock == NULL) {
				free(new_node);
				return NULL;
			}
			available_space = BUFFER_SIZE;
			node->previous = new_node;
			new_node->next = node;
			new_node->previous = NULL;
			new_node->start = node->datablock + available_space;
		}
		new_node->trailer_space = 0;
		if (size > available_space) {
			new_node->header_space = 0;
			new_node->size = available_space;
			new_node->start -= available_space;
			*_contiguousBuffer = NULL;
			return data_node_prepend_size(new_node, size - available_space, NULL);
		} else {
			new_node->header_space = available_space - size;
			new_node->start -= size;
			new_node->size = size;
			if (_contiguousBuffer != NULL)
				*_contiguousBuffer = new_node->start;
			return new_node;
		}
	} else {
		node->header_space -= size;
		node->start -= size;
		node->size += size;
		if (_contiguousBuffer != NULL)
			*_contiguousBuffer = node->start;
	}

	return node;
}


/*!
 * Allocates \a size bytes of space at the beginning of \a buffer.
 * \a contiguousBuffer is a pointer to this region, or NULL if it
 * is not contiguous
 */
static status_t
net_buffer_prepend_size(net_buffer *_buffer, size_t size, void **_contiguousBuffer)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	data_node *node = data_node_prepend_size(buffer->first_node, size, _contiguousBuffer);
	if (node == NULL)
		return ENOBUFS;

	buffer->first_node = node;
	buffer->size += size;
	return B_OK;
}


static status_t
net_buffer_prepend_data(net_buffer *buffer, const void *data, size_t size)
{
	void *contiguousBuffer;
	status_t status = net_buffer_prepend_size(buffer, size, &contiguousBuffer);
	if (status < B_OK)
		return status;

	if (contiguousBuffer)
		memcpy(contiguousBuffer, data, size);
	else
		net_buffer_write_data(buffer, 0, data, size);

	return B_OK;
}


static status_t
data_node_append_size(data_node *node, size_t size, void **_contiguousBuffer)
{
	if (node == NULL)
		return ENOBUFS;

	while (node->next != NULL)
		node = node->next;

	if (node->trailer_space < size) {
		data_node *new_node;
		size_t available_space;
		if (node->size == 0) {
			// if this node is empty, use it before making new nodes
			available_space = node->header_space + node->trailer_space;	
			node->start -= node->header_space;
			new_node = node;
		} else {
			// need one or more new buffers
			new_node = (data_node *)malloc(sizeof(data_node));
			new_node->datablock = (uint8 *)get_datablock(&sDatastore);
			if (new_node->datablock == NULL) {
				free(new_node);
				return ENOBUFS;
			}
			available_space = BUFFER_SIZE;
			new_node->previous = node;
			node->next = new_node;
			new_node->next = NULL;
			new_node->start = new_node->datablock;
		}
		new_node->header_space = 0;
		if (size > available_space) {
			new_node->trailer_space = 0;
			new_node->size = available_space;
			*_contiguousBuffer = NULL;
			return data_node_append_size(new_node, size - available_space, NULL);
		} else {
			new_node->trailer_space = available_space - size;
			new_node->size = size;
			if (_contiguousBuffer != NULL)
				*_contiguousBuffer = new_node->start;
		}
	} else {
		if (_contiguousBuffer != NULL)
			*_contiguousBuffer = node->start + node->size;
		node->trailer_space -= size;
		node->size += size;
	}

	return B_OK;
}


static status_t
net_buffer_append_size(net_buffer *_buffer, size_t size, void **_contiguousBuffer)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	status_t status = data_node_append_size(buffer->first_node, size, _contiguousBuffer);
	if (status < B_OK)
		return status;

	buffer->size += size;
	return B_OK;
}


static status_t
net_buffer_append_data(net_buffer *buffer, const void *data, size_t size)
{
	size_t used = buffer->size;

	void *contiguousBuffer;
	status_t status = net_buffer_append_size(buffer, size, &contiguousBuffer);
	if (status < B_OK)
		return status;

	if (contiguousBuffer)
		memcpy(contiguousBuffer, data, size);
	else
		net_buffer_write_data(buffer, used, data, size);

	return B_OK;
}


static data_node *
data_node_remove_header(data_node *node, size_t bytes)
{
	if (node == NULL)
		return NULL;

	if (node->size < bytes) {
		data_node *next_node = data_node_remove_header(node->next, bytes - node->size);
		put_datablock(&sDatastore, (void *)node->datablock);
		free(node);
		return next_node;
	} else {
		if (node->size == bytes && node->next != NULL) {
			data_node *next_node = node->next;
			put_datablock(&sDatastore, (void *)node->datablock);
			free(node);
			return next_node;
		} else {
			node->start += bytes;
			node->header_space += bytes;
			node->size -= bytes;
		}
	}
	return node;
}


/*!
	Removes bytes from the beginning of the buffer.
*/
static status_t
net_buffer_remove_header(net_buffer *_buffer, size_t bytes)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	if (bytes > buffer->size)
		return B_BAD_VALUE;

	data_node *node = data_node_remove_header(buffer->first_node, bytes);
	if (node == NULL)
		return ENOBUFS;

	buffer->size -= bytes;
	buffer->first_node = node;
	return B_OK;
}


static status_t
data_node_trim_data(data_node *node, size_t newSize)
{
	if (node == NULL)
		return ENOBUFS;

	if (newSize > node->size)
		return data_node_trim_data(node->next, newSize - node->size);
	else {
		if (newSize == 0 && node->previous != NULL) {
			node->previous->next = NULL;
			data_node_free(node);
		} else {
			node->trailer_space += node->size - newSize;
			node->size = newSize;
			if (node->next != NULL) {
				data_node_free(node->next);
				node->next = NULL;
			}
		}
	}
	return B_OK;
}


/*!
	Trims the buffer to the specified \a newSize by removing space from
	the end of the buffer.
*/
static status_t
net_buffer_trim_data(net_buffer *_buffer, size_t newSize)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	if (newSize > buffer->size)
		return B_BAD_VALUE;

	status_t status = data_node_trim_data(buffer->first_node, newSize);
	if (status < B_OK)
		return ENOBUFS;

	buffer->size = newSize;
	return B_OK;
}


/*!
 *	Removes \a bytes from the end of \a buffer
 */
static status_t
net_buffer_remove_trailer(net_buffer *buffer, size_t bytes)
{
	if (buffer->size < bytes)
		return B_BAD_VALUE;

	return net_buffer_trim_data(buffer, buffer->size - bytes);
}


/*!
	Tries to directly access the requested space in the buffer.
	If the space is contiguous, the function will succeed and place a pointer
	to that space into \a _contiguousBuffer.

	\return B_BAD_VALUE if the offset is outside of the buffer's bounds.
	\return B_ERROR in case the buffer is not contiguous at that location.
*/
static status_t
net_buffer_direct_access(net_buffer *_buffer, uint32 offset, size_t size,
	void **_contiguousBuffer)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	if (offset + size > buffer->size)
		return B_BAD_VALUE;

	// find node to access

	data_node *node = buffer->first_node;
	while (node->size < offset) {
		offset -= node->size;
		node = node->next;
		if (node == NULL)
			return B_BAD_VALUE;
	}

	if (size > node->size - offset)
		return B_ERROR;

	*_contiguousBuffer = node->start + offset;
	return B_OK;
}


static int32
net_buffer_checksum_data(net_buffer *_buffer, uint32 offset, size_t size, bool finalize)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	if (offset + size > buffer->size || size == 0)
		return B_BAD_VALUE;

	// find first node to read from

	data_node *node = buffer->first_node;
	while (node->size < offset) {
		offset -= node->size;
		node = node->next;
		if (node == NULL)
			return B_ERROR;
	}

	// Since the maximum buffer size is 65536 bytes, it's impossible
	// to overlap 32 bit - we don't need to handle this overlap in
	// the loop, we can safely do it afterwards
	uint32 sum = 0;

	while (true) {
		size_t bytes = min_c(size, node->size - offset);
		if (offset & 1) {
			// if we're at an uneven offset, we have to swap the checksum
			sum += __swap_int16(compute_checksum(node->start + offset, bytes));
		} else
			sum += compute_checksum(node->start + offset, bytes);

		size -= bytes;
		if (size == 0)
			break;

		offset = 0;

		node = node->next;
		if (node == NULL)
			return B_ERROR;
	}

	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}

	if (!finalize)
		return (uint16)sum;

	return (uint16)~sum;
}


static uint32
net_buffer_get_iovecs(net_buffer *_buffer, struct iovec *iovecs, uint32 vecCount)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;
	data_node *node = buffer->first_node;
	uint32 count = 0;

	while (count < vecCount) {
		if (node == NULL)
			break;
		iovecs[count].iov_base = node->start;
		iovecs[count].iov_len = node->size;
		count++;

		node = node->next;
	}

	return count;
}


static uint32
net_buffer_count_iovecs(net_buffer *_buffer)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;
	data_node *node = buffer->first_node;
	uint32 count = 0;

	while (true) {
		if (node == NULL)
			break;
		
		count++;

		node = node->next;
	}

	return count;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return init_datastore(&sDatastore, BUFFER_SIZE, MAX_BUFFERS);
		case B_MODULE_UNINIT:
			return uninit_datastore(&sDatastore);

		default:
			return B_ERROR;
	}
}


net_buffer_module_info gNetBufferModule = {
	{
		NET_BUFFER_MODULE_NAME,
		0,
		std_ops
	},
	net_buffer_create,
	net_buffer_free,

	net_buffer_duplicate,
	net_buffer_clone,
	net_buffer_split,
	net_buffer_merge,

	net_buffer_prepend_size,
	net_buffer_prepend_data,
	net_buffer_append_size,
	net_buffer_append_data,
	NULL,	// net_buffer_insert
	NULL,	// net_buffer_remove
	net_buffer_remove_header,
	net_buffer_remove_trailer,
	net_buffer_trim_data,

	NULL,	// net_buffer_associate_data

	net_buffer_direct_access,
	net_buffer_read_data,
	net_buffer_write_data,

	net_buffer_checksum_data,

	NULL,	// net_buffer_get_memory_map
	net_buffer_get_iovecs,
	net_buffer_count_iovecs,

	NULL,	// net_buffer_dump
};

