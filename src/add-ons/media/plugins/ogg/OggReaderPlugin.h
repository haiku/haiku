#ifndef _OGG_READER_PLUGIN_H
#define _OGG_READER_PLUGIN_H

#include "ReaderPlugin.h"
#include "ogg/ogg.h"

namespace BPrivate { namespace media {

class oggReader : public Reader
{
public:
				oggReader();
				~oggReader();
	
	const char *Copyright();
	
	status_t	Sniff(int32 *streamCount);

	void		GetFileFormatInfo(media_file_format *mff);

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
	BPositionIO *	fSeekableSource;
	ogg_stream_state fOggStreamState;
	ogg_sync_state	fOggSyncState;
};


class oggReaderPlugin : public ReaderPlugin
{
public:
	Reader *NewReader();
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif
