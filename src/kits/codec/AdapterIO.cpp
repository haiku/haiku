/*
 * Copyright 2016 Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */


#include <AdapterIO.h>

#include <MediaIO.h>
#include <RWLocker.h>

#include <string.h>

#include "MediaDebug.h"


namespace BCodecKit {


#define TIMEOUT_QUANTA 100000


class RelativePositionIO : public BPositionIO {
public:
	RelativePositionIO(BAdapterIO* owner, BPositionIO* buffer,
		bigtime_t timeout)
		:
		BPositionIO(),
		fOwner(owner),
		fBackPosition(0),
		fStartOffset(0),
		fBuffer(buffer),
		fTimeout(timeout)
	{
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

		fBackPosition = 0;
		fStartOffset = offset;
		return B_OK;
	}

	status_t EvaluatePosition(off_t position, off_t totalSize)
	{
		if (position < 0)
			return B_ERROR;

		if (position < fStartOffset)
			return B_RESOURCE_UNAVAILABLE;

		if (totalSize > 0 && position > totalSize) {
			// This is an endless stream, we don't know
			// how much data will come and when, we could
			// block on that.
			if (IsMutable())
				return B_WOULD_BLOCK;
			else
				return B_ERROR;
		}

		return B_OK;
	}

	status_t WaitForData(off_t position, off_t size)
	{
		off_t bufferSize = 0;
		status_t ret = GetSize(&bufferSize);
		if (ret != B_OK)
			return B_ERROR;

		bigtime_t totalTimeOut = 0;

		while (bufferSize < position + size) {
			// We are not running, no luck to receive
			// more data, let's return and avoid locking.
			if (!fOwner->IsRunning())
				return B_NOT_SUPPORTED;

			if (fTimeout != B_INFINITE_TIMEOUT && totalTimeOut >= fTimeout)
				return B_TIMED_OUT;

			snooze(TIMEOUT_QUANTA);

			totalTimeOut += TIMEOUT_QUANTA;
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

		// We use the backend position to make our buffer
		// independant of that.
		*size = _RelativeToPosition(fBackPosition);

		return B_OK;
	}

	ssize_t BackWrite(const void* buffer, size_t size)
	{
		AutoWriteLocker _(fLock);

		off_t ret = fBuffer->WriteAt(fBackPosition, buffer, size);
		fBackPosition += ret;
		return ret;
	}

	void SetBuffer(BPositionIO* buffer)
	{
		delete fBuffer;
		fBuffer = buffer;
	}

	bool IsStreaming() const
	{
		int32 flags = 0;
		fOwner->GetFlags(&flags);
		return ((flags & B_MEDIA_STREAMING) == B_MEDIA_STREAMING);
	}

	bool IsMutable() const
	{
		int32 flags = 0;
		fOwner->GetFlags(&flags);
		return ((flags & B_MEDIA_MUTABLE_SIZE) == B_MEDIA_MUTABLE_SIZE);
	}

	bool IsSeekable() const
	{
		int32 flags = 0;
		fOwner->GetFlags(&flags);
		return ((flags & B_MEDIA_SEEKABLE) == B_MEDIA_SEEKABLE);
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

	mutable	RWLocker	fLock;

	bigtime_t			fTimeout;
};


BAdapterIO::BAdapterIO(int32 flags, bigtime_t timeout)
	:
	fFlags(flags),
	fBuffer(NULL),
	fTotalSize(0),
	fOpened(false),
	fSeekSem(-1),
	fInputAdapter(NULL)
{
	CALLED();

	fBuffer = new RelativePositionIO(this, new BMallocIO(), timeout);
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

	status_t ret = _EvaluateWait(position, size);
	if (ret != B_OK)
		return ret;

	return fBuffer->ReadAt(position, buffer, size);
}


ssize_t
BAdapterIO::WriteAt(off_t position, const void* buffer, size_t size)
{
	CALLED();

	return fBuffer->WriteAt(position, buffer, size);
}


off_t
BAdapterIO::Seek(off_t position, uint32 seekMode)
{
	CALLED();

	off_t absolutePosition = 0;
	off_t size = 0;

	if (seekMode == SEEK_CUR)
		absolutePosition = Position()+position;
	else if (seekMode == SEEK_END) {
		if (GetSize(&size) != B_OK)
			return B_NOT_SUPPORTED;

		absolutePosition = size-position;
	}

	status_t ret = _EvaluateWait(absolutePosition, 0);

	if (ret == B_RESOURCE_UNAVAILABLE && fBuffer->IsStreaming()
			&& fBuffer->IsSeekable()) {

		fSeekSem = create_sem(0, "BAdapterIO seek sem");

		if (SeekRequested(absolutePosition) != B_OK)
			return B_NOT_SUPPORTED;

		TRACE("BAdapterIO::Seek: Locking on backend seek\n");
		acquire_sem(fSeekSem);
		TRACE("BAdapterIO::Seek: Seek completed!\n");
		fBuffer->ResetStartOffset(absolutePosition);
	} else if (ret != B_OK)
		return B_NOT_SUPPORTED;

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


status_t
BAdapterIO::Open()
{
	CALLED();

	fOpened = true;
	return B_OK;
}


bool
BAdapterIO::IsRunning() const
{
	return fOpened;
}


void
BAdapterIO::SeekCompleted()
{
	CALLED();
	release_sem(fSeekSem);
	delete_sem(fSeekSem);
	fSeekSem = -1;
}


status_t
BAdapterIO::SetBuffer(BPositionIO* buffer)
{
	// We can't change the buffer while we
	// are running.
	if (fOpened)
		return B_ERROR;

	fBuffer->SetBuffer(buffer);
	return B_OK;
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


ssize_t
BAdapterIO::BackWrite(const void* buffer, size_t size)
{
	return fBuffer->BackWrite(buffer, size);
}


status_t
BAdapterIO::_EvaluateWait(off_t pos, off_t size)
{
	CALLED();

	off_t totalSize = 0;
	if (GetSize(&totalSize) != B_OK)
		TRACE("BAdapterIO::ReadAt: Can't get our size!\n");

	TRACE("BAdapterIO::_EvaluateWait TS %" B_PRId64 " P %" B_PRId64
		" S %" B_PRId64 "\n", totalSize, pos, size);

	status_t err = fBuffer->EvaluatePosition(pos, totalSize);

	TRACE("BAdapterIO::_EvaluateWait: %s\n", strerror(err));

	if (err != B_OK && err != B_WOULD_BLOCK)
		return err;

	TRACE("BAdapterIO::_EvaluateWait: waiting for data\n");

	return fBuffer->WaitForData(pos, size);
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


} // namespace BCodecKit
