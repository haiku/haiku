#ifndef _OGG_SPEEX_SEEKABLE_H
#define _OGG_SPEEX_SEEKABLE_H

#include "OggSeekable.h"

namespace BPrivate { namespace media {

class OggSpeexSeekable : public OggSeekable {
private:
	typedef OggSeekable inherited;
public:
	static bool	IsValidHeader(const ogg_packet & packet);
public:
				OggSpeexSeekable(long serialno);
	virtual		~OggSpeexSeekable();

	virtual	status_t	GetStreamInfo(int64 *frameCount, bigtime_t *duration,
						              media_format *format);
	virtual status_t	GetNextChunk(void **chunkBuffer, int32 *chunkSize,
						             media_header *mediaHeader);

};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif // _OGG_SPEEX_SEEKABLE_H
