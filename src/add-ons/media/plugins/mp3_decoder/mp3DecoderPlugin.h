#include "DecoderPlugin.h"

#include "mpglib/mpglib.h"

class mp3Decoder : public Decoder
{
public:
				mp3Decoder();
				~mp3Decoder();
	
	status_t	Setup(media_format *ioEncodedFormat, media_format *ioDecodedFormat,
					  const void *infoBuffer, int32 infoSize);

	status_t	Seek(uint32 seekTo,
					 int64 seekFrame, int64 *frame,
					 bigtime_t seekTime, bigtime_t *time);

							 
	status_t	Decode(void *buffer, int64 *frameCount,
					   media_header *mediaHeader, media_decode_info *info);
private:
	struct mpstr	fMpgLibPrivate;
	int32			fResidualBytes;
	uint8 *			fResidualBuffer;
	uint8 *			fDecodeBuffer;
	int32			fFrameSize;	
};


class mp3DecoderPlugin : public DecoderPlugin
{
public:
	Decoder *	NewDecoder();
	status_t	RegisterPlugin();
};
