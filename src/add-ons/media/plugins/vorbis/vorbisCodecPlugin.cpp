#include <stdio.h>
#include <Autolock.h>
#include <DataIO.h>
#include <Locker.h>
#include <MediaFormats.h>
#include <MediaRoster.h>
#include <vector>
#include "vorbisCodecPlugin.h"
#include "OggVorbisFormats.h"

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define DECODE_BUFFER_SIZE	(32 * 1024)


inline void
AdjustBufferSize(media_raw_audio_format * raf, bigtime_t buffer_duration = 50000 /* 50 ms */)
{
	size_t frame_size = (raf->format & 0xf) * raf->channel_count;
	if (raf->buffer_size <= frame_size) {
		raf->buffer_size = frame_size * (size_t)((raf->frame_rate * buffer_duration) / 1000000.0);
	} else {
		raf->buffer_size = (raf->buffer_size / frame_size) * frame_size;
	}
}


static media_format
vorbis_decoded_media_format()
{
	media_format format;
	format.type = B_MEDIA_RAW_AUDIO;
	init_vorbis_media_raw_audio_format(&format.u.raw_audio);
	return format;
}


/*
 * VorbisDecoder
 */


VorbisDecoder::VorbisDecoder()
{
	TRACE("VorbisDecoder::VorbisDecoder\n");
	vorbis_info_init(&fInfo);
	vorbis_comment_init(&fComment);
	fStartTime = 0;
	fFrameSize = 0;
	fOutputBufferSize = 0;
}


VorbisDecoder::~VorbisDecoder()
{
	TRACE("VorbisDecoder::~VorbisDecoder\n");
	vorbis_info_clear(&fInfo);
	vorbis_comment_clear(&fComment);
}


void
VorbisDecoder::GetCodecInfo(media_codec_info *info)
{
	strncpy(info->short_name, "vorbis-libvorbis", sizeof(info->short_name));
	strncpy(info->pretty_name, "vorbis decoder [libvorbis], by Andrew Bachmann", sizeof(info->pretty_name));
}


status_t
VorbisDecoder::Setup(media_format *inputFormat,
				  const void *infoBuffer, int32 infoSize)
{
	TRACE("VorbisDecoder::Setup\n");
	if (!format_is_compatible(vorbis_encoded_media_format(),*inputFormat)) {
		return B_MEDIA_BAD_FORMAT;
	}
	// grab header packets from meta data
	if (inputFormat->MetaDataSize() != sizeof(std::vector<ogg_packet>)) {
		TRACE("VorbisDecoder::Setup not called with ogg_packet<vector> meta data: not vorbis\n");
		return B_ERROR;
	}
	std::vector<ogg_packet> * packets = (std::vector<ogg_packet> *)inputFormat->MetaData();
	if (packets->size() != 3) {
		TRACE("VorbisDecoder::Setup not called with three ogg_packets: not vorbis\n");
		return B_ERROR;
	}
	// parse header packet
	if (vorbis_synthesis_headerin(&fInfo,&fComment,&(*packets)[0]) != 0) {
		TRACE("VorbisDecoder::Setup: vorbis_synthesis_headerin failed: not vorbis header\n");
		return B_ERROR;
	}
	// parse comment packet
	if (vorbis_synthesis_headerin(&fInfo,&fComment,&(*packets)[1]) != 0) {
		TRACE("VorbisDecoder::Setup: vorbis_synthesis_headerin failed: not vorbis comment\n");
		return B_ERROR;
	}
	// parse codebook packet
	if (vorbis_synthesis_headerin(&fInfo,&fComment,&(*packets)[2]) != 0) {
		TRACE("VorbisDecoder::Setup: vorbis_synthesis_headerin failed: not vorbis codebook\n");
		return B_ERROR;
	}
	// initialize decoder
	vorbis_synthesis_init(&fDspState,&fInfo);
	vorbis_block_init(&fDspState,&fBlock);
	// setup default output
	media_format requested_format = vorbis_decoded_media_format();
	((media_raw_audio_format)requested_format.u.raw_audio) = inputFormat->u.encoded_audio.output;
	return NegotiateOutputFormat(&requested_format);
}


status_t
VorbisDecoder::NegotiateOutputFormat(media_format *ioDecodedFormat)
{
	TRACE("VorbisDecoder::NegotiateOutputFormat\n");
	// BMediaTrack::DecodedFormat
	// Pass in ioFormat the format that you want (with wildcards
	// as applicable). The codec will find and return in ioFormat 
	// its best matching format.
	//
	// BMediaDecoder::SetOutputFormat
	// sets the format the decoder should output. On return, 
	// the outputFormat is changed to match the actual format
	// that will be output; this can be different if you 
	// specified any wildcards.
	//
	// Be R5 behavior seems to be that we can never fail.  If we
	// don't support the requested format, just return one we do.
	media_format format = vorbis_decoded_media_format();
	format.u.raw_audio.frame_rate = (float)fInfo.rate;
	format.u.raw_audio.channel_count = fInfo.channels;
	format.u.raw_audio.channel_mask = B_CHANNEL_LEFT | (fInfo.channels != 1 ? B_CHANNEL_RIGHT : 0);
	if (!format_is_compatible(format,*ioDecodedFormat)) {
		*ioDecodedFormat = format;
	}
	ioDecodedFormat->SpecializeTo(&format);
	AdjustBufferSize(&ioDecodedFormat->u.raw_audio);
	// setup output variables
	fFrameSize = ioDecodedFormat->AudioFrameSize();
	fOutputBufferSize = ioDecodedFormat->u.raw_audio.buffer_size;
	return B_OK;
}


status_t
VorbisDecoder::Seek(uint32 seekTo,
				 int64 seekFrame, int64 *frame,
				 bigtime_t seekTime, bigtime_t *time)
{
	TRACE("VorbisDecoder::Seek\n");
	float **pcm;
	// throw the old samples away!
	int samples = vorbis_synthesis_pcmout(&fDspState,&pcm);
	vorbis_synthesis_read(&fDspState,samples);
	return B_OK;
}


status_t
VorbisDecoder::Decode(void *buffer, int64 *frameCount,
				   media_header *mediaHeader, media_decode_info *info /* = 0 */)
{
//	TRACE("VorbisDecoder::Decode\n");
	uint8 * out_buffer = static_cast<uint8 *>(buffer);
	int32	out_bytes_needed = fOutputBufferSize;

	bool synced = false;
	
	int total_samples = 0;
	while (out_bytes_needed > 0) {
		int samples;
		float **pcm;
		while ((samples = vorbis_synthesis_pcmout(&fDspState,&pcm)) == 0) {
			// get a new packet
			void *chunkBuffer;
			int32 chunkSize;
			media_header mh;
			status_t status = GetNextChunk(&chunkBuffer, &chunkSize, &mh);
			if (status == B_LAST_BUFFER_ERROR) {
				goto done;
			}
			if (status != B_OK) {
				TRACE("VorbisDecoder::Decode: GetNextChunk failed\n");
				return status;
			}
			ogg_packet packet;
			if (mh.user_data_type == OGG_PACKET_DATA_TYPE) {
				memcpy(&packet, mh.user_data, sizeof(packet));
			} else {
				// According to http://lists.xiph.org/pipermail/theora-dev/2004-May/002161.html
				// this is invalid, but results from it are tolerable and better than nothing.
				TRACE("VorbisDecoder::Decode: using compatibility chunk interpretation\n");
				packet.b_o_s = 0;
				packet.e_o_s = 0;
				packet.granulepos = -1;
				packet.packetno = 7;
				packet.packet = static_cast<unsigned char *>(chunkBuffer);
				packet.bytes = chunkSize;
			}
			if (!synced) {
				if (mh.start_time > 0) {
					mediaHeader->start_time = mh.start_time - (1000000LL * total_samples) / fInfo.rate;
					synced = true;
				}
			}
			if (vorbis_synthesis(&fBlock, &packet)==0) {
				vorbis_synthesis_blockin(&fDspState,&fBlock);
			}
		}
		// reduce samples to the amount of samples we will actually consume
		samples = min_c(samples,out_bytes_needed/fFrameSize);
		total_samples += samples;
		for (int sample = 0; sample < samples ; sample++) {
			for (int channel = 0; channel < fInfo.channels; channel++) {
				*((float*)out_buffer) = pcm[channel][sample];
				out_buffer += sizeof(float);
			}
		}
		out_bytes_needed -= samples * fFrameSize;
		// report back how many samples we consumed
		vorbis_synthesis_read(&fDspState,samples);
		
	}
	
done:
	if (!synced) {
		mediaHeader->start_time = fStartTime;
	}
	fStartTime = mediaHeader->start_time + (1000000LL * total_samples) / fInfo.rate;

	*frameCount = (fOutputBufferSize - out_bytes_needed) / fFrameSize;

	if (out_buffer != buffer) {
		return B_OK;
	}
	return B_LAST_BUFFER_ERROR;
}


/*
 * VorbisDecoderPlugin
 */


Decoder *
VorbisDecoderPlugin::NewDecoder(uint index)
{
	static BLocker locker;
	static bool initdone = false;
	BAutolock lock(locker);
	if (!initdone) {
		initdone = true;
	}
	return new VorbisDecoder;
}

static media_format vorbis_formats[1];

status_t
VorbisDecoderPlugin::GetSupportedFormats(media_format ** formats, size_t * count)
{
	media_format_description description = vorbis_description();
	media_format format = vorbis_encoded_media_format();

	BMediaFormats mediaFormats;
	status_t result = mediaFormats.InitCheck();
	if (result != B_OK) {
		return result;
	}
	result = mediaFormats.MakeFormatFor(&description, 1, &format);
	if (result != B_OK) {
		return result;
	}
	vorbis_formats[0] = format;

	*formats = vorbis_formats;
	*count = 1;

	return result;
}


/*
 * instantiate_plugin
 */


MediaPlugin *instantiate_plugin()
{
	return new VorbisDecoderPlugin;
}
