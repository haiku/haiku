#include "MediaExtractor.h"
#include <stdio.h>
#include <string.h>

status_t _CreateReader(Reader **reader, int32 *streamCount, media_file_format *mff, BDataIO *source);
status_t _CreateDecoder(Decoder **decoder, media_codec_info *mci, const media_format *format);

MediaExtractor::MediaExtractor(BDataIO * source, int32 flags)
{
	fSource = source;
	fStreamInfo = 0;
	fStreamCount = 0;
	fReader = 0;
	
	fErr = _CreateReader(&fReader, &fStreamCount, &fMff, source);
	if (fErr)
		return;
		
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
	// free all stream cookies
	for (int32 i = 0; i < fStreamCount; i++) {
		if (fStreamInfo[i].cookie)
			fReader->FreeCookie(fStreamInfo[i].cookie);
	}

	delete fStreamInfo;
	delete fSource;
}

status_t
MediaExtractor::InitCheck()
{
	return fErr;
}

void
MediaExtractor::GetFileFormatInfo(media_file_format *mfi) const
{
	*mfi = fMff;
}

int32
MediaExtractor::StreamCount()
{
	return fStreamCount;
}

media_format *
MediaExtractor::EncodedFormat(int32 stream)
{
	return &fStreamInfo[stream].encodedFormat;
}

int64
MediaExtractor::CountFrames(int32 stream) const
{
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
	int64 frameCount;
	bigtime_t duration;
	media_format format;
	void *infoBuffer;
	int32 infoSize;
	
	fReader->GetStreamInfo(fStreamInfo[stream].cookie, &frameCount, &duration, &format, &infoBuffer, &infoSize);

	return duration;
}

void *
MediaExtractor::InfoBuffer(int32 stream) const
{
	return fStreamInfo[stream].infoBuffer;
}

int32
MediaExtractor::InfoBufferSize(int32 stream) const
{
	return fStreamInfo[stream].infoBufferSize;
}

status_t
MediaExtractor::Seek(int32 stream, uint32 seekTo,
					 int64 *frame, bigtime_t *time)
{
	status_t result;
	result = fReader->Seek(fStreamInfo[stream].cookie, seekTo, frame, time);
	if (result != B_OK)
		return result;
	
	// clear buffered chunks
}

status_t
MediaExtractor::GetNextChunk(int32 stream,
							 void **chunkBuffer, int32 *chunkSize,
							 media_header *mediaHeader)
{
	// get buffered chunk
	
	// XXX this should be done in a different thread, and double buffered for each stream
	return fReader->GetNextChunk(fStreamInfo[stream].cookie, chunkBuffer, chunkSize, mediaHeader);
}

status_t
MediaExtractor::CreateDecoder(int32 stream, Decoder **decoder, media_codec_info *mci)
{
	if (fStreamInfo[stream].status != B_OK) {
		printf("MediaExtractor::CreateDecoder can't create decoder for stream %ld\n", stream);
		return B_ERROR;
	}
	
	if (B_OK != _CreateDecoder(decoder, mci, &fStreamInfo[stream].encodedFormat)) {
		printf("MediaExtractor::CreateDecoder failed for stream %ld\n", stream);
		return B_ERROR;
	}

	(*decoder)->Setup(this, stream);	
	
	return B_OK;
}

status_t
_CreateReader(Reader **reader, int32 *streamCount, media_file_format *mff, BDataIO *source)
{
	return B_OK;
}

status_t
_CreateDecoder(Decoder **decoder, media_codec_info *mci, const media_format *format)
{
	return B_OK;
}

