/*
 * Copyright 2006-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */

#include "utility.h"

#include <net_buffer.h>
#include <slab/Slab.h>
#include <tracing.h>
#include <util/list.h>

#include <ByteOrder.h>
#include <debug.h>
#include <KernelExport.h>
#include <util/DoublyLinkedList.h>

#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/uio.h>

#include "paranoia_config.h"


//#define TRACE_BUFFER
#ifdef TRACE_BUFFER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define BUFFER_SIZE 2048
	// maximum implementation derived buffer size is 65536

//#define ENABLE_DEBUGGER_COMMANDS	1
#define PARANOID_BUFFER_CHECK	NET_BUFFER_PARANOIA

#define COMPONENT_PARANOIA_LEVEL	NET_BUFFER_PARANOIA
#include <debug_paranoia.h>

#define DATA_NODE_READ_ONLY		0x1

struct header_space {
	uint16	size;
	uint16	free;
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
	header_space	space;
	uint16		tail_space;
};

struct data_node {
	struct list_link link;
	struct data_header *header;
	struct data_header *located;
	size_t		offset;			// the net_buffer-wide offset of this node
	uint8		*start;			// points to the start of the data
	uint16		flags;
	uint16		used;			// defines how much memory is used by this node

	uint16 HeaderSpace() const
	{
		if ((flags & DATA_NODE_READ_ONLY) != 0)
			return 0;
		return header->space.free;
	}

	void AddHeaderSpace(uint16 toAdd)
	{
		if ((flags & DATA_NODE_READ_ONLY) == 0) {
			header->space.size += toAdd;
			header->space.free += toAdd;
		}
	}

	void SubtractHeaderSpace(uint16 toSubtract)
	{
		if ((flags & DATA_NODE_READ_ONLY) == 0) {
			header->space.size -= toSubtract;
			header->space.free -= toSubtract;
		}
	}

	uint16 TailSpace() const
	{
		if ((flags & DATA_NODE_READ_ONLY) != 0)
			return 0;
		return header->tail_space;
	}

	void SetTailSpace(uint16 space)
	{
		if ((flags & DATA_NODE_READ_ONLY) == 0)
			header->tail_space = space;
	}

	void FreeSpace()
	{
		if ((flags & DATA_NODE_READ_ONLY) == 0) {
			uint16 space = used + header->tail_space;
			header->space.size += space;
			header->space.free += space;
			header->tail_space = 0;
		}
	}
};

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

struct net_buffer_private : net_buffer {
	struct list			buffers;
	data_header			*allocation_header;	// the current place where we
											// allocate header space (nodes,...)
	ancillary_data_list	ancillary_data;

	struct {
		struct sockaddr_storage source;
		struct sockaddr_storage destination;
	} storage;
};


#define DATA_HEADER_SIZE				_ALIGN(sizeof(data_header))
#define DATA_NODE_SIZE					_ALIGN(sizeof(data_node))
#define MAX_FREE_BUFFER_SIZE			(BUFFER_SIZE - DATA_HEADER_SIZE)


static object_cache *sNetBufferCache;
static object_cache *sDataNodeCache;


static status_t append_data(net_buffer *buffer, const void *data, size_t size);
static status_t trim_data(net_buffer *_buffer, size_t newSize);
static status_t remove_header(net_buffer *_buffer, size_t bytes);
static status_t remove_trailer(net_buffer *_buffer, size_t bytes);
static status_t append_cloned_data(net_buffer *_buffer, net_buffer *_source,
					uint32 offset, size_t bytes);
static status_t read_data(net_buffer *_buffer, size_t offset, void *data,
					size_t size);


#if ENABLE_DEBUGGER_COMMANDS
static vint32 sAllocatedDataHeaderCount = 0;
static vint32 sAllocatedNetBufferCount = 0;
static vint32 sEverAllocatedDataHeaderCount = 0;
static vint32 sEverAllocatedNetBufferCount = 0;
#endif


#if NET_BUFFER_TRACING


namespace NetBufferTracing {


class NetBufferTraceEntry : public AbstractTraceEntry {
	public:
		NetBufferTraceEntry(net_buffer* buffer)
			:
			fBuffer(buffer)
		{
#if NET_BUFFER_TRACING_STACK_TRACE
		fStackTrace = capture_tracing_stack_trace(
			NET_BUFFER_TRACING_STACK_TRACE, 0, false);
#endif
		}

#if NET_BUFFER_TRACING_STACK_TRACE
		virtual void DumpStackTrace(TraceOutput& out)
		{
			out.PrintStackTrace(fStackTrace);
		}
#endif

	protected:
		net_buffer*	fBuffer;
#if NET_BUFFER_TRACING_STACK_TRACE
		tracing_stack_trace* fStackTrace;
#endif
};


class Create : public NetBufferTraceEntry {
	public:
		Create(size_t headerSpace, net_buffer* buffer)
			:
			NetBufferTraceEntry(buffer),
			fHeaderSpace(headerSpace)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("net buffer create: header space: %lu -> buffer: %p",
				fHeaderSpace, fBuffer);
		}

	private:
		size_t		fHeaderSpace;
};


class Free : public NetBufferTraceEntry {
	public:
		Free(net_buffer* buffer)
			:
			NetBufferTraceEntry(buffer)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("net buffer free: buffer: %p", fBuffer);
		}
};


class Duplicate : public NetBufferTraceEntry {
	public:
		Duplicate(net_buffer* buffer, net_buffer* clone)
			:
			NetBufferTraceEntry(buffer),
			fClone(clone)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("net buffer dup: buffer: %p -> %p", fBuffer, fClone);
		}

	private:
		net_buffer*		fClone;
};


class Clone : public NetBufferTraceEntry {
	public:
		Clone(net_buffer* buffer, bool shareFreeSpace, net_buffer* clone)
			:
			NetBufferTraceEntry(buffer),
			fClone(clone),
			fShareFreeSpace(shareFreeSpace)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("net buffer clone: buffer: %p, share free space: %s "
				"-> %p", fBuffer, fShareFreeSpace ? "true" : "false", fClone);
		}

	private:
		net_buffer*		fClone;
		bool			fShareFreeSpace;
};


class Split : public NetBufferTraceEntry {
	public:
		Split(net_buffer* buffer, uint32 offset, net_buffer* newBuffer)
			:
			NetBufferTraceEntry(buffer),
			fNewBuffer(newBuffer),
			fOffset(offset)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("net buffer split: buffer: %p, offset: %lu "
				"-> %p", fBuffer, fOffset, fNewBuffer);
		}

	private:
		net_buffer*		fNewBuffer;
		uint32			fOffset;
};


class Merge : public NetBufferTraceEntry {
	public:
		Merge(net_buffer* buffer, net_buffer* otherBuffer, bool after)
			:
			NetBufferTraceEntry(buffer),
			fOtherBuffer(otherBuffer),
			fAfter(after)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("net buffer merge: buffers: %p + %p, after: %s "
				"-> %p", fBuffer, fOtherBuffer, fAfter ? "true" : "false",
				fOtherBuffer);
		}

	private:
		net_buffer*		fOtherBuffer;
		bool			fAfter;
};


class AppendCloned : public NetBufferTraceEntry {
	public:
		AppendCloned(net_buffer* buffer, net_buffer* source, uint32 offset,
			size_t size)
			:
			NetBufferTraceEntry(buffer),
			fSource(source),
			fOffset(offset),
			fSize(size)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("net buffer append cloned: buffer: %p, from: %p, "
				"offset: %lu, size: %lu", fBuffer, fSource, fOffset, fSize);
		}

	private:
		net_buffer*		fSource;
		uint32			fOffset;
		size_t			fSize;
};


class PrependSize : public NetBufferTraceEntry {
	public:
		PrependSize(net_buffer* buffer, size_t size)
			:
			NetBufferTraceEntry(buffer),
			fSize(size)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("net buffer prepend size: buffer: %p, size: %lu", fBuffer,
				fSize);
		}

	private:
		size_t			fSize;
};


class AppendSize : public NetBufferTraceEntry {
	public:
		AppendSize(net_buffer* buffer, size_t size)
			:
			NetBufferTraceEntry(buffer),
			fSize(size)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("net buffer append size: buffer: %p, size: %lu", fBuffer,
				fSize);
		}

	private:
		size_t			fSize;
};


class RemoveHeader : public NetBufferTraceEntry {
	public:
		RemoveHeader(net_buffer* buffer, size_t size)
			:
			NetBufferTraceEntry(buffer),
			fSize(size)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("net buffer remove header: buffer: %p, size: %lu",
				fBuffer, fSize);
		}

	private:
		size_t			fSize;
};


class Trim : public NetBufferTraceEntry {
	public:
		Trim(net_buffer* buffer, size_t size)
			:
			NetBufferTraceEntry(buffer),
			fSize(size)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("net buffer trim: buffer: %p, size: %lu",
				fBuffer, fSize);
		}

	private:
		size_t			fSize;
};


class Read : public NetBufferTraceEntry {
	public:
		Read(net_buffer* buffer, uint32 offset, void* data, size_t size)
			:
			NetBufferTraceEntry(buffer),
			fData(data),
			fOffset(offset),
			fSize(size)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("net buffer read: buffer: %p, offset: %lu, size: %lu, "
				"data: %p", fBuffer, fOffset, fSize, fData);
		}

	private:
		void*			fData;
		uint32			fOffset;
		size_t			fSize;
};


class Write : public NetBufferTraceEntry {
	public:
		Write(net_buffer* buffer, uint32 offset, const void* data, size_t size)
			:
			NetBufferTraceEntry(buffer),
			fData(data),
			fOffset(offset),
			fSize(size)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("net buffer write: buffer: %p, offset: %lu, size: %lu, "
				"data: %p", fBuffer, fOffset, fSize, fData);
		}

	private:
		const void*		fData;
		uint32			fOffset;
		size_t			fSize;
};


#if NET_BUFFER_TRACING >= 2

class DataHeaderTraceEntry : public AbstractTraceEntry {
	public:
		DataHeaderTraceEntry(data_header* header)
			:
			fHeader(header)
		{
		}

	protected:
		data_header*	fHeader;
};


class CreateDataHeader : public DataHeaderTraceEntry {
	public:
		CreateDataHeader(data_header* header)
			:
			DataHeaderTraceEntry(header)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("net buffer data header create:  header: %p", fHeader);
		}
};


class AcquireDataHeader : public DataHeaderTraceEntry {
	public:
		AcquireDataHeader(data_header* header, int32 refCount)
			:
			DataHeaderTraceEntry(header),
			fRefCount(refCount)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("net buffer data header acquire: header: %p "
				"-> ref count: %ld", fHeader, fRefCount);
		}

	private:
		int32			fRefCount;
};


class ReleaseDataHeader : public DataHeaderTraceEntry {
	public:
		ReleaseDataHeader(data_header* header, int32 refCount)
			:
			DataHeaderTraceEntry(header),
			fRefCount(refCount)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("net buffer data header release: header: %p "
				"-> ref count: %ld", fHeader, fRefCount);
		}

	private:
		int32			fRefCount;
};

#	define T2(x)	new(std::nothrow) NetBufferTracing::x
#else
#	define T2(x)
#endif	// NET_BUFFER_TRACING >= 2

}	// namespace NetBufferTracing

#	define T(x)	new(std::nothrow) NetBufferTracing::x

#else
#	define T(x)
#	define T2(x)
#endif	// NET_BUFFER_TRACING


#if 1
static void
dump_buffer(net_buffer *_buffer)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	dprintf("buffer %p, size %ld\n", buffer, buffer->size);
	data_node *node = NULL;
	while ((node = (data_node *)list_get_next_item(&buffer->buffers, node)) != NULL) {
		dprintf("  node %p, offset %lu, used %u, header %u, tail %u, header %p\n",
			node, node->offset, node->used, node->HeaderSpace(), node->TailSpace(), node->header);
		//dump_block((char *)node->start, node->used, "    ");
		dump_block((char *)node->start, min_c(node->used, 32), "    ");
	}
}
#endif


#if ENABLE_DEBUGGER_COMMANDS

static int
dump_net_buffer_stats(int argc, char** argv)
{
	kprintf("allocated data headers: %7ld / %7ld\n", sAllocatedDataHeaderCount,
		sEverAllocatedDataHeaderCount);
	kprintf("allocated net buffers:  %7ld / %7ld\n", sAllocatedNetBufferCount,
		sEverAllocatedNetBufferCount);
	return 0;
}

#endif	// ENABLE_DEBUGGER_COMMANDS


#if PARANOID_BUFFER_CHECK

static void
check_buffer(net_buffer *_buffer)
{
	net_buffer_private *buffer = (net_buffer_private*)_buffer;

	// sum up the size of all nodes
	size_t size = 0;

	data_node *node = (data_node *)list_get_first_item(&buffer->buffers);
	while (node != NULL) {
		if (node->offset != size) {
			panic("net_buffer %p: bad node %p offset (%lu vs. %lu)",
				buffer, node, node->offset, size);
			return;
		}
		size += node->used;
		node = (data_node *)list_get_next_item(&buffer->buffers, node);
	}

	if (size != buffer->size) {
		panic("net_buffer %p size != sum of its data node sizes (%lu vs. %lu)",
			buffer, buffer->size, size);
		return;
	}
}


#if 0
static void
check_buffer_contents(net_buffer *buffer, size_t offset, const void* data,
	size_t size)
{
	void* bufferData = malloc(size);
	if (bufferData == NULL)
		return;

	if (read_data(buffer, offset, bufferData, size) == B_OK) {
		if (memcmp(bufferData, data, size) != 0) {
			int32 index = 0;
			while (((uint8*)data)[index] == ((uint8*)bufferData)[index])
				index++;
			panic("check_buffer_contents(): contents check failed at index "
				"%ld, buffer: %p, offset: %lu, size: %lu", index, buffer,
				offset, size);
		}
	} else {
		panic("failed to read from buffer %p, offset: %lu, size: %lu",
			buffer, offset, size);
	}

	free(bufferData);
}


static void
check_buffer_contents(net_buffer *buffer, size_t offset, net_buffer *source,
	size_t sourceOffset, size_t size)
{
	void* bufferData = malloc(size);
	if (bufferData == NULL)
		return;

	if (read_data(source, sourceOffset, bufferData, size) == B_OK) {
		check_buffer_contents(buffer, offset, bufferData, size);
	} else {
		panic("failed to read from source buffer %p, offset: %lu, size: %lu",
			source, sourceOffset, size);
	}

	free(bufferData);
}
#endif


# 	define CHECK_BUFFER(buffer)	check_buffer(buffer)
#else
# 	define CHECK_BUFFER(buffer)	do {} while (false)
#endif	// !PARANOID_BUFFER_CHECK


static inline data_header *
allocate_data_header()
{
#if ENABLE_DEBUGGER_COMMANDS
	atomic_add(&sAllocatedDataHeaderCount, 1);
	atomic_add(&sEverAllocatedDataHeaderCount, 1);
#endif
	return (data_header *)object_cache_alloc(sDataNodeCache, CACHE_DONT_SLEEP);
}


static inline net_buffer_private *
allocate_net_buffer()
{
#if ENABLE_DEBUGGER_COMMANDS
	atomic_add(&sAllocatedNetBufferCount, 1);
	atomic_add(&sEverAllocatedNetBufferCount, 1);
#endif
	return (net_buffer_private *)object_cache_alloc(sNetBufferCache,
		CACHE_DONT_SLEEP);
}


static inline void
free_data_header(data_header *header)
{
#if ENABLE_DEBUGGER_COMMANDS
	if (header != NULL)
		atomic_add(&sAllocatedDataHeaderCount, -1);
#endif
	object_cache_free(sDataNodeCache, header);
}


static inline void
free_net_buffer(net_buffer_private *buffer)
{
#if ENABLE_DEBUGGER_COMMANDS
	if (buffer != NULL)
		atomic_add(&sAllocatedNetBufferCount, -1);
#endif
	object_cache_free(sNetBufferCache, buffer);
}


static data_header *
create_data_header(size_t headerSpace)
{
	data_header *header = allocate_data_header();
	if (header == NULL)
		return NULL;

	header->ref_count = 1;
	header->physical_address = 0;
		// TODO: initialize this correctly
	header->space.size = headerSpace;
	header->space.free = headerSpace;
	header->data_end = (uint8 *)header + DATA_HEADER_SIZE;
	header->tail_space = (uint8 *)header + BUFFER_SIZE - header->data_end
		- headerSpace;
	header->first_free = NULL;

	TRACE(("%ld:   create new data header %p\n", find_thread(NULL), header));
	T2(CreateDataHeader(header));
	return header;
}


static void
release_data_header(data_header *header)
{
	int32 refCount = atomic_add(&header->ref_count, -1);
	T2(ReleaseDataHeader(header, refCount - 1));
	if (refCount != 1)
		return;

	TRACE(("%ld:   free header %p\n", find_thread(NULL), header));
	free_data_header(header);
}


inline void
acquire_data_header(data_header *header)
{
	int32 refCount = atomic_add(&header->ref_count, 1);
	(void)refCount;
	T2(AcquireDataHeader(header, refCount + 1));
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
}


/*!
	Tries to allocate \a size bytes from the free space in the header.
*/
static uint8 *
alloc_data_header_space(data_header *header, size_t size)
{
	if (size < sizeof(free_data))
		size = sizeof(free_data);
	size = _ALIGN(size);

	if (header->first_free != NULL && header->first_free->size >= size) {
		// the first entry of the header space matches the allocation's needs
// TODO: If the free space is greater than what shall be allocated, we leak
// the remainder of the space. We should only allocate multiples of
// _ALIGN(sizeof(free_data)) and split free space in this case. It's not that
// pressing, since the only thing allocated ATM are data_nodes, and thus the
// free space entries will always have the right size.
		uint8 *data = (uint8 *)header->first_free;
		header->first_free = header->first_free->next;
		return data;
	}

	if (header->space.free < size) {
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
	header->space.free -= size;

	return data;
}


static uint8 *
alloc_data_header_space(net_buffer_private *buffer, size_t size,
	data_header **_header = NULL)
{
	// try to allocate in our current allocation header
	uint8 *allocated = alloc_data_header_space(buffer->allocation_header, size);
	if (allocated == NULL) {
		// not enough header space left -- create a fresh buffer for headers
		data_header *header = create_data_header(MAX_FREE_BUFFER_SIZE);
		if (header == NULL)
			return NULL;

		// release our reference to the old header -- it will will stay around
		// until the last reference to it is released
		release_data_header(buffer->allocation_header);
		buffer->allocation_header = header;
			// We keep the initial reference.

		// now the allocation can only fail, if size is too big
		allocated = alloc_data_header_space(buffer->allocation_header, size);
	}

	if (_header != NULL)
		*_header = buffer->allocation_header;

	return allocated;
}


static data_node *
add_first_data_node(data_header *header)
{
	data_node *node = (data_node *)alloc_data_header_space(header,
		sizeof(data_node));
	if (node == NULL)
		return NULL;

	TRACE(("%ld:   add first data node %p to header %p\n", find_thread(NULL),
		node, header));

	acquire_data_header(header);

	memset(node, 0, sizeof(struct data_node));
	node->located = header;
	node->header = header;
	node->offset = 0;
	node->start = header->data_end + header->space.free;
	node->used = 0;
	node->flags = 0;

	return node;
}


static data_node *
add_data_node(net_buffer_private *buffer, data_header *header)
{
	data_header *located;
	data_node *node = (data_node *)alloc_data_header_space(buffer,
		sizeof(data_node), &located);
	if (node == NULL)
		return NULL;

	TRACE(("%ld:   add data node %p to header %p\n", find_thread(NULL), node,
		header));

	acquire_data_header(header);
	if (located != header)
		acquire_data_header(located);

	memset(node, 0, sizeof(struct data_node));
	node->located = located;
	node->header = header;
	node->flags = 0;
	return node;
}


void
remove_data_node(data_node *node)
{
	data_header *located = node->located;

	TRACE(("%ld:   remove data node %p from header %p (located %p)\n",
		find_thread(NULL), node, node->header, located));

	// Move all used and tail space to the header space, which is useful in case
	// this is the first node of a buffer (i.e. the header is an allocation
	// header).
	node->FreeSpace();

	if (located != node->header)
		release_data_header(node->header);

	if (located == NULL)
		return;

	free_data_header_space(located, (uint8 *)node, sizeof(data_node));

	release_data_header(located);
}


static inline data_node *
get_node_at_offset(net_buffer_private *buffer, size_t offset)
{
	data_node *node = (data_node *)list_get_first_item(&buffer->buffers);
	while (node->offset + node->used <= offset) {
		node = (data_node *)list_get_next_item(&buffer->buffers, node);
		if (node == NULL)
			return NULL;
	}

	return node;
}


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
	net_buffer_private *buffer = allocate_net_buffer();
	if (buffer == NULL)
		return NULL;

	TRACE(("%ld: create buffer %p\n", find_thread(NULL), buffer));

	// Make sure headerSpace is valid and at least the initial node fits.
	headerSpace = _ALIGN(headerSpace);
	if (headerSpace < DATA_NODE_SIZE)
		headerSpace = DATA_NODE_SIZE;
	else if (headerSpace > MAX_FREE_BUFFER_SIZE)
		headerSpace = MAX_FREE_BUFFER_SIZE;

	data_header *header = create_data_header(headerSpace);
	if (header == NULL) {
		free_net_buffer(buffer);
		return NULL;
	}
	buffer->allocation_header = header;

	data_node *node = add_first_data_node(header);

	list_init(&buffer->buffers);
	list_add_item(&buffer->buffers, node);

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

	CHECK_BUFFER(buffer);
	CREATE_PARANOIA_CHECK_SET(buffer, "net_buffer");
	SET_PARANOIA_CHECK(PARANOIA_SUSPICIOUS, buffer, &buffer->size,
		sizeof(buffer->size));

	T(Create(headerSpace, buffer));

	return buffer;
}


static void
free_buffer(net_buffer *_buffer)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	TRACE(("%ld: free buffer %p\n", find_thread(NULL), buffer));
	T(Free(buffer));

	CHECK_BUFFER(buffer);
	DELETE_PARANOIA_CHECK_SET(buffer);

	data_node *node;
	while ((node = (data_node *)list_remove_head_item(&buffer->buffers)) != NULL) {
		remove_data_node(node);
	}

	while (ancillary_data* data = buffer->ancillary_data.RemoveHead()) {
		if (data->destructor != NULL)
			data->destructor(&data->header, data->Data());
		free(data);
	}

	release_data_header(buffer->allocation_header);

	free_net_buffer(buffer);
}


/*!	Creates a duplicate of the \a buffer. The new buffer does not share internal
	storage; they are completely independent from each other.
*/
static net_buffer *
duplicate_buffer(net_buffer *_buffer)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	ParanoiaChecker _(buffer);

	TRACE(("%ld: duplicate_buffer(buffer %p)\n", find_thread(NULL), buffer));

	// TODO: We might want to choose a better header space. The minimal
	// one doesn't allow to prepend any data without allocating a new header.
	// The same holds for appending cloned data.
	net_buffer *duplicate = create_buffer(DATA_NODE_SIZE);
	if (duplicate == NULL)
		return NULL;

	TRACE(("%ld:   duplicate: %p)\n", find_thread(NULL), duplicate));
	
	// copy the data from the source buffer

	data_node *node = (data_node *)list_get_first_item(&buffer->buffers);
	while (node != NULL) {
		if (append_data(duplicate, node->start, node->used) < B_OK) {
			free_buffer(duplicate);
			CHECK_BUFFER(buffer);
			return NULL;
		}

		node = (data_node *)list_get_next_item(&buffer->buffers, node);
	}

	copy_metadata(duplicate, buffer);

	CHECK_BUFFER(buffer);
	CHECK_BUFFER(duplicate);
	RUN_PARANOIA_CHECKS(duplicate);

	T(Duplicate(buffer, duplicate));

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
// TODO: See, if the commented out code can be fixed in a safe way. We could
// probably place cloned nodes on a header not belonging to our buffer, if
// we don't free the header space for the node when removing it. Otherwise we
// mess with the header's free list which might at the same time be accessed
// by another thread.
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	net_buffer *clone = create_buffer(MAX_FREE_BUFFER_SIZE);
	if (clone == NULL)
		return NULL;

	if (append_cloned_data(clone, buffer, 0, buffer->size) != B_OK) {
		free_buffer(clone);
		return NULL;
	}

	copy_metadata(clone, buffer);

	return clone;

#if 0
	ParanoiaChecker _(buffer);

	TRACE(("%ld: clone_buffer(buffer %p)\n", find_thread(NULL), buffer));

	net_buffer_private *clone = allocate_net_buffer();
	if (clone == NULL)
		return NULL;

	TRACE(("%ld:   clone: %p\n", find_thread(NULL), buffer));

	data_node *sourceNode = (data_node *)list_get_first_item(&buffer->buffers);
	if (sourceNode == NULL) {
		free_net_buffer(clone);
		return NULL;
	}

	clone->source = (sockaddr *)&clone->storage.source;
	clone->destination = (sockaddr *)&clone->storage.destination;

	list_init(&clone->buffers);

	// grab reference to this buffer - all additional nodes will get
	// theirs in add_data_node()
	acquire_data_header(sourceNode->header);
	data_node *node = &clone->first_node;
	node->header = sourceNode->header;
	node->located = NULL;
	node->used_header_space = &node->own_header_space;

	while (sourceNode != NULL) {
		node->start = sourceNode->start;
		node->used = sourceNode->used;
		node->offset = sourceNode->offset;

		if (shareFreeSpace) {
			// both buffers could claim the free space - note that this option
			// has to be used carefully
			node->used_header_space = &sourceNode->header->space;
			node->tail_space = sourceNode->tail_space;
		} else {
			// the free space stays with the original buffer
			node->used_header_space->size = 0;
			node->used_header_space->free = 0;
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
			free_net_buffer(clone);
			return NULL;
		}
	}

	copy_metadata(clone, buffer);

	CREATE_PARANOIA_CHECK_SET(clone, "net_buffer");
	SET_PARANOIA_CHECK(PARANOIA_SUSPICIOUS, clone, &clone->size,
		sizeof(clone->size));
	CHECK_BUFFER(buffer);
	CHECK_BUFFER(clone);

	T(Clone(buffer, shareFreeSpace, clone));

	return clone;
#endif
}


/*!
	Split the buffer at offset, the header data
	is returned as new buffer.
	TODO: optimize and avoid making a copy.
*/
static net_buffer *
split_buffer(net_buffer *from, uint32 offset)
{
	// TODO: Copying the whole buffer becomes less and less efficient with
	// greater size - offset differences. What we actually want is a method
	// to copy data from one buffer to another. Then the following should be:
	// create buffer, resize, copy data. An additional append_data_from_buffer()
	// method would be even better.
	net_buffer *buffer = duplicate_buffer(from);
	if (buffer == NULL)
		return NULL;

	ParanoiaChecker _(from);
	ParanoiaChecker _2(buffer);

	TRACE(("%ld: split_buffer(buffer %p -> %p, offset %ld)\n",
		find_thread(NULL), from, buffer, offset));

	if (trim_data(buffer, offset) == B_OK) {
		if (remove_header(from, offset) == B_OK) {
			CHECK_BUFFER(from);
			CHECK_BUFFER(buffer);
			T(Split(from, offset, buffer));
			return buffer;
		}
	}

	free_buffer(buffer);
	CHECK_BUFFER(from);
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

	TRACE(("%ld: merge buffer %p with %p (%s)\n", find_thread(NULL), buffer,
		with, after ? "after" : "before"));
	T(Merge(buffer, with, after));
	//dump_buffer(buffer);
	//dprintf("with:\n");
	//dump_buffer(with);

	ParanoiaChecker _(buffer);
	CHECK_BUFFER(buffer);
	CHECK_BUFFER(with);

	// TODO: this is currently very simplistic, I really need to finish the
	//	harder part of this implementation (data_node management per header)

	data_node *before = NULL;

// TODO: Do allocating nodes (the only part that can fail) upfront. Put them
// in a list, so we can easily clean up, if necessary.

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
			data_node *newNode = add_data_node(buffer, node->header);
			if (newNode == NULL)
// TODO: try to revert buffers to their initial state!!
				return ENOBUFS;

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

	SET_PARANOIA_CHECK(PARANOIA_SUSPICIOUS, buffer, &buffer->size,
		sizeof(buffer->size));

	// the data has been merged completely at this point
	free_buffer(with);

	//dprintf(" merge result:\n");
	//dump_buffer(buffer);
	CHECK_BUFFER(buffer);

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

	T(Write(buffer, offset, data, size));

	ParanoiaChecker _(buffer);

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

	CHECK_BUFFER(buffer);

	return B_OK;
}


static status_t
read_data(net_buffer *_buffer, size_t offset, void *data, size_t size)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;

	T(Read(buffer, offset, data, size));

	ParanoiaChecker _(buffer);

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

	CHECK_BUFFER(buffer);

	return B_OK;
}


static status_t
prepend_size(net_buffer *_buffer, size_t size, void **_contiguousBuffer)
{
	net_buffer_private *buffer = (net_buffer_private *)_buffer;
	data_node *node = (data_node *)list_get_first_item(&buffer->buffers);

	T(PrependSize(buffer, size));

	ParanoiaChecker _(buffer);

	TRACE(("%ld: prepend_size(buffer %p, size %ld) [has %u]\n",
		find_thread(NULL), buffer, size, node->HeaderSpace()));
	//dump_buffer(buffer);

	if (node->HeaderSpace() < size) {
		// we need to prepend new buffers

		size_t bytesLeft = size;
		size_t sizePrepended = 0;
		do {
			if (node->HeaderSpace() == 0) {
				size_t headerSpace = MAX_FREE_BUFFER_SIZE;
				data_header *header = create_data_header(headerSpace);
				if (header == NULL) {
					remove_header(buffer, sizePrepended);
					return B_NO_MEMORY;
				}

				data_node *previous = node;

				node = (data_node *)add_first_data_node(header);

				list_insert_item_before(&buffer->buffers, previous, node);

				// Release the initial reference to the header, so that it will
				// be deleted when the node is removed.
				release_data_header(header);
			}

			size_t willConsume = min_c(bytesLeft, node->HeaderSpace());

			node->SubtractHeaderSpace(willConsume);
			node->start -= willConsume;
			node->used += willConsume;
			bytesLeft -= willConsume;
			sizePrepended += willConsume;
		} while (bytesLeft > 0);

		// correct data offset in all nodes

		size_t offset = 0;
		node = NULL;
		while ((node = (data_node *)list_get_next_item(&buffer->buffers,
				node)) != NULL) {
			node->offset = offset;
			offset += node->used;
		}

		if (_contiguousBuffer)
			*_contiguousBuffer = NULL;
	} else {
		// the data fits into this buffer
		node->SubtractHeaderSpace(size);
		node->start -= size;
		node->used += size;

		if (_contiguousBuffer)
			*_contiguousBuffer = node->start;

		// adjust offset of following nodes	
		while ((node = (data_node *)list_get_next_item(&buffer->buffers, node)) != NULL) {
			node->offset += size;
		}
	}

	buffer->size += size;

	SET_PARANOIA_CHECK(PARANOIA_SUSPICIOUS, buffer, &buffer->size,
		sizeof(buffer->size));

	//dprintf(" prepend_size result:\n");
	//dump_buffer(buffer);
	CHECK_BUFFER(buffer);
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

	T(AppendSize(buffer, size));

	ParanoiaChecker _(buffer);

	TRACE(("%ld: append_size(buffer %p, size %ld)\n", find_thread(NULL),
		buffer, size));
	//dump_buffer(buffer);

	if (node->TailSpace() < size) {
		// we need to append at least one new buffer
		uint32 previousTailSpace = node->TailSpace();
		uint32 headerSpace = DATA_NODE_SIZE;
		uint32 sizeUsed = MAX_FREE_BUFFER_SIZE - headerSpace;

		// allocate space left in the node
		node->SetTailSpace(0);
		node->used += previousTailSpace;
		buffer->size += previousTailSpace;
		uint32 sizeAdded = previousTailSpace;
		SET_PARANOIA_CHECK(PARANOIA_SUSPICIOUS, buffer, &buffer->size,
			sizeof(buffer->size));

		// allocate all buffers

		while (sizeAdded < size) {
			if (sizeAdded + sizeUsed > size) {
				// last data_header and not all available space is used
				sizeUsed = size - sizeAdded;
			}

			data_header *header = create_data_header(headerSpace);
			if (header == NULL) {
				remove_trailer(buffer, sizeAdded);
				return B_NO_MEMORY;
			}

			node = add_first_data_node(header);

			node->SetTailSpace(node->TailSpace() - sizeUsed);
			node->used = sizeUsed;
			node->offset = buffer->size;

			buffer->size += sizeUsed;
			sizeAdded += sizeUsed;
			SET_PARANOIA_CHECK(PARANOIA_SUSPICIOUS, buffer, &buffer->size,
				sizeof(buffer->size));

			list_add_item(&buffer->buffers, node);

			// Release the initial reference to the header, so that it will
			// be deleted when the node is removed.
			release_data_header(header);
		}

		if (_contiguousBuffer)
			*_contiguousBuffer = NULL;

		//dprintf(" append result 1:\n");
		//dump_buffer(buffer);
		CHECK_BUFFER(buffer);

		return B_OK;
	}

	// the data fits into this buffer
	node->SetTailSpace(node->TailSpace() - size);

	if (_contiguousBuffer)
		*_contiguousBuffer = node->start + node->used;

	node->used += size;
	buffer->size += size;
	SET_PARANOIA_CHECK(PARANOIA_SUSPICIOUS, buffer, &buffer->size,
		sizeof(buffer->size));

	//dprintf(" append result 2:\n");
	//dump_buffer(buffer);
	CHECK_BUFFER(buffer);

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

	T(RemoveHeader(buffer, bytes));

	ParanoiaChecker _(buffer);

	if (bytes > buffer->size)
		return B_BAD_VALUE;

	TRACE(("%ld: remove_header(buffer %p, %ld bytes)\n", find_thread(NULL),
		buffer, bytes));
	//dump_buffer(buffer);

	size_t left = bytes;
	data_node *node = NULL;

	while (left >= 0) {
		node = (data_node *)list_get_first_item(&buffer->buffers);
		if (node == NULL) {
			if (left == 0)
				break;
			CHECK_BUFFER(buffer);
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
		node->AddHeaderSpace(cut);
		node->used -= cut;

		node = (data_node *)list_get_next_item(&buffer->buffers, node);
	}

	// adjust offset of following nodes	
	while (node != NULL) {
		node->offset -= bytes;
		node = (data_node *)list_get_next_item(&buffer->buffers, node);
	}

	buffer->size -= bytes;
	SET_PARANOIA_CHECK(PARANOIA_SUSPICIOUS, buffer, &buffer->size,
		sizeof(buffer->size));

	//dprintf(" remove result:\n");
	//dump_buffer(buffer);
	CHECK_BUFFER(buffer);

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
	TRACE(("%ld: trim_data(buffer %p, newSize = %ld, buffer size = %ld)\n",
		find_thread(NULL), buffer, newSize, buffer->size));
	T(Trim(buffer, newSize));
	//dump_buffer(buffer);

	ParanoiaChecker _(buffer);

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
	node->SetTailSpace(node->TailSpace() + diff);
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
	SET_PARANOIA_CHECK(PARANOIA_SUSPICIOUS, buffer, &buffer->size,
		sizeof(buffer->size));

	//dprintf(" trim result:\n");
	//dump_buffer(buffer);
	CHECK_BUFFER(buffer);

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
	TRACE(("%ld: append_cloned_data(buffer %p, source %p, offset = %ld, "
		"bytes = %ld)\n", find_thread(NULL), buffer, source, offset, bytes));
	T(AppendCloned(buffer, source, offset, bytes));

	ParanoiaChecker _(buffer);
	ParanoiaChecker _2(source);

	if (source->size < offset + bytes || source->size < offset)
		return B_BAD_VALUE;

	// find data_node to start with from the source buffer
	data_node *node = get_node_at_offset(source, offset);
	if (node == NULL) {
		// trim size greater than buffer size
		return B_BAD_VALUE;
	}

	size_t sizeAppended = 0;

	while (node != NULL && bytes > 0) {
		data_node *clone = add_data_node(buffer, node->header);
		if (clone == NULL) {
			remove_trailer(buffer, sizeAppended);
			return ENOBUFS;
		}

		if (offset)
			offset -= node->offset;

		clone->offset = buffer->size;
		clone->start = node->start + offset;
		clone->used = min_c(bytes, node->used - offset);
		clone->flags |= DATA_NODE_READ_ONLY;

		list_add_item(&buffer->buffers, clone);

		offset = 0;
		bytes -= clone->used;
		buffer->size += clone->used;
		sizeAppended += clone->used;
		node = (data_node *)list_get_next_item(&source->buffers, node);
	}

	if (bytes != 0)
		panic("add_cloned_data() failed, bytes != 0!\n");

	//dprintf(" append cloned result:\n");
	//dump_buffer(buffer);
	CHECK_BUFFER(source);
	CHECK_BUFFER(buffer);
	SET_PARANOIA_CHECK(PARANOIA_SUSPICIOUS, buffer, &buffer->size,
		sizeof(buffer->size));

	return B_OK;
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

	ParanoiaChecker _(buffer);

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


static void
swap_addresses(net_buffer *buffer)
{
	std::swap(buffer->source, buffer->destination);
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			// TODO: improve our code a bit so we can add constructors
			//	and keep around half-constructed buffers in the slab

			sNetBufferCache = create_object_cache("net buffer cache",
				sizeof(net_buffer_private), 8, NULL, NULL, NULL);
			if (sNetBufferCache == NULL)
				return B_NO_MEMORY;

			sDataNodeCache = create_object_cache("data node cache", BUFFER_SIZE,
				0, NULL, NULL, NULL);
			if (sDataNodeCache == NULL) {
				delete_object_cache(sNetBufferCache);
				return B_NO_MEMORY;
			}

#if ENABLE_DEBUGGER_COMMANDS
			add_debugger_command_etc("net_buffer_stats", &dump_net_buffer_stats,
				"Print net buffer statistics",
				"\nPrint net buffer statistics.\n", 0);
#endif

			return B_OK;

		case B_MODULE_UNINIT:
#if ENABLE_DEBUGGER_COMMANDS
			remove_debugger_command("net_buffer_stats", &dump_net_buffer_stats);
#endif
			delete_object_cache(sNetBufferCache);
			delete_object_cache(sDataNodeCache);
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

