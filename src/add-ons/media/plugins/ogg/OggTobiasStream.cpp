#include "OggTobiasFormats.h"
#include "OggTobiasStream.h"
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

/*
 * OggTobiasStream implementations
 */

/* static */ bool
OggTobiasStream::IsValidHeader(const ogg_packet & packet)
{
	return findIdentifier(packet,"video",1);
}

OggTobiasStream::OggTobiasStream(long serialno)
	: OggStream(serialno)
{
	TRACE("OggTobiasStream::OggTobiasStream\n");
}

OggTobiasStream::~OggTobiasStream()
{
	TRACE("OggTobiasStream::~OggTobiasStream\n");
}

status_t
OggTobiasStream::GetStreamInfo(int64 *frameCount, bigtime_t *duration,
                               media_format *format)
{
	TRACE("OggTobiasStream::GetStreamInfo\n");
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
	if (packet.bytes < 1+(signed)sizeof(tobias_stream_header)) {
		return B_ERROR;
	}
	void * data = &(packet.packet[1]);
	tobias_stream_header * header = (tobias_stream_header *)data;

	if (strcmp(header->streamtype, "video") == 0) {
		// get the format for the description
		media_format_description description = tobias_video_description();
		description.u.avi.codec = header->subtype[3] << 24 | header->subtype[2] << 16 
		                        | header->subtype[1] <<  8 | header->subtype[0];
		BMediaFormats formats;
		result = formats.InitCheck();
		if (result == B_OK) {
			result = formats.GetFormatFor(description, format);
		}
		if (result != B_OK) {
			*format = tobias_video_encoded_media_format();
			// ignore error, allow user to use ReadChunk interface
		}
	
		// fill out format from header packet
		format->user_data_type = B_CODEC_TYPE_INFO;
		strncpy((char*)format->user_data, header->subtype, 4);
		format->u.encoded_video.frame_size
		   = header->video.width * header->video.height;
		format->u.encoded_video.output.field_rate = 10000000.0 / header->time_unit;
		format->u.encoded_video.output.interlace = 1;
		format->u.encoded_video.output.first_active = 0;
		format->u.encoded_video.output.last_active = header->video.height - 1;
		format->u.encoded_video.output.orientation = B_VIDEO_TOP_LEFT_RIGHT;
		format->u.encoded_video.output.pixel_width_aspect = 1;
		format->u.encoded_video.output.pixel_height_aspect = 1;
		format->u.encoded_video.output.display.line_width = header->video.width;
		format->u.encoded_video.output.display.line_count = header->video.height;
		format->u.encoded_video.output.display.bytes_per_row = 0;
		format->u.encoded_video.output.display.pixel_offset = 0;
		format->u.encoded_video.output.display.line_offset = 0;
		format->u.encoded_video.output.display.flags = 0;

		// TODO: wring more info out of the headers
	}

	// get comment packet
	if (GetHeaderPackets().size() < 2) {
		result = GetPacket(&packet);
		if (result != B_OK) {
			return result;
		}
		SaveHeaderPacket(packet);
	}

	format->SetMetaData((void*)&GetHeaderPackets(),sizeof(GetHeaderPackets()));
	*frameCount = 2000000;
	*duration = *frameCount * format->u.encoded_video.output.field_rate;
	fMediaFormat = *format;
	return B_OK;
}

status_t
OggTobiasStream::GetNextChunk(void **chunkBuffer, int32 *chunkSize,
                             media_header *mediaHeader)
{
	status_t result = GetPacket(&fChunkPacket);
	if (result != B_OK) {
		TRACE("OggTobiasStream::GetNextChunk failed: GetPacket = %s\n", strerror(result));
		return result;
	}
	*chunkBuffer = fChunkPacket.packet;
	*chunkSize = fChunkPacket.bytes;
	bool keyframe = fChunkPacket.packet[0] & (1 << 3); // ??
	if (fMediaFormat.type == B_MEDIA_ENCODED_VIDEO) {
		mediaHeader->type = fMediaFormat.type;
		mediaHeader->start_time = fCurrentTime;
		mediaHeader->u.encoded_video.field_flags = (keyframe ? B_MEDIA_KEY_FRAME : 0);
		mediaHeader->u.encoded_video.first_active_line
		   = fMediaFormat.u.encoded_video.output.first_active;
		mediaHeader->u.encoded_video.line_count
		   = fMediaFormat.u.encoded_video.output.display.line_count;
		fCurrentFrame++;
		fCurrentTime = 1000000LL * fCurrentFrame / fMediaFormat.u.encoded_video.output.field_rate;
	}
	return B_OK;
}
