#ifndef _OGG_STREAM_H
#define _OGG_STREAM_H

#include <SupportDefs.h>
#include <MediaDefs.h>
#include "ogg/ogg.h"
#include "OggReaderPlugin.h"
#include "OggFrameInfo.h"
#include <map>
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

protected:
	int64				fCurrentFrame;
	bigtime_t			fCurrentTime;

private:
	long				fSerialno;
	std::vector<off_t>	fPagePositions;
	std::vector<OggFrameInfo> fOggFrameInfos;
	ogg_sync_state		fSync;
	BLocker				fSyncLock;
	ogg_stream_state	fSeekStreamState;
	uint				fCurrentPage;
	uint				fPacketOnCurrentPage;
	uint				fCurrentPacket;
	ogg_stream_state	fEndStreamState;
	uint				fEndPage;
	uint				fPacketOnEndPage;
	uint				fEndPacket;
	OggReader::GetPageInterface * fReaderInterface;
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif // _OGG_STREAM_H
