#include "OggTheoraFormats.h"
#include "OggTheoraStream.h"
#include <ogg/ogg.h>
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

// based on theora/lib/toplevel.c _theora_unpack_info

#define theora_read(x,y,z) ( *z = oggpack_read(x,y) )

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
 * OggTheoraStream implementations
 */

/* static */ bool
OggTheoraStream::IsValidHeader(const ogg_packet & packet)
{
	return findIdentifier(packet,"theora",1);
}

OggTheoraStream::OggTheoraStream(long serialno)
	: OggStream(serialno)
{
	TRACE("OggTheoraStream::OggTheoraStream\n");
}

OggTheoraStream::~OggTheoraStream()
{

}

status_t
OggTheoraStream::GetStreamInfo(int64 *frameCount, bigtime_t *duration,
                               media_format *format)
{
	TRACE("OggTheoraStream::GetStreamInfo\n");
	status_t result = B_OK;
	ogg_packet packet;

	// get header packet
	if (GetHeaderPackets().size() < 1) {
		result = GetPacket(&packet);
		if (result != B_OK) {
			return result;
		}
		SaveHeaderPacket(packet);
	}
	packet = GetHeaderPackets()[0];
	if (!packet.b_o_s) {
		return B_ERROR; // first packet was not beginning of stream
	}

	// parse header packet
	// based on libvorbis/info.c vorbis_synthesis_headerin(...)
	oggpack_buffer opb;
	oggpack_readinit(&opb, packet.packet, packet.bytes);
	int typeflag = oggpack_read(&opb, 8);
	if (typeflag != 0x80) {
		return B_ERROR; // first packet was not an info packet
	}
	// discard theora string
	for (uint i = 0 ; i < sizeof("theora") - 1 ; i++) {
		oggpack_read(&opb, 8);
	}
	theora_info info;
	if (_theora_unpack_info(&info, &opb) != 0) {
		return B_ERROR; // couldn't unpack info
	}

	// get the format for the description
	media_format_description description = theora_description();
	BMediaFormats formats;
	result = formats.InitCheck();
	if (result == B_OK) {
		result = formats.GetFormatFor(description, format);
	}
	if (result != B_OK) {
		*format = theora_encoded_media_format();
		// ignore error, allow user to use ReadChunk interface
	}

	// fill out format from header packet
	format->u.encoded_video.frame_size = info.frame_width * info.frame_height ;
	format->u.encoded_video.output.display.line_width = info.frame_width;
	format->u.encoded_video.output.display.line_count = info.frame_height;
	// TODO: wring more info out of the headers

	format->SetMetaData((void*)&GetHeaderPackets(),sizeof(GetHeaderPackets()));
	*duration = 80000000;
	*frameCount = 60000;
	return B_OK;
}
