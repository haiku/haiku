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
	return fReader->GetNextChunk(fReaderCookie, chunkBuffer, chunkSize, mediaHeader);
}

void
Decoder::Setup(Reader *reader, void *readerCookie)
{
	fReader = reader;
	fReaderCookie = readerCookie;
}

DecoderPlugin::DecoderPlugin()
{
}

status_t
DecoderPlugin::PublishDecoder(const char *short_name, const char *pretty_name, const media_format_description &fmt_desc, media_type fmt_type)
{
	media_format fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = fmt_type;
	BMediaFormats formats;
	if (B_OK != formats.MakeFormatFor(&fmt_desc, 1, &fmt))
		return B_ERROR;
	return PublishDecoder(short_name, pretty_name, fmt);
}

status_t
DecoderPlugin::PublishDecoder(const char *short_name, const char *pretty_name, const media_format &fmt)
{
	char s[1024];
	string_for_format(fmt, s, sizeof(s));
	printf("DecoderPlugin::PublishDecoder: short_name \"%s\", pretty_name \"%s\", format %s\n",
		short_name, pretty_name, s);
		

	return B_OK;
}
