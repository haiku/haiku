#include "DecoderPlugin.h"

class RawDecoder : public Decoder
{
public:
				RawDecoder();
				~RawDecoder();
	
	status_t	Sniff(media_format *format, void **infoBuffer, int32 *infoSize);
	
	status_t	GetOutputFormat(media_format *format);

	status_t	Seek(media_seek_type seekTo,
					 int64 *frame, bigtime_t *time);
							 
	status_t	Decode(void *buffer, int64 *frameCount,
					   media_header *mediaHeader, media_decode_info *info);
private:
	media_format fFormat;
};


class RawDecoderPlugin : public DecoderPlugin
{
public:
	Decoder *NewDecoder();
};

MediaPlugin *instantiate_plugin();
