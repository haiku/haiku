/*
 * Copyright 2016 Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */

#include "AdapterIO.h"

#include <MediaIO.h>

#include <stdio.h>


BAdapterIO::BAdapterIO(int32 flags, bigtime_t timeout)
	:
	fFlags(flags),
	fTimeout(timeout),
	fBackPosition(0),
	fBuffer(NULL),
	fInputAdapter(NULL)
{
	fBuffer = new BMallocIO();
}


BAdapterIO::BAdapterIO(const BAdapterIO &)
{
	// copying not allowed...
}


BAdapterIO::~BAdapterIO()
{
	delete fInputAdapter;
	delete fBuffer;
}


void
BAdapterIO::GetFlags(int32* flags) const
{
	*flags = fFlags;
}


ssize_t
BAdapterIO::ReadAt(off_t position, void* buffer, size_t size)
{
	printf("read at %d  %d \n", (int)position, (int)size);
	_WaitForData(position+size);
	AutoReadLocker(fLock);

	return fBuffer->ReadAt(position, buffer, size);
}


ssize_t
BAdapterIO::WriteAt(off_t position, const void* buffer, size_t size)
{
	_WaitForData(position+size);
	AutoWriteLocker(fLock);

	return fBuffer->WriteAt(position, buffer, size);
}


off_t
BAdapterIO::Seek(off_t position, uint32 seekMode)
{
	_WaitForData(position);
	AutoWriteLocker(fLock);

	return fBuffer->Seek(position, seekMode);
}


off_t
BAdapterIO::Position() const
{
	AutoReadLocker(fLock);

	return fBuffer->Position();
}


status_t
BAdapterIO::SetSize(off_t size)
{
	AutoWriteLocker(fLock);

	return fBuffer->SetSize(size);
}


status_t
BAdapterIO::GetSize(off_t* size) const
{
	AutoReadLocker(fLock);

	return fBuffer->GetSize(size);
}


BInputAdapter*
BAdapterIO::BuildInputAdapter()
{
	if (fInputAdapter != NULL)
		return fInputAdapter;

	fInputAdapter = new BInputAdapter(this);
	return fInputAdapter;
}


ssize_t
BAdapterIO::BackWrite(const void* buffer, size_t size)
{
	AutoWriteLocker(fLock);

	off_t currentPos = Position();
	off_t ret = fBuffer->WriteAt(fBackPosition, buffer, size);
	fBackPosition += ret;
	return fBuffer->Seek(currentPos, SEEK_SET);
}


void
BAdapterIO::_WaitForData(size_t size)
{
	off_t bufferSize = 0;

	status_t ret = GetSize(&bufferSize);
	if (ret != B_OK)
		return;

	while((size_t)bufferSize < size) {
		GetSize(&bufferSize);
		snooze(100000);
	}
}


BInputAdapter::BInputAdapter(BAdapterIO* io)
	:
	fIO(io)
{
}


BInputAdapter::~BInputAdapter()
{
}


ssize_t
BInputAdapter::Write(const void* buffer, size_t size)
{
	return fIO->BackWrite(buffer, size);
}


// FBC
void BAdapterIO::_ReservedAdapterIO1() {}
void BAdapterIO::_ReservedAdapterIO2() {}
void BAdapterIO::_ReservedAdapterIO3() {}
void BAdapterIO::_ReservedAdapterIO4() {}
void BAdapterIO::_ReservedAdapterIO5() {}

void BInputAdapter::_ReservedInputAdapter1() {}
void BInputAdapter::_ReservedInputAdapter2() {}
