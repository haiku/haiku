#include "MediaExtractor.h"

MediaExtractor::MediaExtractor(BDataIO * source, int32 flags)
{
}

MediaExtractor::~MediaExtractor()
{
}

status_t
MediaExtractor::InitCheck()
{
	return B_OK;
}

void
MediaExtractor::GetFileFormatInfo(media_file_format *mfi) const
{
}

int32
MediaExtractor::StreamCount()
{
	return 0;
}

media_format *
MediaExtractor::EncodedFormat(int32 stream)
{
	return 0;
}

int64
MediaExtractor::CountFrames(int32 stream) const
{
	return 0;
}

bigtime_t
MediaExtractor::Duration(int32 stream) const
{
	return 0;
}

void *
MediaExtractor::InfoBuffer(int32 stream) const
{
	return 0;
}

int32
MediaExtractor::InfoBufferSize(int32 stream) const
{
	return 0;
}

status_t
MediaExtractor::Seek(int32 stream, uint32 seekTo,
					 int64 *frame, bigtime_t *time)
{
	return B_OK;
}

status_t
MediaExtractor::GetNextChunk(int32 stream,
							 void **chunkBuffer, int32 *chunkSize,
							 media_header *mediaHeader)
{
	return B_OK;
}

status_t
MediaExtractor::CreateDecoder(int32 stream, Decoder **decoder, media_codec_info *mci)
{
	return B_OK;
}

