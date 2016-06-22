/*
 * Copyright 2016 Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */

#include "AdapterIO.h"

#include <MediaIO.h>

#include <stdio.h>

#include "debug.h"


class RelativePositionIO : public BPositionIO
{
public:
	RelativePositionIO(BAdapterIO* owner, BPositionIO* buffer)
		:
		BPositionIO(),
		fOwner(owner),
		fBackPosition(0),
		fStartOffset(0),
		fBuffer(buffer)
	{
		fOwner->GetFlags(&fFlags);
	}

	virtual	~RelativePositionIO()
	{
		delete fBuffer;
	}

	status_t ResetStartOffset(off_t offset)
	{
		status_t ret = fBuffer->SetSize(0);
		if (ret != B_OK)
			return ret;

		fBackPosition = offset;
		fStartOffset = offset;
		return B_OK;
	}

	status_t EvaluatePosition(off_t position)
	{
		if (position < 0)
			return B_ERROR;

		if (position < fStartOffset)
			return B_RESOURCE_UNAVAILABLE;

		off_t size = 0;
		if (fOwner->GetSize(&size) != B_OK)
			return B_ERROR;

		if (position > size) {
			// This is an endless stream, we don't know
			// how much data will come and when, we could
			// block on that.
			if (IsMutable())
				return B_WOULD_BLOCK;
			else
				return B_ERROR;
		}

		off_t bufSize = 0;
		if (GetSize(&bufSize) != B_OK)
			return B_ERROR;

		if (position > bufSize)
			return B_RESOURCE_UNAVAILABLE;

		return B_OK;
	}

	status_t WaitForData(off_t position)
	{
		off_t bufferSize = 0;
		status_t ret = GetSize(&bufferSize);
		if (ret != B_OK)
			return B_ERROR;

		while(bufferSize < position) {
			snooze(100000);
			GetSize(&bufferSize);
		}
		return B_OK;
	}

	virtual	ssize_t	ReadAt(off_t position, void* buffer,
		size_t size)
	{
		AutoReadLocker _(fLock);

		return fBuffer->ReadAt(
			_PositionToRelative(position), buffer, size);

	}

	virtual	ssize_t	WriteAt(off_t position,
		const void* buffer, size_t size)
	{
		AutoWriteLocker _(fLock);

		return fBuffer->WriteAt(
			_PositionToRelative(position), buffer, size);
	}

	virtual	off_t Seek(off_t position, uint32 seekMode)
	{
		AutoWriteLocker _(fLock);

		return fBuffer->Seek(_PositionToRelative(position), seekMode);
	}

	virtual off_t Position() const
	{
		AutoReadLocker _(fLock);

		return _RelativeToPosition(fBuffer->Position());
	}

	virtual	status_t SetSize(off_t size)
	{
		AutoWriteLocker _(fLock);

		return fBuffer->SetSize(_PositionToRelative(size));
	}

	virtual	status_t GetSize(off_t* size) const
	{
		AutoReadLocker _(fLock);

		off_t bufferSize;
		status_t ret = fBuffer->GetSize(&bufferSize);
		if (ret == B_OK)
			*size = _RelativeToPosition(bufferSize);

		return ret;
	}

	ssize_t BackWrite(const void* buffer, size_t size)
	{
		AutoWriteLocker _(fLock);

		off_t currentPos = Position();
		off_t ret = fBuffer->WriteAt(fBackPosition, buffer, size);
		fBackPosition += ret;
		return fBuffer->Seek(currentPos, SEEK_SET);
	}

	bool IsStreaming() const
	{
		return (fFlags & B_MEDIA_STREAMING) == true;
	}

	bool IsMutable() const
	{
		return (fFlags & B_MEDIA_MUTABLE_SIZE) == true;
	}

	bool IsSeekable() const
	{
		return (fFlags & B_MEDIA_SEEKABLE) == true;
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

	BAdapterIO*			fOwner;
	off_t				fBackPosition;
	off_t				fStartOffset;

	BPositionIO*		fBuffer;
	int32				fFlags;

	mutable	RWLocker	fLock;
};


BAdapterIO::BAdapterIO(int32 flags, bigtime_t timeout)
	:
	fFlags(flags),
	fTimeout(timeout),
	fBuffer(NULL),
	fInputAdapter(NULL)
{
	CALLED();

	fBuffer = new RelativePositionIO(this, new BMallocIO());
}


BAdapterIO::BAdapterIO(const BAdapterIO &)
{
	// copying not allowed...
}


BAdapterIO::~BAdapterIO()
{
	CALLED();

	delete fInputAdapter;
	delete fBuffer;
}


void
BAdapterIO::GetFlags(int32* flags) const
{
	CALLED();

	*flags = fFlags;
}


ssize_t
BAdapterIO::ReadAt(off_t position, void* buffer, size_t size)
{
	CALLED();

	printf("read at %d  %d \n", (int)position, (int)size);
	status_t ret = _EvaluateWait(position+size);
	if (ret != B_OK)
		return ret;

	return fBuffer->ReadAt(position, buffer, size);
}


ssize_t
BAdapterIO::WriteAt(off_t position, const void* buffer, size_t size)
{
	CALLED();

	status_t ret = _EvaluateWait(position+size);
	if (ret != B_OK)
		return ret;

	return fBuffer->WriteAt(position, buffer, size);
}


off_t
BAdapterIO::Seek(off_t position, uint32 seekMode)
{
	CALLED();

	status_t ret = _EvaluateWait(position);
	if (ret != B_OK)
		return ret;

	return fBuffer->Seek(position, seekMode);
}


off_t
BAdapterIO::Position() const
{
	CALLED();

	return fBuffer->Position();
}


status_t
BAdapterIO::SetSize(off_t size)
{
	CALLED();

	if (!fBuffer->IsMutable()) {
		fTotalSize = size;
		return B_OK;
	}

	return fBuffer->SetSize(size);
}


status_t
BAdapterIO::GetSize(off_t* size) const
{
	CALLED();

	if (!fBuffer->IsMutable()) {
		*size = fTotalSize;
		return B_OK;
	}

	return fBuffer->GetSize(size);
}


ssize_t
BAdapterIO::BackWrite(const void* buffer, size_t size)
{
	CALLED();

	return fBuffer->BackWrite(buffer, size);
}


status_t
BAdapterIO::_EvaluateWait(off_t pos)
{
	CALLED();

	status_t err = fBuffer->EvaluatePosition(pos);

	if (err != B_RESOURCE_UNAVAILABLE && err != B_OK)
		return B_UNSUPPORTED;

	if (err == B_RESOURCE_UNAVAILABLE && fBuffer->IsStreaming()
			&& fBuffer->IsSeekable()) {
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


status_t
BAdapterIO::SeekRequested(off_t position)
{
	CALLED();

	return B_ERROR;
}


status_t
BAdapterIO::SeekCompleted(off_t position)
{
	CALLED();

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
