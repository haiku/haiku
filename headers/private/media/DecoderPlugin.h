#include <MediaTrack.h>
#include "MediaPlugin.h"

namespace BPrivate { namespace media {

class Decoder
{
public:
						Decoder();
	virtual				~Decoder();
	
						// Sniff get's called with the data from Reader::GetStreamInfo
	virtual status_t	Sniff(media_format *format, void **infoBuffer, int32 *infoSize) = 0;
	
	virtual status_t	GetOutputFormat(media_format *format) = 0;

	virtual status_t	Seek(media_seek_type seekTo,
							 int64 *frame, bigtime_t *time) = 0;
							 
	virtual status_t	Decode(void *buffer, int64 *frameCount,
							   media_header *mediaHeader, media_decode_info *info) = 0;
							   
	status_t			GetNextChunk(void **chunkBuffer, int32 *chunkSize,
									 media_header *mediaHeader);

private:
	void				Setup(BMediaTrack *reader);
	BMediaTrack *		fReader;
};


class DecoderPlugin : public MediaPlugin
{
public:
	virtual Decoder *NewDecoder() = 0;
};

MediaPlugin *instantiate_plugin();

} } // namespace BPrivate::media

using namespace BPrivate::media;

