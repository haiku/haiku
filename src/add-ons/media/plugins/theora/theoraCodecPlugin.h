#ifndef _THEORA_CODEC_PLUGIN_H_
#define _THEORA_CODEC_PLUGIN_H_

#include "DecoderPlugin.h"

#include "libtheora/theora/theora.h"

class TheoraDecoder : public Decoder
{
public:
				TheoraDecoder();
				~TheoraDecoder();
	
	void		GetCodecInfo(media_codec_info *info);
	status_t	Setup(media_format *inputFormat,
					  const void *infoBuffer, int32 infoSize);

	status_t	NegotiateOutputFormat(media_format *ioDecodedFormat);

	status_t	Seek(uint32 seekTo,
					 int64 seekFrame, int64 *frame,
					 bigtime_t seekTime, bigtime_t *time);

							 
	status_t	Decode(void *buffer, int64 *frameCount,
					   media_header *mediaHeader, media_decode_info *info);
					   
private:
	theora_info      fInfo;
	theora_comment   fComment;
	theora_state     fState;

	bigtime_t		fStartTime;
	media_raw_video_format	fOutput;
};


class TheoraDecoderPlugin : public DecoderPlugin
{
public:
	Decoder *	NewDecoder(uint index);
	status_t	GetSupportedFormats(media_format ** formats, size_t * count);
};

#endif _THEORA_CODEC_PLUGIN_H_
