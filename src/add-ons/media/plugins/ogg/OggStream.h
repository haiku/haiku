#ifndef _OGG_STREAM_H
#define _OGG_STREAM_H

#include <SupportDefs.h>
#include <MediaDefs.h>
#include "ogg/ogg.h"
#include "OggReaderPlugin.h"
#include <vector>

namespace BPrivate { namespace media {

class OggStream {
public:
	static OggStream *	makeOggStream(OggReader::GetPageInterface * interface,
						              long serialno, const ogg_packet & packet);

protected:
	static bool			findIdentifier(const ogg_packet & packet,
						               const char * id, uint pos);
				OggStream(long serialno);
public:
	virtual		~OggStream();
	long		GetSerial() const;

	// reader push input function
	status_t	AddPage(off_t position, ogg_page * page);

	// reader pull output functions
	virtual	status_t	GetStreamInfo(int64 *frameCount, bigtime_t *duration,
						              media_format *format);
	virtual status_t	Seek(uint32 seekTo, int64 *frame, bigtime_t *time);
	virtual status_t	GetNextChunk(void **chunkBuffer, int32 *chunkSize,
						              media_header *mediaHeader);

protected:
	// subclass pull input function
	status_t	GetPacket(ogg_packet * packet);

	// GetStreamInfo helpers
	void		SaveHeaderPacket(ogg_packet packet);
	std::vector<ogg_packet> fHeaderPackets;

	// Seek helpers
	virtual int64		PositionToFrame(off_t position);
	virtual off_t		FrameToPosition(int64 frame);
	virtual bigtime_t	PositionToTime(off_t position);
	virtual off_t		TimeToPosition(bigtime_t time);
private:
	long				fSerialno;
	std::vector<off_t>	fPagePositions;
	ogg_stream_state	fStreamState;
	OggReader::GetPageInterface * fReaderInterface;
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif // _OGG_STREAM_H
