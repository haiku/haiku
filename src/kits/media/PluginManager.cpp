#include "PluginManager.h"


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


void
_DestroyReader(Reader *reader)
{
}

void
_DestroyDecoder(Decoder *decoder)
{
}
