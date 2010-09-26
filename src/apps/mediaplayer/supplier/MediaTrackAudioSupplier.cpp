/*
 * Copyright © 2000-2004 Ingo Weinhold <ingo_weinhold@gmx.de>
 * Copyright © 2006-2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "MediaTrackAudioSupplier.h"

#include <new>
#include <algorithm>
#include <stdio.h>
#include <string.h>

#include <MediaFile.h>
#include <MediaTrack.h>

using namespace std;

//#define TRACE_AUDIO_SUPPLIER
#ifdef TRACE_AUDIO_SUPPLIER
# define TRACE(x...)	printf("MediaTrackAudioSupplier::" x)
#else
# define TRACE(x...)
#endif


// #pragma mark - Buffer


struct MediaTrackAudioSupplier::Buffer {
			void*				data;
			int64				offset;
			int64				size;
			bigtime_t			time_stamp;

	static	int					CompareOffset(const void* a, const void* b);
};


int
MediaTrackAudioSupplier::Buffer::CompareOffset(const void* a, const void* b)
{
	const Buffer* buffer1 = *(const Buffer**)a;
	const Buffer* buffer2 = *(const Buffer**)b;
	int result = 0;
	if (buffer1->offset < buffer2->offset)
		result = -1;
	else if (buffer1->offset > buffer2->offset)
		result = 1;
	return result;
}


// #pragma mark - MediaTrackAudioSupplier


MediaTrackAudioSupplier::MediaTrackAudioSupplier(BMediaTrack* mediaTrack,
		int32 trackIndex)
	:
	AudioTrackSupplier(),
	fMediaTrack(mediaTrack),
	fBuffer(NULL),
	fBufferOffset(0),
	fBufferSize(0),
	fBuffers(10),
	fHasKeyFrames(false),
	fCountFrames(0),
	fReportSeekError(true),
	fTrackIndex(trackIndex)
{
	_InitFromTrack();
}


MediaTrackAudioSupplier::~MediaTrackAudioSupplier()
{
	_FreeBuffers();
	delete[] fBuffer;
}


const media_format&
MediaTrackAudioSupplier::Format() const
{
	return AudioReader::Format();
}


status_t
MediaTrackAudioSupplier::GetEncodedFormat(media_format* format) const
{
	if (!fMediaTrack)
		return B_NO_INIT;
	return fMediaTrack->EncodedFormat(format);
}


status_t
MediaTrackAudioSupplier::GetCodecInfo(media_codec_info* info) const
{
	if (!fMediaTrack)
		return B_NO_INIT;
	return fMediaTrack->GetCodecInfo(info);
}


bigtime_t
MediaTrackAudioSupplier::Duration() const
{
	if (!fMediaTrack)
		return 0;

	return fMediaTrack->Duration();
}


// #pragma mark - AudioReader


bigtime_t
MediaTrackAudioSupplier::InitialLatency() const
{
	// TODO: this is just a wild guess, and not really founded on anything.
	return 100000;
}


status_t
MediaTrackAudioSupplier::Read(void* buffer, int64 pos, int64 frames)
{
	TRACE("Read(%p, %lld, %lld)\n", buffer, pos,
		frames);
	TRACE("  this: %p, fOutOffset: %lld\n", this, fOutOffset);

//printf("MediaTrackAudioSupplier::Read(%p, %lld, %lld)\n", buffer, pos, frames);

	status_t error = InitCheck();
	if (error != B_OK) {
		TRACE("Read() InitCheck failed\n");
		return error;
	}

	// convert pos according to our offset
	pos += fOutOffset;
	// Fill the frames after the end of the track with silence.
	if (fCountFrames > 0 && pos + frames > fCountFrames) {
		int64 size = max(0LL, fCountFrames - pos);
		ReadSilence(SkipFrames(buffer, size), frames - size);
		frames = size;
	}

	TRACE("  after eliminating the frames after the track end: %p, %lld, %lld\n",
		buffer, pos, frames);

#if 0
	const media_format& format = Format();
	int64 size = format.u.raw_audio.buffer_size;
	uint32 bytesPerFrame = format.u.raw_audio.channel_count
		* (format.u.raw_audio.format
			& media_raw_audio_format::B_AUDIO_SIZE_MASK);
	uint32 framesPerBuffer = size / bytesPerFrame;

	if (fMediaTrack->CurrentFrame() != pos) {
printf("  needing to seek: %lld (%lld)\n", pos,
	fMediaTrack->CurrentFrame());

		int64 keyFrame = pos;
		error = fMediaTrack->FindKeyFrameForFrame(&keyFrame,
			B_MEDIA_SEEK_CLOSEST_BACKWARD);
		if (error == B_OK) {
			error = fMediaTrack->SeekToFrame(&keyFrame,
				B_MEDIA_SEEK_CLOSEST_BACKWARD);
		}
		if (error != B_OK) {
printf("  error seeking to position: %lld (%lld)\n", pos,
	fMediaTrack->CurrentFrame());

			return error;
		}

		if (keyFrame < pos) {
printf("  need to skip %lld frames\n", pos - keyFrame);
			uint8 dummyBuffer[size];
			while (pos - keyFrame >= framesPerBuffer) {
printf("  skipped %lu frames (full buffer)\n", framesPerBuffer);
				int64 sizeToRead = size;
				fMediaTrack->ReadFrames(dummyBuffer, &sizeToRead);
				keyFrame += framesPerBuffer;
			}
			int64 restSize = pos - keyFrame;
			if (restSize > 0) {
printf("  skipped %lu frames (rest)\n", framesPerBuffer);
				fMediaTrack->ReadFrames(dummyBuffer, &restSize);
			}
		}
	}
	while (frames > 0) {
printf("  reading %lu frames (full buffer)\n", framesPerBuffer);
		int64 sizeToRead = min_c(size, frames * bytesPerFrame);
		fMediaTrack->ReadFrames(buffer, &sizeToRead);
		buffer = (uint8*)buffer + sizeToRead;
		frames -= framesPerBuffer;
	}
printf("  done\n\n");

#else
	// read the cached frames
	bigtime_t time = system_time();
	if (frames > 0)
		_ReadCachedFrames(buffer, pos, frames, time);

	TRACE("  after reading from cache: %p, %lld, %lld\n", buffer, pos, frames);

	// read the remaining (uncached) frames
	if (frames > 0)
		_ReadUncachedFrames(buffer, pos, frames, time);

#endif
	TRACE("Read() done\n");

	return B_OK;
}

// InitCheck
status_t
MediaTrackAudioSupplier::InitCheck() const
{
	status_t error = AudioReader::InitCheck();
	if (error == B_OK && (!fMediaTrack || !fBuffer))
		error = B_NO_INIT;
	return error;
}

// #pragma mark -

// _InitFromTrack
void
MediaTrackAudioSupplier::_InitFromTrack()
{
	TRACE("_InitFromTrack()\n");
	// Try to suggest a big buffer size, we do a lot of caching...
	fFormat.u.raw_audio.buffer_size = 16384;
	if (fMediaTrack == NULL || fMediaTrack->DecodedFormat(&fFormat) != B_OK
		|| fFormat.type != B_MEDIA_RAW_AUDIO) {
		fMediaTrack = NULL;
		return;
	}

	#ifdef TRACE_AUDIO_SUPPLIER
		char formatString[256];
		string_for_format(fFormat, formatString, 256);
		TRACE("_InitFromTrack(): format is: %s\n", formatString);
		TRACE("_InitFromTrack(): buffer size: %ld\n",
			fFormat.u.raw_audio.buffer_size);
	#endif

	fBuffer = new (nothrow) char[fFormat.u.raw_audio.buffer_size];
	_AllocateBuffers();

	// Find out, if the track has key frames: as a heuristic we
	// check, if the first and the second frame have the same backward
	// key frame.
	// Note: It shouldn't harm that much, if we're wrong and the
	// track has key frame although we found out that it has not.
#if 0
	int64 keyFrame0 = 0;
	int64 keyFrame1 = 1;
	fMediaTrack->FindKeyFrameForFrame(&keyFrame0,
		B_MEDIA_SEEK_CLOSEST_BACKWARD);
	fMediaTrack->FindKeyFrameForFrame(&keyFrame1,
		B_MEDIA_SEEK_CLOSEST_BACKWARD);
	fHasKeyFrames = (keyFrame0 == keyFrame1);
#else
	fHasKeyFrames = true;
#endif

	// get the length of the track
	fCountFrames = fMediaTrack->CountFrames();

	TRACE("_InitFromTrack(): keyframes: %d, frame count: %lld\n",
		fHasKeyFrames, fCountFrames);
	printf("_InitFromTrack(): keyframes: %d, frame count: %lld\n",
		fHasKeyFrames, fCountFrames);
}

// _FramesPerBuffer
int64
MediaTrackAudioSupplier::_FramesPerBuffer() const
{
	int64 sampleSize = fFormat.u.raw_audio.format
		& media_raw_audio_format::B_AUDIO_SIZE_MASK;
	int64 frameSize = sampleSize * fFormat.u.raw_audio.channel_count;
	return fFormat.u.raw_audio.buffer_size / frameSize;
}

// _CopyFrames
//
// Given two buffers starting at different frame offsets, this function
// copies /frames/ frames at position /position/ from the source to the
// target buffer.
// Note that no range checking is done.
void
MediaTrackAudioSupplier::_CopyFrames(void* source, int64 sourceOffset,
							  void* target, int64 targetOffset,
							  int64 position, int64 frames) const
{
	int64 sampleSize = fFormat.u.raw_audio.format
					   & media_raw_audio_format::B_AUDIO_SIZE_MASK;
	int64 frameSize = sampleSize * fFormat.u.raw_audio.channel_count;
	source = (char*)source + frameSize * (position - sourceOffset);
	target = (char*)target + frameSize * (position - targetOffset);
	memcpy(target, source, frames * frameSize);
}

// _CopyFrames
//
// Given two buffers starting at different frame offsets, this function
// copies /frames/ frames at position /position/ from the source to the
// target buffer. This version expects a cache buffer as source.
// Note that no range checking is done.
void
MediaTrackAudioSupplier::_CopyFrames(Buffer* buffer,
							  void* target, int64 targetOffset,
							  int64 position, int64 frames) const
{
	_CopyFrames(buffer->data, buffer->offset, target, targetOffset, position,
				frames);
}

// _AllocateBuffers
//
// Allocates a set of buffers.
void
MediaTrackAudioSupplier::_AllocateBuffers()
{
	int32 count = 10;
	_FreeBuffers();
	int32 bufferSize = fFormat.u.raw_audio.buffer_size;
	char* data = new (nothrow) char[bufferSize * count];
	for (; count > 0; count--) {
		Buffer* buffer = new (nothrow) Buffer;
		if (!buffer || !fBuffers.AddItem(buffer)) {
			delete buffer;
			if (fBuffers.CountItems() == 0)
				delete[] data;
			return;
		}
		buffer->data = data;
		data += bufferSize;
		buffer->offset = 0;
		buffer->size = 0;
		buffer->time_stamp = 0;
	}
}

// _FreeBuffers
//
// Frees the allocated buffers.
void
MediaTrackAudioSupplier::_FreeBuffers()
{
	if (fBuffers.CountItems() > 0) {
		delete[] (char*)_BufferAt(0)->data;
		for (int32 i = 0; Buffer* buffer = _BufferAt(i); i++)
			delete buffer;
		fBuffers.MakeEmpty();
	}
}

// _BufferAt
//
// Returns the buffer at index /index/.
MediaTrackAudioSupplier::Buffer*
MediaTrackAudioSupplier::_BufferAt(int32 index) const
{
	return (Buffer*)fBuffers.ItemAt(index);
}

// _FindBufferAtFrame
//
// If any buffer starts at offset /frame/, it is returned, NULL otherwise.
MediaTrackAudioSupplier::Buffer*
MediaTrackAudioSupplier::_FindBufferAtFrame(int64 frame) const
{
	Buffer* buffer = NULL;
	for (int32 i = 0;
		 ((buffer = _BufferAt(i))) && buffer->offset != frame;
		 i++);
	return buffer;
}

// _FindUnusedBuffer
//
// Returns the first unused buffer or NULL if all buffers are used.
MediaTrackAudioSupplier::Buffer*
MediaTrackAudioSupplier::_FindUnusedBuffer() const
{
	Buffer* buffer = NULL;
	for (int32 i = 0; ((buffer = _BufferAt(i))) && buffer->size != 0; i++)
		;
	return buffer;
}

// _FindUsableBuffer
//
// Returns either an unused buffer or, if all buffers are used, the least
// recently used buffer.
// In every case a buffer is returned.
MediaTrackAudioSupplier::Buffer*
MediaTrackAudioSupplier::_FindUsableBuffer() const
{
	Buffer* result = _FindUnusedBuffer();
	if (!result) {
		// find the least recently used buffer.
		result = _BufferAt(0);
		for (int32 i = 1; Buffer* buffer = _BufferAt(i); i++) {
			if (buffer->time_stamp < result->time_stamp)
				result = buffer;
		}
	}
	return result;
}

// _FindUsableBufferFor
//
// In case there already exists a buffer that starts at position this
// one is returned. Otherwise the function returns either an unused
// buffer or, if all buffers are used, the least recently used buffer.
// In every case a buffer is returned.
MediaTrackAudioSupplier::Buffer*
MediaTrackAudioSupplier::_FindUsableBufferFor(int64 position) const
{
	Buffer* buffer = _FindBufferAtFrame(position);
	if (buffer == NULL)
		buffer = _FindUsableBuffer();
	return buffer;
}

// _GetBuffersFor
//
// Adds pointers to all buffers to the list that contain data of the
// supplied interval.
void
MediaTrackAudioSupplier::_GetBuffersFor(BList& buffers, int64 position,
								 int64 frames) const
{
	buffers.MakeEmpty();
	for (int32 i = 0; Buffer* buffer = _BufferAt(i); i++) {
		// Calculate the intersecting interval and add the buffer if it is
		// not empty.
		int32 startFrame = max(position, buffer->offset);
		int32 endFrame = min(position + frames, buffer->offset + buffer->size);
		if (startFrame < endFrame)
			buffers.AddItem(buffer);
	}
}

// _TouchBuffer
//
// Sets a buffer's time stamp to the current system time.
void
MediaTrackAudioSupplier::_TouchBuffer(Buffer* buffer)
{
	buffer->time_stamp = system_time();
}

// _ReadBuffer
//
// Read a buffer from the current position (which is supplied in /position/)
// into /buffer/. The buffer's time stamp is set to the current system time.
status_t
MediaTrackAudioSupplier::_ReadBuffer(Buffer* buffer, int64 position)
{
	return _ReadBuffer(buffer, position, system_time());
}

// _ReadBuffer
//
// Read a buffer from the current position (which is supplied in /position/)
// into /buffer/. The buffer's time stamp is set to the supplied time.
status_t
MediaTrackAudioSupplier::_ReadBuffer(Buffer* buffer, int64 position,
	bigtime_t time)
{
	status_t error = fMediaTrack->ReadFrames(buffer->data, &buffer->size);

	TRACE("_ReadBuffer(%p, %lld): %s\n", buffer->data, buffer->size,
		strerror(error));

	buffer->offset = position;
	buffer->time_stamp = time;
	if (error != B_OK)
		buffer->size = 0;
	return error;
}

// _ReadCachedFrames
//
// Tries to read as much as possible data from the cache. The supplied
// buffer pointer as well as position and number of frames are adjusted
// accordingly. The used cache buffers are stamped with the supplied
// time.
void
MediaTrackAudioSupplier::_ReadCachedFrames(void*& dest, int64& pos,
	int64& frames, bigtime_t time)
{
	// Get a list of all cache buffers that contain data of the interval,
	// and sort it.
	BList buffers(10);
	_GetBuffersFor(buffers, pos, frames);
	buffers.SortItems(Buffer::CompareOffset);
	// Step forward through the list of cache buffers and try to read as
	// much data from the beginning as possible.
	for (int32 i = 0; Buffer* buffer = (Buffer*)buffers.ItemAt(i); i++) {
		if (buffer->offset <= pos && buffer->offset + buffer->size > pos) {
			// read from the beginning
			int64 size = min(frames, buffer->offset + buffer->size - pos);
			_CopyFrames(buffer->data, buffer->offset, dest, pos, pos, size);
			pos += size;
			frames -= size;
			dest = SkipFrames(dest, size);
			buffer->time_stamp = time;
		}
	}
	// Step backward through the list of cache buffers and try to read as
	// much data from the end as possible.
	for (int32 i = buffers.CountItems() - 1;
		 Buffer* buffer = (Buffer*)buffers.ItemAt(i);
		 i++) {
		if (buffer->offset < pos + frames
			&& buffer->offset + buffer->size >= pos + frames) {
			// read from the end
			int64 size = min(frames, pos + frames - buffer->offset);
			_CopyFrames(buffer->data, buffer->offset, dest, pos,
						pos + frames - size, size);
			frames -= size;
			buffer->time_stamp = time;
		}
	}
}


/*!	Reads /frames/ frames from /position/ into /buffer/. The frames are not
	read from the cache, but read frames are cached, if possible.
	New cache buffers are stamped with the supplied time.
	If an error occurs, the untouched part of the buffer is set to 0.
*/
status_t
MediaTrackAudioSupplier::_ReadUncachedFrames(void* buffer, int64 position,
	int64 frames, bigtime_t time)
{
	TRACE("_ReadUncachedFrames()\n");
	status_t error = B_OK;
	// seek to the position
	int64 currentPos = position;
	if (frames > 0) {
		error = _SeekToKeyFrameBackward(currentPos);
		TRACE("_ReadUncachedFrames() - seeked to position: %lld\n", currentPos);
//		if (position - currentPos > 100000)
//			printf("MediaTrackAudioSupplier::_ReadUncachedFrames() - "
//				"keyframe was far away: %lld -> %lld\n", position, currentPos);
	}
	// read the frames
	// TODO: Calculate timeout, 0.25 times duration of "frames" seems good.
	bigtime_t timeout = 10000;
	while (error == B_OK && frames > 0) {
		Buffer* cacheBuffer = _FindUsableBufferFor(currentPos);
		TRACE("_ReadUncachedFrames() - usable buffer found: %p, "
			"position: %lld/%lld\n", cacheBuffer, currentPos, position);
		error = _ReadBuffer(cacheBuffer, currentPos, time);
		if (error == B_OK) {
			int64 size = min(position + frames,
				cacheBuffer->offset + cacheBuffer->size) - position;
			if (size > 0) {
				_CopyFrames(cacheBuffer, buffer, position, position, size);
				buffer = SkipFrames(buffer, size);
				position += size;
				frames -= size;
			}
			currentPos += cacheBuffer->size;
		}
		if (system_time() - time > timeout) {
			error = B_TIMED_OUT;
			break;
		}
	}

#if 0
	// Ensure that all frames up to the next key frame are cached.
	// This avoids, that each read reaches the BMediaTrack.
	if (error == B_OK) {
		int64 nextKeyFrame = currentPos;
		if (_FindKeyFrameForward(nextKeyFrame) == B_OK) {
			while (currentPos < nextKeyFrame) {
				// Check, if data at this position are cache.
				// If not read it.
				Buffer* cacheBuffer = _FindBufferAtFrame(currentPos);
				if (!cacheBuffer || cacheBuffer->size == 0) {
					cacheBuffer = _FindUsableBufferFor(currentPos);
					if (_ReadBuffer(cacheBuffer, currentPos, time) != B_OK)
						break;
				}
				if (cacheBuffer)
					currentPos += cacheBuffer->size;
			}
		}
	}
#endif

	// on error fill up the buffer with silence
	if (error != B_OK && frames > 0)
		ReadSilence(buffer, frames);
	return error;
}

// _FindKeyFrameForward
status_t
MediaTrackAudioSupplier::_FindKeyFrameForward(int64& position)
{
	status_t error = B_OK;
	if (fHasKeyFrames) {
		error = fMediaTrack->FindKeyFrameForFrame(
			&position, B_MEDIA_SEEK_CLOSEST_FORWARD);
	} else {
		int64 framesPerBuffer = _FramesPerBuffer();
		position += framesPerBuffer - 1;
		position = position % framesPerBuffer;
	}
	return error;
}

// _FindKeyFrameBackward
status_t
MediaTrackAudioSupplier::_FindKeyFrameBackward(int64& position)
{
	status_t error = B_OK;
	if (fHasKeyFrames) {
		error = fMediaTrack->FindKeyFrameForFrame(
			&position, B_MEDIA_SEEK_CLOSEST_BACKWARD);
	} else
		position -= position % _FramesPerBuffer();
	return error;
}

#if 0
// _SeekToKeyFrameForward
status_t
MediaTrackAudioSupplier::_SeekToKeyFrameForward(int64& position)
{
	if (position == fMediaTrack->CurrentFrame())
		return B_OK;

	status_t error = B_OK;
	if (fHasKeyFrames) {
		#ifdef TRACE_AUDIO_SUPPLIER
		int64 oldPosition = position;
		#endif
		error = fMediaTrack->SeekToFrame(&position,
			B_MEDIA_SEEK_CLOSEST_FORWARD);
		TRACE("_SeekToKeyFrameForward() - seek to key frame forward: "
			"%lld -> %lld (%lld)\n", oldPosition, position,
			fMediaTrack->CurrentFrame());
	} else {
		_FindKeyFrameForward(position);
		error = fMediaTrack->SeekToFrame(&position);
	}
	return error;
}
#endif

// _SeekToKeyFrameBackward
status_t
MediaTrackAudioSupplier::_SeekToKeyFrameBackward(int64& position)
{
	int64 currentPosition = fMediaTrack->CurrentFrame();
	if (position == currentPosition)
		return B_OK;

	status_t error = B_OK;
	if (fHasKeyFrames) {
		int64 wantedPosition = position;
		error = fMediaTrack->FindKeyFrameForFrame(&position,
			B_MEDIA_SEEK_CLOSEST_BACKWARD);
		if (error == B_OK && currentPosition > position
			&& currentPosition < wantedPosition) {
			// The current position is before the wanted position,
			// but later than the keyframe, so seeking is worse.
			position = currentPosition;
			return B_OK;
		}
		if (error == B_OK && position > wantedPosition) {
			// We asked to seek backwards, but the extractor seeked
			// forwards! Returning an error here will cause silence
			// to be produced.
			return B_ERROR;
		}
		if (error == B_OK)
			error = fMediaTrack->SeekToFrame(&position, 0);
		if (error != B_OK) {
			position = fMediaTrack->CurrentFrame();
//			if (fReportSeekError) {
				printf("  seek to key frame backward: %lld -> %lld (%lld) "
					"- %s\n", wantedPosition, position,
					fMediaTrack->CurrentFrame(), strerror(error));
				fReportSeekError = false;
//			}
		} else {
			fReportSeekError = true;
		}
	} else {
		_FindKeyFrameBackward(position);
		error = fMediaTrack->SeekToFrame(&position);
	}
	return error;
}

