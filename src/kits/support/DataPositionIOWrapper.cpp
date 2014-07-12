/*
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <DataPositionIOWrapper.h>

#include <stdio.h>


BDataPositionIOWrapper::BDataPositionIOWrapper(BDataIO* io)
	:
	BPositionIO(),
	fIO(io),
	fPosition(0)
{
}


BDataPositionIOWrapper::~BDataPositionIOWrapper()
{
}


ssize_t
BDataPositionIOWrapper::Read(void* buffer, size_t size)
{
	ssize_t bytesRead = fIO->Read(buffer, size);
	if (bytesRead > 0)
		fPosition += bytesRead;

	return bytesRead;
}


ssize_t
BDataPositionIOWrapper::Write(const void* buffer, size_t size)
{
	ssize_t bytesWritten = fIO->Write(buffer, size);
	if (bytesWritten > 0)
		fPosition += bytesWritten;

	return bytesWritten;
}


ssize_t
BDataPositionIOWrapper::ReadAt(off_t position, void* buffer, size_t size)
{
	if (position != fPosition)
		return B_NOT_SUPPORTED;

	return Read(buffer, size);
}


ssize_t
BDataPositionIOWrapper::WriteAt(off_t position, const void* buffer,
	size_t size)
{
	if (position != fPosition)
		return B_NOT_SUPPORTED;

	return Write(buffer, size);
}


off_t
BDataPositionIOWrapper::Seek(off_t position, uint32 seekMode)
{
	switch (seekMode) {
		case SEEK_CUR:
			return position == 0 ? B_OK : B_NOT_SUPPORTED;
		case SEEK_SET:
			return position == fPosition ? B_OK : B_NOT_SUPPORTED;
		case SEEK_END:
			return B_NOT_SUPPORTED;
		default:
			return B_BAD_VALUE;
	}
}


off_t
BDataPositionIOWrapper::Position() const
{
	return fPosition;
}


status_t
BDataPositionIOWrapper::SetSize(off_t size)
{
	return size == fPosition ? B_OK : B_NOT_SUPPORTED;
}


status_t
BDataPositionIOWrapper::GetSize(off_t* size) const
{
	return B_NOT_SUPPORTED;
}
