#include "OggTobiasFormats.h"
#include "OggCodecs.h"
#include <stdio.h>

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...) ((void)0)
#endif


/*
 * tobias header structs from http://tobias.everwicked.com/packfmt.htm
 */


typedef struct tobias_stream_header_video
{
	ogg_int32_t width;
	ogg_int32_t height;
} tobias_stream_header_video;


typedef struct tobias_stream_header_audio
{
	ogg_int16_t channels;
	ogg_int16_t blockalign;
	ogg_int32_t avgbytespersec;
} tobias_stream_header_audio;


typedef struct tobias_stream_header
{
	char streamtype[8];
	char subtype[4];

	ogg_int32_t size; // size of the structure

	ogg_int64_t time_unit; // in reference time (100 ns units)
	ogg_int64_t samples_per_unit;
	ogg_int32_t default_len; // in media time

	ogg_int32_t buffersize;
	ogg_int16_t bits_per_sample;

	union {
		// Video specific
		tobias_stream_header_video video;
		// Audio specific
		tobias_stream_header_audio audio;
	};
} tobias_stream_header;


/*
static int64
count_granules(const ogg_packet & packet)
{
	// thanks to Marcus:
	int lenbytes = ((packet.packet[0] & 0xc0) >> 6) | ((packet.packet[0] & 0x02) << 1);
	if (lenbytes == 0) {
		return 1;
	}
	int64 granules = 0;
	int count = 0;
//	fprintf(stderr, "lenbytes = %d, ", lenbytes);
	while (lenbytes-- > 0) {
		granules += ((uint8*)packet.packet)[count+1] * (1LL << 8 * count);
		count++;
	}
//	fprintf(stderr, "granules = %lld\n", granules);
	return granules;
}
*/


/*
 * OggTobiasCodec
 */


class OggTobiasCodec : public OggCodec {
public:
			OggTobiasCodec() {}
	virtual	~OggTobiasCodec() {}

	virtual bool		IsHeaderPacket(const ogg_packet & packet, uint packetno) const;
	virtual status_t	HandlePacket(const ogg_packet & packet);

protected:
	virtual status_t	HeaderToFormat(const tobias_stream_header & header) = 0;

	std::vector<ogg_packet>	fMetaDataPackets;
	unsigned char *			fHeaderPacketData;
	unsigned char *			fCommentPacketData;
	unsigned int			fPacketCount;
};


/* virtual */ bool
OggTobiasCodec::IsHeaderPacket(const ogg_packet & packet, uint packetno) const
{
	oggpack_buffer opb;
	oggpack_readinit(&opb, packet.packet, packet.bytes);
	uint packtype = oggpack_read(&opb, 8);
	return (packetno == 0 && packtype == 0x01)
	    || (packetno == 1 && packtype == 0x03);
}


/* virtual */ status_t
OggTobiasCodec::HandlePacket(const ogg_packet & packet)
{
	TRACE("OggTobiasCodec::HandlePacket\n");
	if (!IsHeaderPacket(packet, fPacketCount)) {
		return B_ERROR;
	}
	switch (fPacketCount) {
	case 0: {
		if (!packet.b_o_s) {
			return B_ERROR; // first packet was not beginning of stream
		}
	
		// parse header packet
		if (packet.bytes < 1+(signed)sizeof(tobias_stream_header)) {
			return B_ERROR;
		}
		void * data = &(packet.packet[1]);
		tobias_stream_header * header = (tobias_stream_header *)data;

		status_t result = HeaderToFormat(*header);
		if (result != B_OK) {
			return result;
		}

		// initialize codec info
		fMediaFormat.user_data_type = B_CODEC_TYPE_INFO;
		strncpy((char*)fMediaFormat.user_data, header->subtype, 4);

		// save the header packet for use in meta data
		fHeaderPacketData = new unsigned char[packet.bytes];
		memcpy(fHeaderPacketData, packet.packet, packet.bytes);
		fMetaDataPackets.push_back(packet);
		fMetaDataPackets[fPacketCount].packet = fHeaderPacketData;
		break;
	}
	case 1: {
		// save the comment packet for use in meta data
		fCommentPacketData = new unsigned char[packet.bytes];
		memcpy(fCommentPacketData, packet.packet, packet.bytes);
		fMetaDataPackets.push_back(packet);
		fMetaDataPackets[fPacketCount].packet = fCommentPacketData;

		// ready for showtime
		fInitCheck = B_OK;
		fMediaFormat.SetMetaData(&fMetaDataPackets,sizeof(fMetaDataPackets));
		break;
	}
	default:
		// huh?
		break;
	}
	fPacketCount++;
	return B_OK;
}


/*
 * OggTobiasVideoCodec
 */


class OggTobiasVideoCodec : public OggTobiasCodec {
public:
			OggTobiasVideoCodec();
	virtual	~OggTobiasVideoCodec();

	virtual status_t	GetChunk(ogg_stream_state * stream, bool streaming,
						         void **chunkBuffer, int32 *chunkSize,
						         media_header *mediaHeader);

protected:
	virtual status_t	HeaderToFormat(const tobias_stream_header & header);
};


OggTobiasVideoCodec::OggTobiasVideoCodec()
{
	TRACE("OggTobiasVideoCodec::OggTobiasVideoCodec\n");
}


OggTobiasVideoCodec::~OggTobiasVideoCodec()
{
	TRACE("OggTobiasVideoCodec::~OggTobiasVideoCodec\n");
}


/* virtual */ status_t
OggTobiasVideoCodec::HeaderToFormat(const tobias_stream_header & header)
{
	TRACE("OggTobiasVideoCodec::HeaderToFormat\n");
	
	// get the format for the description
	media_format_description description = tobias_video_description();
	description.u.avi.codec = header.subtype[3] << 24 | header.subtype[2] << 16 
	                        | header.subtype[1] <<  8 | header.subtype[0];
	BMediaFormats formats;
	if ((formats.InitCheck() != B_OK) ||
	    (formats.GetFormatFor(description, &fMediaFormat) != B_OK)) {
		fMediaFormat = tobias_video_encoded_media_format();
	}

	// fill out format from header packet
	fMediaFormat.u.encoded_video.frame_size
	   = header.video.width * header.video.height;
	fMediaFormat.u.encoded_video.output.field_rate = 10000000.0 / header.time_unit;
	fMediaFormat.u.encoded_video.output.interlace = 1;
	fMediaFormat.u.encoded_video.output.first_active = 0;
	fMediaFormat.u.encoded_video.output.last_active = header.video.height - 1;
	fMediaFormat.u.encoded_video.output.orientation = B_VIDEO_TOP_LEFT_RIGHT;
	fMediaFormat.u.encoded_video.output.pixel_width_aspect = 1;
	fMediaFormat.u.encoded_video.output.pixel_height_aspect = 1;
	fMediaFormat.u.encoded_video.output.display.line_width = header.video.width;
	fMediaFormat.u.encoded_video.output.display.line_count = header.video.height;
	fMediaFormat.u.encoded_video.output.display.bytes_per_row = 0;
	fMediaFormat.u.encoded_video.output.display.pixel_offset = 0;
	fMediaFormat.u.encoded_video.output.display.line_offset = 0;
	fMediaFormat.u.encoded_video.output.display.flags = 0;

	// TODO: wring more info out of the headers?
	return B_OK;
}


/* virtual */ status_t
OggTobiasVideoCodec::GetChunk(ogg_stream_state * stream, bool streaming,
                              void **chunkBuffer, int32 *chunkSize,
                              media_header *mediaHeader)
{
	status_t result = OggCodec::GetChunk(stream, streaming, chunkBuffer, chunkSize, mediaHeader);
	if (result != B_OK) {
		return result;
	}
	if (streaming) {
		if (fChunkPacket.granulepos == -1) {
			int new_granulepos = fOldGranulePos + 1;
			fCurrentFrame = fOldFrame + GranulesToFrames(new_granulepos)
			                - GranulesToFrames(fOldGranulePos);
			fOldFrame = fCurrentFrame;
			fOldGranulePos = new_granulepos;
		} else {
//			fprintf(stderr, "granulepos: %lld  ", fChunkPacket.granulepos);
//			fprintf(stderr, "out: frame = %lld\n", fCurrentFrame);
		}
	} else {
		debugger("kaboom");
	}
	fCurrentTime = (bigtime_t) ((1000000LL * fCurrentFrame)
	             / (fMediaFormat.u.encoded_video.output.field_rate * 
	                fMediaFormat.u.encoded_video.output.interlace));
//	fprintf(stderr, "video in: frame = %lld  time = %lld\n", fCurrentFrame, fCurrentTime);
	int lenbytes = ((fChunkPacket.packet[0] & 0xc0) >> 6) | ((fChunkPacket.packet[0] & 0x02) << 1);
	*chunkBuffer = (void*)&(fChunkPacket.packet[lenbytes+1]);
	*chunkSize = fChunkPacket.bytes-(lenbytes+1);
	bool keyframe = (fChunkPacket.packet[0] & (1 << 3));
	mediaHeader->u.encoded_video.field_flags = (keyframe ? B_MEDIA_KEY_FRAME : 0);
	return result;
}


// OggTobiasVideoCodecTest

static class OggTobiasVideoCodecTest : public OggCodecTest {
public:
			OggTobiasVideoCodecTest() {}
	virtual	~OggTobiasVideoCodecTest() {}
	virtual	bool		RecognizesInitialPacket(const ogg_packet & packet) const
	{
		return findIdentifier(packet, "video", 1);
	}
	virtual	OggCodec *	InstantiateCodec() const
	{
		return new OggTobiasVideoCodec();
	}
} ogg_tobias_video_test;


/*
 * OggTobiasAudioCodec
 */


class OggTobiasAudioCodec : public OggTobiasCodec {
public:
			OggTobiasAudioCodec();
	virtual	~OggTobiasAudioCodec();

	virtual status_t	GetChunk(ogg_stream_state * stream, bool streaming,
						         void **chunkBuffer, int32 *chunkSize,
						         media_header *mediaHeader);

protected:
	virtual status_t	HeaderToFormat(const tobias_stream_header & header);
};


OggTobiasAudioCodec::OggTobiasAudioCodec()
{
	TRACE("OggTobiasAudioCodec::OggTobiasAudioCodec\n");
}


OggTobiasAudioCodec::~OggTobiasAudioCodec()
{
	TRACE("OggTobiasAudioCodec::~OggTobiasAudioCodec\n");
}


/* virtual */ status_t
OggTobiasAudioCodec::HeaderToFormat(const tobias_stream_header & header)
{
	TRACE("OggTobiasAudioCodec::HeaderToFormat\n");
	
	debugger("get_audio_format");

	return B_OK;
}


/* virtual */ status_t
OggTobiasAudioCodec::GetChunk(ogg_stream_state * stream, bool streaming,
                              void **chunkBuffer, int32 *chunkSize,
                              media_header *mediaHeader)
{
	if (fCurrentFrame == -1) {
		fCurrentFrame = fOldFrame + 1;
		fOldFrame = fCurrentFrame;
		fOldGranulePos++;
	}
	fCurrentTime = (bigtime_t) (1000000LL * fCurrentFrame)
	             / (long long)fMediaFormat.u.encoded_audio.output.frame_rate;
	status_t result = OggCodec::GetChunk(stream, streaming, chunkBuffer, chunkSize, mediaHeader);
	if (result != B_OK) {
		return result;
	}
	int lenbytes = ((fChunkPacket.packet[0] & 0xc0) >> 6) | ((fChunkPacket.packet[0] & 0x02) << 1);
	*chunkBuffer = (void*)&(fChunkPacket.packet[lenbytes+1]);
	*chunkSize = fChunkPacket.bytes-(lenbytes+1);
	return result;
}


// OggTobiasAudioCodecTest

static class OggTobiasAudioCodecTest : public OggCodecTest {
public:
			OggTobiasAudioCodecTest() {}
	virtual	~OggTobiasAudioCodecTest() {}
	virtual	bool		RecognizesInitialPacket(const ogg_packet & packet) const
	{
		return findIdentifier(packet, "audio", 1);
	}
	virtual	OggCodec *	InstantiateCodec() const
	{
		return new OggTobiasAudioCodec();
	}
} ogg_tobias_audio_test;


/*
 * OggTobiasTextCodec
 */

class OggTobiasTextCodec : public OggTobiasCodec {
public:
			OggTobiasTextCodec();
	virtual	~OggTobiasTextCodec();

	virtual status_t	GetChunk(ogg_stream_state * stream, bool streaming,
						         void **chunkBuffer, int32 *chunkSize,
						         media_header *mediaHeader);

protected:
	virtual status_t	HeaderToFormat(const tobias_stream_header & header);
};


OggTobiasTextCodec::OggTobiasTextCodec()
{
	TRACE("OggTobiasTextCodec::OggTobiasTextCodec\n");
}


OggTobiasTextCodec::~OggTobiasTextCodec()
{
	TRACE("OggTobiasTextCodec::~OggTobiasTextCodec\n");
}



/* virtual */ status_t
OggTobiasTextCodec::HeaderToFormat(const tobias_stream_header & header)
{
	TRACE("OggTobiasTextCodec::HeaderToFormat\n");
	
	// get the format for the description
	media_format_description description = tobias_text_description();
	BMediaFormats formats;
	if ((formats.InitCheck() != B_OK) ||
	    (formats.GetFormatFor(description, &fMediaFormat) != B_OK)) {
		fMediaFormat = tobias_text_encoded_media_format();
	}

	// fill out format from header packet
	(void)header;

	return B_OK;
}


/* virtual */ status_t
OggTobiasTextCodec::GetChunk(ogg_stream_state * stream, bool streaming,
                             void **chunkBuffer, int32 *chunkSize,
                             media_header *mediaHeader)
{
	if (fCurrentFrame == -1) {
		fCurrentFrame = fOldFrame + 1;
		fOldFrame = fCurrentFrame;
		fOldGranulePos++;
	}
	fCurrentTime = (bigtime_t) (1000000LL * fCurrentFrame)
	             / (long long)fMediaFormat.u.encoded_audio.output.frame_rate;
	status_t result = OggCodec::GetChunk(stream, streaming, chunkBuffer, chunkSize, mediaHeader);
	if (result != B_OK) {
		return result;
	}
	// thanks to Marcus:
	int lenbytes = ((fChunkPacket.packet[0] & 0xc0) >> 6) | ((fChunkPacket.packet[0] & 0x02) << 1);
	*chunkBuffer = (void*)&(fChunkPacket.packet[lenbytes+1]);
	*chunkSize = fChunkPacket.bytes-(lenbytes+1);
	bool keyframe = (fChunkPacket.packet[0] & (1 << 3));
	mediaHeader->u.encoded_video.field_flags = (keyframe ? B_MEDIA_KEY_FRAME : 0);
	return result;
}


// OggTobiasTextCodecTest

static class OggTobiasTextCodecTest : public OggCodecTest {
public:
			OggTobiasTextCodecTest() {}
	virtual	~OggTobiasTextCodecTest() {}
	virtual	bool		RecognizesInitialPacket(const ogg_packet & packet) const
	{
		return findIdentifier(packet,"text",1);
	}
	virtual	OggCodec *	InstantiateCodec() const
	{
		return new OggTobiasTextCodec();
	}
} ogg_tobias_text_test;


