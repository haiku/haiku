// UserDataWriter.cpp

#include <util/kernel_cpp.h>
#include <ddm_userland_interface.h>
#include <Vector.h>

#include "UserDataWriter.h"

// RelocationEntryList
struct UserDataWriter::RelocationEntryList : Vector<addr_t*> {};

// constructor
UserDataWriter::UserDataWriter()
	: fBuffer(NULL),
	  fBufferSize(0),
	  fAllocatedSize(0),
	  fRelocationEntries(NULL)
{
}

// constructor
UserDataWriter::UserDataWriter(user_disk_device_data *buffer,
							   size_t bufferSize)
	: fBuffer(NULL),
	  fBufferSize(0),
	  fAllocatedSize(0),
	  fRelocationEntries(NULL)
{
	SetTo(buffer, bufferSize);
}

// destructor
UserDataWriter::~UserDataWriter()
{
	delete fRelocationEntries;
}

// SetTo
status_t
UserDataWriter::SetTo(user_disk_device_data *buffer, size_t bufferSize)
{
	Unset();
	fBuffer = buffer;
	fBufferSize = bufferSize;
	fAllocatedSize = 0;
	if (fBuffer && fBufferSize > 0) {
		fRelocationEntries = new(nothrow) RelocationEntryList;
		if (!fRelocationEntries)
			return B_NO_MEMORY;
	}
	return B_OK;
}

// Unset
void
UserDataWriter::Unset()
{
	delete fRelocationEntries;
	fBuffer = NULL;
	fBufferSize = 0;
	fAllocatedSize = 0;
	fRelocationEntries = NULL;
}

// AllocateData
void *
UserDataWriter::AllocateData(size_t size, size_t align)
{
	// handles size == 0 gracefully
	// get a properly aligned offset
	size_t offset = fAllocatedSize;
	if (align > 1)
		offset = (fAllocatedSize + align - 1) / align * align;
	// get the result pointer
	void *result = NULL;
	if (fBuffer && offset + size <= fBufferSize)
		result = (uint8*)fBuffer + offset;
	// always update the allocated size, even if there wasn't enough space
	fAllocatedSize = offset + size;
	return result;
}

// AllocatePartitionData
user_partition_data *
UserDataWriter::AllocatePartitionData(size_t childCount)
{
	return (user_partition_data*)AllocateData(
		sizeof(user_partition_data)
		+ sizeof(user_partition_data*) * ((int32)childCount - 1),
		sizeof(int));
}

// AllocateDeviceData
user_disk_device_data *
UserDataWriter::AllocateDeviceData(size_t childCount)
{
	return (user_disk_device_data*)AllocateData(
		sizeof(user_disk_device_data)
		+ sizeof(user_partition_data*) * ((int32)childCount - 1),
		sizeof(int));
}

// PlaceString
char *
UserDataWriter::PlaceString(const char *str)
{
	if (!str)
		return NULL;
	size_t len = strlen(str) + 1;
	char *data = (char*)AllocateData(len);
	if (data)
		memcpy(data, str, len);
	return data;
}

// AllocatedSize
size_t
UserDataWriter::AllocatedSize() const
{
	return fAllocatedSize;
}

// AddRelocationEntry
status_t
UserDataWriter::AddRelocationEntry(void *address)
{
	if (fRelocationEntries && (addr_t)address >= (addr_t)fBuffer
		&& (addr_t)address < (addr_t)fBuffer + fBufferSize - sizeof(void*)) {
		return fRelocationEntries->PushBack((addr_t*)address);
	}
	return B_ERROR;
}

// Relocate
status_t
UserDataWriter::Relocate(void *address)
{
	if (!fRelocationEntries || !fBuffer)
		return B_BAD_VALUE;
	int32 count = fRelocationEntries->Count();
	for (int32 i = 0; i < count; i++) {
		addr_t *entry = fRelocationEntries->ElementAt(i);
		if (*entry)
			*entry += (addr_t)address - (addr_t)fBuffer;
	}
	return B_OK;
}

