#include <stdio.h>
#include <Autolock.h>
#include <DataIO.h>
#include <Locker.h>
#include <MediaFormats.h>
#include <MediaRoster.h>
#include <vector>
#include "ogg/ogg.h"
#include "speexCodecPlugin.h"

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define DECODE_BUFFER_SIZE	(32 * 1024)

inline size_t
AudioBufferSize(int32 channel_count, uint32 sample_format, float frame_rate, bigtime_t buffer_duration = 50000 /* 50 ms */)
{
	return (sample_format & 0xf) * channel_count * (size_t)((frame_rate * buffer_duration) / 1000000.0);
}

speexDecoder::speexDecoder()
{
	TRACE("speexDecoder::speexDecoder\n");

	fStartTime = 0;
	fFrameSize = 0;
	fOutputBufferSize = 0;
}


speexDecoder::~speexDecoder()
{
	TRACE("speexDecoder::~speexDecoder\n");
}


status_t
speexDecoder::Setup(media_format *inputFormat,
				  const void *infoBuffer, int32 infoSize)
{
	if (inputFormat->type != B_MEDIA_ENCODED_AUDIO) {
		TRACE("speexDecoder::Setup not called with audio stream: not speex\n");
		return B_ERROR;
	}
	if (inputFormat->u.encoded_audio.encoding != 'Spee') {
		TRACE("speexDecoder::Setup not called with 'vorb' stream: not speex\n");
		return B_ERROR;
	}
	if (inputFormat->MetaDataSize() != sizeof(std::vector<ogg_packet> *)) {
		TRACE("speexDecoder::Setup not called with ogg_packet<vector> meta data: not speex\n");
		return B_ERROR;
	}
	std::vector<ogg_packet> * packets = (std::vector<ogg_packet> *)inputFormat->MetaData();
	if (packets->size() != 2) {
		TRACE("speexDecoder::Setup not called with two ogg_packets: not speex\n");
		return B_ERROR;
	}
	debugger("speexDecoder::Setup");
	// fill out the encoding format
	CopyInfoToEncodedFormat(inputFormat);
	return B_OK;
}

void speexDecoder::CopyInfoToEncodedFormat(media_format * format) {
	format->type = B_MEDIA_ENCODED_AUDIO;
	format->u.encoded_audio.encoding
	 = (media_encoded_audio_format::audio_encoding)'Spee';
/*	if (fInfo.bitrate_nominal > 0) {
		format->u.encoded_audio.bit_rate = fInfo.bitrate_nominal;
	} else if (fInfo.bitrate_upper > 0) {
		format->u.encoded_audio.bit_rate = fInfo.bitrate_upper;
	} else if (fInfo.bitrate_lower > 0) {
		format->u.encoded_audio.bit_rate = fInfo.bitrate_lower;
	}
*/
	CopyInfoToDecodedFormat(&format->u.encoded_audio.output);
	format->u.encoded_audio.frame_size = sizeof(ogg_packet);
}	

void speexDecoder::CopyInfoToDecodedFormat(media_raw_audio_format * raf) {
/*
	raf->frame_rate = (float)fInfo.rate; // XXX long->float ??
	raf->channel_count = fInfo.channels;
	raf->format = media_raw_audio_format::B_AUDIO_FLOAT; // XXX verify: support others?
	raf->byte_order = B_MEDIA_HOST_ENDIAN; // XXX should support other endain, too

	if (raf->buffer_size < 512 || raf->buffer_size > 65536) {
		raf->buffer_size = AudioBufferSize(raf->channel_count,raf->format,raf->frame_rate);
    }
	// setup output variables
	fFrameSize = (raf->format & 0xf) * fInfo.channels;
*/
	fOutputBufferSize = raf->buffer_size;
}

status_t
speexDecoder::NegotiateOutputFormat(media_format *ioDecodedFormat)
{
	TRACE("speexDecoder::NegotiateOutputFormat\n");
	// BeBook says: The codec will find and return in ioFormat its best matching format
	// => This means, we never return an error, and always change the format values
	//    that we don't support to something more applicable
	ioDecodedFormat->type = B_MEDIA_RAW_AUDIO;
	CopyInfoToDecodedFormat(&ioDecodedFormat->u.raw_audio);
	// add the media_mult_audio_format fields
	if (ioDecodedFormat->u.raw_audio.channel_mask == 0) {
/*		if (fInfo.channels == 1) {
			ioDecodedFormat->u.raw_audio.channel_mask = B_CHANNEL_LEFT;
		} else {
			ioDecodedFormat->u.raw_audio.channel_mask = B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
		}
*/
	}
	return B_OK;
}


status_t
speexDecoder::Seek(uint32 seekTo,
				 int64 seekFrame, int64 *frame,
				 bigtime_t seekTime, bigtime_t *time)
{
	TRACE("speexDecoder::Seek\n");
/*
	float **pcm;
	// throw the old samples away!
	int samples = speex_synthesis_pcmout(&fDspState,&pcm);
	speex_synthesis_read(&fDspState,samples);
*/
	return B_OK;
}


status_t
speexDecoder::Decode(void *buffer, int64 *frameCount,
				   media_header *mediaHeader, media_decode_info *info /* = 0 */)
{
//	TRACE("speexDecoder::Decode\n");
	uint8 * out_buffer = static_cast<uint8 *>(buffer);
	int32	out_bytes_needed = fOutputBufferSize;
	
	mediaHeader->start_time = fStartTime;
	//TRACE("speexDecoder: Decoding start time %.6f\n", fStartTime / 1000000.0);

	debugger("speexDecoder::Decode");
	while (out_bytes_needed > 0) {
/*		int samples;
		float **pcm;
		while ((samples = speex_synthesis_pcmout(&fDspState,&pcm)) == 0) {
			// get a new packet
			void *chunkBuffer;
			int32 chunkSize;
			media_header mh;
			status_t status = GetNextChunk(&chunkBuffer, &chunkSize, &mh);
			if (status == B_LAST_BUFFER_ERROR) {
				goto done;
			}			
			if (status != B_OK) {
				TRACE("speexDecoder::Decode: GetNextChunk failed\n");
				return status;
			}
			if (chunkSize != sizeof(ogg_packet)) {
				TRACE("speexDecoder::Decode: chunk not ogg_packet-sized\n");
				return B_ERROR;
			}
			ogg_packet * packet = static_cast<ogg_packet*>(chunkBuffer);
			if (speex_synthesis(&fBlock,packet)==0) {
				speex_synthesis_blockin(&fDspState,&fBlock);
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
		speex_synthesis_read(&fDspState,samples);
		
		fStartTime += (1000000LL * samples) / fInfo.rate;
*/
		//TRACE("speexDecoder: fStartTime inc'd to %.6f\n", fStartTime / 1000000.0);
	}
	
done:
	*frameCount = (fOutputBufferSize - out_bytes_needed) / fFrameSize;

	if (out_buffer != buffer) {
		return B_OK;
	}
	return B_LAST_BUFFER_ERROR;
}

Decoder *
speexDecoderPlugin::NewDecoder()
{
	static BLocker locker;
	static bool initdone = false;
	BAutolock lock(locker);
	if (!initdone) {
		initdone = true;
	}
	return new speexDecoder;
}


status_t
speexDecoderPlugin::RegisterPlugin()
{
	PublishDecoder("audiocodec/speex", "speex", "speex decoder, based on libspeex");
	return B_OK;
}


MediaPlugin *instantiate_plugin()
{
	return new speexDecoderPlugin;
}
