#include "DecoderPlugin.h"

#include "xvidcore/xvid.h"

class xvidDecoder : public Decoder
{
public:
				xvidDecoder();
				~xvidDecoder();
	
	status_t	Setup(media_format *ioEncodedFormat, media_format *ioDecodedFormat,
					  const void *infoBuffer, int32 infoSize);

	status_t	Seek(uint32 seekTo,
					 int64 seekFrame, int64 *frame,
					 bigtime_t seekTime, bigtime_t *time);

							 
	status_t	Decode(void *buffer, int64 *frameCount,
					   media_header *mediaHeader, media_decode_info *info);
private:
	XVID_DEC_PARAM	fXvidDecoderParams;
	int32			fResidualBytes;
	uint8 *			fResidualBuffer;
	uint8 *			fDecodeBuffer;
	int32			fFrameSize;	
	uint32			fBytesPerRow;
	int				fColorSpace;
};


class xvidDecoderPlugin : public DecoderPlugin
{
public:
	Decoder *	NewDecoder();
	status_t	RegisterPlugin();
};
