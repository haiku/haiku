#ifndef _OGG_THEORA_STREAM_H
#define _OGG_THEORA_STREAM_H

#include "OggStream.h"

namespace BPrivate { namespace media {

class OggTheoraStream : public OggStream {
public:
	static bool	IsValidHeader(const ogg_packet & packet);
public:
				OggTheoraStream(long serialno);
	virtual		~OggTheoraStream();

	virtual	status_t	GetStreamInfo(int64 *frameCount, bigtime_t *duration,
						              media_format *format);
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif // _OGG_THEORA_STREAM_H
