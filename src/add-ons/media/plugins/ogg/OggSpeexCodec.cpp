#include "OggSpeexFormats.h"
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
 * speex header from libspeex/speex_header.h
 * also documented at http://www.speex.org/manual/node7.html#SECTION00073000000000000000
 */

typedef struct SpeexHeader {
   char speex_string[8];       /**< Identifies a Speex bit-stream, always set to "Speex   " */
   char speex_version[20];     /**< Speex version */
   int speex_version_id;       /**< Version for Speex (for checking compatibility) */
   int header_size;            /**< Total size of the header ( sizeof(SpeexHeader) ) */
   int rate;                   /**< Sampling rate used */
   int mode;                   /**< Mode used (0 for narrowband, 1 for wideband) */
   int mode_bitstream_version; /**< Version ID of the bit-stream */
   int nb_channels;            /**< Number of channels encoded */
   int bitrate;                /**< Bit-rate used */
   int frame_size;             /**< Size of frames */
   int vbr;                    /**< 1 for a VBR encoding, 0 otherwise */
   int frames_per_packet;      /**< Number of frames stored per Ogg packet */
   int extra_headers;          /**< Number of additional headers after the comments */
   int reserved1;              /**< Reserved for future use, must be zero */
   int reserved2;              /**< Reserved for future use, must be zero */
} SpeexHeader;


/*
 * OggSpeexCodec implementations
 */

class OggSpeexCodec : public OggCodec {
public:
			OggSpeexCodec();
	virtual	~OggSpeexCodec();

	virtual bool		IsHeaderPacket(const ogg_packet & packet, uint packetno) const;
	virtual status_t	HandlePacket(const ogg_packet & packet);

	virtual status_t	GetChunk(ogg_stream_state * stream, bool streaming,
						         void **chunkBuffer, int32 *chunkSize,
						         media_header *mediaHeader);
private:
	std::vector<ogg_packet>	fMetaDataPackets;
	unsigned char *			fHeaderPacketData;
	unsigned char *			fCommentPacketData;
	std::vector<uchar *>	fExtraPacketData;
	unsigned int			fPacketCount;
	unsigned int			fExtraHeadersField;
};


OggSpeexCodec::OggSpeexCodec()
{
	TRACE("OggSpeexCodec::OggSpeexCodec\n");
	fHeaderPacketData = NULL;
	fCommentPacketData = NULL;
	fPacketCount = 0;
}


OggSpeexCodec::~OggSpeexCodec()
{
	TRACE("OggSpeexCodec::~OggSpeexCodec\n");
	delete fHeaderPacketData;
	delete fCommentPacketData;
	std::vector<uchar *>::iterator iter;
	for (iter = fExtraPacketData.begin() ; iter != fExtraPacketData.end() ; iter++) {
		uchar * data = *iter;
		delete data;
	}
}


/* virtual */ bool
OggSpeexCodec::IsHeaderPacket(const ogg_packet & packet, uint packetno) const
{
	if (packetno < 2) {
		return true;
	}
	if (fPacketCount == 0) {
		debugger("IsHeaderPacket called for packetno >= 2, before receiving the header packet");
		return false;
	}
	return (packetno < 2 + fExtraHeadersField);
}


/* virtual */ status_t
OggSpeexCodec::HandlePacket(const ogg_packet & packet)
{
	TRACE("OggSpeexCodec::HandlePacket\n");
	if (!IsHeaderPacket(packet, fPacketCount)) {
		return B_BAD_VALUE;
	}
	switch (fPacketCount) {
	case 0: {
		// header packet
		if (!packet.b_o_s) {
			return B_ERROR; // first packet was not beginning of stream
		}

		// parse header packet, check size against struct minus optional fields
		if (packet.bytes < 1 + (signed)sizeof(SpeexHeader) - 2*(signed)sizeof(int)) {
			return B_ERROR;
		}
		void * data = &(packet.packet[0]);
		SpeexHeader * header = (SpeexHeader *)data;

		// store how many extra header packets
		fExtraHeadersField = header->extra_headers;

		// get the format for the description
		media_format_description description = speex_description();
		BMediaFormats formats;
		if ((formats.InitCheck() != B_OK) ||
		    (formats.GetFormatFor(description, &fMediaFormat) != B_OK)) {
			fMediaFormat = speex_encoded_media_format();
		}

		// fill out format from header packet
		if (header->bitrate > 0) {
			fMediaFormat.u.encoded_audio.bit_rate = header->bitrate;
		} else {
			// TODO: manually compute it where possible
		}
		if (header->nb_channels == 1) {
			fMediaFormat.u.encoded_audio.multi_info.channel_mask = B_CHANNEL_LEFT;
		} else {
			fMediaFormat.u.encoded_audio.multi_info.channel_mask = B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
		}
		fMediaFormat.u.encoded_audio.output.frame_rate = header->rate;
		fMediaFormat.u.encoded_audio.output.channel_count = header->nb_channels;
		// allocate buffer, round up to nearest speex output_length size
		int buffer_size = AudioBufferSize(&fMediaFormat.u.encoded_audio.output);
		int output_length = header->frame_size * header->nb_channels *
		                    (fMediaFormat.u.encoded_audio.output.format & 0xf);
		buffer_size = ((buffer_size - 1) / output_length + 1) * output_length;
		fMediaFormat.u.encoded_audio.output.buffer_size = buffer_size;
		
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
	default:
		// extra headers
		uchar * extraHeaderData = new unsigned char[packet.bytes];
		memcpy(extraHeaderData, packet.packet, packet.bytes);
		fExtraPacketData.push_back(extraHeaderData);
		fMetaDataPackets.push_back(packet);
		fMetaDataPackets[fPacketCount].packet = extraHeaderData;
		break;
	}
	if (fPacketCount == 1 + fExtraHeadersField) {
		fMediaFormat.SetMetaData(&fMetaDataPackets,sizeof(fMetaDataPackets));
		fInitCheck = B_OK;
	}
	fPacketCount++;
	return B_OK;
}


/* virtual */ status_t
OggSpeexCodec::GetChunk(ogg_stream_state * stream, bool streaming,
                        void **chunkBuffer, int32 *chunkSize,
                        media_header *mediaHeader)
{
	if (fCurrentFrame == -1) {
		fCurrentTime = -1;
	} else {
		fCurrentTime = (1000000LL * fCurrentFrame)
		             / (long long)fMediaFormat.u.encoded_audio.output.frame_rate;
	}
	status_t result = OggCodec::GetChunk(stream, streaming, chunkBuffer, chunkSize, mediaHeader);
	if (result != B_OK) {
		return result;
	}
	*chunkBuffer = fChunkPacket.packet;
	*chunkSize = fChunkPacket.bytes;
	return B_OK;
}


/*
 * OggSpeexCodecTest
 */

static class OggSpeexCodecTest : public OggCodecTest {
public:
			OggSpeexCodecTest() {}
	virtual	~OggSpeexCodecTest() {}
	virtual	bool		RecognizesInitialPacket(const ogg_packet & packet) const
	{
		return findIdentifier(packet, "Speex   ", 0);
	}
	virtual	OggCodec *	InstantiateCodec() const
	{
		return new OggSpeexCodec();
	}
} ogg_speex_codec_test;
