#ifndef _OGG_TRACK_H
#define _OGG_TRACK_H

#include <SupportDefs.h>
#include <MediaDefs.h>
#include "ogg/ogg.h"
#include <vector>

namespace BPrivate { namespace media {

class OggTrack {
protected:
	static bool			findIdentifier(const ogg_packet & packet,
						               const char * id, uint pos);
				OggTrack(long serialno);
public:
	virtual		~OggTrack();
	long		GetSerial() const;

	// push interface
	virtual status_t	AddPage(off_t position, const ogg_page & page) = 0;

	// interface for OggReader
	virtual status_t	GetStreamInfo(int64 *frameCount, bigtime_t *duration,
						              media_format *format);
	virtual status_t	Seek(uint32 seekTo, int64 *frame, bigtime_t *time);
	virtual status_t	GetNextChunk(void **chunkBuffer, int32 *chunkSize,
						             media_header *mediaHeader);

protected:
	// GetStreamInfo helpers
	void		SaveHeaderPacket(ogg_packet packet);
	const std::vector<ogg_packet> & GetHeaderPackets();

private:
	std::vector<ogg_packet> fHeaderPackets;
	long		fSerialno;
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif // _OGG_TRACK_H
