#ifndef _OGG_READER_PLUGIN_H
#define _OGG_READER_PLUGIN_H

#include "ReaderPlugin.h"
#include "ogg/ogg.h"
#include <map>
#include <vector>

namespace BPrivate { namespace media {

class OggStream;

typedef std::map<long,OggStream*> serialno_OggStream_map;
typedef std::vector<OggStream*> OggStream_vector;

class OggReader : public Reader
{
public:
				OggReader();
				~OggReader();
	
	const char *Copyright();
	
	status_t	Sniff(int32 *streamCount);

	void		GetFileFormatInfo(media_file_format *mff);

	status_t	AllocateCookie(int32 streamNumber, void **cookie);
	status_t	FreeCookie(void *cookie);

	// delegated to OggStream	
	status_t	GetStreamInfo(void *cookie, int64 *frameCount, bigtime_t *duration,
							  media_format *format, void **infoBuffer, int32 *infoSize);
	status_t	Seek(void *cookie,
					 uint32 seekTo,
					 int64 *frame, bigtime_t *time);
	status_t	GetNextChunk(void *cookie,
							 void **chunkBuffer, int32 *chunkSize,
							 media_header *mediaHeader);
									 
protected:
	status_t	GetPage(ogg_page * page, int read_size = 4*B_PAGE_SIZE,
				        bool short_page = false);

	ogg_sync_state			fSync;
	serialno_OggStream_map	fStreams;
	OggStream_vector		fCookies;
	BPositionIO *			fSeekable;

private:
	class GetPageInterface {
	public:
		virtual status_t	GetNextPage() = 0;
	};
};

class OggReaderPlugin : public ReaderPlugin
{
public:
	Reader *NewReader();
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif
