#include "DecoderPlugin.h"
#include "speex.h"
#include "speex_header.h"
#include "speex_callbacks.h"

class speexDecoder : public Decoder
{
public:
				speexDecoder();
				~speexDecoder();
	
	status_t	Setup(media_format *inputFormat,
					  const void *infoBuffer, int32 infoSize);

	status_t	NegotiateOutputFormat(media_format *ioDecodedFormat);

	status_t	Seek(uint32 seekTo,
					 int64 seekFrame, int64 *frame,
					 bigtime_t seekTime, bigtime_t *time);

							 
	status_t	Decode(void *buffer, int64 *frameCount,
					   media_header *mediaHeader, media_decode_info *info);
					   
private:
	void		CopyInfoToEncodedFormat(media_format * format);
	void		CopyInfoToDecodedFormat(media_raw_audio_format * raf);

	SpeexBits		fBits;
	void *			fDecoderState;
	SpeexHeader	*	fHeader;

	int				fSpeexFrameSize;
	float *			fSpeexBuffer;
	int				fSpeexBytesRemaining;

	bigtime_t		fStartTime;
	int				fFrameSize;
	int				fOutputBufferSize;
};


class speexDecoderPlugin : public DecoderPlugin
{
public:
	Decoder *	NewDecoder();
	status_t	RegisterPlugin();
};
