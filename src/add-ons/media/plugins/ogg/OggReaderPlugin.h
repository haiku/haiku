#ifndef _OGG_READER_PLUGIN_H
#define _OGG_READER_PLUGIN_H

#include "ReaderPlugin.h"
#include "ogg/ogg.h"
#include <map>
#include <vector>

namespace BPrivate { namespace media {

class OggTrack;

typedef std::map<long,OggTrack*> serialno_OggTrack_map;
typedef std::vector<long> serialno_vector;

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

	// delegated to OggTrack	
	status_t	GetStreamInfo(void *cookie, int64 *frameCount, bigtime_t *duration,
							  media_format *format, void **infoBuffer, int32 *infoSize);
	status_t	Seek(void *cookie, uint32 seekTo, int64 *frame, bigtime_t *time);
	status_t	GetNextChunk(void *cookie, void **chunkBuffer, int32 *chunkSize,
							 media_header *mediaHeader);
									 
private:
	status_t	FindLastPages();

	ogg_sync_state			fSync;
	BLocker					fSyncLock;
	serialno_OggTrack_map	fTracks;
	serialno_vector			fCookies;

	BPositionIO *			fSeekable;
	BLocker					fSeekableLock;
	BFile *					fFile;

	off_t					fPosition;

	// interface for OggStreams
	ssize_t		ReadPage(bool first_page = false);

	class StreamInterface {
	public:
		virtual ssize_t		ReadPage() = 0;
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
