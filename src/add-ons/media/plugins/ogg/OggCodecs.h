#ifndef _OGG_CODECS_H
#define _OGG_CODECS_H

#include <SupportDefs.h>
#include <MediaDefs.h>
#include "ogg/ogg.h"
#include <vector>

namespace BPrivate { namespace media {


/**
 * Useful ogg_packet functions
 */

bool		findIdentifier(const ogg_packet & packet, const char * id, uint pos);

/**
 * OggCodec
 *
 * An OggCodec is instantiated to assist an OggTrack with interacting with the codecs.
 * It helps with seeking, timing, and transforming the ogg_packets into chunks.
 */
class OggCodec {
public:
			OggCodec();
	virtual	~OggCodec();

	virtual bool		IsHeaderPacket(const ogg_packet & packet, uint packetno) const;
	virtual status_t	HandlePacket(const ogg_packet & packet);
	virtual status_t	GetFormat(media_format * format) const;
	virtual int64		GranulesToFrames(int64 granules) const;
	virtual int64		FramesToGranules(int64 frames) const;
	virtual status_t	GetChunk(ogg_stream_state * stream, bool streaming,
						         void **chunkBuffer, int32 *chunkSize,
						         media_header *mediaHeader);

protected:
	ogg_packet		fChunkPacket;
	int64			fCurrentFrame;
	bigtime_t		fCurrentTime;
	int64			fOldFrame;
	ogg_int64_t		fOldGranulePos;
	media_format	fMediaFormat;
	status_t		fInitCheck;

private:
	unsigned char * fPacketData;
};


/**
 * OggCodecTest
 *
 * An OggCodecTest is capable of determining if an initial packet belongs to a
 * particular codec.  It can instantiate that particular codec.
 */
class OggCodecTest {
protected:
			OggCodecTest();
public:
	virtual	~OggCodecTest();
	virtual	bool		RecognizesInitialPacket(const ogg_packet & packet) const = 0;
	virtual	OggCodec *	InstantiateCodec() const = 0;
};


/**
 * OggCodecRoster
 *
 * An OggCodecRoster provides a single location to register OggCodecTests
 * and retrieve an appropriate OggCodec for an ogg_packet.
 */
class OggCodecRoster {
public:
static OggCodecRoster *	Roster(status_t * outError = NULL);

	status_t	AddCodecTest(const OggCodecTest * wrapper);
	OggCodec * 	CodecFor(const ogg_packet & packet) const;

private:
	OggCodecRoster();
	~OggCodecRoster();

	std::vector<const OggCodecTest *> fCodecs;
};


} } // BPrivate::media

using namespace BPrivate::media;

#endif // _OGG_CODECS_H
