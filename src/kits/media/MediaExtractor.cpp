#include "MediaExtractor.h"
#include "PluginManager.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>

MediaExtractor::MediaExtractor(BDataIO * source, int32 flags)
{
	CALLED();
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
		
		char sz[1024];
		string_for_format(fStreamInfo[i].encodedFormat, sz, sizeof(sz));
		printf("MediaExtractor::MediaExtractor: stream %d has format %s\n", i, sz);
		fStreamInfo[i].encodedFormat.type = B_MEDIA_ENCODED_AUDIO;
		fStreamInfo[i].encodedFormat.u.encoded_audio.encoding = (enum media_encoded_audio_format::audio_encoding) 1;
		fStreamInfo[i].encodedFormat.u.encoded_audio.bit_rate = 128000;
		fStreamInfo[i].encodedFormat.u.encoded_audio.frame_size = 3000;
		fStreamInfo[i].encodedFormat.u.encoded_audio.output.frame_rate = 44100;
		fStreamInfo[i].encodedFormat.u.encoded_audio.output.channel_count = 2;
		fStreamInfo[i].encodedFormat.u.encoded_audio.output.format = 2;
		fStreamInfo[i].encodedFormat.u.encoded_audio.output.byte_order = B_MEDIA_LITTLE_ENDIAN;
		fStreamInfo[i].encodedFormat.u.encoded_audio.output.buffer_size = 4 * 1024;
		string_for_format(fStreamInfo[i].encodedFormat, sz, sizeof(sz));
		printf("MediaExtractor::MediaExtractor: stream %d has new format %s\n", i, sz);
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

media_format *
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
	
	// XXX this should be done in a different thread, and double buffered for each stream
	return fReader->GetNextChunk(fStreamInfo[stream].cookie, chunkBuffer, chunkSize, mediaHeader);
}

status_t
MediaExtractor::CreateDecoder(int32 stream, Decoder **decoder, media_codec_info *mci)
{
	CALLED();
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

