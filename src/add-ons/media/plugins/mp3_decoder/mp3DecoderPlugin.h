#include "DecoderPlugin.h"

#include "mpglib/mpglib.h"

class mp3Decoder : public Decoder
{
public:
				mp3Decoder();
				~mp3Decoder();
	
	void		GetCodecInfo(media_codec_info *codecInfo);

	status_t	Setup(media_format *ioEncodedFormat,
					  const void *infoBuffer, int32 infoSize);

	status_t	NegotiateOutputFormat(media_format *ioDecodedFormat);

	status_t	Seek(uint32 seekTo,
					 int64 seekFrame, int64 *frame,
					 bigtime_t seekTime, bigtime_t *time);

							 
	status_t	Decode(void *buffer, int64 *frameCount,
					   media_header *mediaHeader, media_decode_info *info);
					   
private:
	status_t	DecodeNextChunk();
	bool		IsValidStream(uint8 *buffer, int size);
	int			GetFrameLength(void *header);	
	
private:
	struct mpstr	fMpgLibPrivate;
	int32			fResidualBytes;
	uint8 *			fResidualBuffer;
	uint8 *			fDecodeBuffer;
	bigtime_t		fStartTime;
	int				fFrameSize;
	int				fFrameRate;
	int				fBitRate;
	int				fChannelCount;
	int				fOutputBufferSize;
	bool			fNeedSync;
};


class mp3DecoderPlugin : public DecoderPlugin
{
public:
	Decoder *	NewDecoder(uint index);
	status_t	GetSupportedFormats(media_format ** formats, size_t * count);
};
