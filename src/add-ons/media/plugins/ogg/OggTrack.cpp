#include "OggTrack.h"
#include <stdio.h>

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...) ((void)0)
#endif

/* static */ bool
OggTrack::findIdentifier(const ogg_packet & packet, const char * id, uint pos)
{
	uint length = strlen(id);
	if ((unsigned)packet.bytes < pos+length) {
		return false;
	}
	return !memcmp(&packet.packet[pos], id, length);
}


OggTrack::OggTrack(long serialno)
{
	TRACE("OggTrack::OggTrack\n");
	fSerialno = serialno;
}


OggTrack::~OggTrack()
{
	// free internal header packet storage
	std::vector<ogg_packet>::iterator iter = fHeaderPackets.begin();
	while (iter != fHeaderPackets.end()) {
		delete iter->packet;
		iter++;
	}
}


long
OggTrack::GetSerial() const
{
	return fSerialno;
}


status_t
OggTrack::GetStreamInfo(int64 *frameCount, bigtime_t *duration,
              media_format *format)
{
	*frameCount = 0;
	*duration = 0;
	media_format f;
	*format = f;
	return B_UNSUPPORTED;
}


status_t
OggTrack::Seek(uint32 seekTo, int64 *frame, bigtime_t *time)
{
	*frame = 0;
	*time = 0;
	return B_UNSUPPORTED;
}


status_t
OggTrack::GetNextChunk(void **chunkBuffer, int32 *chunkSize,
             media_header *mediaHeader)
{
	*chunkBuffer = 0;
	*chunkSize = 0;
	media_header mh;
	*mediaHeader = mh;
	return B_UNSUPPORTED;
}


// GetStreamInfo helpers
void
OggTrack::SaveHeaderPacket(ogg_packet packet)
{
	uint8 * buffer = new uint8[packet.bytes];
	memcpy(buffer, packet.packet, packet.bytes);
	packet.packet = buffer;
	fHeaderPackets.push_back(packet);
}


const std::vector<ogg_packet> &
OggTrack::GetHeaderPackets()
{
	return fHeaderPackets;
}
