#include "DecoderPlugin.h"

#include "libvorbis/vorbis/codec.h"

class vorbisDecoder : public Decoder
{
public:
				vorbisDecoder();
				~vorbisDecoder();
	
	status_t	Setup(media_format *ioEncodedFormat,
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

	vorbis_info			fInfo;
	vorbis_comment		fComment;
	vorbis_dsp_state	fDspState;
	vorbis_block		fBlock;
	media_raw_video_format fOutput;
	bigtime_t		fStartTime;
	int				fFrameSize;
	int				fOutputBufferSize;
};


class vorbisDecoderPlugin : public DecoderPlugin
{
public:
	Decoder *	NewDecoder();
	status_t	RegisterPlugin();
};
