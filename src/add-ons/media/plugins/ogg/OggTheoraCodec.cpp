#include "OggTheoraFormats.h"
#include "OggCodecs.h"
#include <stdio.h>

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...) ((void)0)
#endif


/*
 * theora header parsing code from theora/theara.h
 */

typedef enum {
  OC_CS_UNSPECIFIED,
  OC_CS_ITU_REC_470M,
  OC_CS_ITU_REC_470BG,
} theora_colorspace;

typedef struct {
  ogg_uint32_t  width;
  ogg_uint32_t  height;
  ogg_uint32_t  frame_width;
  ogg_uint32_t  frame_height;
  ogg_uint32_t  offset_x;
  ogg_uint32_t  offset_y;
  ogg_uint32_t  fps_numerator;
  ogg_uint32_t  fps_denominator;
  ogg_uint32_t  aspect_numerator;
  ogg_uint32_t  aspect_denominator;
  theora_colorspace colorspace;
  int           target_bitrate;
  int           quality;
  int           quick_p;  /* quick encode/decode */

  /* decode only */
  unsigned char version_major;
  unsigned char version_minor;
  unsigned char version_subminor;

  void *codec_setup;

  /* encode only */
  int           dropframes_p;
  int           keyframe_auto_p;
  ogg_uint32_t  keyframe_frequency;
  ogg_uint32_t  keyframe_frequency_force;  /* also used for decode init to
                                              get granpos shift correct */
  ogg_uint32_t  keyframe_data_target_bitrate;
  ogg_int32_t   keyframe_auto_threshold;
  ogg_uint32_t  keyframe_mindistance;
  ogg_int32_t   noise_sensitivity;
  ogg_int32_t   sharpness;

} theora_info;

typedef struct theora_comment{
  char **user_comments;
  int   *comment_lengths;
  int    comments;
  char  *vendor;

} theora_comment;

// based on theora/lib/toplevel.c _theora_unpack_info

#define theora_read(x,y,z) ( *z = oggpackB_read(x,y) )

#define OC_BADHEADER -1

static int _theora_unpack_info(theora_info *ci, oggpack_buffer *opb){
  long ret;

  theora_read(opb,8,&ret);
  ci->version_major=(unsigned char)ret;
  theora_read(opb,8,&ret);
  ci->version_minor=(unsigned char)ret;
  theora_read(opb,8,&ret);
  ci->version_subminor=(unsigned char)ret;

//  if(ci->version_major!=VERSION_MAJOR)return(OC_VERSION);
//  if(ci->version_minor>VERSION_MINOR)return(OC_VERSION);

  theora_read(opb,16,&ret);
  ci->width=ret<<4;
  theora_read(opb,16,&ret);
  ci->height=ret<<4;
  theora_read(opb,24,&ret);
  ci->frame_width=ret;
  theora_read(opb,24,&ret);
  ci->frame_height=ret;
  theora_read(opb,8,&ret);
  ci->offset_x=ret;
  theora_read(opb,8,&ret);
  ci->offset_y=ret;

  theora_read(opb,32,&ret);
  ci->fps_numerator=ret;
  theora_read(opb,32,&ret);
  ci->fps_denominator=ret;
  theora_read(opb,24,&ret);
  ci->aspect_numerator=ret;
  theora_read(opb,24,&ret);
  ci->aspect_denominator=ret;

  theora_read(opb,8,&ret);
  ci->colorspace=(theora_colorspace)ret;
  theora_read(opb,24,&ret);
  ci->target_bitrate=ret;
  theora_read(opb,6,&ret);
  ci->quality=ret=ret;

  theora_read(opb,5,&ret);
  ci->keyframe_frequency_force=1<<ret;

  /* spare configuration bits */
  if ( theora_read(opb,5,&ret) == -1 )
    return (OC_BADHEADER);

  return(0);
}

/*
 * OggTheoraCodec
 */


class OggTheoraCodec : public OggCodec {
public:
			OggTheoraCodec();
	virtual	~OggTheoraCodec();

	virtual bool		IsHeaderPacket(const ogg_packet & packet, uint packetno) const;
	virtual status_t	HandlePacket(const ogg_packet & packet);

	virtual status_t	GetChunk(ogg_stream_state * stream, bool streaming,
						         void **chunkBuffer, int32 *chunkSize,
						         media_header *mediaHeader);
private:
	std::vector<ogg_packet>	fMetaDataPackets;
	unsigned char *			fHeaderPacketData;
	unsigned char *			fCommentPacketData;
	unsigned char *			fCodecPacketData;
	unsigned int			fPacketCount;
};


OggTheoraCodec::OggTheoraCodec()
{
	TRACE("OggTheoraCodec::OggTheoraCodec\n");
	fHeaderPacketData = NULL;
	fCommentPacketData = NULL;
	fCodecPacketData = NULL;
	fPacketCount = 0;
}


OggTheoraCodec::~OggTheoraCodec()
{
	delete fHeaderPacketData;
	delete fCommentPacketData;
	delete fCodecPacketData;
}


/* virtual */ bool
OggTheoraCodec::IsHeaderPacket(const ogg_packet & packet, uint packetno) const
{
	oggpack_buffer opb;
	oggpackB_readinit(&opb, packet.packet, packet.bytes);
	uint packtype = oggpackB_read(&opb, 8);
	return (packetno == 0 && packtype == 0x80)
        || (packetno == 1 && packtype == 0x81)
	    || (packetno == 2 && packtype == 0x82);
}


/* virtual */ status_t
OggTheoraCodec::HandlePacket(const ogg_packet & packet)
{
	TRACE("OggTheoraCodec::HandlePacket\n");
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
		oggpackB_readinit(&opb, packet.packet, packet.bytes);
		// discard packet type (already validated in IsHeaderPacket)
		oggpackB_read(&opb, 8);
		// discard theora string
		for (uint i = 0 ; i < sizeof("theora") - 1 ; i++) {
			oggpackB_read(&opb, 8);
		}
		theora_info info;
		if (_theora_unpack_info(&info, &opb) != 0) {
			return B_ERROR; // couldn't unpack info
		}
	
		// get the format for the description
		media_format_description description = theora_description();
		BMediaFormats formats;
		if ((formats.InitCheck() != B_OK) ||
		    (formats.GetFormatFor(description, &fMediaFormat) != B_OK)) {
			fMediaFormat = theora_encoded_media_format();
		}

		// fill out format from header packet
		// sanity check
		if (info.fps_denominator != 0) {
			fMediaFormat.u.encoded_video.output.field_rate =
				(double)info.fps_numerator / (double)info.fps_denominator;
		}
		fMediaFormat.u.encoded_video.output.first_active = info.offset_y;
		fMediaFormat.u.encoded_video.output.last_active = info.offset_y + info.height;
		fMediaFormat.u.encoded_video.output.pixel_width_aspect = info.aspect_numerator;
		fMediaFormat.u.encoded_video.output.pixel_height_aspect = info.aspect_denominator;
		fMediaFormat.u.encoded_video.output.display.line_width = info.frame_width;
		fMediaFormat.u.encoded_video.output.display.line_count = info.frame_height;
		fMediaFormat.u.encoded_video.avg_bit_rate = info.target_bitrate; // advisory only
		fMediaFormat.u.encoded_video.frame_size = info.width * info.height;
		// TODO: wring more info out of the headers

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
		// codec setup packet
		fCodecPacketData = new unsigned char[packet.bytes];
		memcpy(fCodecPacketData, packet.packet, packet.bytes);
		fMetaDataPackets.push_back(packet);
		fMetaDataPackets[fPacketCount].packet = fCodecPacketData;

		fMediaFormat.SetMetaData(&fMetaDataPackets,sizeof(fMetaDataPackets));
		fInitCheck = B_OK;
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
OggTheoraCodec::GetChunk(ogg_stream_state * stream, bool streaming,
                         void **chunkBuffer, int32 *chunkSize,
                         media_header *mediaHeader)
{
	if (streaming) {
		if (fChunkPacket.granulepos == -1) {
			fCurrentTime = -1;
		} else {
			fCurrentTime = (bigtime_t) ((1000000LL * fCurrentFrame)
			             / (fMediaFormat.u.encoded_video.output.field_rate * 
			                fMediaFormat.u.encoded_video.output.interlace));
		}
	} else {
		debugger("kaboom");
	}
	status_t result = OggCodec::GetChunk(stream, streaming, chunkBuffer, chunkSize, mediaHeader);
	if (result != B_OK) {
		return result;
	}
	oggpack_buffer opb;
	oggpackB_readinit(&opb, fChunkPacket.packet, fChunkPacket.bytes);
	// Read 1 bit. This must be 0 for all data packets.
	uint packtype = oggpackB_read(&opb, 1);
	if (packtype != 0) {
		// packet was not a data packet
		return B_ERROR; 
	}
	// read 1 bit (frametype) this is 0 for keyframes, 1 for P frames.
	uint keyframe = oggpackB_read(&opb, 1);
	// read 6 bits (quality mask) this is the quality index for this frame.
//	uint quality = oggpackB_read(&opb, 6);
/*
if the frametype is 0 (keyframe):
read 1 bit (keyframetype) this indicates the coding variety of the keyframe.
read 2 bits (spare) these bits are unused.
*/
	mediaHeader->u.encoded_video.field_number = fCurrentFrame;
	mediaHeader->u.encoded_video.field_flags = (keyframe == 1 ? B_MEDIA_KEY_FRAME : 0);
	return result;
}


/*
 * OggTheoraCodecWrapper
 */

static class OggTheoraCodecTest : public OggCodecTest {
public:
			OggTheoraCodecTest() {}
	virtual	~OggTheoraCodecTest() {}
	virtual	bool		RecognizesInitialPacket(const ogg_packet & packet) const
	{
		return findIdentifier(packet, "theora", 1);
	}
	virtual	OggCodec *	InstantiateCodec() const
	{
		return new OggTheoraCodec();
	}
} ogg_theora_codec_test;
