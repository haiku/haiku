#include <MediaTrack.h>
#include <MediaFormats.h>
#include "MediaPlugin.h"
#include "ReaderPlugin.h"

namespace BPrivate { namespace media {

class Decoder
{
public:
						Decoder();
	virtual				~Decoder();
	
						// Setup get's called with the info data from Reader::GetStreamInfo
	virtual status_t	Setup(media_format *ioEncodedFormat, media_format *ioDecodedFormat,
							  const void *infoBuffer, int32 infoSize) = 0;
	
	virtual status_t	Seek(uint32 seekTo,
							 int64 seekFrame, int64 *frame,
							 bigtime_t seekTime, bigtime_t *time) = 0;
							 
	virtual status_t	Decode(void *buffer, int64 *frameCount,
							   media_header *mediaHeader, media_decode_info *info = 0) = 0;
							   
	status_t			GetNextChunk(void **chunkBuffer, int32 *chunkSize,
									 media_header *mediaHeader);

private:
	void				Setup(Reader *reader, void *readerCookie);
	Reader *			fReader;
	void *				fReaderCookie;
};


class DecoderPlugin : public MediaPlugin
{
public:
	DecoderPlugin();

	virtual Decoder *NewDecoder() = 0;
	
	status_t PublishDecoder(const char *short_name, const char *pretty_name, const media_format_description &fmt_desc, media_type fmt_type);
	status_t PublishDecoder(const char *short_name, const char *pretty_name, const media_format &fmt);
};

} } // namespace BPrivate::media

using namespace BPrivate::media;
