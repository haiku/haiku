/*
 * Copyright 2016 Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */

#include "AdapterIO.h"

#include <MediaIO.h>

#include <stdio.h>


class RelativePositionIO : public BPositionIO
{
public:
	RelativePositionIO(BPositionIO* buffer)
		:
		BPositionIO(),
		fStartOffset(0),
		fTotalSize(0),
		fBuffer(buffer)
		{}

	virtual	~RelativePositionIO()
	{
		delete fBuffer;
	}

	void SetTotalSize(size_t size)
	{
		fTotalSize = size;
	}

	size_t TotalSize() const
	{
		return fTotalSize;
	}

	status_t ResetStartOffset(off_t offset)
	{
		status_t ret = fBuffer->SetSize(0);
		if (ret == B_OK)
			fStartOffset = offset;

		return ret;
	}

	status_t EvaluatePosition(off_t position)
	{
		if (position < 0)
			return B_ERROR;

		if (position < fStartOffset)
			return B_RESOURCE_UNAVAILABLE;

		// This is an endless stream, we don't know
		// how much data will come and when, we could
		// block on that.
		if (fTotalSize == 0)
			return B_WOULD_BLOCK;

		if (position >= fTotalSize)
			return B_ERROR;

		off_t size = 0;
		fBuffer->GetSize(&size);
		if (position >= size)
			return B_RESOURCE_UNAVAILABLE;

		return B_OK;
	}

	status_t WaitForData(off_t position)
	{
		off_t bufferSize = 0;
		position = _PositionToRelative(position);

		status_t ret = fBuffer->GetSize(&bufferSize);
		if (ret != B_OK)
			return B_ERROR;

		while(bufferSize < position) {
			snooze(100000);
			fBuffer->GetSize(&bufferSize);
		}
		return B_OK;
	}

	virtual	ssize_t	ReadAt(off_t position, void* buffer,
		size_t size)
	{
		return fBuffer->ReadAt(
			_PositionToRelative(position), buffer, size);

	}

	virtual	ssize_t	WriteAt(off_t position,
		const void* buffer, size_t size)
	{
		return fBuffer->WriteAt(
			_PositionToRelative(position), buffer, size);
	}

	virtual	off_t Seek(off_t position, uint32 seekMode)
	{
		return fBuffer->Seek(_PositionToRelative(position), seekMode);
	}

	virtual off_t Position() const
	{
		return _RelativeToPosition(fBuffer->Position());
	}

	virtual	status_t SetSize(off_t size)
	{
		return fBuffer->SetSize(_PositionToRelative(size));
	}

	virtual	status_t GetSize(off_t* size) const
	{
		if (fTotalSize > 0) {
			*size = fTotalSize;
			return B_OK;
		}

		off_t bufferSize;
		status_t ret = fBuffer->GetSize(&bufferSize);
		if (ret == B_OK)
			*size = _RelativeToPosition(bufferSize);

		return ret;
	}

private:

	off_t _PositionToRelative(off_t position) const
	{
		return position - fStartOffset;
	}

	off_t _RelativeToPosition(off_t position) const
	{
		return position + fStartOffset;
	}

	off_t			fStartOffset;
	off_t			fTotalSize;

	BPositionIO*	fBuffer;
};


BAdapterIO::BAdapterIO(int32 flags, bigtime_t timeout)
	:
	fFlags(flags),
	fTimeout(timeout),
	fBackPosition(0),
	fBuffer(NULL),
	fInputAdapter(NULL)
{
	fBuffer = new RelativePositionIO(new BMallocIO());
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
	status_t ret = _EvaluateWait(position+size);
	if (ret != B_OK)
		return ret;

	AutoReadLocker _(fLock);

	return fBuffer->ReadAt(position, buffer, size);
}


ssize_t
BAdapterIO::WriteAt(off_t position, const void* buffer, size_t size)
{
	status_t ret = _EvaluateWait(position+size);
	if (ret != B_OK)
		return ret;

	AutoWriteLocker _(fLock);

	return fBuffer->WriteAt(position, buffer, size);
}


off_t
BAdapterIO::Seek(off_t position, uint32 seekMode)
{
	status_t ret = _EvaluateWait(position);
	if (ret != B_OK)
		return ret;

	AutoWriteLocker _(fLock);

	return fBuffer->Seek(position, seekMode);
}


off_t
BAdapterIO::Position() const
{
	AutoReadLocker _(fLock);

	return fBuffer->Position();
}


status_t
BAdapterIO::SetSize(off_t size)
{
	AutoWriteLocker _(fLock);

	return fBuffer->SetSize(size);
}


status_t
BAdapterIO::GetSize(off_t* size) const
{
	AutoReadLocker _(fLock);

	return fBuffer->GetSize(size);
}


status_t
BAdapterIO::_EvaluateWait(off_t pos)
{
	status_t err = fBuffer->EvaluatePosition(pos);
	if (err == B_ERROR && err != B_WOULD_BLOCK)
		return B_ERROR;
	else if (err == B_RESOURCE_UNAVAILABLE) {
		if (SeekRequested(pos) != B_OK)
			return B_UNSUPPORTED;
	}

	return fBuffer->WaitForData(pos);
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
	AutoWriteLocker _(fLock);

	off_t currentPos = Position();
	off_t ret = fBuffer->WriteAt(fBackPosition, buffer, size);
	fBackPosition += ret;
	return fBuffer->Seek(currentPos, SEEK_SET);
}


status_t
BAdapterIO::SeekRequested(off_t position)
{
	return B_ERROR;
}


status_t
BAdapterIO::SeekCompleted(off_t position)
{
	return fBuffer->ResetStartOffset(position);
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
