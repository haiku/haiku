#include "OggVorbisFormats.h"
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
	fFrameCount = -1;
	fDuration = -1;
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

	// use cached frame count and duration for now
	if (fDuration >= 0) {
		*frameCount = fFrameCount;
		*duration = fDuration;
		return B_OK;
	}

	void *chunkBuffer = 0;
	int32 chunkSize = 0;
	media_header mediaHeader;
	if (GetNextChunk(&chunkBuffer, &chunkSize, &mediaHeader) != B_OK) {
		*frameCount = 0;
		*duration = 0;
		return B_OK;
	}
	ogg_packet * chunk_packet = static_cast<ogg_packet *>(chunkBuffer);
	ogg_int64_t first_packet_granulepos = chunk_packet->granulepos;
	ogg_int64_t last_packet_granulepos = chunk_packet->granulepos;
	ogg_int64_t next_packet = chunk_packet->packetno + 1;
	int64 samples = 0;
	while (GetNextChunk(&chunkBuffer, &chunkSize, &mediaHeader) == B_OK) {
		chunk_packet = static_cast<ogg_packet *>(chunkBuffer);
		if (chunk_packet->packetno != next_packet) {
			// there's a hole in the data, add samples so far, and start counting again
			TRACE("OggVorbisStream::GetStreamInfo: substream end\n");
			samples += last_packet_granulepos - first_packet_granulepos;
			first_packet_granulepos = chunk_packet->granulepos;
		}
		last_packet_granulepos = chunk_packet->granulepos;
		next_packet = chunk_packet->packetno + 1;
	}
	samples += last_packet_granulepos - first_packet_granulepos;

	// compute frame count and duration from sample count
	*frameCount = fFrameCount = samples;
	*duration = fDuration = (1000000LL * samples) / (long long)format->u.encoded_audio.output.frame_rate;

	// restore our position to the start
	int64 start_frame = 0;
	bigtime_t start_time = 0;
	Seek(B_MEDIA_SEEK_TO_FRAME, &start_frame, &start_time);

	return B_OK;
}
