#ifndef _OGG_VORBIS_STREAM_H
#define _OGG_VORBIS_STREAM_H

#include "OggStream.h"

namespace BPrivate { namespace media {

class OggVorbisStream : public OggStream {
public:
	static bool	IsValidHeader(const ogg_packet & packet);
public:
				OggVorbisStream(long serialno);
	virtual		~OggVorbisStream();

	virtual	status_t	GetStreamInfo(int64 *frameCount, bigtime_t *duration,
						              media_format *format);
private:
	int64		fFrameCount;
	bigtime_t	fDuration;
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif // _OGG_VORBIS_STREAM_H
