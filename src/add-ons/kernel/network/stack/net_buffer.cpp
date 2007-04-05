/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "utility.h"

#include <net_buffer.h>
#include <util/list.h>

#include <ByteOrder.h>
#include <KernelExport.h>

#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>


//#define TRACE_BUFFER
#ifdef TRACE_BUFFER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define BUFFER_SIZE 2048
	// maximum implementation derived buffer size is 65536

struct data_node {
	struct list_link link;
	struct data_header *header;
	struct data_header *located;
	size_t		offset;			// the net_buffer-wide offset of this node
	uint8		*start;			// points to the start of the data
	uint16		used;			// defines how much memory is used by this node
	uint16		header_space;
	uint16		tail_space;
};

struct free_data {
	struct free_data *next;
	uint16		size;
};

struct data_header {
	int32		ref_count;
	addr_t		physical_address;
	free_data	*first_free;
	uint8		*data_end;
	size_t		data_space;
	data_node	*first_node;
};

struct net_buffer_private : net_buffer {
	struct list	buffers;
	data_node	first_node;
};


static status_t append_data(net_buffer *buffer, const void *data, size_t size);
static status_t trim_data(net_buffer *_buffer, size_t newSize);
static status_t remove_header(net_buffer *_buffer, size_t bytes);
static status_t remove_trailer(net_buffer *_buffer, size_t bytes);


#if 1
static void
dump_buffer(net_buffer *_buffer)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	dprintf("buffer %p, size %ld\n", buffer, buffer->size);
	data_node *node = NULL;
	while ((node = (data_node *)list_get_next_item(&buffer->buffers, node)) != NULL) {
		dprintf("  node %p, offset %lu, used %u, header %u, tail %u, header %p\n",
			node, node->offset, node->used, node->header_space, node->tail_space, node->header);
		//dump_block((char *)node->start, node->used, "    ");
		dump_block((char *)node->start, min_c(node->used, 32), "    ");
	}
}
#endif


static data_header *
create_data_header(size_t headerSpace)
{
	// TODO: don't use malloc!
	data_header *header = (data_header *)malloc(BUFFER_SIZE);
	if (header == NULL)
		return NULL;

	header->ref_count = 1;
	header->physical_address = 0;
		// TODO: initialize this correctly
	header->data_space = headerSpace;
	header->data_end = (uint8 *)header + sizeof(struct data_header);
	header->first_free = NULL;
	header->first_node = NULL;

	TRACE(("  create new data header %p\n", header));
	return header;
}


static void
release_data_header(data_header *header)
{
	if (atomic_add(&header->ref_count, -1) != 1)
		return;

	TRACE(("  free header %p\n", header));
	free(header);
}


inline void
acquire_data_header(data_header *header)
{
	atomic_add(&header->ref_count, 1);
}


static void
free_data_header_space(data_header *header, uint8 *data, size_t size)
{
	if (size < sizeof(free_data))
		size = sizeof(free_data);

	free_data *freeData = (free_data *)data;
	freeData->next = header->first_free;
	freeData->size = size;

	header->first_free = freeData;
	header->data_space += size;
	// TODO: the first node's header space could grow again
}


static uint8 *
alloc_data_header_space(data_header *header, size_t size)
{
	if (size < sizeof(free_data))
		size = sizeof(free_data);

	if (header->first_free != NULL && header->first_free->size >= size) {
		// the first entry of the header space matches the allocation's needs
		uint8 *data = (uint8 *)header->first_free;
		header->first_free = header->first_free->next;
		return data;
	}

	if (header->data_space < size) {
		// there is no free space left, search free list
		free_data *freeData = header->first_free;
		free_data *last = NULL;
		while (freeData != NULL) {
			if (last != NULL && freeData->size >= size) {
				// take this one
				last->next = freeData->next;
				return (uint8 *)freeData;
			}

			last = freeData;
			freeData = freeData->next;
		}

		return NULL;
	}

	// allocate new space

	uint8 *data = header->data_end;
	header->data_end += size;
	header->data_space -= size;

	if (header->first_node != NULL)
		header->first_node->header_space -= size;

	return data;
}


static void
init_data_node(data_node *node, size_t headerSpace)
{
	node->offset = 0;
	node->start = (uint8 *)node->header + sizeof(data_header) + headerSpace;
	node->used = 0;
	node->header_space = headerSpace;
	node->tail_space = BUFFER_SIZE - headerSpace - sizeof(data_header);
}


static data_node *
add_data_node(data_header *header, data_header *located = NULL)
{
	if (located == NULL)
		located = header;

	data_node *node = (data_node *)alloc_data_header_space(located, sizeof(data_node));
	if (node == NULL)
		return NULL;

	TRACE(("  add data node %p to header %p\n", node, header));
	acquire_data_header(header);
	if (located != header)
		acquire_data_header(located);

	memset(node, 0, sizeof(struct data_node));
	node->located = located;
	node->header = header;
	return node;
}


void
remove_data_node(data_node *node)
{
	data_header *located = node->located;

	TRACE(("  remove data node %p from header %p (located %p)\n", node, node->header, located));

	if (located != node->header)
		release_data_header(node->header);

	if (located == NULL)
		return;

	free_data_header_space(located, (uint8 *)node, sizeof(data_node));
	if (located->first_node == node) {
		located->first_node = NULL;
		located->data_space = 0;
	}

	release_data_header(located);
}


static inline data_node *
get_node_at_offset(net_buffer_private *buffer, size_t offset)
{
	data_node *node = (data_node *)list_get_first_item(&buffer->buffers);
	while (node->offset + node->used < offset) {
		node = (data_node *)list_get_next_item(&buffer->buffers, node);
		if (node == NULL)
			return NULL;
	}

	return node;
}


//	#pragma mark -


static net_buffer *
create_buffer(size_t headerSpace)
{
	net_buffer_private *buffer = (net_buffer_private *)malloc(sizeof(struct net_buffer_private));
	if (buffer == NULL)
		return NULL;

	TRACE(("create buffer %p\n", buffer));

	data_header *header = create_data_header(headerSpace);
	if (header == NULL) {
		free(buffer);
		return NULL;
	}

	buffer->first_node.header = header;
	buffer->first_node.located = NULL;
	init_data_node(&buffer->first_node, headerSpace);
	header->first_node = &buffer->first_node;

	list_init(&buffer->buffers);
	list_add_item(&buffer->buffers, &buffer->first_node);

	buffer->source.ss_len = 0;
	buffer->destination.ss_len = 0;
	buffer->interface = NULL;
	buffer->offset = 0;
	buffer->flags = 0;
	buffer->size = 0;

	return buffer;
}


static void
free_buffer(net_buffer *_buffer)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	TRACE(("free buffer %p\n", buffer));

	data_node *node;
	while ((node = (data_node *)list_remove_head_item(&buffer->buffers)) != NULL) {
		remove_data_node(node);
	}

	free(buffer);
}


/*!	Creates a duplicate of the \a buffer. The new buffer does not share internal
	storage; they are completely independent from each other.
*/
static net_buffer *
duplicate_buffer(net_buffer *_buffer)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	net_buffer *duplicate = create_buffer(buffer->first_node.header_space);
	if (duplicate == NULL)
		return NULL;

	// copy the data from the source buffer

	data_node *node = (data_node *)list_get_first_item(&buffer->buffers);
	while (true) {
		if (append_data(duplicate, node->start, node->used) < B_OK) {
			free_buffer(duplicate);
			return NULL;
		}

		node = (data_node *)list_get_next_item(&buffer->buffers, node);
		if (node == NULL)
			break;
	}

	// copy meta data from source buffer

	memcpy(&duplicate->source, &buffer->source,
		min_c(buffer->source.ss_len, sizeof(sockaddr_storage)));
	memcpy(&duplicate->destination, &buffer->destination,
		min_c(buffer->destination.ss_len, sizeof(sockaddr_storage)));

	duplicate->flags = buffer->flags;
	duplicate->interface = buffer->interface;
	duplicate->offset = buffer->offset;
	duplicate->size = buffer->size;
	duplicate->protocol = buffer->protocol;

	return duplicate;
}


/*!	Clones the buffer by grabbing another reference to the underlying data.
	If that data changes, it will be changed in the clone as well.

	If \a shareFreeSpace is \c true, the cloned buffer may claim the free
	space in the original buffer as the original buffer can still do. If you
	are using this, it's your responsibility that only one of the buffers
	will do this.
*/
static net_buffer *
clone_buffer(net_buffer *_buffer, bool shareFreeSpace)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	TRACE(("clone_buffer(buffer %p)\n", buffer));

	net_buffer_private *clone = (net_buffer_private *)malloc(sizeof(struct net_buffer_private));
	if (clone == NULL)
		return NULL;

	data_node *sourceNode = (data_node *)list_get_first_item(&buffer->buffers);
	if (sourceNode == NULL) {
		free(clone);
		return NULL;
	}

	list_init(&clone->buffers);

	// grab reference to this buffer - all additional nodes will get
	// theirs in add_data_node()
	acquire_data_header(sourceNode->header);
	data_node *node = &clone->first_node;
	node->header = sourceNode->header;
	node->located = NULL;

	while (sourceNode != NULL) {
		node->start = sourceNode->start;
		node->used = sourceNode->used;
		node->offset = sourceNode->offset;

		if (shareFreeSpace) {
			// both buffers could claim the free space - note that this option
			// has to be used carefully
			node->header_space = sourceNode->header_space;
			node->tail_space = sourceNode->tail_space;
		} else {
			// the free space stays with the original buffer
			node->header_space = 0;
			node->tail_space = 0;
		}

		// add node to clone's list of buffers
		list_add_item(&clone->buffers, node);

		sourceNode = (data_node *)list_get_next_item(&buffer->buffers, sourceNode);
		if (sourceNode == NULL)
			break;

		node = add_data_node(sourceNode->header);
		if (node == NULL) {
			// There was not enough space left for another node in this buffer
			// TODO: handle this case!
			panic("clone buffer hits size limit... (fix me)");
			free(clone);
			return NULL;
		}
	}

	// copy meta data from source buffer

	memcpy(&clone->source, &buffer->source,
		min_c(buffer->source.ss_len, sizeof(sockaddr_storage)));
	memcpy(&clone->destination, &buffer->destination,
		min_c(buffer->destination.ss_len, sizeof(sockaddr_storage)));

	clone->flags = buffer->flags;
	clone->interface = buffer->interface;
	clone->offset = buffer->offset;
	clone->size = buffer->size;
	clone->protocol = buffer->protocol;

	return clone;
}


/*!
	Split the buffer at offset, the header data
	is returned as new buffer.
	TODO: optimize and avoid making a copy.
*/
static net_buffer *
split_buffer(net_buffer *from, uint32 offset)
{
	net_buffer *buffer = duplicate_buffer(from);
	if (buffer == NULL)
		return NULL;

	TRACE(("split_buffer(buffer %p -> %p, offset %ld)\n", from, buffer, offset));

	if (remove_header(from, offset) == B_OK
		&& trim_data(buffer, offset) == B_OK)
		return buffer;

	free_buffer(buffer);
	return NULL;
}


/*!
	Merges the second buffer with the first. If \a after is \c true, the
	second buffer's contents will be appended to the first ones, else they
	will be prepended.
	The second buffer will be freed if this function succeeds.
*/
static status_t
merge_buffer(net_buffer *_buffer, net_buffer *_with, bool after)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;
	net_buffer_private *with = (net_buffer_private *)_with;
	if (with == NULL)
		return B_BAD_VALUE;

	TRACE(("merge buffer %p with %p (%s)\n", buffer, with, after ? "after" : "before"));
	//dump_buffer(buffer);
	//dprintf("with:\n");
	//dump_buffer(with);

	// TODO: this is currently very simplistic, I really need to finish the
	//	harder part of this implementation (data_node management per header)

	data_node *before = NULL;

	if (!after) {
		// change offset of all nodes already in the buffer
		data_node *node = NULL;
		while (true) {
			node = (data_node *)list_get_next_item(&buffer->buffers, node);
			if (node == NULL)
				break;

			node->offset += with->size;
			if (before == NULL)
				before = node;
		}
	}

	data_node *last = NULL;

	while (true) {
		data_node *node = (data_node *)list_get_next_item(&with->buffers, last);
		if (node == NULL)
			break;

		if ((uint8 *)node > (uint8 *)node->header
			&& (uint8 *)node < (uint8 *)node->header + BUFFER_SIZE) {
			// The node is already in the buffer, we can just move it
			// over to the new owner
			list_remove_item(&with->buffers, node);
		} else {
			// we need a new place for this node
			data_node *newNode = add_data_node(node->header);
			if (newNode == NULL) {
				// try again on the buffers own header
				newNode = add_data_node(node->header, buffer->first_node.header);
				if (newNode == NULL)
// TODO: try to revert buffers to their initial state!!
					return ENOBUFS;
			}

			last = node;
			*newNode = *node;
			node = newNode;
				// the old node will get freed with its buffer
		}

		if (after) {
			list_add_item(&buffer->buffers, node);
			node->offset = buffer->size;
		} else
			list_insert_item_before(&buffer->buffers, before, node);

		buffer->size += node->used;
	}

	// the data has been merged completely at this point
	free_buffer(with);

	//dprintf(" merge result:\n");
	//dump_buffer(buffer);
	return B_OK;
}


/*!	Writes into existing allocated memory.
	\return B_BAD_VALUE if you write outside of the buffers current
		bounds.
*/
static status_t
write_data(net_buffer *_buffer, size_t offset, const void *data, size_t size)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	if (offset + size > buffer->size)
		return B_BAD_VALUE;
	if (size == 0)
		return B_OK;

	// find first node to write into
	data_node *node = get_node_at_offset(buffer, offset);
	if (node == NULL)
		return B_BAD_VALUE;

	offset -= node->offset;

	while (true) {
		size_t written = min_c(size, node->used - offset);
		memcpy(node->start + offset, data, written);

		size -= written;
		if (size == 0)
			break;

		offset = 0;
		data = (void *)((uint8 *)data + written);

		node = (data_node *)list_get_next_item(&buffer->buffers, node);
		if (node == NULL)
			return B_BAD_VALUE;
	}

	return B_OK;
}


static status_t
read_data(net_buffer *_buffer, size_t offset, void *data, size_t size)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	if (offset + size > buffer->size)
		return B_BAD_VALUE;
	if (size == 0)
		return B_OK;

	// find first node to read from
	data_node *node = get_node_at_offset(buffer, offset);
	if (node == NULL)
		return B_BAD_VALUE;

	offset -= node->offset;

	while (true) {
		size_t bytesRead = min_c(size, node->used - offset);
		memcpy(data, node->start + offset, bytesRead);

		size -= bytesRead;
		if (size == 0)
			break;

		offset = 0;
		data = (void *)((uint8 *)data + bytesRead);

		node = (data_node *)list_get_next_item(&buffer->buffers, node);
		if (node == NULL)
			return B_BAD_VALUE;
	}

	return B_OK;
}


static status_t
prepend_size(net_buffer *_buffer, size_t size, void **_contiguousBuffer)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;
	data_node *node = (data_node *)list_get_first_item(&buffer->buffers);

	TRACE(("prepend_size(buffer %p, size %ld)\n", buffer, size));
	//dump_buffer(buffer);

	if (node->header_space < size) {
		// we need to prepend a new buffer

		// TODO: implement me!
		panic("prepending buffer not implemented\n");

		if (_contiguousBuffer)
			*_contiguousBuffer = NULL;

		return B_ERROR;
	}

	// the data fits into this buffer
	node->header_space -= size;
	node->start -= size;
	node->used += size;

	if (_contiguousBuffer)
		*_contiguousBuffer = node->start;

	// adjust offset of following nodes	
	while ((node = (data_node *)list_get_next_item(&buffer->buffers, node)) != NULL) {
		node->offset += size;
	}

	buffer->size += size;

	//dprintf(" prepend_size result:\n");
	//dump_buffer(buffer);
	return B_OK;
}


static status_t
prepend_data(net_buffer *buffer, const void *data, size_t size)
{
	void *contiguousBuffer;
	status_t status = prepend_size(buffer, size, &contiguousBuffer);
	if (status < B_OK)
		return status;

	if (contiguousBuffer)
		memcpy(contiguousBuffer, data, size);
	else
		write_data(buffer, 0, data, size);

	//dprintf(" prepend result:\n");
	//dump_buffer(buffer);

	return B_OK;
}


static status_t
append_size(net_buffer *_buffer, size_t size, void **_contiguousBuffer)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;
	data_node *node = (data_node *)list_get_last_item(&buffer->buffers);

	TRACE(("append_size(buffer %p, size %ld)\n", buffer, size));
	//dump_buffer(buffer);

	if (node->tail_space < size) {
		// we need to append a new buffer

		// compute how many buffers we're going to need
		// TODO: this doesn't leave any tail space, if that should be desired...
		uint32 tailSpace = node->tail_space;
		uint32 minimalHeaderSpace = sizeof(data_header) + 3 * sizeof(data_node);
		uint32 sizeNeeded = size - tailSpace;
		uint32 count = (sizeNeeded + BUFFER_SIZE - minimalHeaderSpace - 1)
			/ (BUFFER_SIZE - minimalHeaderSpace);
		uint32 headerSpace = BUFFER_SIZE - sizeNeeded / count - sizeof(data_header);
		uint32 sizeUsed = BUFFER_SIZE - sizeof(data_header) - headerSpace;
		uint32 sizeAdded = tailSpace;

		// allocate space left in the node
		node->tail_space -= tailSpace;
		node->used += tailSpace;
		buffer->size += tailSpace;

		// allocate all buffers

		for (uint32 i = 0; i < count; i++) {
			if (i == count - 1) {
				// last data_header - compensate rounding errors
				sizeUsed = size - sizeAdded;
				headerSpace = BUFFER_SIZE - sizeof(data_header) - sizeUsed;
			}

			data_header *header = create_data_header(headerSpace);
			if (header == NULL) {
				// TODO: free up headers we already allocated!
				return B_NO_MEMORY;
			}

			node = (data_node *)add_data_node(header);
				// this can't fail as we made sure there will be enough header space

			init_data_node(node, headerSpace);
			node->header_space = header->data_space;
			node->tail_space -= sizeUsed;
			node->used = sizeUsed;
			node->offset = buffer->size;

			header->first_node = node;
			buffer->size += sizeUsed;
			sizeAdded += sizeUsed;

			list_add_item(&buffer->buffers, node);
		}

		if (_contiguousBuffer)
			*_contiguousBuffer = NULL;

		//dprintf(" append result 1:\n");
		//dump_buffer(buffer);
		return B_OK;
	}

	// the data fits into this buffer
	node->tail_space -= size;

	if (_contiguousBuffer)
		*_contiguousBuffer = node->start + node->used;

	node->used += size;
	buffer->size += size;

	//dprintf(" append result 2:\n");
	//dump_buffer(buffer);
	return B_OK;
}


static status_t
append_data(net_buffer *buffer, const void *data, size_t size)
{
	size_t used = buffer->size;

	void *contiguousBuffer;
	status_t status = append_size(buffer, size, &contiguousBuffer);
	if (status < B_OK)
		return status;

	if (contiguousBuffer)
		memcpy(contiguousBuffer, data, size);
	else
		write_data(buffer, used, data, size);

	return B_OK;
}


/*!
	Removes bytes from the beginning of the buffer.
*/
static status_t
remove_header(net_buffer *_buffer, size_t bytes)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	if (bytes > buffer->size)
		return B_BAD_VALUE;

	TRACE(("remove_header(buffer %p, %ld bytes)\n", buffer, bytes));
	//dump_buffer(buffer);

	size_t left = bytes;
	data_node *node = NULL;

	while (left >= 0) {
		node = (data_node *)list_get_first_item(&buffer->buffers);
		if (node == NULL) {
			if (left == 0)
				break;
			return B_ERROR;
		}

		if (node->used > left)
			break;

		// node will be removed completely
		list_remove_item(&buffer->buffers, node);
		left -= node->used;
		remove_data_node(node);
		node = NULL;
	}

	// cut remaining node, if any

	if (node != NULL) {
		size_t cut = min_c(node->used, left);
		node->offset = 0;
		node->start += cut;
		node->header_space += cut;
		node->used -= cut;

		node = (data_node *)list_get_next_item(&buffer->buffers, node);
	}

	// adjust offset of following nodes	
	while (node != NULL) {
		node->offset -= bytes;
		node = (data_node *)list_get_next_item(&buffer->buffers, node);
	}

	buffer->size -= bytes;

	//dprintf(" remove result:\n");
	//dump_buffer(buffer);
	return B_OK;
}


/*!
	Removes bytes from the end of the buffer.
*/
static status_t
remove_trailer(net_buffer *buffer, size_t bytes)
{
	return trim_data(buffer, buffer->size - bytes);
}


/*!
	Trims the buffer to the specified \a newSize by removing space from
	the end of the buffer.
*/
static status_t
trim_data(net_buffer *_buffer, size_t newSize)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;
	TRACE(("trim_data(buffer %p, newSize = %ld, buffer size = %ld)\n",
		buffer, newSize, buffer->size));
	//dump_buffer(buffer);

	if (newSize > buffer->size)
		return B_BAD_VALUE;
	if (newSize == buffer->size)
		return B_OK;

	data_node *node = get_node_at_offset(buffer, newSize);
	if (node == NULL) {
		// trim size greater than buffer size
		return B_BAD_VALUE;
	}

	int32 diff = node->used + node->offset - newSize;
	node->tail_space += diff;
	node->used -= diff;

	if (node->used > 0)
		node = (data_node *)list_get_next_item(&buffer->buffers, node);

	while (node != NULL) {
		data_node *next = (data_node *)list_get_next_item(&buffer->buffers, node);
		list_remove_item(&buffer->buffers, node);
		remove_data_node(node);

		node = next;
	}

	buffer->size = newSize;

	//dprintf(" trim result:\n");
	//dump_buffer(buffer);
	return B_OK;
}


/*!
	Appends data coming from buffer \a source to the buffer \a buffer. It only
	clones the data, though, that is the data is not copied, just referenced.
*/
status_t
append_cloned_data(net_buffer *_buffer, net_buffer *_source, uint32 offset,
	size_t bytes)
{
	if (bytes == 0)
		return B_OK;

	net_buffer_private *buffer = (net_buffer_private *)_buffer;
	net_buffer_private *source = (net_buffer_private *)_source;
	TRACE(("append_cloned_data(buffer %p, source %p, offset = %ld, bytes = %ld)\n",
		buffer, source, offset, bytes));

	if (source->size < offset + bytes || source->size < offset)
		return B_BAD_VALUE;

	// find data_node to start with from the source buffer
	data_node *node = get_node_at_offset(source, offset);
	if (node == NULL) {
		// trim size greater than buffer size
		return B_BAD_VALUE;
	}

	while (node != NULL && bytes > 0) {
		data_node *clone = add_data_node(node->header, buffer->first_node.header);
		if (clone == NULL)
			clone = add_data_node(node->header);
		if (clone == NULL) {
			// There is not enough space in the buffer for another node
			// TODO: handle this case!
			dump_buffer(buffer);
			dprintf("SOURCE:\n");
			dump_buffer(source);
			panic("appending clone buffer in new header not implemented\n");
			return ENOBUFS;
		}

		if (offset)
			offset -= node->offset;

		clone->offset = buffer->size;
		clone->start = node->start + offset;
		clone->used = min_c(bytes, node->used - offset);
		clone->header_space = 0;
		clone->tail_space = 0;

		list_add_item(&buffer->buffers, clone);

		offset = 0;
		bytes -= clone->used;
		buffer->size += clone->used;
		node = (data_node *)list_get_next_item(&source->buffers, node);
	}

	if (bytes != 0)
		panic("add_cloned_data() failed, bytes != 0!\n");

	//dprintf(" append cloned result:\n");
	//dump_buffer(buffer);
	return B_OK;
}


/*!
	Tries to directly access the requested space in the buffer.
	If the space is contiguous, the function will succeed and place a pointer
	to that space into \a _contiguousBuffer.

	\return B_BAD_VALUE if the offset is outside of the buffer's bounds.
	\return B_ERROR in case the buffer is not contiguous at that location.
*/
static status_t
direct_access(net_buffer *_buffer, uint32 offset, size_t size,
	void **_contiguousBuffer)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	//TRACE(("direct_access(buffer %p, offset %ld, size %ld)\n", buffer, offset, size));

	if (offset + size > buffer->size)
		return B_BAD_VALUE;

	// find node to access
	data_node *node = get_node_at_offset(buffer, offset);
	if (node == NULL)
		return B_BAD_VALUE;

	offset -= node->offset;

	if (size > node->used - offset)
		return B_ERROR;

	*_contiguousBuffer = node->start + offset;
	return B_OK;
}


static int32
checksum_data(net_buffer *_buffer, uint32 offset, size_t size, bool finalize)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	if (offset + size > buffer->size || size == 0)
		return B_BAD_VALUE;

	// find first node to read from
	data_node *node = get_node_at_offset(buffer, offset);
	if (node == NULL)
		return B_ERROR;

	offset -= node->offset;

	// Since the maximum buffer size is 65536 bytes, it's impossible
	// to overlap 32 bit - we don't need to handle this overlap in
	// the loop, we can safely do it afterwards
	uint32 sum = 0;

	while (true) {
		size_t bytes = min_c(size, node->used - offset);
		if ((offset + node->offset) & 1) {
			// if we're at an uneven offset, we have to swap the checksum
			sum += __swap_int16(compute_checksum(node->start + offset, bytes));
		} else
			sum += compute_checksum(node->start + offset, bytes);

		size -= bytes;
		if (size == 0)
			break;

		offset = 0;

		node = (data_node *)list_get_next_item(&buffer->buffers, node);
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
get_iovecs(net_buffer *_buffer, struct iovec *iovecs, uint32 vecCount)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;
	data_node *node = (data_node *)list_get_first_item(&buffer->buffers);
	uint32 count = 0;

	while (node != NULL && count < vecCount) {
		if (node->used > 0) {
			iovecs[count].iov_base = node->start;
			iovecs[count].iov_len = node->used;
			count++;
		}

		node = (data_node *)list_get_next_item(&buffer->buffers, node);
	}

	return count;
}


static uint32
count_iovecs(net_buffer *_buffer)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;
	data_node *node = (data_node *)list_get_first_item(&buffer->buffers);
	uint32 count = 0;

	while (node != NULL) {
		if (node->used > 0)
			count++;

		node = (data_node *)list_get_next_item(&buffer->buffers, node);
	}

	return count;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

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
	create_buffer,
	free_buffer,

	duplicate_buffer,
	clone_buffer,
	split_buffer,
	merge_buffer,

	prepend_size,
	prepend_data,
	append_size,
	append_data,
	NULL,	// insert
	NULL,	// remove
	remove_header,
	remove_trailer,
	trim_data,
	append_cloned_data,

	NULL,	// associate_data

	direct_access,
	read_data,
	write_data,

	checksum_data,

	NULL,	// get_memory_map
	get_iovecs,
	count_iovecs,

	dump_buffer,	// dump
};

