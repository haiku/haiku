#ifndef _PLUGIN_MANAGER_H
#define _PLUGIN_MANAGER_H

#include "ReaderPlugin.h"
#include "DecoderPlugin.h"

status_t _CreateReader(Reader **reader, int32 *streamCount, media_file_format *mff, BDataIO *source);
status_t _CreateDecoder(Decoder **decoder, media_codec_info *mci, const media_format *format);

void _DestroyReader(Reader *reader);
void _DestroyDecoder(Decoder *decoder);

#endif
