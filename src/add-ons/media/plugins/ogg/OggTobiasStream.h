#ifndef _OGG_TOBIAS_STREAM_H
#define _OGG_TOBIAS_STREAM_H

#include "OggStream.h"

namespace BPrivate { namespace media {

class OggTobiasStream : public OggStream {
public:
	static bool	IsValidHeader(const ogg_packet & packet);
public:
				OggTobiasStream(long serialno);
	virtual		~OggTobiasStream();

	virtual	status_t	GetStreamInfo(int64 *frameCount, bigtime_t *duration,
						              media_format *format);
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif // _OGG_TOBIAS_STREAM_H
