#include "DecoderPlugin.h"
#include <MediaFormats.h>
#include <stdio.h>
#include <string.h>

Decoder::Decoder()
{
}

Decoder::~Decoder()
{
}
	
status_t
Decoder::GetNextChunk(void **chunkBuffer, int32 *chunkSize,
					  media_header *mediaHeader)
{
	return fExtractor->GetNextChunk(fStream, chunkBuffer, chunkSize, mediaHeader);
}

void
Decoder::Setup(MediaExtractor *extractor, int32 stream)
{
	fExtractor = extractor;
	fStream = stream;
}

DecoderPlugin::DecoderPlugin()
{
}

status_t
DecoderPlugin::PublishDecoder(const char *meta_description,
							  const char *short_name,
							  const char *pretty_name,
							  const char *default_mapping /* = 0 */)
{
/*
	media_format fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = fmt_type;
	BMediaFormats formats;
	if (B_OK != formats.MakeFormatFor(&fmt_desc, 1, &fmt))
		return B_ERROR;

	char s[1024];
	string_for_format(fmt, s, sizeof(s));
	printf("DecoderPlugin::PublishDecoder: short_name \"%s\", pretty_name \"%s\", format %s\n",
		short_name, pretty_name, s);
*/
	return B_OK;
}
