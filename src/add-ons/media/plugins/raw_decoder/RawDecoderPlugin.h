#include "DecoderPlugin.h"

class RawDecoder : public Decoder
{
public:
	void		GetCodecInfo(media_codec_info *info);

	status_t	Setup(media_format *ioEncodedFormat,
					  const void *infoBuffer, int32 infoSize);
					  
	status_t	NegotiateOutputFormat(media_format *ioDecodedFormat);
	
	status_t	Seek(uint32 seekTo,
					 int64 seekFrame, int64 *frame,
					 bigtime_t seekTime, bigtime_t *time);
							 
	status_t	Decode(void *buffer, int64 *frameCount,
					   media_header *mediaHeader, media_decode_info *info);

private:
	status_t	NegotiateVideoOutputFormat(media_format *ioDecodedFormat);
	status_t	NegotiateAudioOutputFormat(media_format *ioDecodedFormat);

private:
	int32			fFrameRate;
	int32			fInputFrameSize;
	int32			fOutputFrameSize;
	int32			fInputSampleSize;
	int32			fOutputSampleSize;
	int32			fOutputBufferFrameCount;
	void 			(*fSwapInput)(void *data, int32 count);
	void 			(*fConvert)(void *dst, void *src, int32 count);
	void 			(*fSwapOutput)(void *data, int32 count);
	
	char *			fChunkBuffer;
	int32			fChunkSize;
	
	bigtime_t		fStartTime;
	
	media_format	fInputFormat;
};


class RawDecoderPlugin : public DecoderPlugin
{
public:
	Decoder *	NewDecoder(uint index);
	status_t	GetSupportedFormats(media_format ** formats, size_t * count);
};
