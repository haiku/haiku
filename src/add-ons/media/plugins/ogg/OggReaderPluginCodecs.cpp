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

/*---- vorbis ----*/


static bool
isVorbisPacket(ogg_packet * packet)
{
	return findIdentifier(packet,"vorbis",1);
}

static const int vorbis_extra_packet_count = 2;

static status_t
GetVorbisCodecStreamInfo(std::vector<ogg_packet> * packets,
                         int64 *frameCount, bigtime_t *duration,
                         media_format *format)
{
	format->type = B_MEDIA_ENCODED_AUDIO;
	format->u.encoded_audio.encoding
     = (media_encoded_audio_format::audio_encoding)'vorb';
	return B_OK;
}


/*---- speex ----*/


static bool
isSpeexPacket(ogg_packet * packet)
{
	return findIdentifier(packet,"Speex",0);
}

static const int speex_extra_packet_count = 2;

static status_t
GetSpeexCodecStreamInfo(std::vector<ogg_packet> * packets,
                         int64 *frameCount, bigtime_t *duration,
                         media_format *format)
{
	format->type = B_MEDIA_ENCODED_AUDIO;
	format->u.encoded_audio.encoding
     = (media_encoded_audio_format::audio_encoding)'Spee';
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

static const int tobias_extra_packet_count = 1;

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
	int32 encoding = header->subtype[3] << 24 | header->subtype[2] << 16 
                   | header->subtype[1] <<  8 | header->subtype[0];
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
		return vorbis_extra_packet_count;
	}
	if (isTobiasPacket(packet)) {
		return tobias_extra_packet_count;
	}
	if (isSpeexPacket(packet)) {
		return speex_extra_packet_count;
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
