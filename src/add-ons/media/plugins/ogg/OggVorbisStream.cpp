#include "OggVorbisStream.h"
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
	oggpack_read(opb, 4); // discard blocksizes 0
	oggpack_read(opb, 4); // discard blocksizes 1
	if (vi->rate < 1) {
		return -1;
	}
	if (vi->channels < 1) {
		return -1;
	}
	if (oggpack_read(opb, 1) != 1) {
		return -1;
	} /* EOP check */
	return 0;
}

/*
 * OggVorbisStream implementations
 */

/* static */ bool
OggVorbisStream::IsValidHeader(const ogg_packet & packet)
{
	return findIdentifier(packet,"vorbis",1);
}

OggVorbisStream::OggVorbisStream(long serialno)
	: OggStream(serialno)
{
	TRACE("OggVorbisStream::OggVorbisStream\n");
}

OggVorbisStream::~OggVorbisStream()
{

}

status_t
OggVorbisStream::GetStreamInfo(int64 *frameCount, bigtime_t *duration,
                               media_format *format)
{
	TRACE("OggVorbisStream::GetStreamInfo\n");
	status_t result = B_OK;
	ogg_packet packet;

	// get header packet
	if (fHeaderPackets.size() < 1) {
		result = GetPacket(&packet);
		if (result != B_OK) {
			return result;
		}
		SaveHeaderPacket(packet);
	}
	packet = fHeaderPackets[0];
	if (!packet.b_o_s) {
		return B_ERROR; // first packet was not beginning of stream
	}

	// parse header packet
	// based on libvorbis/info.c vorbis_synthesis_headerin(...)
	oggpack_buffer opb;
	oggpack_readinit(&opb, packet.packet, packet.bytes);
	int packtype = oggpack_read(&opb, 8);
	if (packtype != 0x01) {
		return B_ERROR; // first packet was not an info packet
	}
	// discard vorbis string
	for (uint i = 0 ; i < sizeof("vorbis") - 1 ; i++) {
		oggpack_read(&opb, 8);
	}
	vorbis_info info;
	if (_vorbis_unpack_info(&info, &opb) != 0) {
		return B_ERROR; // couldn't unpack info
	}

	// get the format for the description
	media_format_description description;
	description.family = B_MISC_FORMAT_FAMILY;
	description.u.misc.file_format = 'OggS';
	description.u.misc.codec = 'vorb';
	BMediaFormats formats;
	result = formats.InitCheck();
	if (result != B_OK) {
		return result;
	}
	if (!formats.Lock()) {
		return B_ERROR;
	}
	result = formats.GetFormatFor(description, format);
	formats.Unlock();
	if (result != B_OK) {
		return result;
	}

	// fill out format from header packet
	if (info.bitrate_nominal > 0) {
		format->u.encoded_audio.bit_rate = info.bitrate_nominal;
	} else if (info.bitrate_upper > 0) {
		format->u.encoded_audio.bit_rate = info.bitrate_upper;
	} else if (info.bitrate_lower > 0) {
		format->u.encoded_audio.bit_rate = info.bitrate_lower;
	}
	if (info.channels == 1) {
		format->u.encoded_audio.multi_info.channel_mask = B_CHANNEL_LEFT;
	} else {
		format->u.encoded_audio.multi_info.channel_mask = B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
	}
	format->u.encoded_audio.output.frame_rate = (float)info.rate;
	format->u.encoded_audio.output.channel_count = info.channels;
	format->u.encoded_audio.output.buffer_size
	  = AudioBufferSize(&format->u.encoded_audio.output);

	// get comment packet
	if (fHeaderPackets.size() < 2) {
		result = GetPacket(&packet);
		if (result != B_OK) {
			return result;
		}
		SaveHeaderPacket(packet);
	}

	// get codebook packet
	if (fHeaderPackets.size() < 3) {
		result = GetPacket(&packet);
		if (result != B_OK) {
			return result;
		}
		SaveHeaderPacket(packet);
	}

	format->SetMetaData((void*)&fHeaderPackets,sizeof(fHeaderPackets));
	*duration = 100000000;
	*frameCount = 60000;
	return B_OK;
}
