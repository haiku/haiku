#include "MediaExtractor.h"
#include "PluginManager.h"
#include "debug.h"

#include <Autolock.h>

#include <stdio.h>
#include <string.h>


MediaExtractor::MediaExtractor(BDataIO *source, int32 flags)
{
	CALLED();
	fSource = source;
	fStreamInfo = 0;

	fErr = _CreateReader(&fReader, &fStreamCount, &fMff, source);
	if (fErr) {
		fStreamCount = 0;
		fReader = 0;
		return;
	}

	fStreamInfo = new stream_info[fStreamCount];

	// initialize stream infos
	for (int32 i = 0; i < fStreamCount; i++) {
		fStreamInfo[i].status = B_OK;
		fStreamInfo[i].cookie = 0;
		fStreamInfo[i].infoBuffer = 0;
		fStreamInfo[i].infoBufferSize = 0;
		memset(&fStreamInfo[i].encodedFormat, 0, sizeof(fStreamInfo[i].encodedFormat));
	}

	// create all stream cookies
	for (int32 i = 0; i < fStreamCount; i++) {
		if (B_OK != fReader->AllocateCookie(i, &fStreamInfo[i].cookie)) {
			fStreamInfo[i].cookie = 0;
			fStreamInfo[i].status = B_ERROR;
			printf("MediaExtractor::MediaExtractor: AllocateCookie for stream %ld failed\n", i);
		}
	}

	// get info for all streams
	for (int32 i = 0; i < fStreamCount; i++) {
		if (fStreamInfo[i].status != B_OK)
			continue;
		int64 frameCount;
		bigtime_t duration;
		if (B_OK != fReader->GetStreamInfo(fStreamInfo[i].cookie, &frameCount, &duration,
										   &fStreamInfo[i].encodedFormat,
										   &fStreamInfo[i].infoBuffer,
										   &fStreamInfo[i].infoBufferSize)) {
			fStreamInfo[i].status = B_ERROR;
			printf("MediaExtractor::MediaExtractor: GetStreamInfo for stream %ld failed\n", i);
		}
	}
}

MediaExtractor::~MediaExtractor()
{
	CALLED();

	// free all stream cookies
	for (int32 i = 0; i < fStreamCount; i++) {
		if (fStreamInfo[i].cookie)
			fReader->FreeCookie(fStreamInfo[i].cookie);
	}

	if (fReader)
		_DestroyReader(fReader);

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

const media_format *
MediaExtractor::EncodedFormat(int32 stream)
{
	return &fStreamInfo[stream].encodedFormat;
}

int64
MediaExtractor::CountFrames(int32 stream) const
{
	CALLED();
	int64 frameCount;
	bigtime_t duration;
	media_format format;
	void *infoBuffer;
	int32 infoSize;
	
	fReader->GetStreamInfo(fStreamInfo[stream].cookie, &frameCount, &duration, &format, &infoBuffer, &infoSize);

	return frameCount;
}

bigtime_t
MediaExtractor::Duration(int32 stream) const
{
	CALLED();
	int64 frameCount;
	bigtime_t duration;
	media_format format;
	void *infoBuffer;
	int32 infoSize;
	
	fReader->GetStreamInfo(fStreamInfo[stream].cookie, &frameCount, &duration, &format, &infoBuffer, &infoSize);

	return duration;
}

status_t
MediaExtractor::Seek(int32 stream, uint32 seekTo,
					 int64 *frame, bigtime_t *time)
{
	CALLED();
	status_t result;
	result = fReader->Seek(fStreamInfo[stream].cookie, seekTo, frame, time);
	if (result != B_OK)
		return result;
	
	// clear buffered chunks
	return B_OK;
}

status_t
MediaExtractor::GetNextChunk(int32 stream,
							 void **chunkBuffer, int32 *chunkSize,
							 media_header *mediaHeader)
{
	//CALLED();
	// get buffered chunk
	
	// XXX until this is done in a single extractor thread,
	// XXX make calls to GetNextChunk thread save
	static BLocker locker;
	BAutolock lock(locker);
	
	// XXX this should be done in a different thread, and double buffered for each stream
	return fReader->GetNextChunk(fStreamInfo[stream].cookie, chunkBuffer, chunkSize, mediaHeader);
}

class MediaExtractorChunkProvider : public ChunkProvider {
private:
	MediaExtractor * fExtractor;
	int32 fStream;
public:
	MediaExtractorChunkProvider(MediaExtractor * extractor, int32 stream) {
		fExtractor = extractor;
		fStream = stream;
	}
	virtual status_t GetNextChunk(void **chunkBuffer, int32 *chunkSize,
	                              media_header *mediaHeader) {
		return fExtractor->GetNextChunk(fStream,chunkBuffer,chunkSize,mediaHeader);
	}
};

status_t
MediaExtractor::CreateDecoder(int32 stream, Decoder **decoder, media_codec_info *mci)
{
	CALLED();
	status_t res;

	res = fStreamInfo[stream].status;
	if (res != B_OK) {
		printf("MediaExtractor::CreateDecoder can't create decoder for stream %ld\n", stream);
		return res;
	}
	
	res = _CreateDecoder(decoder, mci, &fStreamInfo[stream].encodedFormat);
	if (res != B_OK) {
		printf("MediaExtractor::CreateDecoder failed for stream %ld\n", stream);
		return res;
	}

	(*decoder)->Setup(new MediaExtractorChunkProvider(this, stream));
	
	res = (*decoder)->Setup(&fStreamInfo[stream].encodedFormat, fStreamInfo[stream].infoBuffer , fStreamInfo[stream].infoBufferSize);
	if (res != B_OK) {
		printf("MediaExtractor::CreateDecoder Setup failed for stream %ld: %ld (%s)\n",
			stream, res, strerror(res));
	}
	return res;
}

