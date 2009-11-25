/*
 * Copyright 2004-2007, Marcus Overhagen. All rights reserved.
 * Copyright 2008, Maurice Kalinowski. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "MediaExtractor.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Autolock.h>

#include "debug.h"
#include "ChunkCache.h"
#include "PluginManager.h"


// should be 0, to disable the chunk cache set it to 1
#define DISABLE_CHUNK_CACHE 0


class MediaExtractorChunkProvider : public ChunkProvider {
public:
	MediaExtractorChunkProvider(MediaExtractor* extractor, int32 stream)
		:
		fExtractor(extractor),
		fStream(stream)
	{
	}

	virtual status_t GetNextChunk(const void** _chunkBuffer, size_t* _chunkSize,
		media_header *mediaHeader)
	{
		return fExtractor->GetNextChunk(fStream, _chunkBuffer, _chunkSize,
			mediaHeader);
	}

private:
	MediaExtractor*	fExtractor;
	int32			fStream;
};


// #pragma mark -


MediaExtractor::MediaExtractor(BDataIO* source, int32 flags)
{
	CALLED();
	fSource = source;
	fStreamInfo = NULL;
	fExtractorThread = -1;
	fExtractorWaitSem = -1;
	fTerminateExtractor = false;

	fInitStatus = _plugin_manager.CreateReader(&fReader, &fStreamCount,
		&fFileFormat, source);
	if (fInitStatus != B_OK) {
		fStreamCount = 0;
		fReader = NULL;
		return;
	}

	fStreamInfo = new stream_info[fStreamCount];

	// initialize stream infos
	for (int32 i = 0; i < fStreamCount; i++) {
		fStreamInfo[i].status = B_OK;
		fStreamInfo[i].cookie = 0;
		fStreamInfo[i].hasCookie = true;
		fStreamInfo[i].infoBuffer = 0;
		fStreamInfo[i].infoBufferSize = 0;
		fStreamInfo[i].chunkCache = new ChunkCache;
		memset(&fStreamInfo[i].encodedFormat, 0,
			sizeof(fStreamInfo[i].encodedFormat));
	}

	// create all stream cookies
	for (int32 i = 0; i < fStreamCount; i++) {
		if (B_OK != fReader->AllocateCookie(i, &fStreamInfo[i].cookie)) {
			fStreamInfo[i].cookie = 0;
			fStreamInfo[i].hasCookie = false;
			fStreamInfo[i].status = B_ERROR;
			ERROR("MediaExtractor::MediaExtractor: AllocateCookie for stream "
				"%ld failed\n", i);
		}
	}

	// get info for all streams
	for (int32 i = 0; i < fStreamCount; i++) {
		if (fStreamInfo[i].status != B_OK)
			continue;

		int64 frameCount;
		bigtime_t duration;
		if (fReader->GetStreamInfo(fStreamInfo[i].cookie, &frameCount,
				&duration, &fStreamInfo[i].encodedFormat,
				&fStreamInfo[i].infoBuffer, &fStreamInfo[i].infoBufferSize)
					!= B_OK) {
			fStreamInfo[i].status = B_ERROR;
			ERROR("MediaExtractor::MediaExtractor: GetStreamInfo for "
				"stream %ld failed\n", i);
		}
	}

#if DISABLE_CHUNK_CACHE == 0
	// start extractor thread
	fExtractorWaitSem = create_sem(1, "media extractor thread sem");
	fExtractorThread = spawn_thread(_ExtractorEntry, "media extractor thread",
		40, this);
	resume_thread(fExtractorThread);
#endif
}


MediaExtractor::~MediaExtractor()
{
	CALLED();

	// terminate extractor thread
	fTerminateExtractor = true;
	release_sem(fExtractorWaitSem);
	status_t err;
	wait_for_thread(fExtractorThread, &err);
	delete_sem(fExtractorWaitSem);

	// free all stream cookies
	// and chunk caches
	for (int32 i = 0; i < fStreamCount; i++) {
		if (fStreamInfo[i].hasCookie)
			fReader->FreeCookie(fStreamInfo[i].cookie);
		delete fStreamInfo[i].chunkCache;
	}

	_plugin_manager.DestroyReader(fReader);

	delete [] fStreamInfo;
	// fSource is owned by the BMediaFile
}


status_t
MediaExtractor::InitCheck()
{
	CALLED();
	return fInitStatus;
}


void
MediaExtractor::GetFileFormatInfo(media_file_format* fileFormat) const
{
	CALLED();
	*fileFormat = fFileFormat;
}


int32
MediaExtractor::StreamCount()
{
	CALLED();
	return fStreamCount;
}


const char*
MediaExtractor::Copyright()
{
	return fReader->Copyright();
}


const media_format*
MediaExtractor::EncodedFormat(int32 stream)
{
	return &fStreamInfo[stream].encodedFormat;
}


int64
MediaExtractor::CountFrames(int32 stream) const
{
	CALLED();
	if (fStreamInfo[stream].status != B_OK)
		return 0LL;

	int64 frameCount;
	bigtime_t duration;
	media_format format;
	const void* infoBuffer;
	size_t infoSize;

	fReader->GetStreamInfo(fStreamInfo[stream].cookie, &frameCount, &duration,
		&format, &infoBuffer, &infoSize);

	return frameCount;
}


bigtime_t
MediaExtractor::Duration(int32 stream) const
{
	CALLED();

	if (fStreamInfo[stream].status != B_OK)
		return 0LL;

	int64 frameCount;
	bigtime_t duration;
	media_format format;
	const void *infoBuffer;
	size_t infoSize;

	fReader->GetStreamInfo(fStreamInfo[stream].cookie, &frameCount, &duration,
		&format, &infoBuffer, &infoSize);

	return duration;
}


status_t
MediaExtractor::Seek(int32 stream, uint32 seekTo, int64* frame, bigtime_t* time)
{
	CALLED();
	if (fStreamInfo[stream].status != B_OK)
		return fStreamInfo[stream].status;

	status_t result;
	result = fReader->Seek(fStreamInfo[stream].cookie, seekTo, frame, time);
	if (result != B_OK)
		return result;

	// clear buffered chunks
	fStreamInfo[stream].chunkCache->MakeEmpty();
	release_sem(fExtractorWaitSem);

	return B_OK;
}


status_t
MediaExtractor::FindKeyFrame(int32 stream, uint32 seekTo, int64* _frame,
	bigtime_t* _time) const
{
	CALLED();
	if (fStreamInfo[stream].status != B_OK)
		return fStreamInfo[stream].status;

	return fReader->FindKeyFrame(fStreamInfo[stream].cookie,
		seekTo, _frame, _time);
}


status_t
MediaExtractor::GetNextChunk(int32 stream, const void** _chunkBuffer,
	size_t* _chunkSize, media_header* mediaHeader)
{
	if (fStreamInfo[stream].status != B_OK)
		return fStreamInfo[stream].status;

#if DISABLE_CHUNK_CACHE > 0
	static BLocker locker("media extractor next chunk");
	BAutolock lock(locker);
	return fReader->GetNextChunk(fStreamInfo[stream].cookie, _chunkBuffer,
		_chunkSize, mediaHeader);
#endif

	status_t status = fStreamInfo[stream].chunkCache->GetNextChunk(_chunkBuffer,
		_chunkSize, mediaHeader);
	release_sem(fExtractorWaitSem);
	return status;
}


status_t
MediaExtractor::CreateDecoder(int32 stream, Decoder** _decoder,
	media_codec_info* codecInfo)
{
	CALLED();

	status_t status = fStreamInfo[stream].status;
	if (status != B_OK) {
		ERROR("MediaExtractor::CreateDecoder can't create decoder for "
			"stream %ld: %s\n", stream, strerror(status));
		return status;
	}

	// TODO: Here we should work out a way so that if there is a setup
	// failure we can try the next decoder
	Decoder* decoder;
	status = _plugin_manager.CreateDecoder(&decoder,
		fStreamInfo[stream].encodedFormat);
	if (status != B_OK) {
#if DEBUG
		char formatString[256];
		string_for_format(fStreamInfo[stream].encodedFormat, formatString,
			sizeof(formatString));

		ERROR("MediaExtractor::CreateDecoder _plugin_manager.CreateDecoder "
			"failed for stream %ld, format: %s: %s\n", stream, formatString,
			strerror(status));
#endif
		return status;
	}

	ChunkProvider* chunkProvider
		= new(std::nothrow) MediaExtractorChunkProvider(this, stream);
	if (chunkProvider == NULL) {
		_plugin_manager.DestroyDecoder(decoder);
		ERROR("MediaExtractor::CreateDecoder can't create chunk provider "
			"for stream %ld\n", stream);
		return B_NO_MEMORY;
	}

	decoder->SetChunkProvider(chunkProvider);

	status = decoder->Setup(&fStreamInfo[stream].encodedFormat,
		fStreamInfo[stream].infoBuffer, fStreamInfo[stream].infoBufferSize);
	if (status != B_OK) {
		_plugin_manager.DestroyDecoder(decoder);
		ERROR("MediaExtractor::CreateDecoder Setup failed for stream %ld: %s\n",
			stream, strerror(status));
		return status;
	}

	status = _plugin_manager.GetDecoderInfo(decoder, codecInfo);
	if (status != B_OK) {
		_plugin_manager.DestroyDecoder(decoder);
		ERROR("MediaExtractor::CreateDecoder GetCodecInfo failed for stream "
			"%ld: %s\n", stream, strerror(status));
		return status;
	}

	*_decoder = decoder;
	return B_OK;
}


status_t
MediaExtractor::_ExtractorEntry(void* extractor)
{
	static_cast<MediaExtractor*>(extractor)->_ExtractorThread();
	return B_OK;
}


void
MediaExtractor::_ExtractorThread()
{
	while (true) {
		acquire_sem(fExtractorWaitSem);
		if (fTerminateExtractor)
			return;

		bool refillDone;
		do {
			refillDone = false;
			for (int32 stream = 0; stream < fStreamCount; stream++) {
				if (fStreamInfo[stream].status != B_OK)
					continue;

				if (fStreamInfo[stream].chunkCache->NeedsRefill()) {
					media_header mediaHeader;
					const void* chunkBuffer;
					size_t chunkSize;
					status_t status = fReader->GetNextChunk(
						fStreamInfo[stream].cookie, &chunkBuffer, &chunkSize,
						&mediaHeader);
					fStreamInfo[stream].chunkCache->PutNextChunk(chunkBuffer,
						chunkSize, mediaHeader, status);
					refillDone = true;
				}
			}
			if (fTerminateExtractor)
				return;
		} while (refillDone);
	}
}
