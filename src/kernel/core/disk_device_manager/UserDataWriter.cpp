// UserDataWriter.cpp

#include <ddm_userland_interface.h>

#include "UserDataWriter.h"

// constructor
UserDataWriter::UserDataWriter()
	: fBuffer(NULL),
	  fBufferSize(0),
	  fAllocatedSize(0)
{
}

// constructor
UserDataWriter::UserDataWriter(user_disk_device_data *buffer,
							   size_t bufferSize)
	: fBuffer(buffer),
	  fBufferSize(bufferSize),
	  fAllocatedSize(0)
{
}

// destructor
UserDataWriter::~UserDataWriter()
{
}

// AllocateData
void *
UserDataWriter::AllocateData(size_t size, size_t align = 1)
{
	if (size == 0)
		return NULL;
	if (align > 1)
		fAllocatedSize = (fAllocatedSize + align - 1) / align * align;
	void *result = NULL;
	if (fAllocatedSize + size <= fBufferSize)
		result = (uint8*)fBuffer + fAllocatedSize;
	fAllocatedSize += size;
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

