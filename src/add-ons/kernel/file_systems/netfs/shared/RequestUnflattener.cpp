// RequestUnflattener.cpp

#include <stdlib.h>

#include <ByteOrder.h>

#include "Compatibility.h"
#include "DebugSupport.h"
#include "RequestUnflattener.h"

const int32 kMaxSaneStringSize	= 4096;			// 4 KB
const int32 kMaxSaneDataSize	= 128 * 1024;	// 128 KB

// constructor
Reader::Reader()
{
}

// destructor
Reader::~Reader()
{
}

// Read
status_t
Reader::Read(int32 size, void** buffer, bool* mustFree)
{
	// check params
	if (size < 0 || !buffer || !mustFree)
		return B_BAD_VALUE;

	// deal with size == 0
	if (size == 0) {
		*buffer = NULL;
		*mustFree = false;
		return B_OK;
	}

	// allocate the buffer and read
	*buffer = malloc(size);
	if (!*buffer)
		return B_NO_MEMORY;
	status_t error = Read(*buffer, size);
	if (error != B_OK) {
		free(*buffer);
		return error;
	}
	return error;
}

// Skip
status_t
Reader::Skip(int32 size)
{
	if (size <= 0)
		return B_OK;

	if (size > 8)
		return B_BAD_VALUE;

	char buffer[8];
	return Read(buffer, size);
}



// RequestUnflattener

// constructor
RequestUnflattener::RequestUnflattener(Reader* reader)
	: RequestMemberVisitor(),
	  fReader(reader),
	  fStatus(B_OK),
	  fBytesRead(0)
{
}

// GetStatus
status_t
RequestUnflattener::GetStatus() const
{
	return fStatus;
}

// GetBytesRead
int32
RequestUnflattener::GetBytesRead() const
{
	return fBytesRead;
}

// Visit
void
RequestUnflattener::Visit(RequestMember* member, bool& data)
{
	uint8 netData;
	if (Read(&netData, 1) == B_OK)
		data = netData;
}

// Visit
void
RequestUnflattener::Visit(RequestMember* member, int8& data)
{
	Read(&data, 1);
}

// Visit
void
RequestUnflattener::Visit(RequestMember* member, uint8& data)
{
	Read(&data, 1);
}

// Visit
void
RequestUnflattener::Visit(RequestMember* member, int16& data)
{
	if (Read(&data, 2) == B_OK)
		data = B_BENDIAN_TO_HOST_INT16(data);
}

// Visit
void
RequestUnflattener::Visit(RequestMember* member, uint16& data)
{
	if (Read(&data, 2) == B_OK)
		data = B_BENDIAN_TO_HOST_INT16(data);
}

// Visit
void
RequestUnflattener::Visit(RequestMember* member, int32& data)
{
	if (Read(&data, 4) == B_OK)
		data = B_BENDIAN_TO_HOST_INT32(data);
}

// Visit
void
RequestUnflattener::Visit(RequestMember* member, uint32& data)
{
	if (Read(&data, 4) == B_OK)
		data = B_BENDIAN_TO_HOST_INT32(data);
}

// Visit
void
RequestUnflattener::Visit(RequestMember* member, int64& data)
{
	if (Read(&data, 8) == B_OK)
		data = B_BENDIAN_TO_HOST_INT64(data);
}

// Visit
void
RequestUnflattener::Visit(RequestMember* member, uint64& data)
{
	if (Read(&data, 8) == B_OK)
		data = B_BENDIAN_TO_HOST_INT64(data);
}

// Visit
void
RequestUnflattener::Visit(RequestMember* member, Data& data)
{
	void* buffer;
	int32 size;
	bool mustFree;
	if (ReadData(buffer, size, mustFree) != B_OK)
		return;

	// we can't deal with mustFree buffers currently
	if (mustFree) {
		free(buffer);
		fStatus = B_ERROR;
		return;
	}

	data.address = buffer;
	data.size = size;
}

// Visit
void
RequestUnflattener::Visit(RequestMember* member, StringData& data)
{
	void* buffer;
	int32 size;
	bool mustFree;
	if (ReadData(buffer, size, mustFree) != B_OK)
		return;

	// we can't deal with mustFree buffers currently
	if (mustFree) {
		free(buffer);
		fStatus = B_ERROR;
		return;
	}

	data.address = buffer;
	data.size = size;
// TODO: Check null termination.
}

// Visit
void
RequestUnflattener::Visit(RequestMember* member, RequestMember& subMember)
{
	subMember.ShowAround(this);
}

// Visit
void
RequestUnflattener::Visit(RequestMember* member,
	FlattenableRequestMember& subMember)
{
	if (fStatus != B_OK)
		return;

	status_t status = subMember.Unflatten(this);
	if (fStatus == B_OK)
		fStatus = status;
}

// Read
status_t
RequestUnflattener::Read(void* buffer, int32 size)
{
	if (fStatus != B_OK)
		return fStatus;

	fStatus = fReader->Read(buffer, size);
	if (fStatus == B_OK)
		fBytesRead += size;

	return fStatus;
}

// Read
status_t
RequestUnflattener::Read(int32 size, void*& buffer, bool& mustFree)
{
	if (fStatus != B_OK)
		return fStatus;

	fStatus = fReader->Read(size, &buffer, &mustFree);
	if (fStatus == B_OK)
		fBytesRead += size;

	return fStatus;
}


// Align
status_t
RequestUnflattener::Align(int32 align)
{
	if (fStatus != B_OK)
		return fStatus;

	if (align > 1) {
		int32 newBytesRead = fBytesRead;
		if (!(align & 0x3))
			newBytesRead = (fBytesRead + 3) & ~0x3;
		else if (!(align & 0x1))
			newBytesRead = (fBytesRead + 1) & ~0x1;

		if (newBytesRead > fBytesRead) {
			fStatus = fReader->Skip(newBytesRead - fBytesRead);
			if (fStatus == B_OK)
				fBytesRead = newBytesRead;
		}
	}

	return fStatus;
}

// ReadBool
status_t
RequestUnflattener::ReadBool(bool& data)
{
	return Read(&data, 1);
}

// ReadInt32
status_t
RequestUnflattener::ReadInt32(int32& data)
{
	if (Read(&data, 4) == B_OK)
		data = B_BENDIAN_TO_HOST_INT32(data);

	return fStatus;
}


// ReadData
status_t
RequestUnflattener::ReadData(void*& buffer, int32& _size, bool& mustFree)
{
	if (fStatus != B_OK)
		return fStatus;

	// read size
	int32 size;
	if (ReadInt32(size) != B_OK)
		return fStatus;

	// check size for sanity
	if (size < 0 || size > kMaxSaneDataSize) {
		fStatus = B_BAD_DATA;
		return fStatus;
	}

	// read data
	if (size > 0) {
		if (Read(size, buffer, mustFree) != B_OK)
			return fStatus;
	} else {
		buffer = NULL;
		mustFree = false;
	}

	_size = size;
	return fStatus;
}

// ReadString
status_t
RequestUnflattener::ReadString(HashString& string)
{
	void* buffer;
	int32 size;
	bool mustFree;
	if (ReadData(buffer, size, mustFree) == B_OK) {
		if (!string.SetTo((const char*)buffer))
			fStatus = B_NO_MEMORY;

		if (mustFree)
			free(buffer);
	}

	return fStatus;
}

