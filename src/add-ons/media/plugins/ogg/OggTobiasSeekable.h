#ifndef _OGG_TOBIAS_SEEKABLE_H
#define _OGG_TOBIAS_SEEKABLE_H

#include "OggSeekable.h"

namespace BPrivate { namespace media {

class OggTobiasSeekable : public OggSeekable {
public:
	static bool	IsValidHeader(const ogg_packet & packet);
public:
				OggTobiasSeekable(long serialno);
	virtual		~OggTobiasSeekable();

	virtual	status_t	GetStreamInfo(int64 *frameCount, bigtime_t *duration,
						              media_format *format);
	virtual status_t	GetNextChunk(void **chunkBuffer, int32 *chunkSize,
						              media_header *mediaHeader);

private:
	media_format	fMediaFormat;
	double			fMicrosecPerFrame;
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif // _OGG_TOBIAS_SEEKABLE_H
