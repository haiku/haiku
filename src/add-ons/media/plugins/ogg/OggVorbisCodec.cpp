#include "OggVorbisFormats.h"
#include "OggCodecs.h"
#include <stdio.h>

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...) ((void)0)
#endif


inline size_t
AudioBufferSize(media_raw_audio_format * raf, bigtime_t buffer_duration = 50000 /* 50 ms */)
{
	return (raf->format & 0xf) * (raf->channel_count)
         * (size_t)((raf->frame_rate * buffer_duration) / 1000000.0);
}


/*
 * vorbis header parsing code from libvorbis/info.c
 */


typedef struct vorbis_info{
  int version;
  int channels;
  long rate;

  /* The below bitrate declarations are *hints*.
     Combinations of the three values carry the following implications:
     
     all three set to the same value: 
       implies a fixed rate bitstream
     only nominal set: 
       implies a VBR stream that averages the nominal bitrate.  No hard 
       upper/lower limit
     upper and or lower set: 
       implies a VBR bitstream that obeys the bitrate limits. nominal 
       may also be set to give a nominal rate.
     none set:
       the coder does not care to speculate.
  */

  long bitrate_upper;
  long bitrate_nominal;
  long bitrate_lower;
  long bitrate_window;

  void *codec_setup;
} vorbis_info;


// based on libvorbis/info.c _vorbis_unpack_info
static int _vorbis_unpack_info(vorbis_info *vi,oggpack_buffer *opb){
	vi->version = oggpack_read(opb, 32);
	if (vi->version != 0) {
		return -1;
	}
	vi->channels = oggpack_read(opb, 8);
	vi->rate = oggpack_read(opb, 32);
	vi->bitrate_upper = oggpack_read(opb, 32);
	vi->bitrate_nominal = oggpack_read(opb, 32);
	vi->bitrate_lower = oggpack_read(opb, 32);
	long blocksizes0 = oggpack_read(opb, 4);
	long blocksizes1 = oggpack_read(opb, 4);
	if (vi->rate < 1) {
		return -1;
	}
	if (vi->channels < 1) {
		return -1;
	}
	if (blocksizes0 < 8) {
		return -1;
	}
	if (blocksizes1 < blocksizes0) {
		return -1;
	}	
	if (oggpack_read(opb, 1) != 1) {
		return -1;
	} /* EOP check */
	return 0;
}


/*
 * OggVorbisCodec
 */


class OggVorbisCodec : public OggCodec {
public:
			OggVorbisCodec();
	virtual	~OggVorbisCodec();

	virtual bool		IsHeaderPacket(const ogg_packet & packet, uint packetno) const;
	virtual status_t	HandlePacket(const ogg_packet & packet);

	virtual status_t	GetChunk(ogg_stream_state * stream, bool streaming,
						         void **chunkBuffer, int32 *chunkSize,
						         media_header *mediaHeader);
private:
	std::vector<ogg_packet>	fMetaDataPackets;
	unsigned char *			fHeaderPacketData;
	unsigned char *			fCommentPacketData;
	unsigned char *			fCodebookPacketData;
	unsigned int			fPacketCount;
};


OggVorbisCodec::OggVorbisCodec()
{
	TRACE("OggVorbisCodec::OggVorbisCodec\n");
	fHeaderPacketData = NULL;
	fCommentPacketData = NULL;
	fCodebookPacketData = NULL;
	fPacketCount = 0;
}


OggVorbisCodec::~OggVorbisCodec()
{
	delete fHeaderPacketData;
	delete fCommentPacketData;
	delete fCodebookPacketData;
}


/* virtual */ bool
OggVorbisCodec::IsHeaderPacket(const ogg_packet & packet, uint packetno) const
{
	oggpack_buffer opb;
	oggpack_readinit(&opb, packet.packet, packet.bytes);
	uint packtype = oggpack_read(&opb, 8);
	return (packetno == 0 && packtype == 0x01)
	    || (packetno == 1 && packtype == 0x03)
	    || (packetno == 2 && packtype == 0x05);
}


/* virtual */ status_t
OggVorbisCodec::HandlePacket(const ogg_packet & packet)
{
	TRACE("OggVorbisCodec::HandlePacket\n");
	if (!IsHeaderPacket(packet, fPacketCount)) {
		return B_BAD_VALUE;
	}
	switch (fPacketCount) {
	case 0: {
		// header packet
		if (!packet.b_o_s) {
			return B_ERROR; // first packet was not beginning of stream
		}
	
		// parse header packet
		// based on libvorbis/info.c vorbis_synthesis_headerin(...)
		oggpack_buffer opb;
		oggpack_readinit(&opb, packet.packet, packet.bytes);
		// discard packet type (already validated in IsHeaderPacket)
		oggpack_read(&opb, 8);
		// discard vorbis string
		for (uint i = 0 ; i < sizeof("vorbis") - 1 ; i++) {
			oggpack_read(&opb, 8);
		}
		vorbis_info info;
		if (_vorbis_unpack_info(&info, &opb) != 0) {
			return B_ERROR; // couldn't unpack info
		}
	
		// get the format for the description
		media_format_description description = vorbis_description();
		BMediaFormats formats;
		if ((formats.InitCheck() != B_OK) ||
		    (formats.GetFormatFor(description, &fMediaFormat) != B_OK)) {
			fMediaFormat = vorbis_encoded_media_format();
		}
	
		// fill out format from header packet
		if (info.bitrate_nominal > 0) {
			fMediaFormat.u.encoded_audio.bit_rate = info.bitrate_nominal;
		} else if (info.bitrate_upper > 0) {
			fMediaFormat.u.encoded_audio.bit_rate = info.bitrate_upper;
		} else if (info.bitrate_lower > 0) {
			fMediaFormat.u.encoded_audio.bit_rate = info.bitrate_lower;
		}
		if (info.channels == 1) {
			fMediaFormat.u.encoded_audio.multi_info.channel_mask = B_CHANNEL_LEFT;
		} else {
			fMediaFormat.u.encoded_audio.multi_info.channel_mask = B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
		}
		fMediaFormat.u.encoded_audio.output.frame_rate = (float)info.rate;
		fMediaFormat.u.encoded_audio.output.channel_count = info.channels;
		fMediaFormat.u.encoded_audio.output.buffer_size
		  = AudioBufferSize(&fMediaFormat.u.encoded_audio.output);
		
		fHeaderPacketData = new unsigned char[packet.bytes];
		memcpy(fHeaderPacketData, packet.packet, packet.bytes);
		fMetaDataPackets.push_back(packet);
		fMetaDataPackets[fPacketCount].packet = fHeaderPacketData;
		break;
	}
	case 1: {
		// comment packet
		fCommentPacketData = new unsigned char[packet.bytes];
		memcpy(fCommentPacketData, packet.packet, packet.bytes);
		fMetaDataPackets.push_back(packet);
		fMetaDataPackets[fPacketCount].packet = fCommentPacketData;
		break;
	}
	case 2: {
		// codebook packet
		fCodebookPacketData = new unsigned char[packet.bytes];
		memcpy(fCodebookPacketData, packet.packet, packet.bytes);
		fMetaDataPackets.push_back(packet);
		fMetaDataPackets[fPacketCount].packet = fCodebookPacketData;

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


/* virtual */ status_t
OggVorbisCodec::GetChunk(ogg_stream_state * stream, bool streaming,
                         void **chunkBuffer, int32 *chunkSize,
                         media_header *mediaHeader)
{
	if (fCurrentFrame == -1) {
		fCurrentTime = -1;
	} else {
		fCurrentTime = (1000000LL * fCurrentFrame)
		             / (long long)fMediaFormat.u.encoded_audio.output.frame_rate;
	}
	return OggCodec::GetChunk(stream, streaming, chunkBuffer, chunkSize, mediaHeader);
}


/*
 * OggVorbisCodecTest
 */

static class OggVorbisCodecTest : public OggCodecTest {
public:
			OggVorbisCodecTest() {}
	virtual	~OggVorbisCodecTest() {}
	virtual	bool		RecognizesInitialPacket(const ogg_packet & packet) const
	{
		return findIdentifier(packet, "vorbis", 1);
	}
	virtual	OggCodec *	InstantiateCodec() const
	{
		return new OggVorbisCodec();
	}
} ogg_vorbis_codec_test;
