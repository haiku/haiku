#include <stdio.h>
#include <Autolock.h>
#include <DataIO.h>
#include <Locker.h>
#include <MediaFormats.h>
#include <MediaRoster.h>
#include <vector>
#include "ogg/ogg.h"
#include "speexCodecPlugin.h"
#include "speexCodecDefaults.h"

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define DECODE_BUFFER_SIZE	(32 * 1024)

speexDecoder::speexDecoder()
{
	TRACE("speexDecoder::speexDecoder\n");
	speex_bits_init(&fBits);
	fDecoderState = 0;
	fHeader = 0;
	fStereoState = 0;
	fSpeexOutputLength = 0;
	fStartTime = 0;
	fFrameSize = 0;
	fOutputBufferSize = 0;
}


speexDecoder::~speexDecoder()
{
	TRACE("speexDecoder::~speexDecoder\n");
	// the fStereoState is destroyed by fDecoderState
//	delete fStereoState;
	speex_bits_destroy(&fBits);
	speex_decoder_destroy(fDecoderState);
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
		TRACE("speexDecoder::Setup not called with 'Spee' stream: not speex\n");
		return B_ERROR;
	}
	if (inputFormat->MetaDataSize() != sizeof(std::vector<ogg_packet> *)) {
		TRACE("speexDecoder::Setup not called with ogg_packet<vector> meta data: not speex\n");
		return B_ERROR;
	}
	std::vector<ogg_packet> * packets = (std::vector<ogg_packet> *)inputFormat->MetaData();
	if (packets->size() < 2) {
		TRACE("speexDecoder::Setup not called with at least two ogg_packets: not speex\n");
		return B_ERROR;
	}
	// parse header packet
	ogg_packet * packet = &(*packets)[0];
	fHeader = speex_packet_to_header((char*)packet->packet, packet->bytes);
	if (fHeader == NULL) {
		TRACE("speexDecoder::Setup failed in ogg_packet to speex_header conversion\n");
		return B_ERROR;
	}
	if (packets->size() != 2 + (unsigned)fHeader->extra_headers) {
		TRACE("speexDecoder::Setup not called with all the extra headers\n");
		delete fHeader;
		fHeader = 0;
		return B_ERROR;
	}
	// modify header to reflect settings
	switch (SpeexSettings::PreferredBand()) {
	case narrow_band:
		fHeader->mode = 0; 
		break;
	case wide_band:
		fHeader->mode = 1;
		break;
	case ultra_wide_band:
		fHeader->mode = 2;
		break;
	case automatic_band:
	default:
		break;
	}
	switch (SpeexSettings::PreferredChannels()) {
	case mono_channels:
		fHeader->nb_channels = 1;
		break;
	case stereo_channels:
		fHeader->nb_channels = 2;
		break;
	case automatic_channels:
	default:
		break;
	}
	if (fHeader->nb_channels == 0) {
		fHeader->nb_channels = 1;
	}
	if (fHeader->frames_per_packet == 0) {
		fHeader->frames_per_packet = 1;
	}
	if (SpeexSettings::SamplingRate() != 0) {
		fHeader->rate = SpeexSettings::SamplingRate();
	}
	// sanity checks
#ifdef STRICT_SPEEX
	if (header->speex_version_id > 1) {
		TRACE("speexDecoder::Setup failed: version id too new");
		return B_ERROR;
	}
#endif
	if (fHeader->mode >= SPEEX_NB_MODES) {
		TRACE("speexDecoder::Setup failed: unknown speex mode\n");
		return B_ERROR;
	}
	// setup from header
	SpeexMode * mode = speex_mode_list[fHeader->mode];
#ifdef STRICT_SPEEX
	if (mode->bitstream_version != fHeader->mode_bitstream_version) {
		TRACE("speexDecoder::Setup failed: bitstream version mismatch");
		return B_ERROR;
	}
#endif
	fDecoderState = speex_decoder_init(mode);
	if (fDecoderState == NULL) {
		TRACE("speexDecoder::Setup failed to initialize the decoder state");
		return B_ERROR;
	}
	if (SpeexSettings::PerceptualPostFilter()) {
		int enabled = 1;
		speex_decoder_ctl(fDecoderState, SPEEX_SET_ENH, &enabled);
	}
	speex_decoder_ctl(fDecoderState, SPEEX_GET_FRAME_SIZE, &fHeader->frame_size);
	fSpeexOutputLength = fHeader->frame_size * sizeof(float) * fHeader->nb_channels;
	if (fHeader->nb_channels == 2) {
		SpeexCallback callback;
		SpeexStereoState stereo = SPEEX_STEREO_STATE_INIT;
		callback.callback_id = SPEEX_INBAND_STEREO;
		callback.func = speex_std_stereo_request_handler;
		fStereoState = new SpeexStereoState(stereo);
		callback.data = fStereoState;
		speex_decoder_ctl(fDecoderState, SPEEX_SET_HANDLER, &callback);
	}
	speex_decoder_ctl(fDecoderState, SPEEX_SET_SAMPLING_RATE, &fHeader->rate);
	// fill out the encoding format
	CopyInfoToEncodedFormat(inputFormat);
	return B_OK;
}

void speexDecoder::CopyInfoToEncodedFormat(media_format * format) {
	format->type = B_MEDIA_ENCODED_AUDIO;
	format->user_data_type = B_CODEC_TYPE_INFO;
	strncpy((char*)format->user_data,"Spee",4);
	format->u.encoded_audio.encoding
	 = (media_encoded_audio_format::audio_encoding)'Spee';
	if (fHeader->bitrate > 0) {
		format->u.encoded_audio.bit_rate = fHeader->bitrate;
	} else {
		speex_decoder_ctl(fDecoderState, SPEEX_GET_BITRATE, &fHeader->bitrate);
		format->u.encoded_audio.bit_rate = fHeader->bitrate;
	}
	if (fHeader->nb_channels == 1) {
		format->u.encoded_audio.multi_info.channel_mask = B_CHANNEL_LEFT;
	} else {
		format->u.encoded_audio.multi_info.channel_mask = B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
	}
	CopyInfoToDecodedFormat(&format->u.encoded_audio.output);
	format->u.encoded_audio.frame_size = sizeof(ogg_packet);
}	

inline size_t
AudioBufferSize(media_raw_audio_format * raf, bigtime_t buffer_duration = 50000 /* 50 ms */)
{
	return (raf->format & 0xf) * (raf->channel_count)
         * (size_t)((raf->frame_rate * buffer_duration) / 1000000.0);
}

void speexDecoder::CopyInfoToDecodedFormat(media_raw_audio_format * raf) {
	raf->frame_rate = (float)fHeader->rate; // XXX int32->float ??
	raf->channel_count = fHeader->nb_channels;
	raf->format = media_raw_audio_format::B_AUDIO_FLOAT; // XXX verify: support others?
	raf->byte_order = B_MEDIA_HOST_ENDIAN; // XXX should support other endain, too
	if (raf->buffer_size < 512 || raf->buffer_size > 65536) {
		raf->buffer_size = AudioBufferSize(raf);
	}
	raf->buffer_size = ((raf->buffer_size - 1) / fSpeexOutputLength + 1) * fSpeexOutputLength;
	// setup output variables
	fFrameSize = (raf->format & 0xf) * raf->channel_count;
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
		if (fHeader->nb_channels == 1) {
			ioDecodedFormat->u.raw_audio.channel_mask = B_CHANNEL_LEFT;
		} else {
			ioDecodedFormat->u.raw_audio.channel_mask = B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
		}
	}
	return B_OK;
}


status_t
speexDecoder::Seek(uint32 seekTo,
				 int64 seekFrame, int64 *frame,
				 bigtime_t seekTime, bigtime_t *time)
{
	TRACE("speexDecoder::Seek\n");
	int ignore = 0;
//	speex_decoder_ctl(fDecoderState, SPEEX_RESET_STATE, &ignore);
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

//	debugger("speexDecoder::Decode");
	while (out_bytes_needed >= fSpeexOutputLength) {
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
		speex_bits_read_from(&fBits, (char*)packet->packet, packet->bytes);
		for (int frame = 0 ; frame < fHeader->frames_per_packet ; frame++) {
			int ret = speex_decode(fDecoderState, &fBits, (float*)out_buffer);
			if (ret == -1) {
				break;
			}
			if (ret == -2) {
				TRACE("speexDecoder::Decode: corrupted stream?\n");
				break;
			}
			if (speex_bits_remaining(&fBits) < 0) {
				TRACE("speexDecoder::Decode: decoding overflow: corrupted stream?\n");
				break;
			}
			if (fHeader->nb_channels == 2) {
				speex_decode_stereo((float*)out_buffer, fSpeexOutputLength, fStereoState);
			}
			out_buffer += fSpeexOutputLength;
			out_bytes_needed -= fSpeexOutputLength;
		}
	}

done:	
	uint samples = (out_buffer - (uint8*)buffer) / fFrameSize;
	fStartTime += (1000000LL * samples) / fHeader->rate;
	//TRACE("speexDecoder: fStartTime inc'd to %.6f\n", fStartTime / 1000000.0);
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
