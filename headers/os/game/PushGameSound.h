/*
 *  Copyright 2020, Haiku, Inc. All Rights Reserved.
 *  Distributed under the terms of the MIT License.
 */
#ifndef _PUSHGAMESOUND_H
#define _PUSHGAMESOUND_H


#include <StreamingGameSound.h>


class BList;

class BPushGameSound : public BStreamingGameSound {
public:
						BPushGameSound(size_t inBufferFrameCount,
							const gs_audio_format* format,
							size_t inBufferCount = 2,
							BGameSoundDevice* device = NULL);
	virtual				~BPushGameSound();

	enum lock_status {
		lock_failed = -1,
		lock_ok = 0,
		lock_ok_frames_dropped
	};

	virtual	lock_status	LockNextPage(void** _pagePtr, size_t* _pageSize);
	virtual	status_t	UnlockPage(void* pagePtr);

	virtual	lock_status	LockForCyclic(void** _basePtr, size_t* _size);
	virtual	status_t	UnlockCyclic();
	virtual	size_t		CurrentPosition();

	virtual	BGameSound*	Clone() const;

	virtual	status_t	Perform(int32 selector, void* data);

protected:
						BPushGameSound(BGameSoundDevice* device);

	virtual	status_t	SetParameters(size_t bufferFrameCount,
							const gs_audio_format* format, size_t bufferCount);

	virtual	status_t	SetStreamHook(void (*hook)(void* inCookie,
								void* buffer, size_t byteCount,
								BStreamingGameSound* me),
							void* cookie);

	virtual	void		FillBuffer(void* buffer, size_t byteCount);

private:
						BPushGameSound();
						BPushGameSound(const BPushGameSound& other);
	BPushGameSound&		operator=(const BPushGameSound& other);

			bool		BytesReady(size_t* bytes);

	virtual	status_t	_Reserved_BPushGameSound_0(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_1(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_2(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_3(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_4(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_5(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_6(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_7(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_8(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_9(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_10(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_11(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_12(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_13(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_14(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_15(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_16(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_17(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_18(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_19(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_20(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_21(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_22(int32 arg, ...);
	virtual	status_t	_Reserved_BPushGameSound_23(int32 arg, ...);

private:
			sem_id		fLock;
			BList*		fPageLocked;
			size_t		fLockPos;

			size_t		fPlayPos;
			char*		fBuffer;

			size_t		fPageSize;
			int32		fPageCount;
			size_t		fBufferSize;

			uint32		_reserved[12];
};

#endif