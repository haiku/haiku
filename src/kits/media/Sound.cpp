/*
 * Copyright 2009-2019, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jacob Secunda
 *		Marcus Overhagen
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include <Sound.h>

#include <new>
#include <string.h>

#include <File.h>
#include <MediaDebug.h>

#include "TrackReader.h"


BSound::BSound(void* data, size_t size, const media_raw_audio_format& format,
	bool freeWhenDone)
	:	fData(data),
		fDataSize(size),
		fFile(NULL),
		fRefCount(1),
		fStatus(B_NO_INIT),
		fFormat(format),
		fFreeWhenDone(freeWhenDone),
		fTrackReader(NULL)
{
	if (fData == NULL)
		return;

	fStatus = B_OK;
}


BSound::BSound(const entry_ref* soundFile, bool loadIntoMemory)
	:	fData(NULL),
		fDataSize(0),
		fFile(new(std::nothrow) BFile(soundFile, B_READ_ONLY)),
		fRefCount(1),
		fStatus(B_NO_INIT),
		fFreeWhenDone(false),
		fTrackReader(NULL)
{
	if (fFile == NULL) {
		fStatus = B_NO_MEMORY;
		return;
	}

	fStatus = fFile->InitCheck();
	if (fStatus != B_OK)
		return;

	memset(&fFormat, 0, sizeof(fFormat));
	fTrackReader = new(std::nothrow) BPrivate::BTrackReader(fFile, fFormat);
	if (fTrackReader == NULL) {
		fStatus = B_NO_MEMORY;
		return;
	}

	fStatus = fTrackReader->InitCheck();
	if (fStatus != B_OK)
		return;

	fFormat = fTrackReader->Format();
	fStatus = B_OK;
}


BSound::BSound(const media_raw_audio_format& format)
	:	fData(NULL),
		fDataSize(0),
		fFile(NULL),
		fRefCount(1),
		fStatus(B_ERROR),
		fFormat(format),
		fFreeWhenDone(false),
		fTrackReader(NULL)
{
	// unimplemented protected constructor
	UNIMPLEMENTED();
}


BSound::~BSound()
{
	delete fTrackReader;
	delete fFile;

	if (fFreeWhenDone)
		free(fData);
}


status_t
BSound::InitCheck()
{
	return fStatus;
}


BSound*
BSound::AcquireRef()
{
	atomic_add(&fRefCount, 1);
	return this;
}


bool
BSound::ReleaseRef()
{
	if (atomic_add(&fRefCount, -1) == 1) {
		delete this;
		return false;
	}

	// TODO: verify those returns
	return true;
}


int32
BSound::RefCount() const
{
	return fRefCount;
}


bigtime_t
BSound::Duration() const
{
	float frameRate = fFormat.frame_rate;

	if (frameRate == 0.0)
		return 0;

	uint32 bytesPerSample = fFormat.format &
		media_raw_audio_format::B_AUDIO_SIZE_MASK;
	int64 frameCount = Size() / (fFormat.channel_count * bytesPerSample);

	return (bigtime_t)ceil((1000000LL * frameCount) / frameRate);
}


const media_raw_audio_format&
BSound::Format() const
{
	return fFormat;
}


const void*
BSound::Data() const
{
	return fData;
}


off_t
BSound::Size() const
{
	if (fFile != NULL) {
		off_t result = 0;
		fFile->GetSize(&result);
		return result;
	}

	return fDataSize;
}


bool
BSound::GetDataAt(off_t offset, void* intoBuffer, size_t bufferSize,
	size_t* outUsed)
{
	if (intoBuffer == NULL)
		return false;

	if (fData != NULL) {
		size_t copySize = MIN(bufferSize, fDataSize - offset);
		memcpy(intoBuffer, (uint8*)fData + offset, copySize);
		if (outUsed != NULL)
			*outUsed = copySize;
		return true;
	}

	if (fTrackReader != NULL) {
		int32 frameSize = fTrackReader->FrameSize();
		int64 frameCount = fTrackReader->CountFrames();
		int64 startFrame = offset / frameSize;
		if (startFrame > frameCount)
			return false;

		if (fTrackReader->SeekToFrame(&startFrame) != B_OK)
			return false;

		off_t bufferOffset = offset - startFrame * frameSize;
		int64 directStartFrame = (offset + frameSize - 1) / frameSize;
		int64 directFrameCount = (offset + bufferSize - directStartFrame
			* frameSize) / frameSize;

		if (bufferOffset != 0) {
			int64 indirectFrameCount = directStartFrame - startFrame;
			size_t indirectSize = indirectFrameCount * frameSize;
			void* buffer = malloc(indirectSize);
			if (buffer == NULL)
				return false;

			if (fTrackReader->ReadFrames(buffer, indirectFrameCount) != B_OK) {
				free(buffer);
				return false;
			}

			memcpy(intoBuffer, (uint8*)buffer + bufferOffset,
				indirectSize - bufferOffset);
			if (outUsed != NULL)
				*outUsed = indirectSize - bufferOffset;

			free(buffer);
		} else if (outUsed != NULL)
			*outUsed = 0;

		if (fTrackReader->ReadFrames((uint8*)intoBuffer + bufferOffset,
			directFrameCount) != B_OK)
			return false;

		if (outUsed != NULL)
			*outUsed += directFrameCount * frameSize;

		return true;
	}

	return false;
}


status_t
BSound::BindTo(BSoundPlayer* player, const media_raw_audio_format& format)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t
BSound::UnbindFrom(BSoundPlayer* player)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t
BSound::Perform(int32 code, ...)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t BSound::_Reserved_Sound_0(void*) { return B_ERROR; }
status_t BSound::_Reserved_Sound_1(void*) { return B_ERROR; }
status_t BSound::_Reserved_Sound_2(void*) { return B_ERROR; }
status_t BSound::_Reserved_Sound_3(void*) { return B_ERROR; }
status_t BSound::_Reserved_Sound_4(void*) { return B_ERROR; }
status_t BSound::_Reserved_Sound_5(void*) { return B_ERROR; }
