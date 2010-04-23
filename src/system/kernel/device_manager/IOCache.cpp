/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "IOCache.h"

#include <algorithm>

#include <condition_variable.h>
#include <heap.h>
#include <low_resource_manager.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <vm/vm.h>


//#define TRACE_IO_CACHE 1
#ifdef TRACE_IO_CACHE
#	define TRACE(format...)	dprintf(format)
#else
#	define TRACE(format...)	do {} while (false)
#endif


// size of the line table
static const size_t kLineTableSize = 128;
	// TODO: The table should shrink/grow dynamically, but we can't allocate
	// memory without risking a deadlock. Use a resource resizer!


struct IOCache::Operation : IOOperation {
	ConditionVariable	finishedCondition;
};


struct IOCache::LineHashDefinition {
	typedef off_t	KeyType;
	typedef	Line	ValueType;

	LineHashDefinition(uint32 sizeShift)
		:
		fSizeShift(sizeShift)
	{
	}

	size_t HashKey(off_t key) const
	{
		return size_t(key >> fSizeShift);
	}

	size_t Hash(const Line* value) const
	{
		return HashKey(value->offset);
	}

	bool Compare(off_t key, const Line* value) const
	{
		return value->offset == key;
	}

	Line*& GetLink(Line* value) const
	{
		return value->hashNext;
	}

private:
	uint32	fSizeShift;
};


struct IOCache::LineTable : public BOpenHashTable<LineHashDefinition> {
	LineTable(const LineHashDefinition& definition)
		:
		BOpenHashTable<LineHashDefinition>(definition)
	{
	}
};


IOCache::IOCache(DMAResource* resource, size_t cacheLineSize)
	:
	fDeviceCapacity(0),
	fLineSize(cacheLineSize),
	fDMAResource(resource),
	fIOCallback(NULL),
	fIOCallbackData(NULL),
	fLineTable(NULL),
	fUnusedLine(0),
	fLineCount(0),
	fLowMemoryHandlerRegistered(false)
{
	TRACE("%p->IOCache::IOCache(%p, %" B_PRIuSIZE ")\n", this, resource,
		cacheLineSize);

	if (cacheLineSize < B_PAGE_SIZE
		|| (cacheLineSize & (cacheLineSize - 1)) != 0) {
		panic("Invalid cache line size (%" B_PRIuSIZE "). Must be a power of 2 "
			"multiple of the page size.", cacheLineSize);
	}

	mutex_init(&fLock, "I/O cache");
	mutex_init(&fSerializationLock, "I/O cache request serialization");

	fLineSizeShift = 0;
	while (cacheLineSize != 1) {
		fLineSizeShift++;
		cacheLineSize >>= 1;
	}
}


IOCache::~IOCache()
{
	if (fLowMemoryHandlerRegistered) {
		unregister_low_resource_handler(&_LowMemoryHandlerEntry, this);
			// TODO: Avoid the race condition with the handler!
	}

	delete fLineTable;

	while (Line* line = fUsedLines.RemoveHead())
		_FreeLine(line);
	_FreeLine(fUnusedLine);

	mutex_destroy(&fLock);
	mutex_destroy(&fSerializationLock);
}


status_t
IOCache::Init(const char* name)
{
	TRACE("%p->IOCache::Init(\"%s\")\n", this, name);

	// create the line hash table
	fLineTable = new(std::nothrow) LineTable(LineHashDefinition(
		fLineSizeShift));
	if (fLineTable == NULL)
		return B_NO_MEMORY;

	status_t error = fLineTable->Init(kLineTableSize);
	if (error != B_OK)
		return error;

	// create at least one cache line
	MutexLocker locker(fLock);
	fUnusedLine = _AllocateLine();
	locker.Unlock();
	if (fUnusedLine == NULL)
		return B_NO_MEMORY;

	// register low memory handler
	error = register_low_resource_handler(&_LowMemoryHandlerEntry, this,
		B_KERNEL_RESOURCE_PAGES | B_KERNEL_RESOURCE_MEMORY
			| B_KERNEL_RESOURCE_ADDRESS_SPACE, 1);
			// higher priority than the block cache, so we should be drained
			// first
	if (error != B_OK)
		return error;

	fLowMemoryHandlerRegistered = true;

	return B_OK;
}


void
IOCache::SetCallback(IOCallback& callback)
{
	SetCallback(&IOCallback::WrapperFunction, &callback);
}


void
IOCache::SetCallback(io_callback callback, void* data)
{
	fIOCallback = callback;
	fIOCallbackData = data;
}


void
IOCache::SetDeviceCapacity(off_t deviceCapacity)
{
	MutexLocker serializationLocker(fLock);
	MutexLocker locker(fSerializationLock);

	fDeviceCapacity = deviceCapacity;

	// new media -- burn all cache lines
	while (Line* line = fUsedLines.Head())
		_DiscardLine(line);
}


status_t
IOCache::ScheduleRequest(IORequest* request)
{
	TRACE("%p->IOCache::ScheduleRequest(%p)\n", this, request);

	// lock the request's memory
	status_t error;
	IOBuffer* buffer = request->Buffer();
	if (buffer->IsVirtual()) {
		error = buffer->LockMemory(request->Team(), request->IsWrite());
		if (error != B_OK) {
			request->SetStatusAndNotify(error);
			return error;
		}
	}

	// we completely serialize all I/O in FIFO order
	MutexLocker serializationLocker(fSerializationLock);
	size_t bytesTransferred = 0;
	error = _DoRequest(request, bytesTransferred);
	serializationLocker.Unlock();

	// unlock memory
	if (buffer->IsVirtual())
		buffer->UnlockMemory(request->Team(), request->IsWrite());

	// set status and notify
	if (error == B_OK) {
		request->SetTransferredBytes(bytesTransferred < request->Length(),
			bytesTransferred);
		request->SetStatusAndNotify(B_OK);
	} else
		request->SetStatusAndNotify(error);

	return error;
}


void
IOCache::OperationCompleted(IOOperation* operation, status_t status,
	size_t transferredBytes)
{
	if (status == B_OK) {
		// always fail in case of partial transfers
		((Operation*)operation)->finishedCondition.NotifyAll(false,
			transferredBytes == operation->Length() ? B_OK : B_ERROR);
	} else
		((Operation*)operation)->finishedCondition.NotifyAll(false, status);
}


status_t
IOCache::_DoRequest(IORequest* request, size_t& _bytesTransferred)
{
	off_t offset = request->Offset();
	size_t length = request->Length();

	TRACE("%p->IOCache::ScheduleRequest(%p): offset: %" B_PRIdOFF
		", length: %" B_PRIuSIZE "\n", this, request, offset, length);

	if (offset < 0 || offset > fDeviceCapacity)
		return B_BAD_VALUE;

	// truncate the request to the device capacity
	if (fDeviceCapacity - offset < length)
		length = fDeviceCapacity - offset;

	_bytesTransferred = 0;

	while (length > 0) {
		// the start of the current cache line
		off_t lineOffset = (offset >> fLineSizeShift) << fLineSizeShift;

		// intersection of request and cache line
		off_t cacheLineEnd = std::min(lineOffset + fLineSize, fDeviceCapacity);
		size_t requestLineLength
			= std::min(size_t(cacheLineEnd - offset), length);

		// transfer the data of the cache line
		status_t error = _TransferRequestLine(request, lineOffset, offset,
			requestLineLength);
		if (error != B_OK)
			return error;

		offset = cacheLineEnd;
		length -= requestLineLength;
		_bytesTransferred += requestLineLength;
	}

	return B_OK;
}


status_t
IOCache::_TransferRequestLine(IORequest* request, off_t lineOffset,
	off_t requestOffset, size_t requestLength)
{
	TRACE("%p->IOCache::_TransferRequestLine(%p, %" B_PRIdOFF
		", %" B_PRIdOFF  ", %" B_PRIuSIZE "\n", this, request, lineOffset,
		requestOffset, requestLength);

	bool isVIP = (request->Flags() & B_VIP_IO_REQUEST) != 0;

	MutexLocker locker(fLock);

	// get the cache line
	Line* line = _LookupLine(lineOffset);
	if (line == NULL) {
		// line not cached yet -- create it
		TRACE("%p->IOCache::_TransferRequestLine(): line not cached yet\n",
			this);

		line = _PrepareLine(lineOffset);

		// in case of a read or partial write, read the line from disk
		if (request->IsRead() || requestLength < line->size) {
			line->inUse = true;
			locker.Unlock();

			status_t error = _TransferLine(line, false, isVIP);

			locker.Lock();
			line->inUse = false;

			if (error != B_OK) {
				_DiscardLine(line);
				return error;
			}
		}
	} else {
		TRACE("%p->IOCache::_TransferRequestLine(): line cached: %p\n", this,
			line);
	}

	// requeue the cache line -- it's most recently used now
	fUsedLines.Remove(line);
	fUsedLines.Add(line);

	if (request->IsRead()) {
		// copy data from cache line to request
		return request->CopyData(line->buffer + (requestOffset - lineOffset),
			requestOffset, requestLength);
	} else {
		// copy data from request to cache line
		status_t error = request->CopyData(requestOffset,
			line->buffer + (requestOffset - lineOffset), requestLength);
		if (error != B_OK)
			return error;

		// write the cache line to disk
		line->inUse = true;
		locker.Unlock();

		error = _TransferLine(line, true, isVIP);
			// TODO: In case of a partial write, there's really no point in
			// writing the complete cache line.

		locker.Lock();
		line->inUse = false;

		if (error != B_OK)
			_DiscardLine(line);
		return error;
	}
}


status_t
IOCache::_TransferLine(Line* line, bool isWrite, bool isVIP)
{
	TRACE("%p->IOCache::_TransferLine(%p, write: %d, vip: %d)\n", this, line,
		isWrite, isVIP);

	// create a request for the transfer
	IORequest request;
	status_t error = request.Init(line->offset, (void*)line->physicalBuffer,
		line->size, isWrite,
		B_PHYSICAL_IO_REQUEST | (isVIP ? B_VIP_IO_REQUEST : 0));
	if (error != B_OK)
		return error;

	// Process single operations until the complete request is finished or
	// until an error occurs.
	Operation operation;
	operation.finishedCondition.Init(this, "I/O cache operation finished");

	while (request.RemainingBytes() > 0 && request.Status() > 0) {
		error = fDMAResource->TranslateNext(&request, &operation, line->size);
		if (error != B_OK)
			return error;

		error = _DoOperation(operation);

		request.RemoveOperation(&operation);

		if (fDMAResource != NULL)
			fDMAResource->RecycleBuffer(operation.Buffer());

		if (error != B_OK) {
			TRACE("%p->IOCache::_TransferLine(): operation at %" B_PRIdOFF
				" failed: %s\n", this, operation.Offset(), strerror(error));
			return error;
		}
	}

	return B_OK;
}


status_t
IOCache::_DoOperation(Operation& operation)
{
	TRACE("%p->IOCache::_DoOperation(%" B_PRIdOFF ", %" B_PRIuSIZE ")\n", this,
		operation.Offset(), operation.Length());

	ConditionVariableEntry waitEntry;
	operation.finishedCondition.Add(&waitEntry);

	status_t error = fIOCallback(fIOCallbackData, &operation);
	if (error != B_OK) {
		operation.finishedCondition.NotifyAll(false, error);
			// removes the entry from the variable
		return error;
	}

	// wait for the operation to finish
	return waitEntry.Wait();
}


IOCache::Line*
IOCache::_LookupLine(off_t lineOffset)
{
	ASSERT_LOCKED_MUTEX(&fLock);

	return fLineTable->Lookup(lineOffset);
}


IOCache::Line*
IOCache::_PrepareLine(off_t lineOffset)
{
	ASSERT_LOCKED_MUTEX(&fLock);

	Line* line = NULL;

	// if there is an unused line recycle it
	if (fUnusedLine != NULL) {
		line = fUnusedLine;
		fUnusedLine = NULL;
	} else if (!_MemoryIsLow()) {
		// resources look fine -- allocate a new line
		line = _AllocateLine();
	}

	if (line == NULL) {
		// recycle the least recently used line
		line = fUsedLines.RemoveHead();
		fLineTable->RemoveUnchecked(line);

		TRACE("%p->IOCache::_PrepareLine(%" B_PRIdOFF "): recycled line: %p\n",
			this, lineOffset, line);
	} else {
		TRACE("%p->IOCache::_PrepareLine(%" B_PRIdOFF
			"): allocated new line: %p\n", this, lineOffset, line);
	}

	line->offset = lineOffset;
	line->size = std::min((off_t)fLineSize, fDeviceCapacity - lineOffset);
	line->inUse = false;

	fUsedLines.Add(line);
	fLineTable->InsertUnchecked(line);

	return line;
}


void
IOCache::_DiscardLine(Line* line)
{
	ASSERT_LOCKED_MUTEX(&fLock);

	fLineTable->RemoveUnchecked(line);
	fUsedLines.Remove(line);

	if (fUsedLines.IsEmpty()) {
		// We keep the last cache line around, so I/O cannot fail due to a
		// failing allocation.
		TRACE("%p->IOCache::_DiscardLine(): parking line: %p\n", this, line);
		fUnusedLine = line;
		line->offset = -1;
		line->inUse = false;
	} else {
		TRACE("%p->IOCache::_DiscardLine(): freeing line: %p\n", this, line);
		_FreeLine(line);
	}
}


IOCache::Line*
IOCache::_AllocateLine()
{
	ASSERT_LOCKED_MUTEX(&fLock);

	// create the line object
	Line* line = new(malloc_flags(HEAP_DONT_WAIT_FOR_MEMORY)) Line;
	if (line == NULL)
		return NULL;

	// create the buffer area
	void* address;
	line->area = vm_create_anonymous_area(B_SYSTEM_TEAM, "I/O cache line",
		&address, B_ANY_KERNEL_ADDRESS, fLineSize, B_CONTIGUOUS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, 0,
		CREATE_AREA_DONT_WAIT | CREATE_AREA_DONT_CLEAR, true);
	if (line->area < 0) {
		delete line;
		return NULL;
	}

	// get the physical address of the buffer
	physical_entry entry;
	get_memory_map(address, B_PAGE_SIZE, &entry, 1);

	// init the line object
	line->offset = -1;
	line->buffer = (uint8*)address;
	line->physicalBuffer = (addr_t)entry.address;
	line->inUse = false;

	fLineCount++;

	return line;
}


void
IOCache::_FreeLine(Line* line)
{
	ASSERT_LOCKED_MUTEX(&fLock);

	if (line == NULL)
		return;

	fLineCount--;

	delete_area(line->area);
	delete line;
}


/*static*/ void
IOCache::_LowMemoryHandlerEntry(void* data, uint32 resources, int32 level)
{
	((IOCache*)data)->_LowMemoryHandler(resources, level);
}


void
IOCache::_LowMemoryHandler(uint32 resources, int32 level)
{
	TRACE("%p->IOCache::_LowMemoryHandler(): level: %ld\n", this, level);

	MutexLocker locker(fLock);

	// determine how many cache lines to keep
	size_t linesToKeep = fLineCount;

	switch (level) {
		case B_NO_LOW_RESOURCE:
			return;
		case B_LOW_RESOURCE_NOTE:
			linesToKeep = fLineCount / 2;
			break;
		case B_LOW_RESOURCE_WARNING:
			linesToKeep = fLineCount / 4;
			break;
		case B_LOW_RESOURCE_CRITICAL:
			linesToKeep = 1;
			break;
	}

	if (linesToKeep < 1)
		linesToKeep = 1;

	// free lines until we reach our target
	Line* line = fUsedLines.Head();
	while (linesToKeep < fLineCount && line != NULL) {
		Line* nextLine = fUsedLines.GetNext(line);

		if (!line->inUse)
			_DiscardLine(line);

		line = nextLine;
	}
}


bool
IOCache::_MemoryIsLow() const
{
	return low_resource_state(B_KERNEL_RESOURCE_PAGES
			| B_KERNEL_RESOURCE_MEMORY | B_KERNEL_RESOURCE_ADDRESS_SPACE)
		!= B_NO_LOW_RESOURCE;
}
