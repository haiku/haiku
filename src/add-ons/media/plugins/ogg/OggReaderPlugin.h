#ifndef _OGG_READER_PLUGIN_H
#define _OGG_READER_PLUGIN_H

#include "ReaderPlugin.h"
#include "ogg/ogg.h"
#include <map>

namespace BPrivate { namespace media {

typedef std::map<int,ogg_stream_state*> ogg_stream_map;
typedef std::map<int,ogg_packet> ogg_packet_map;

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
									 
private:
	status_t	GetPage(ogg_page * page, int read_size = 4*B_PAGE_SIZE,
				        bool short_page = false);

	BPositionIO *	fSeekable;
	off_t			fPostSniffPosition;
	
	ogg_sync_state	fSync;
	ogg_packet_map	fInitialHeaderPackets;
	ogg_stream_map	fStreams;
	ogg_packet_map	fPackets;
};


class oggReaderPlugin : public ReaderPlugin
{
public:
	Reader *NewReader();
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif
