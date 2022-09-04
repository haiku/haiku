/*
 * Copyright 2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Leorize, leorize+oss@disroot.org
 */


#include <MemoryRingIO.h>

#include <AutoLocker.h>

#include <algorithm>

#include <stdlib.h>
#include <string.h>


class PThreadLocking {
public:
	inline bool Lock(pthread_mutex_t* mutex)
	{
		return pthread_mutex_lock(mutex) == 0;
	}

	inline void Unlock(pthread_mutex_t* mutex)
	{
		pthread_mutex_unlock(mutex);
	}
};


typedef AutoLocker<pthread_mutex_t, PThreadLocking> PThreadAutoLocker;


struct ReadCondition {
	inline bool operator()(BMemoryRingIO &ring) {
		return ring.BytesAvailable() != 0;
	}
};


struct WriteCondition {
	inline bool operator()(BMemoryRingIO &ring) {
		return ring.SpaceAvailable() != 0;
	}
};


#define RING_MASK(x) ((x) & (fBufferSize - 1))


static size_t
next_power_of_two(size_t value)
{
	value--;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
#if SIZE_MAX >= UINT64_MAX
	value |= value >> 32;
#endif
	value++;

	return value;
}


BMemoryRingIO::BMemoryRingIO(size_t size)
	:
	fBuffer(NULL),
	fBufferSize(0),
	fWriteAtNext(0),
	fReadAtNext(0),
	fBufferFull(false),
	fWriteDisabled(false)
{
	// We avoid the use of pthread_mutexattr as it can possibly fail.
	//
	// The only Haiku-specific behavior that we depend on is that
	// PTHREAD_MUTEX_DEFAULT mutexes check for double-locks.
	pthread_mutex_init(&fLock, NULL);
	pthread_cond_init(&fEvent, NULL);

	SetSize(size);
}


BMemoryRingIO::~BMemoryRingIO()
{
	SetSize(0);

	pthread_mutex_destroy(&fLock);
	pthread_cond_destroy(&fEvent);
}


status_t
BMemoryRingIO::InitCheck() const
{
	if (fBufferSize == 0)
		return B_NO_INIT;

	return B_OK;
}


ssize_t
BMemoryRingIO::Read(void* _buffer, size_t size)
{
	if (_buffer == NULL)
		return B_BAD_VALUE;
	if (size == 0)
		return 0;

	PThreadAutoLocker _(fLock);

	if (!fWriteDisabled)
		WaitForRead();

	size = std::min(size, BytesAvailable());
	uint8* buffer = reinterpret_cast<uint8*>(_buffer);
	if (fReadAtNext + size < fBufferSize)
		memcpy(buffer, fBuffer + fReadAtNext, size);
	else {
		const size_t upper = fBufferSize - fReadAtNext;
		const size_t lower = size - upper;
		memcpy(buffer, fBuffer + fReadAtNext, upper);
		memcpy(buffer + upper, fBuffer, lower);
	}
	fReadAtNext = RING_MASK(fReadAtNext + size);
	fBufferFull = false;

	pthread_cond_signal(&fEvent);

	return size;
}


ssize_t
BMemoryRingIO::Write(const void* _buffer, size_t size)
{
	if (_buffer == NULL)
		return B_BAD_VALUE;
	if (size == 0)
		return 0;

	PThreadAutoLocker locker(fLock);

	if (!fWriteDisabled)
		WaitForWrite();

	// We separate this check from WaitForWrite() as the boolean
	// might have been toggled during our wait on the conditional.
	if (fWriteDisabled)
		return B_READ_ONLY_DEVICE;

	const uint8* buffer = reinterpret_cast<const uint8*>(_buffer);
	size = std::min(size, SpaceAvailable());
	if (fWriteAtNext + size < fBufferSize)
		memcpy(fBuffer + fWriteAtNext, buffer, size);
	else {
		const size_t upper = fBufferSize - fWriteAtNext;
		const size_t lower = size - upper;
		memcpy(fBuffer + fWriteAtNext, buffer, size);
		memcpy(fBuffer, buffer + upper, lower);
	}
	fWriteAtNext = RING_MASK(fWriteAtNext + size);
	fBufferFull = fReadAtNext == fWriteAtNext;

	pthread_cond_signal(&fEvent);

	return size;
}


status_t
BMemoryRingIO::SetSize(size_t _size)
{
	PThreadAutoLocker locker(fLock);

	const size_t size = next_power_of_two(_size);

	const size_t availableBytes = BytesAvailable();
	if (size < availableBytes)
		return B_BAD_VALUE;

	if (size == 0) {
		free(fBuffer);
		fBuffer = NULL;
		fBufferSize = 0;
		Clear(); // resets other internal counters
		return B_OK;
	}

	uint8* newBuffer = reinterpret_cast<uint8*>(malloc(size));
	if (newBuffer == NULL)
		return B_NO_MEMORY;

	Read(newBuffer, availableBytes);
	free(fBuffer);

	fBuffer = newBuffer;
	fBufferSize = size;
	fReadAtNext = 0;
	fWriteAtNext = RING_MASK(availableBytes);
	fBufferFull = fBufferSize == availableBytes;

	pthread_cond_signal(&fEvent);

	return B_OK;
}


void
BMemoryRingIO::Clear()
{
	PThreadAutoLocker locker(fLock);

	fReadAtNext = 0;
	fWriteAtNext = 0;
	fBufferFull = false;
}


size_t
BMemoryRingIO::BytesAvailable()
{
	PThreadAutoLocker locker(fLock);

	if (fWriteAtNext == fReadAtNext) {
		if (fBufferFull)
			return fBufferSize;
		return 0;
	}
	return RING_MASK(fWriteAtNext - fReadAtNext);
}


size_t
BMemoryRingIO::SpaceAvailable()
{
	PThreadAutoLocker locker(fLock);

	return fBufferSize - BytesAvailable();
}


size_t
BMemoryRingIO::BufferSize()
{
	PThreadAutoLocker locker(fLock);

	return fBufferSize;
}


template<typename Condition>
status_t
BMemoryRingIO::_WaitForCondition(bigtime_t timeout)
{
	PThreadAutoLocker autoLocker;

	struct timespec absTimeout;
	if (timeout == B_INFINITE_TIMEOUT) {
		autoLocker.SetTo(fLock, false);
	} else {
		memset(&absTimeout, 0, sizeof(absTimeout));
		bigtime_t target = system_time() + timeout;
		absTimeout.tv_sec = target / 100000;
		absTimeout.tv_nsec = (target % 100000) * 1000L;
		int err = pthread_mutex_timedlock(&fLock, &absTimeout);
		if (err == ETIMEDOUT)
			return B_TIMED_OUT;
		if (err != EDEADLK)
			autoLocker.SetTo(fLock, true);
	}

	Condition cond;
	while (!cond(*this)) {
		if (fWriteDisabled)
			return B_READ_ONLY_DEVICE;

		int err = 0;

		if (timeout == B_INFINITE_TIMEOUT)
			err = pthread_cond_wait(&fEvent, &fLock);
		else
			err = pthread_cond_timedwait(&fEvent, &fLock, &absTimeout);

		if (err != 0)
			return err;
	}

	return B_OK;
}


status_t
BMemoryRingIO::WaitForRead(bigtime_t timeout)
{
	return _WaitForCondition<ReadCondition>(timeout);
}


status_t
BMemoryRingIO::WaitForWrite(bigtime_t timeout)
{
	return _WaitForCondition<WriteCondition>(timeout);
}


void
BMemoryRingIO::SetWriteDisabled(bool disabled)
{
	PThreadAutoLocker autoLocker(fLock);

	fWriteDisabled = disabled;

	pthread_cond_broadcast(&fEvent);
}


bool
BMemoryRingIO::WriteDisabled()
{
	PThreadAutoLocker autoLocker(fLock);

	return fWriteDisabled;
}
