#include "OggVorbisFormats.h"
#include "OggVorbisSeekable.h"
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
 * OggVorbisSeekable implementations
 */

/* static */ bool
OggVorbisSeekable::IsValidHeader(const ogg_packet & packet)
{
	return findIdentifier(packet,"vorbis",1);
}

OggVorbisSeekable::OggVorbisSeekable(long serialno)
	: OggSeekable(serialno)
{
	TRACE("OggVorbisSeekable::OggVorbisSeekable\n");
}

OggVorbisSeekable::~OggVorbisSeekable()
{
	TRACE("OggVorbisSeekable::~OggVorbisSeekable\n");
}

status_t
OggVorbisSeekable::GetStreamInfo(int64 *frameCount, bigtime_t *duration,
                               media_format *format)
{
	TRACE("OggVorbisSeekable::GetStreamInfo\n");
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
	media_format_description description = vorbis_description();
	BMediaFormats formats;
	result = formats.InitCheck();
	if (result == B_OK) {
		result = formats.GetFormatFor(description, format);
	}
	if (result != B_OK) {
		*format = vorbis_encoded_media_format();
		// ignore error, allow user to use ReadChunk interface
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
	if (GetHeaderPackets().size() < 2) {
		result = GetPacket(&packet);
		if (result != B_OK) {
			return result;
		}
		SaveHeaderPacket(packet);
	}

	// get codebook packet
	if (GetHeaderPackets().size() < 3) {
		result = GetPacket(&packet);
		if (result != B_OK) {
			return result;
		}
		SaveHeaderPacket(packet);
	}

	format->SetMetaData((void*)&GetHeaderPackets(),sizeof(GetHeaderPackets()));

	// TODO: count the frames in the first page.. somehow.. :-/
	int64 frames = 0;

	ogg_page page;
	// read the first page
	result = ReadPage(&page);
	if (result != B_OK) {
		return result;
	}
	int64 first_granulepos = ogg_page_granulepos(&page);
	if (first_granulepos < 0) {
		// negative start granulepos indicates that we discard that many frames
		frames -= first_granulepos;
		first_granulepos = 0;
	}

	// read our last page
	off_t last = Seek(GetLastPagePosition(), SEEK_SET);
	if (last < 0) {
		return last;
	}
	result = ReadPage(&page);
	if (result != B_OK) {
		return result;
	}
	int64 last_granulepos = ogg_page_granulepos(&page);

	// seek back to the start
	int64 frame = 0;
	bigtime_t time = 0;
	result = Seek(B_MEDIA_SEEK_TO_TIME, &frame, &time);
	if (result != B_OK) {
		return result;
	}

	// compute frame count and duration from sample count
	frames += last_granulepos - first_granulepos;
	*frameCount = frames;
	*duration = (1000000LL * frames) / (long long)format->u.encoded_audio.output.frame_rate;

	return B_OK;
}
