#include <stdio.h>
#include <DataIO.h>
#include "RawDecoderPlugin.h"

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE ((void)0)
#endif

RawDecoder::RawDecoder()
{
}

RawDecoder::~RawDecoder()
{
}


status_t
RawDecoder::Sniff(media_format *format, void **infoBuffer, int32 *infoSize)
{
	if (format->type != B_MEDIA_RAW_AUDIO)
		return B_ERROR;
		
	fFormat = *format;
	return B_OK;
}

	
status_t
RawDecoder::GetOutputFormat(media_format *format)
{
	*format = fFormat;
	return B_OK;
}

status_t
RawDecoder::Seek(media_seek_type seekTo,
				 int64 *frame, bigtime_t *time)
{
	return B_OK;
}


status_t
RawDecoder::Decode(void *buffer, int64 *frameCount,
				   media_header *mediaHeader, media_decode_info *info)
{
	return B_OK;
}


Decoder *
RawDecoderPlugin::NewDecoder()
{
	return new RawDecoder;
}


MediaPlugin *instantiate_plugin()
{
	return new RawDecoderPlugin;
}
