#include <unistd.h>
#include "OggReaderPlugin.h"

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...) ((void)0)
#endif

// codec identification 

static bool
findIdentifier(ogg_packet * packet, const char * id, uint location)
{
	uint length = strlen(id);
	if ((unsigned)packet->bytes < location+length) {
		return false;
	}
	return !memcmp(&packet->packet[location], id, length);
}

inline size_t
AudioBufferSize(media_raw_audio_format * raf, bigtime_t buffer_duration = 50000 /* 50 ms */)
{
	return (raf->format & 0xf) * (raf->channel_count)
         * (size_t)((raf->frame_rate * buffer_duration) / 1000000.0);
}

/*---- vorbis ----*/

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

static bool
isVorbisPacket(ogg_packet * packet)
{
	return findIdentifier(packet,"vorbis",1);
}

static int
GetVorbisExtraHeaderPacketCount(ogg_packet * packet)
{
	return 2;
}

static status_t
GetVorbisCodecStreamInfo(std::vector<ogg_packet> * packets,
                         int64 *frameCount, bigtime_t *duration,
                         media_format *format)
{
	ogg_packet * packet = &((*packets)[0]);
	
	// based on libvorbis/info.c vorbis_synthesis_headerin(...)
	oggpack_buffer opb;
	oggpack_readinit(&opb, packet->packet, packet->bytes);
	int packtype = oggpack_read(&opb, 8);
	// discard vorbis string
	for (uint i = 0 ; i < sizeof("vorbis") - 1 ; i++) {
		oggpack_read(&opb, 8);
	}
	if (packtype != 0x01) {
		return B_ERROR; // first packet was not an info packet
	}
	if (!packet->b_o_s) {
		return B_ERROR; // first packet was not beginning of stream
	}
	vorbis_info info;
	if (_vorbis_unpack_info(&info, &opb) != 0) {
		return B_ERROR; // couldn't unpack info
	}

	format->type = B_MEDIA_ENCODED_AUDIO;
	format->user_data_type = B_CODEC_TYPE_INFO;
	strncpy((char*)format->user_data, "vorb", 4);
	format->u.encoded_audio.encoding
      = (media_encoded_audio_format::audio_encoding)'vorb';
	if (info.bitrate_nominal > 0) {
		format->u.encoded_audio.bit_rate = info.bitrate_nominal;
	} else if (info.bitrate_upper > 0) {
		format->u.encoded_audio.bit_rate = info.bitrate_upper;
	} else if (info.bitrate_lower > 0) {
		format->u.encoded_audio.bit_rate = info.bitrate_lower;
	}
	format->u.encoded_audio.frame_size = sizeof(ogg_packet);
	format->u.encoded_audio.output.frame_rate = (float)info.rate;
	format->u.encoded_audio.output.channel_count = info.channels;
	format->u.encoded_audio.output.format = media_raw_audio_format::B_AUDIO_FLOAT;
	format->u.encoded_audio.output.byte_order = B_MEDIA_HOST_ENDIAN;
	format->u.encoded_audio.output.buffer_size
	  = AudioBufferSize(&format->u.encoded_audio.output);
	return B_OK;
}


/*---- speex ----*/


// http://www.speex.org/manual/node7.html#SECTION00073000000000000000
typedef struct speex_header
{
	char speex_string[8];
	char speex_version[20];
	int32 speex_version_id;
	int32 header_size;
	int32 rate;
	int32 mode;
	int32 mode_bitstream_version;
	int32 nb_channels;
	int32 bitrate;
	int32 frame_size;
	int32 vbr;
	int32 frames_per_packet;
	int32 extra_headers;
	int32 reserved1;
	int32 reserved2;
} speex_header;

static bool
isSpeexPacket(ogg_packet * packet)
{
	return findIdentifier(packet,"Speex   ",0);
}

static int
GetSpeexExtraHeaderPacketCount(ogg_packet * packet)
{
	return 1;
}

static status_t
GetSpeexCodecStreamInfo(std::vector<ogg_packet> * packets,
                         int64 *frameCount, bigtime_t *duration,
                         media_format *format)
{
	ogg_packet * packet = &((*packets)[0]);
	if (packet->bytes < (signed)sizeof(speex_header)) {
		return B_ERROR;
	}
	void * data = &(packet->packet[0]);
	speex_header * header = (speex_header *)data;

	format->type = B_MEDIA_ENCODED_AUDIO;
	format->user_data_type = B_CODEC_TYPE_INFO;
	strncpy((char*)format->user_data,"Spee",4);
	format->u.encoded_audio.encoding
       = (media_encoded_audio_format::audio_encoding)'Spee';
	if (header->bitrate > 0) {
		format->u.encoded_audio.bit_rate = header->bitrate;
	} else {
		// TODO: manually compute it where possible
	}
	format->u.encoded_audio.frame_size = header->frame_size;
	if (header->nb_channels == 1) {
		format->u.encoded_audio.multi_info.channel_mask = B_CHANNEL_LEFT;
	} else {
		format->u.encoded_audio.multi_info.channel_mask = B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
	}
	format->u.encoded_audio.frame_size = sizeof(ogg_packet);
	format->u.encoded_audio.output.frame_rate = header->rate;
	format->u.encoded_audio.output.channel_count = header->nb_channels;
	format->u.encoded_audio.output.format = media_raw_audio_format::B_AUDIO_FLOAT;
	format->u.encoded_audio.output.byte_order = B_MEDIA_HOST_ENDIAN;
	format->u.encoded_audio.output.buffer_size
	   = AudioBufferSize(&format->u.encoded_audio.output);
	return B_OK;
}


/*---- tobias ----*/


// http://tobias.everwicked.com/packfmt.htm
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

	ogg_int64_t time_unit; // in reference time
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


static bool
isTobiasPacket(ogg_packet * packet)
{
	return findIdentifier(packet,"video",1);
}

static int
GetTobiasExtraHeaderPacketCount(ogg_packet * packet)
{
	return 1;
}

static status_t
GetTobiasCodecStreamInfo(std::vector<ogg_packet> * packets,
                        int64 *frameCount, bigtime_t *duration,
                        media_format *format)
{
	ogg_packet * packet = &((*packets)[0]);
	if (packet->bytes < (signed)sizeof(tobias_stream_header)) {
		return B_ERROR;
	}
	void * data = &(packet->packet[1]);
	tobias_stream_header * header = (tobias_stream_header *)data;

	format->type = B_MEDIA_ENCODED_VIDEO;
	format->user_data_type = B_CODEC_TYPE_INFO;
	strncpy((char*)format->user_data,header->subtype,4);
	int32 encoding = header->subtype[0] << 24 | header->subtype[1] << 16 
                   | header->subtype[2] <<  8 | header->subtype[3];
	format->u.encoded_video.encoding
	   = (media_encoded_video_format::video_encoding)encoding;
	format->u.encoded_video.frame_size
	   = header->video.width * header->video.height;
	format->u.encoded_video.output.display.line_width = header->video.width;
	format->u.encoded_video.output.display.line_count = header->video.height;
	// TODO: wring more info out of the headers
	return B_OK;
}

/*---- generic routines ----*/


int
oggReader::GetExtraHeaderPacketCount(ogg_packet * packet)
{
	if (isVorbisPacket(packet)) {
		return GetVorbisExtraHeaderPacketCount(packet);
	}
	if (isTobiasPacket(packet)) {
		return GetTobiasExtraHeaderPacketCount(packet);
	}
	if (isSpeexPacket(packet)) {
		return GetSpeexExtraHeaderPacketCount(packet);
	}
	return 0;
}

status_t
oggReader::GetCodecStreamInfo(ogg_stream_state * stream,
                              int64 *frameCount, bigtime_t *duration,
                              media_format *format)
{
	std::vector<ogg_packet> * packets = &fHeaderPackets[stream->serialno];
	if (fSeekable) {
		off_t input_length;
		{
			off_t current_position = fSeekable->Seek(0,SEEK_CUR);
			input_length = fSeekable->Seek(0,SEEK_END)-fPostSniffPosition;
			fSeekable->Seek(current_position,SEEK_SET);
		}
		*frameCount = positionToFrame(stream,input_length);
		*duration = positionToTime(stream,input_length);
	} else {
		assert(sizeof(bigtime_t)==sizeof(uint64_t));
		// if the input is not seekable, we return really large sizes
		*frameCount = INT64_MAX;
		*duration = INT64_MAX;
	}
	if (!packets || packets->size() < 1) {
		return B_BAD_VALUE;
	}
	ogg_packet * packet = &((*packets)[0]);
	if (isVorbisPacket(packet)) {
		return GetVorbisCodecStreamInfo(packets,frameCount,duration,format);
	}
	if (isTobiasPacket(packet)) {
		return GetTobiasCodecStreamInfo(packets,frameCount,duration,format);
	}
	if (isSpeexPacket(packet)) {
		return GetSpeexCodecStreamInfo(packets,frameCount,duration,format);
	}
	return B_OK;
}

// estimate: 1 frame = 256 bytes
int64
oggReader::positionToFrame(ogg_stream_state * stream, off_t position)
{
	return position/256;
}

off_t
oggReader::frameToPosition(ogg_stream_state * stream, int64 frame)
{
	return frame*256;
}

bigtime_t
oggReader::positionToTime(ogg_stream_state * stream, off_t position)
{
	return position*1000/8;
}

off_t
oggReader::timeToPosition(ogg_stream_state * stream, bigtime_t time)
{
	return time*8/1000;
}
