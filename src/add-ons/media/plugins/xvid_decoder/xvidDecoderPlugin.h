#include "DecoderPlugin.h"

#include "xvid.h"

class xvidDecoder : public Decoder
{
public:
				xvidDecoder();
				~xvidDecoder();
	
	status_t	Setup(media_format *ioEncodedFormat,
					  const void *infoBuffer, int32 infoSize);

	status_t	NegotiateOutputFormat(media_format *ioDecodedFormat);

	status_t	Seek(uint32 seekTo,
					 int64 seekFrame, int64 *frame,
					 bigtime_t seekTime, bigtime_t *time);

							 
	status_t	Decode(void *buffer, int64 *frameCount,
					   media_header *mediaHeader, media_decode_info *info);
					   
	status_t	DecodeNextChunk();
	
private:
	XVID_DEC_PARAM	fXvidDecoderParams;
	int				fXvidColorSpace;
	struct media_raw_video_format fOutput;
	int32			fResidualBytes;
	uint8 *			fResidualBuffer;
	uint8 *			fDecodeBuffer;
	bigtime_t		fStartTime;
	int				fFrameSize;
	float			fBitRate;
	int				fOutputBufferSize;
};


class xvidDecoderPlugin : public DecoderPlugin
{
public:
	Decoder *	NewDecoder();
	status_t	RegisterPlugin();
};
