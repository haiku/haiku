#include "OggCodecs.h"
#include <MediaFormats.h>
#include <stdio.h>

#define TRACE_THIS 0
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...) ((void)0)
#endif

/**
 * Useful ogg_packet functions
 */

bool
BPrivate::media::findIdentifier(const ogg_packet & packet, const char * id, uint pos)
{
	TRACE("findIdentifier\n");
	uint length = strlen(id);
	if ((unsigned)packet.bytes < pos+length) {
		return false;
	}
	return !memcmp(&packet.packet[pos], id, length);
}


/**
 * OggCodec implementations
 */


OggCodec::OggCodec()
 : fCurrentFrame(-1),
   fCurrentTime(0),
   fOldFrame(-1),
   fOldGranulePos(-1),
   fInitCheck(B_NO_INIT),
   fPacketData(NULL)
{
}


/* virtual */
OggCodec::~OggCodec()
{
	delete fPacketData;
}


/* virtual */ bool
OggCodec::IsHeaderPacket(const ogg_packet & packet, uint packetno) const
{
	return (packetno == 0);
}


/* virtual */ status_t
OggCodec::HandlePacket(const ogg_packet & packet)
{
	TRACE("OggCodec::HandlePacket\n");
	if (!IsHeaderPacket(packet, (fInitCheck == B_OK ? 1 : 0))) {
		return B_BAD_VALUE;
	}
	if (!packet.b_o_s) {
		TRACE("OggCodec::HandlePacket failed : not beginning of stream\n");
		return B_ERROR; // first packet was not beginning of stream
	}

	// parse header packet
	uint32 four_bytes = *(uint32*)(&packet);

	// get the format for the description
	media_format_description description;
	description.family = B_MISC_FORMAT_FAMILY;
	description.u.misc.file_format = 'OggS';
	description.u.misc.codec = four_bytes;
	BMediaFormats formats;
	if (formats.InitCheck() == B_OK) {
		formats.GetFormatFor(description, &fMediaFormat);
	}

	// fill out format from header packet
	fMediaFormat.user_data_type = B_CODEC_TYPE_INFO;
	strncpy((char*)fMediaFormat.user_data, (char*)(&packet), 4);

	fPacketData = new unsigned char[packet.bytes];
	if (fPacketData == NULL) {
		return B_NO_MEMORY;
	}
	memcpy(fPacketData, packet.packet, packet.bytes);
	ogg_packet copy = packet;
	copy.packet = fPacketData;

	fMediaFormat.SetMetaData(&copy,sizeof(copy));
	fInitCheck = B_OK;
	return B_OK;
}


/* virtual */ status_t
OggCodec::GetFormat(media_format * format) const
{
	if (format == 0) {
		return B_BAD_VALUE;
	}
	if (fInitCheck != B_OK) {
		return fInitCheck;
	}
	*format = fMediaFormat;
	return B_OK;
}


/* virtual */ int64
OggCodec::GranulesToFrames(int64 granules) const
{
	return granules;
}


/* virtual */ int64
OggCodec::FramesToGranules(int64 frames) const
{
	return frames;
}


/* virtual */ status_t
OggCodec::GetChunk(ogg_stream_state * stream, bool streaming,
                   void **chunkBuffer, int32 *chunkSize,
                   media_header *mediaHeader)
{
	int result;
	while ((result = ogg_stream_packetout(stream, &fChunkPacket)) < 0) {
		// ignore previous packet fragments
	}
	if (result == 0) {
		return B_BUFFER_NOT_AVAILABLE;
	}
	*chunkBuffer = &fChunkPacket;
	*chunkSize = sizeof(fChunkPacket);
	mediaHeader->start_time = fCurrentTime;
	ogg_int64_t granulepos = fChunkPacket.granulepos;
	if (streaming) {
		if (granulepos < 0) {
			fCurrentFrame = -1;
		} else {
			fCurrentFrame = fOldFrame + GranulesToFrames(granulepos)
			                - GranulesToFrames(fOldGranulePos);
			fOldFrame = fCurrentFrame;
			fOldGranulePos = granulepos;
		}
	} else {
		debugger("explode");
	}
	// default (silly) 1 frame/sec
	fCurrentTime = 1000000LL * fCurrentFrame;
	return B_OK;
}


/**
 * OggCodecTest implementations
 */


OggCodecTest::OggCodecTest()
{
	TRACE("OggCodecTest::OggCodecTest\n");
	OggCodecRoster * roster = OggCodecRoster::Roster();
	if (roster != NULL) {
		roster->AddCodecTest(this);
	}
}


/* virtual */
OggCodecTest::~OggCodecTest()
{
	TRACE("OggCodecTest::~OggCodecTest\n");
}


/**
 * OggCodecRoster implementations
 */


/* static */ OggCodecRoster *
OggCodecRoster::Roster(status_t * outError)
{
	TRACE("OggCodecRoster::Roster\n");
	static OggCodecRoster * ogg_codec_roster = NULL;
	if (ogg_codec_roster == NULL) {
		ogg_codec_roster = new OggCodecRoster();
		if ((ogg_codec_roster == NULL) && (outError != NULL)) {
			*outError = B_NO_MEMORY;
		}
	}
	return ogg_codec_roster;
}


status_t
OggCodecRoster::AddCodecTest(const OggCodecTest * test)
{
	TRACE("OggCodecRoster::AddCodecTest\n");
	fCodecs.push_back(test);
	return B_OK;
}


OggCodec *
OggCodecRoster::CodecFor(const ogg_packet & packet) const
{
	TRACE("OggCodecRoster::CodecFor\n");
	std::vector<const OggCodecTest *>::const_iterator iter;
	for(iter = fCodecs.begin() ; iter != fCodecs.end() ; iter++) {
		const OggCodecTest * test = *iter;
		if (test->RecognizesInitialPacket(packet)) {
			return test->InstantiateCodec();
		}
	}
	return NULL;
}


OggCodecRoster::OggCodecRoster()
{
	TRACE("OggCodecRoster::OggCodecRoster\n");
}


OggCodecRoster::~OggCodecRoster()
{
	TRACE("OggCodecRoster::~OggCodecRoster\n");
}
