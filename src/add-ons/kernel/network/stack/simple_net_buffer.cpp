/*
 * Copyright 2006-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */

#include "simple_net_buffer.h"

#include "utility.h"

#include <net_buffer.h>
#include <slab/Slab.h>
#include <tracing.h>
#include <util/list.h>

#include <ByteOrder.h>
#include <debug.h>
#include <KernelExport.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>

#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>

#include "paranoia_config.h"


//#define TRACE_BUFFER
#ifdef TRACE_BUFFER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define MAX_ANCILLARY_DATA_SIZE	128

struct ancillary_data : DoublyLinkedListLinkImpl<ancillary_data> {
	void* Data()
	{
		return (char*)this + _ALIGN(sizeof(ancillary_data));
	}

	static ancillary_data* FromData(void* data)
	{
		return (ancillary_data*)((char*)data - _ALIGN(sizeof(ancillary_data)));
	}

	ancillary_data_header	header;
	void (*destructor)(const ancillary_data_header*, void*);
};

typedef DoublyLinkedList<ancillary_data> ancillary_data_list;


struct net_buffer_private : simple_net_buffer {
	ancillary_data_list	ancillary_data;
};


static status_t append_data(net_buffer *buffer, const void *data, size_t size);
static status_t trim_data(net_buffer *_buffer, size_t newSize);
static status_t remove_header(net_buffer *_buffer, size_t bytes);
static status_t remove_trailer(net_buffer *_buffer, size_t bytes);


static void
copy_metadata(net_buffer *destination, const net_buffer *source)
{
	memcpy(destination->source, source->source,
		min_c(source->source->sa_len, sizeof(sockaddr_storage)));
	memcpy(destination->destination, source->destination,
		min_c(source->destination->sa_len, sizeof(sockaddr_storage)));

	destination->flags = source->flags;
	destination->interface = source->interface;
	destination->offset = source->offset;
	destination->size = source->size;
	destination->protocol = source->protocol;
	destination->type = source->type;
}


//	#pragma mark - module API


static net_buffer *
create_buffer(size_t headerSpace)
{
	net_buffer_private *buffer = new(nothrow) net_buffer_private;
	if (buffer == NULL)
		return NULL;

	TRACE(("%ld: create buffer %p\n", find_thread(NULL), buffer));

	buffer->data = NULL;
	new(&buffer->ancillary_data) ancillary_data_list;

	buffer->source = (sockaddr *)&buffer->storage.source;
	buffer->destination = (sockaddr *)&buffer->storage.destination;

	buffer->storage.source.ss_len = 0;
	buffer->storage.destination.ss_len = 0;

	buffer->interface = NULL;
	buffer->offset = 0;
	buffer->flags = 0;
	buffer->size = 0;

	buffer->type = -1;

	return buffer;
}


static void
free_buffer(net_buffer *_buffer)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	free(buffer->data);
	delete buffer;
}


/*!	Creates a duplicate of the \a buffer. The new buffer does not share internal
	storage; they are completely independent from each other.
*/
static net_buffer *
duplicate_buffer(net_buffer *_buffer)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	net_buffer* duplicate = create_buffer(0);
	if (duplicate == NULL)
		return NULL;

	if (append_data(duplicate, buffer->data, buffer->size) != B_OK) {
		free_buffer(duplicate);
		return NULL;
	}

	copy_metadata(duplicate, buffer);

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
	return duplicate_buffer(_buffer);
}


/*!
	Split the buffer at offset, the header data
	is returned as new buffer.
	TODO: optimize and avoid making a copy.
*/
static net_buffer *
split_buffer(net_buffer *_from, uint32 offset)
{
	net_buffer_private *from = (net_buffer_private *)_from;

	if (offset > from->size)
		return NULL;

	net_buffer_private* buffer = (net_buffer_private*)create_buffer(0);
	if (buffer == NULL)
		return NULL;

	// allocate space for the tail data
	size_t remaining = from->size - offset;
	uint8* tailData = (uint8*)malloc(remaining);
	if (tailData == NULL) {
		free_buffer(buffer);
		return NULL;
	}

	memcpy(tailData, from->data + offset, remaining);

	// truncate original data and move it to the new buffer
	buffer->data = (uint8*)realloc(from->data, offset);
	buffer->size = offset;

	// the old buffer gets the newly allocated tail data
	from->data = tailData;
	from->size = remaining;

	return buffer;
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

	if (after) {
		// the simple case: just append the second buffer
		status_t error = append_data(buffer, with->data, with->size);
		if (error != B_OK)
			return error;
	} else {
		// append buffer to the second buffer, then switch the data
		status_t error = append_data(with, buffer->data, buffer->size);
		if (error != B_OK)
			return error;

		free(buffer->data);
		buffer->data = with->data;
		buffer->size = with->size;

		with->data = NULL;
	}

	free_buffer(with);

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

	memcpy(buffer->data + offset, data, size);

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

	memcpy(data, buffer->data + offset, size);

	return B_OK;
}


static status_t
prepend_size(net_buffer *_buffer, size_t size, void **_contiguousBuffer)
{
	if (size == 0)
		return B_OK;

	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	uint8* newData = (uint8*)malloc(buffer->size + size);
	if (newData == NULL)
		return B_NO_MEMORY;

	memcpy(newData + size, buffer->data, buffer->size);

	free(buffer->data);
	buffer->data = newData;
	buffer->size += size;

	if (_contiguousBuffer != NULL)
		*_contiguousBuffer = buffer->data;

	return B_OK;
}


static status_t
prepend_data(net_buffer *buffer, const void *data, size_t size)
{
	status_t status = prepend_size(buffer, size, NULL);
	if (status < B_OK)
		return status;

	write_data(buffer, 0, data, size);

	return B_OK;
}


static status_t
append_size(net_buffer *_buffer, size_t size, void **_contiguousBuffer)
{
	if (size == 0)
		return B_OK;

	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	uint8* newData = (uint8*)realloc(buffer->data, buffer->size + size);
	if (newData == NULL)
		return B_NO_MEMORY;

	if (_contiguousBuffer != NULL)
		*_contiguousBuffer = newData + buffer->size;

	buffer->data = newData;
	buffer->size += size;

	return B_OK;
}


static status_t
append_data(net_buffer *buffer, const void *data, size_t size)
{
	size_t used = buffer->size;

	status_t status = append_size(buffer, size, NULL);
	if (status < B_OK)
		return status;

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
	if (bytes == 0)
		return B_OK;

	buffer->size -= bytes;
	memmove(buffer->data, buffer->data + bytes, buffer->size);
	buffer->data = (uint8*)realloc(buffer->data, buffer->size);

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

	if (newSize > buffer->size)
		return B_BAD_VALUE;
	if (newSize == buffer->size)
		return B_OK;

	buffer->data = (uint8*)realloc(buffer->data, newSize);
	buffer->size = newSize;

	return B_OK;
}


/*!
	Appends data coming from buffer \a source to the buffer \a buffer. It only
	clones the data, though, that is the data is not copied, just referenced.
*/
static status_t
append_cloned_data(net_buffer *_buffer, net_buffer *_source, uint32 offset,
	size_t bytes)
{
	if (bytes == 0)
		return B_OK;

	net_buffer_private *buffer = (net_buffer_private *)_buffer;
	net_buffer_private *source = (net_buffer_private *)_source;

	if (offset + bytes > source->size)
		return B_BAD_VALUE;

	return append_data(buffer, source->data + offset, bytes);
}


/*!
	Attaches ancillary data to the given buffer. The data are completely
	orthogonal to the data the buffer stores.

	\param buffer The buffer.
	\param header Description of the data.
	\param data If not \c NULL, the data are copied into the allocated storage.
	\param destructor If not \c NULL, this function will be invoked with the
		data as parameter when the buffer is destroyed.
	\param _allocatedData Will be set to the storage allocated for the data.
	\return \c B_OK when everything goes well, another error code otherwise.
*/
static status_t
attach_ancillary_data(net_buffer *_buffer, const ancillary_data_header *header,
	const void *data, void (*destructor)(const ancillary_data_header*, void*),
	void **_allocatedData)
{
	// TODO: Obviously it would be nice to allocate the memory for the
	// ancillary data in the buffer.
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	// check parameters
	if (header == NULL)
		return B_BAD_VALUE;

	if (header->len > MAX_ANCILLARY_DATA_SIZE)
		return ENOBUFS;

	// allocate buffer
	void *dataBuffer = malloc(_ALIGN(sizeof(ancillary_data)) + header->len);
	if (dataBuffer == NULL)
		return B_NO_MEMORY;

	// init and attach the structure
	ancillary_data *ancillaryData = new(dataBuffer) ancillary_data;
	ancillaryData->header = *header;
	ancillaryData->destructor = destructor;

	buffer->ancillary_data.Add(ancillaryData);

	if (data != NULL)
		memcpy(ancillaryData->Data(), data, header->len);

	if (_allocatedData != NULL)
		*_allocatedData = ancillaryData->Data();

	return B_OK;
}


/*!
	Detaches ancillary data from the given buffer. The associated memory is
	free, i.e. the \a data pointer must no longer be used after calling this
	function. Depending on \a destroy, the destructor is invoked before freeing
	the data.

	\param buffer The buffer.
	\param data Pointer to the data to be removed (as returned by
		attach_ancillary_data() or next_ancillary_data()).
	\param destroy If \c true, the destructor, if one was passed to
		attach_ancillary_data(), is invoked for the data.
	\return \c B_OK when everything goes well, another error code otherwise.
*/
static status_t
detach_ancillary_data(net_buffer *_buffer, void *data, bool destroy)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	if (data == NULL)
		return B_BAD_VALUE;

	ancillary_data *ancillaryData = ancillary_data::FromData(data);

	buffer->ancillary_data.Remove(ancillaryData);

	if (destroy && ancillaryData->destructor != NULL) {
		ancillaryData->destructor(&ancillaryData->header,
			ancillaryData->Data());
	}

	free(ancillaryData);

	return B_OK;
}


/*!
	Moves all ancillary data from buffer \c from to the end of the list of
	ancillary data of buffer \c to. Note, that this is the only function that
	transfers or copies ancillary data from one buffer to another.

	\param from The buffer from which to remove the ancillary data.
	\param to The buffer to which to add teh ancillary data.
	\return A pointer to the first of the moved ancillary data, if any, \c NULL
		otherwise.
*/
static void *
transfer_ancillary_data(net_buffer *_from, net_buffer *_to)
{
	net_buffer_private *from = (net_buffer_private *)_from;
	net_buffer_private *to = (net_buffer_private *)_to;

	if (from == NULL || to == NULL)
		return NULL;

	ancillary_data *ancillaryData = from->ancillary_data.Head();
	to->ancillary_data.MoveFrom(&from->ancillary_data);

	return ancillaryData != NULL ? ancillaryData->Data() : NULL;
}


/*!
	Returns the next ancillary data. When iterating over the data, initially
	a \c NULL pointer shall be passed as \a previousData, subsequently the
	previously returned data pointer. After the last item, \c NULL is returned.

	Note, that it is not safe to call detach_ancillary_data() for a data item
	and then pass that pointer to this function. First get the next item, then
	detach the previous one.

	\param buffer The buffer.
	\param previousData The pointer to the previous data returned by this
		function. Initially \c NULL shall be passed.
	\param header Pointer to allocated storage into which the data description
		is written. May be \c NULL.
	\return A pointer to the next ancillary data in the buffer. \c NULL after
		the last one.
*/
static void*
next_ancillary_data(net_buffer *_buffer, void *previousData,
	ancillary_data_header *_header)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	ancillary_data *ancillaryData;

	if (previousData == NULL) {
		ancillaryData = buffer->ancillary_data.Head();
	} else {
		ancillaryData = ancillary_data::FromData(previousData);
		ancillaryData = buffer->ancillary_data.GetNext(ancillaryData);
	}

	if (ancillaryData == NULL)
		return NULL;

	if (_header != NULL)
		*_header = ancillaryData->header;

	return ancillaryData->Data();
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

	if (offset + size > buffer->size)
		return B_BAD_VALUE;

	*_contiguousBuffer = buffer->data + offset;
	return B_OK;
}


static int32
checksum_data(net_buffer *_buffer, uint32 offset, size_t size, bool finalize)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	if (offset + size > buffer->size || size == 0)
		return B_BAD_VALUE;

	uint16 sum = compute_checksum(buffer->data + offset, size);
	if ((offset & 1) != 0) {
		// if we're at an uneven offset, we have to swap the checksum
		sum = __swap_int16(sum);
	}

	if (!finalize)
		return (uint16)sum;

	return (uint16)~sum;
}


static uint32
get_iovecs(net_buffer *_buffer, struct iovec *iovecs, uint32 vecCount)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	iovecs[0].iov_base = buffer->data;
	iovecs[0].iov_len = buffer->size;

	return 1;
}


static uint32
count_iovecs(net_buffer *_buffer)
{
	return 1;
}


static void
swap_addresses(net_buffer *buffer)
{
	std::swap(buffer->source, buffer->destination);
}


static void
dump_buffer(net_buffer *_buffer)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	dprintf("buffer %p, size %ld, data: %p\n", buffer, buffer->size,
		buffer->data);
	dump_block((char*)buffer->data, min_c(buffer->size, 32), "    ");
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return B_OK;

		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


net_buffer_module_info gSimpleNetBufferModule = {
//net_buffer_module_info gNetBufferModule = {
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

	attach_ancillary_data,
	detach_ancillary_data,
	transfer_ancillary_data,
	next_ancillary_data,

	direct_access,
	read_data,
	write_data,

	checksum_data,

	NULL,	// get_memory_map
	get_iovecs,
	count_iovecs,

	swap_addresses,

	dump_buffer,	// dump
};

