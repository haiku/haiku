#include "DecoderPlugin.h"
#include "PluginManager.h"
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
	return _PublishDecoder(this, meta_description, short_name, pretty_name, default_mapping);
}
