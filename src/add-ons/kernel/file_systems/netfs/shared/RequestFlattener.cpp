// RequestFlattener.cpp

#include <ByteOrder.h>

#include "RequestFlattener.h"

// Writer

// constructor
Writer::Writer()
{
}

// destructor
Writer::~Writer()
{
}

// Pad
status_t
Writer::Pad(int32 size)
{
	if (size <= 0)
		return B_OK;

	if (size > 8)
		return B_BAD_VALUE;

	char buffer[8];
	return Write(buffer, size);
}


// DataIOWriter

// constructor
DataIOWriter::DataIOWriter(BDataIO* dataIO)
	: Writer(),
	  fDataIO(dataIO)
{
}

// destructor
DataIOWriter::~DataIOWriter()
{
}

// Write
status_t
DataIOWriter::Write(const void* buffer, int32 size)
{
	ssize_t bytesWritten = fDataIO->Write(buffer, size);
	if (bytesWritten < 0)
		return bytesWritten;
	if (bytesWritten != size)
		return B_ERROR;
	return B_OK;
}


// DummyWriter

// constructor
DummyWriter::DummyWriter()
{
}

// destructor
DummyWriter::~DummyWriter()
{
}

// Write
status_t
DummyWriter::Write(const void* buffer, int32 size)
{
	return B_OK;
}


// RequestFlattener

// constructor
RequestFlattener::RequestFlattener(Writer* writer)
	: RequestMemberVisitor(),
	  fWriter(writer),
	  fStatus(B_OK),
	  fBytesWritten(0)
{
}

// GetStatus
status_t
RequestFlattener::GetStatus() const
{
	return fStatus;
}

// GetBytesWritten
int32
RequestFlattener::GetBytesWritten() const
{
	return fBytesWritten;
}

// Visit
void
RequestFlattener::Visit(RequestMember* member, bool& data)
{
	uint8 netData = (data ? ~0 : 0);
	Write(&netData, 1);
}

// Visit
void
RequestFlattener::Visit(RequestMember* member, int8& data)
{
	Write(&data, 1);
}

// Visit
void
RequestFlattener::Visit(RequestMember* member, uint8& data)
{
	Write(&data, 1);
}

// Visit
void
RequestFlattener::Visit(RequestMember* member, int16& data)
{
	int16 netData = B_HOST_TO_BENDIAN_INT16(data);
	Write(&netData, 2);
}

// Visit
void
RequestFlattener::Visit(RequestMember* member, uint16& data)
{
	uint16 netData = B_HOST_TO_BENDIAN_INT16(data);
	Write(&netData, 2);
}

// Visit
void
RequestFlattener::Visit(RequestMember* member, int32& data)
{
	int32 netData = B_HOST_TO_BENDIAN_INT32(data);
	Write(&netData, 4);
}

// Visit
void
RequestFlattener::Visit(RequestMember* member, uint32& data)
{
	uint32 netData = B_HOST_TO_BENDIAN_INT32(data);
	Write(&netData, 4);
}

// Visit
void
RequestFlattener::Visit(RequestMember* member, int64& data)
{
	int64 netData = B_HOST_TO_BENDIAN_INT64(data);
	Write(&netData, 8);
}

// Visit
void
RequestFlattener::Visit(RequestMember* member, uint64& data)
{
	uint64 netData = B_HOST_TO_BENDIAN_INT64(data);
	Write(&netData, 8);
}

// Visit
void
RequestFlattener::Visit(RequestMember* member, Data& data)
{
	WriteData(data.address, data.size);
}

// Visit
void
RequestFlattener::Visit(RequestMember* member, StringData& data)
{
	WriteData(data.address, data.size);
}

// Visit
void
RequestFlattener::Visit(RequestMember* member, RequestMember& subMember)
{
	subMember.ShowAround(this);
}

// Visit
void
RequestFlattener::Visit(RequestMember* member,
	FlattenableRequestMember& subMember)
{
	if (fStatus != B_OK)
		return;

	status_t status = subMember.Flatten(this);
	if (fStatus == B_OK)
		fStatus = status;
}


// Write
status_t
RequestFlattener::Write(const void* buffer, int32 size)
{
	if (fStatus != B_OK)
		return fStatus;

	fStatus = fWriter->Write(buffer, size);
	if (fStatus == B_OK)
		fBytesWritten += size;

	return fStatus;
}

// Align
status_t
RequestFlattener::Align(int32 align)
{
	if (fStatus != B_OK)
		return fStatus;

	if (align > 1) {
		int32 newBytesWritten = fBytesWritten;
		if (!(align & 0x3))
			newBytesWritten = (fBytesWritten + 3) & ~0x3;
		else if (!(align & 0x1))
			newBytesWritten = (fBytesWritten + 1) & ~0x1;

		if (newBytesWritten > fBytesWritten) {
			fStatus = fWriter->Pad(newBytesWritten - fBytesWritten);
			if (fStatus == B_OK)
				fBytesWritten = newBytesWritten;
		}
	}

	return fStatus;
}

// WriteBool
status_t
RequestFlattener::WriteBool(bool data)
{
	return Write(&data, 1);
}

// WriteInt32
status_t
RequestFlattener::WriteInt32(int32 data)
{
	int32 netData = B_HOST_TO_BENDIAN_INT32(data);
	return Write(&netData, 4);
}

// WriteData
status_t
RequestFlattener::WriteData(const void* buffer, int32 size)
{
	if ((!buffer && size > 0) || size < 0)
		return B_BAD_VALUE;

	int32 netData = B_HOST_TO_BENDIAN_INT32(size);
	Write(&netData, 4);
	return Write(buffer, size);
}

// WriteString
status_t
RequestFlattener::WriteString(const char* string)
{
	int32 size = (string ? strlen(string) + 1 : 0);
	return WriteData(string, size);
}

