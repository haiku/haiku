#ifndef _OGG_VORBIS_SEEKABLE_H
#define _OGG_VORBIS_SEEKABLE_H

#include "OggSeekable.h"

namespace BPrivate { namespace media {

class OggVorbisSeekable : public OggSeekable {
public:
	static bool	IsValidHeader(const ogg_packet & packet);
public:
				OggVorbisSeekable(long serialno);
	virtual		~OggVorbisSeekable();

	virtual	status_t	GetStreamInfo(int64 *frameCount, bigtime_t *duration,
						              media_format *format);
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif // _OGG_VORBIS_SEEKABLE_H
