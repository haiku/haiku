/*
** Copyright 2004-2007, Marcus Overhagen. All rights reserved.
** Copyright 2008, Maurice Kalinowski. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#include "MediaExtractor.h"
#include "PluginManager.h"
#include "ChunkCache.h"
#include "debug.h"

#include <Autolock.h>

#include <stdio.h>
#include <string.h>
#include <new>

// should be 0, to disable the chunk cache set it to 1
#define DISABLE_CHUNK_CACHE 0

MediaExtractor::MediaExtractor(BDataIO *source, int32 flags)
{
	CALLED();
	fSource = source;
	fStreamInfo = NULL;
	fExtractorThread = -1;
	fExtractorWaitSem = -1;
	fTerminateExtractor = false;

	fErr = _plugin_manager.CreateReader(&fReader, &fStreamCount, &fMff, source);
	if (fErr) {
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
		if (B_OK != fReader->GetStreamInfo(fStreamInfo[i].cookie, &frameCount,
				&duration, &fStreamInfo[i].encodedFormat,
				&fStreamInfo[i].infoBuffer, &fStreamInfo[i].infoBufferSize)) {
			fStreamInfo[i].status = B_ERROR;
			ERROR("MediaExtractor::MediaExtractor: GetStreamInfo for "
				"stream %ld failed\n", i);
		}
	}

#if DISABLE_CHUNK_CACHE == 0
	// start extractor thread
	fExtractorWaitSem = create_sem(1, "media extractor thread sem");
	fExtractorThread = spawn_thread(extractor_thread, "media extractor thread",
		10, this);
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
	return fErr;
}


void
MediaExtractor::GetFileFormatInfo(media_file_format *mfi) const
{
	CALLED();
	*mfi = fMff;
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


const media_format *
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
	const void *infoBuffer;
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
MediaExtractor::Seek(int32 stream, uint32 seekTo,
					 int64 *frame, bigtime_t *time)
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
MediaExtractor::FindKeyFrame(int32 stream, uint32 seekTo, int64 *frame,
	bigtime_t *time) const
{
	CALLED();
	if (fStreamInfo[stream].status != B_OK)
		return fStreamInfo[stream].status;

	return fReader->FindKeyFrame(fStreamInfo[stream].cookie,
		seekTo, frame, time);
}


status_t
MediaExtractor::GetNextChunk(int32 stream,
							 const void **chunkBuffer, size_t *chunkSize,
							 media_header *mediaHeader)
{
	if (fStreamInfo[stream].status != B_OK)
		return fStreamInfo[stream].status;

#if DISABLE_CHUNK_CACHE > 0
	static BLocker locker;
	BAutolock lock(locker);
	return fReader->GetNextChunk(fStreamInfo[stream].cookie, chunkBuffer,
		chunkSize, mediaHeader);
#endif

	status_t err;
	err = fStreamInfo[stream].chunkCache->GetNextChunk(chunkBuffer, chunkSize,
		mediaHeader);
	release_sem(fExtractorWaitSem);
	return err;
}


class MediaExtractorChunkProvider : public ChunkProvider {
private:
	MediaExtractor * fExtractor;
	int32 fStream;
public:
	MediaExtractorChunkProvider(MediaExtractor * extractor, int32 stream)
	{
		fExtractor = extractor;
		fStream = stream;
	}

	virtual status_t GetNextChunk(const void **chunkBuffer, size_t *chunkSize,
	                              media_header *mediaHeader)
	{
		return fExtractor->GetNextChunk(fStream, chunkBuffer, chunkSize,
			mediaHeader);
	}
};


status_t
MediaExtractor::CreateDecoder(int32 stream, Decoder **out_decoder,
	media_codec_info *mci)
{
	CALLED();
	status_t res;
	Decoder *decoder;

	res = fStreamInfo[stream].status;
	if (res != B_OK) {
		ERROR("MediaExtractor::CreateDecoder can't create decoder for "
			"stream %ld\n", stream);
		return res;
	}

	res = _plugin_manager.CreateDecoder(&decoder,
		fStreamInfo[stream].encodedFormat);
	if (res != B_OK) {
		char formatString[256];
		string_for_format(fStreamInfo[stream].encodedFormat, formatString,
			256);
		ERROR("MediaExtractor::CreateDecoder _plugin_manager.CreateDecoder "
			"failed for stream %ld, format: %s\n", stream, formatString);
		return res;
	}

	ChunkProvider *chunkProvider
		= new(std::nothrow) MediaExtractorChunkProvider(this, stream);
	if (!chunkProvider) {
		_plugin_manager.DestroyDecoder(decoder);
		ERROR("MediaExtractor::CreateDecoder can't create chunk provider "
			"for stream %ld\n", stream);
		return B_NO_MEMORY;
	}

	decoder->SetChunkProvider(chunkProvider);

	res = decoder->Setup(&fStreamInfo[stream].encodedFormat,
		fStreamInfo[stream].infoBuffer, fStreamInfo[stream].infoBufferSize);
	if (res != B_OK) {
		_plugin_manager.DestroyDecoder(decoder);
		ERROR("MediaExtractor::CreateDecoder Setup failed for stream %ld: "
			"%ld (%s)\n", stream, res, strerror(res));
		return res;
	}

	res = _plugin_manager.GetDecoderInfo(decoder, mci);
	if (res != B_OK) {
		_plugin_manager.DestroyDecoder(decoder);
		ERROR("MediaExtractor::CreateDecoder GetCodecInfo failed for stream "
			"%ld: %ld (%s)\n", stream, res, strerror(res));
		return res;
	}

	*out_decoder = decoder;
	return B_OK;
}


int32
MediaExtractor::extractor_thread(void *arg)
{
	static_cast<MediaExtractor *>(arg)->ExtractorThread();
	return 0;
}


void
MediaExtractor::ExtractorThread()
{
	for (;;) {
		acquire_sem(fExtractorWaitSem);
		if (fTerminateExtractor)
			return;

		bool refill_done;
		do {
			refill_done = false;
			for (int32 stream = 0; stream < fStreamCount; stream++) {
				if (fStreamInfo[stream].status != B_OK)
					continue;
				if (fStreamInfo[stream].chunkCache->NeedsRefill()) {
					media_header mediaHeader;
					const void *chunkBuffer;
					size_t chunkSize;
					status_t err;
					err = fReader->GetNextChunk(fStreamInfo[stream].cookie,
						&chunkBuffer, &chunkSize, &mediaHeader);
					fStreamInfo[stream].chunkCache->PutNextChunk(chunkBuffer,
						chunkSize, mediaHeader, err);
					refill_done = true;
				}
			}
			if (fTerminateExtractor)
				return;
		} while (refill_done);
	}
}
