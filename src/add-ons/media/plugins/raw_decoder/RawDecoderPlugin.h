#include "DecoderPlugin.h"

class RawDecoder : public Decoder
{
public:
	status_t	Setup(media_format *ioEncodedFormat, media_format *ioDecodedFormat,
					  const void *infoBuffer, int32 infoSize);
	
	status_t	Seek(media_seek_type seekTo,
					 int64 seekFrame, int64 *frame,
					 bigtime_t seekTime, bigtime_t *time);
							 
	status_t	Decode(void *buffer, int64 *frameCount,
					   media_header *mediaHeader, media_decode_info *info);
private:
	int32		fFrameSize;
};


class RawDecoderPlugin : public DecoderPlugin
{
public:
	Decoder *	NewDecoder();
	status_t	RegisterPlugin();
};
