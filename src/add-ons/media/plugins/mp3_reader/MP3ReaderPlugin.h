#ifndef _MP3_READER_PLUGIN_H
#define _MP3_READER_PLUGIN_H

#include "ReaderPlugin.h"

namespace BPrivate { namespace media {

class mp3Reader : public Reader
{
public:
				mp3Reader();
				~mp3Reader();
	
	const char *Copyright();
	
	status_t	Sniff(int32 *streamCount);

	status_t	AllocateCookie(int32 streamNumber, void **cookie);
	status_t	FreeCookie(void *cookie);
	
	status_t	GetStreamInfo(void *cookie, int64 *frameCount, bigtime_t *duration,
							  media_format *format, void **infoBuffer, int32 *infoSize);

	status_t	Seek(void *cookie,
					 uint32 seekTo,
					 int64 *frame, bigtime_t *time);

	status_t	GetNextChunk(void *cookie,
							 void **chunkBuffer, int32 *chunkSize,
							 media_header *mediaHeader);
									 
	BPositionIO *Source() { return fSeekableSource; }

private:
	bool 		IsMp3File();
	int			GetFrameLength(void *header);
	
	bool		FindData();
	
	
private:
	BPositionIO *	fSeekableSource;
	int64			fFileSize;
};


class mp3ReaderPlugin : public ReaderPlugin
{
public:
	Reader *NewReader();
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif
