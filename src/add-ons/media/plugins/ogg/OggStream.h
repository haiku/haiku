#ifndef _OGG_STREAM_H
#define _OGG_STREAM_H

#include "OggTrack.h"
#include "OggReaderPlugin.h"
#include <map>

namespace BPrivate { namespace media {

class OggStream : public OggTrack {
public:
	static OggStream *	makeOggStream(OggReader::StreamInterface * interface,
						              long serialno, const ogg_packet & packet);

	// interface for OggReader
	virtual status_t	GetStreamInfo(int64 *frameCount, bigtime_t *duration,
						              media_format *format);
	virtual status_t	GetNextChunk(void **chunkBuffer, int32 *chunkSize,
						             media_header *mediaHeader);

protected:
				OggStream(long serialno);
public:
	virtual		~OggStream();

	// reader push input function
	virtual status_t	AddPage(off_t position, const ogg_page & page);

protected:
	// subclass pull input function
	status_t	GetPacket(ogg_packet * packet);
	ogg_packet			fChunkPacket;

protected:
	int64				fCurrentFrame;
	bigtime_t			fCurrentTime;

private:
	ogg_sync_state		fSync;
	BLocker				fSyncLock;
	ogg_stream_state	fStreamState;
	OggReader::StreamInterface * fReaderInterface;
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif // _OGG_STREAM_H
