#include <stdio.h>
#include <Autolock.h>
#include <DataIO.h>
#include <Locker.h>
#include <MediaFormats.h>
#include <MediaRoster.h>
#include <vector>
#include "vorbisCodecPlugin.h"

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define DECODE_BUFFER_SIZE	(32 * 1024)

inline size_t
AudioBufferSize(media_raw_audio_format * raf, bigtime_t buffer_duration = 50000 /* 50 ms */)
{
	return (raf->format & 0xf) * (raf->channel_count)
         * (size_t)((raf->frame_rate * buffer_duration) / 1000000.0);
}

vorbisDecoder::vorbisDecoder()
{
	TRACE("vorbisDecoder::vorbisDecoder\n");
	vorbis_info_init(&fInfo);
	vorbis_comment_init(&fComment);

	fStartTime = 0;
	fFrameSize = 0;
	fOutputBufferSize = 0;
}


vorbisDecoder::~vorbisDecoder()
{
	TRACE("vorbisDecoder::~vorbisDecoder\n");
}


void
vorbisDecoder::GetCodecInfo(media_codec_info &info)
{
	strcpy(info.short_name, "vorbis");
	strcpy(info.pretty_name, "vorbis decoder, based on libvorbis");
}


status_t
vorbisDecoder::Setup(media_format *inputFormat,
				  const void *infoBuffer, int32 infoSize)
{
	if (inputFormat->type != B_MEDIA_ENCODED_AUDIO) {
		TRACE("vorbisDecoder::Setup not called with audio stream: not vorbis\n");
		return B_ERROR;
	}
	if (inputFormat->u.encoded_audio.encoding != 'vorb') {
		TRACE("vorbisDecoder::Setup not called with 'vorb' stream: not vorbis\n");
		return B_ERROR;
	}
	if (inputFormat->MetaDataSize() != sizeof(std::vector<ogg_packet> *)) {
		TRACE("vorbisDecoder::Setup not called with ogg_packet<vector> meta data: not vorbis\n");
		return B_ERROR;
	}
	std::vector<ogg_packet> * packets = (std::vector<ogg_packet> *)inputFormat->MetaData();
	if (packets->size() != 3) {
		TRACE("vorbisDecoder::Setup not called with three ogg_packets: not vorbis\n");
		return B_ERROR;
	}
	// parse header packet
	if (vorbis_synthesis_headerin(&fInfo,&fComment,&(*packets)[0]) != 0) {
		TRACE("vorbisDecoder::Setup: vorbis_synthesis_headerin failed: not vorbis header\n");
		return B_ERROR;
	}
	// parse comment packet
	if (vorbis_synthesis_headerin(&fInfo,&fComment,&(*packets)[1]) != 0) {
		TRACE("vorbiseDecoder::Setup: vorbis_synthesis_headerin failed: not vorbis comment\n");
		return B_ERROR;
	}
	// parse codebook packet
	if (vorbis_synthesis_headerin(&fInfo,&fComment,&(*packets)[2]) != 0) {
		TRACE("vorbiseDecoder::Setup: vorbis_synthesis_headerin failed: not vorbis codebook\n");
		return B_ERROR;
	}
	// initialize decoder
	vorbis_synthesis_init(&fDspState,&fInfo);
	vorbis_block_init(&fDspState,&fBlock);
	// fill out the encoding format
	CopyInfoToEncodedFormat(inputFormat);
	return B_OK;
}

void vorbisDecoder::CopyInfoToEncodedFormat(media_format * format) {
	format->type = B_MEDIA_ENCODED_AUDIO;
// ToDo: this won't work any longer
//	format->u.encoded_audio.encoding
//	 = (media_encoded_audio_format::audio_encoding)'vorb';
	if (fInfo.bitrate_nominal > 0) {
		format->u.encoded_audio.bit_rate = fInfo.bitrate_nominal;
	} else if (fInfo.bitrate_upper > 0) {
		format->u.encoded_audio.bit_rate = fInfo.bitrate_upper;
	} else if (fInfo.bitrate_lower > 0) {
		format->u.encoded_audio.bit_rate = fInfo.bitrate_lower;
	}
	CopyInfoToDecodedFormat(&format->u.encoded_audio.output);
	format->u.encoded_audio.frame_size = sizeof(ogg_packet);
}	

void vorbisDecoder::CopyInfoToDecodedFormat(media_raw_audio_format * raf) {
	raf->frame_rate = (float)fInfo.rate; // XXX long->float ??
	raf->channel_count = fInfo.channels;
	raf->format = media_raw_audio_format::B_AUDIO_FLOAT; // XXX verify: support others?
	raf->byte_order = B_MEDIA_HOST_ENDIAN; // XXX should support other endain, too

	if (raf->buffer_size < 512 || raf->buffer_size > 65536) {
		raf->buffer_size = AudioBufferSize(raf);
    }
	// setup output variables
	fFrameSize = (raf->format & 0xf) * raf->channel_count;
	fOutputBufferSize = raf->buffer_size;
}

status_t
vorbisDecoder::NegotiateOutputFormat(media_format *ioDecodedFormat)
{
	TRACE("vorbisDecoder::NegotiateOutputFormat\n");
	// BeBook says: The codec will find and return in ioFormat its best matching format
	// => This means, we never return an error, and always change the format values
	//    that we don't support to something more applicable
	ioDecodedFormat->type = B_MEDIA_RAW_AUDIO;
	CopyInfoToDecodedFormat(&ioDecodedFormat->u.raw_audio);
	// add the media_mult_audio_format fields
	if (ioDecodedFormat->u.raw_audio.channel_mask == 0) {
		if (fInfo.channels == 1) {
			ioDecodedFormat->u.raw_audio.channel_mask = B_CHANNEL_LEFT;
		} else {
			ioDecodedFormat->u.raw_audio.channel_mask = B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
		}
	}
	return B_OK;
}


status_t
vorbisDecoder::Seek(uint32 seekTo,
				 int64 seekFrame, int64 *frame,
				 bigtime_t seekTime, bigtime_t *time)
{
	TRACE("vorbisDecoder::Seek\n");
	float **pcm;
	// throw the old samples away!
	int samples = vorbis_synthesis_pcmout(&fDspState,&pcm);
	vorbis_synthesis_read(&fDspState,samples);
	return B_OK;
}


status_t
vorbisDecoder::Decode(void *buffer, int64 *frameCount,
				   media_header *mediaHeader, media_decode_info *info /* = 0 */)
{
//	TRACE("vorbisDecoder::Decode\n");
	uint8 * out_buffer = static_cast<uint8 *>(buffer);
	int32	out_bytes_needed = fOutputBufferSize;
	
	mediaHeader->start_time = fStartTime;
	//TRACE("vorbisDecoder: Decoding start time %.6f\n", fStartTime / 1000000.0);
	
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
				TRACE("vorbisDecoder::Decode: GetNextChunk failed\n");
				return status;
			}
			if (chunkSize != sizeof(ogg_packet)) {
				TRACE("vorbisDecoder::Decode: chunk not ogg_packet-sized\n");
				return B_ERROR;
			}
			ogg_packet * packet = static_cast<ogg_packet*>(chunkBuffer);
			if (vorbis_synthesis(&fBlock,packet)==0) {
				vorbis_synthesis_blockin(&fDspState,&fBlock);
			}
		}
		// reduce samples to the amount of samples we will actually consume
		samples = min_c(samples,out_bytes_needed/fFrameSize);
		for (int sample = 0; sample < samples ; sample++) {
			for (int channel = 0; channel < fInfo.channels; channel++) {
				*((float*)out_buffer) = pcm[channel][sample];
				out_buffer += sizeof(float);
			}
		}
		out_bytes_needed -= samples * fInfo.channels * sizeof(float);
		// report back how many samples we consumed
		vorbis_synthesis_read(&fDspState,samples);
		
		fStartTime += (1000000LL * samples) / fInfo.rate;
		//TRACE("vorbisDecoder: fStartTime inc'd to %.6f\n", fStartTime / 1000000.0);
	}
	
done:
	*frameCount = (fOutputBufferSize - out_bytes_needed) / fFrameSize;

	if (out_buffer != buffer) {
		return B_OK;
	}
	return B_LAST_BUFFER_ERROR;
}

Decoder *
vorbisDecoderPlugin::NewDecoder()
{
	static BLocker locker;
	static bool initdone = false;
	BAutolock lock(locker);
	if (!initdone) {
		initdone = true;
	}
	return new vorbisDecoder;
}


status_t
vorbisDecoderPlugin::RegisterDecoder()
{
	media_format_description description;
	// ToDo: what about B_OGG_FORMAT_FAMILY?
	description.family = B_MISC_FORMAT_FAMILY;
	description.u.misc.file_format = 'ogg ';
	description.u.misc.codec = 'vorb';

	media_format format;
	format.type = B_MEDIA_ENCODED_AUDIO;
	format.u.encoded_audio = media_encoded_audio_format::wildcard;

	return BMediaFormats().MakeFormatFor(&description, 1, &format);
}


MediaPlugin *instantiate_plugin()
{
	return new vorbisDecoderPlugin;
}
