#ifndef _OGG_SEEKABLE_H
#define _OGG_SEEKABLE_H

#include "OggTrack.h"
#include "OggReaderPlugin.h"
#include <map>

namespace BPrivate { namespace media {

class OggSeekable : public OggTrack {
public:
	static OggSeekable * makeOggSeekable(OggReader::SeekableInterface * interface,
						                 long serialno, const ogg_packet & packet);

	// interface for OggReader
	virtual status_t	GetStreamInfo(int64 *frameCount, bigtime_t *duration,
						              media_format *format);
	virtual status_t	GetNextChunk(void **chunkBuffer, int32 *chunkSize,
						             media_header *mediaHeader);

protected:
				OggSeekable(long serialno);
public:
	virtual		~OggSeekable();

	// reader push input function
	status_t	AddPage(off_t position, const ogg_page & page);

protected:
	// subclass pull input function
	status_t	GetPacket(ogg_packet * packet);

private:
	ogg_sync_state		fSync;
	BLocker				fSyncLock;
	ogg_stream_state	fStreamState;
	OggReader::SeekableInterface * fReaderInterface;
	ogg_packet			fChunkPacket;
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif // _OGG_SEEKABLE_H
