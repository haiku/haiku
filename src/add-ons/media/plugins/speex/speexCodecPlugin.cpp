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


inline size_t
AudioBufferSize(media_raw_audio_format * raf, bigtime_t buffer_duration = 50000 /* 50 ms */)
{
	return (raf->format & 0xf) * (raf->channel_count)
         * (size_t)((raf->frame_rate * buffer_duration) / 1000000.0);
}


/*
 * speex descriptions/formats
 */


static media_format_description
speex_description()
{
	media_format_description description;
	description.family = B_MISC_FORMAT_FAMILY;
	description.u.misc.file_format = 'OggS';
	description.u.misc.codec = 'Spee';
	return description;
}


static void
init_speex_media_raw_audio_format(media_raw_audio_format * output)
{
	output->format = media_raw_audio_format::B_AUDIO_FLOAT;
	output->byte_order = B_MEDIA_HOST_ENDIAN;
}


static media_format
speex_encoded_media_format()
{
	media_format format;
	format.type = B_MEDIA_ENCODED_AUDIO;
	format.user_data_type = B_CODEC_TYPE_INFO;
	strncpy((char*)format.user_data, "Spee", 4);
	format.u.encoded_audio.frame_size = sizeof(ogg_packet);
	init_speex_media_raw_audio_format(&format.u.encoded_audio.output);
	return format;
}


static media_format
speex_decoded_media_format()
{
	media_format format;
	format.type = B_MEDIA_RAW_AUDIO;
	init_speex_media_raw_audio_format(&format.u.raw_audio);
	return format;
}


/*
 * VorbisDecoder
 */


SpeexDecoder::SpeexDecoder()
{
	TRACE("SpeexDecoder::SpeexDecoder\n");
	speex_bits_init(&fBits);
	fDecoderState = 0;
	fHeader = 0;
	fStereoState = 0;
	fSpeexOutputLength = 0;
	fStartTime = 0;
	fFrameSize = 0;
	fOutputBufferSize = 0;
}


SpeexDecoder::~SpeexDecoder()
{
	TRACE("SpeexDecoder::~SpeexDecoder\n");
	// the fStereoState is destroyed by fDecoderState
//	delete fStereoState;
	speex_bits_destroy(&fBits);
	speex_decoder_destroy(fDecoderState);
}


void
SpeexDecoder::GetCodecInfo(media_codec_info &info)
{
	strncpy(info.short_name, "speex", sizeof(info.short_name));
	strncpy(info.pretty_name, "speex decoder, by Andrew Bachmann, based on libspeex", sizeof(info.pretty_name));
}


status_t
SpeexDecoder::Setup(media_format *inputFormat,
				  const void *infoBuffer, int32 infoSize)
{
	TRACE("VorbisDecoder::Setup\n");
	if (!format_is_compatible(speex_encoded_media_format(),*inputFormat)) {
		return B_MEDIA_BAD_FORMAT;
	}
	// grab header packets from meta data
	if (inputFormat->MetaDataSize() != sizeof(std::vector<ogg_packet>)) {
		TRACE("SpeexDecoder::Setup not called with ogg_packet<vector> meta data: not speex\n");
		return B_ERROR;
	}
	std::vector<ogg_packet> * packets = (std::vector<ogg_packet> *)inputFormat->MetaData();
	if (packets->size() < 2) {
		TRACE("SpeexDecoder::Setup not called with at least two ogg_packets: not speex\n");
		return B_ERROR;
	}
	// parse header packet
	ogg_packet * packet = &(*packets)[0];
	fHeader = speex_packet_to_header((char*)packet->packet, packet->bytes);
	if (fHeader == NULL) {
		TRACE("SpeexDecoder::Setup failed in ogg_packet to speex_header conversion\n");
		return B_ERROR;
	}
	if (packets->size() != 2 + (unsigned)fHeader->extra_headers) {
		TRACE("SpeexDecoder::Setup not called with all the extra headers\n");
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
		TRACE("SpeexDecoder::Setup failed: version id too new");
		return B_ERROR;
	}
#endif
	if (fHeader->mode >= SPEEX_NB_MODES) {
		TRACE("SpeexDecoder::Setup failed: unknown speex mode\n");
		return B_ERROR;
	}
	// setup from header
	SpeexMode * mode = speex_mode_list[fHeader->mode];
#ifdef STRICT_SPEEX
	if (mode->bitstream_version != fHeader->mode_bitstream_version) {
		TRACE("SpeexDecoder::Setup failed: bitstream version mismatch");
		return B_ERROR;
	}
#endif
	// initialize decoder
	fDecoderState = speex_decoder_init(mode);
	if (fDecoderState == NULL) {
		TRACE("SpeexDecoder::Setup failed to initialize the decoder state");
		return B_ERROR;
	}
	if (SpeexSettings::PerceptualPostFilter()) {
		int enabled = 1;
		speex_decoder_ctl(fDecoderState, SPEEX_SET_ENH, &enabled);
	}
	speex_decoder_ctl(fDecoderState, SPEEX_GET_FRAME_SIZE, &fHeader->frame_size);
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
	media_format requested_format = speex_decoded_media_format();
	((media_raw_audio_format)requested_format.u.raw_audio) = inputFormat->u.encoded_audio.output;
	return NegotiateOutputFormat(&requested_format);
}


status_t
SpeexDecoder::NegotiateOutputFormat(media_format *ioDecodedFormat)
{
	TRACE("SpeexDecoder::NegotiateOutputFormat\n");
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
	media_format format = speex_decoded_media_format();
	format.u.raw_audio.frame_rate = (float)fHeader->rate;
	format.u.raw_audio.channel_count = fHeader->nb_channels;
	format.u.raw_audio.channel_mask = B_CHANNEL_LEFT | (fHeader->nb_channels != 1 ? B_CHANNEL_RIGHT : 0);
	int buffer_size = ioDecodedFormat->u.raw_audio.buffer_size;
	if (buffer_size < 512) {
		buffer_size = AudioBufferSize(&format.u.raw_audio);
	}
	int output_length = fHeader->frame_size * fHeader->nb_channels
	                 * (format.u.raw_audio.format & 0xf);
	buffer_size = ((buffer_size - 1) / output_length + 1) * output_length;
	format.u.raw_audio.buffer_size = buffer_size;
	if (!format_is_compatible(format,*ioDecodedFormat)) {
#ifdef NegotiateOutputFormat_AS_BEBOOK_SPEC
		return B_ERROR;
#else
		// Be R5 behavior: never fail (?) => nuke the input format
		*ioDecodedFormat = format;
#endif
	}
	ioDecodedFormat->SpecializeTo(&format);
	// setup output variables
	fFrameSize = (ioDecodedFormat->u.raw_audio.format & 0xf) * 
	             (ioDecodedFormat->u.raw_audio.channel_count);
	fOutputBufferSize = ioDecodedFormat->u.raw_audio.buffer_size;
	fSpeexOutputLength = output_length;
	return B_OK;
}


status_t
SpeexDecoder::Seek(uint32 seekTo,
				 int64 seekFrame, int64 *frame,
				 bigtime_t seekTime, bigtime_t *time)
{
	TRACE("SpeexDecoder::Seek\n");
//	int ignore = 0;
//	speex_decoder_ctl(fDecoderState, SPEEX_RESET_STATE, &ignore);
	return B_OK;
}


status_t
SpeexDecoder::Decode(void *buffer, int64 *frameCount,
				   media_header *mediaHeader, media_decode_info *info /* = 0 */)
{
//	TRACE("SpeexDecoder::Decode\n");
	uint8 * out_buffer = static_cast<uint8 *>(buffer);
	int32	out_bytes_needed = fOutputBufferSize;
	
	mediaHeader->start_time = fStartTime;
	//TRACE("SpeexDecoder: Decoding start time %.6f\n", fStartTime / 1000000.0);

//	debugger("SpeexDecoder::Decode");
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
			TRACE("SpeexDecoder::Decode: GetNextChunk failed\n");
			return status;
		}
		if (chunkSize != sizeof(ogg_packet)) {
			TRACE("SpeexDecoder::Decode: chunk not ogg_packet-sized\n");
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
				TRACE("SpeexDecoder::Decode: corrupted stream?\n");
				break;
			}
			if (speex_bits_remaining(&fBits) < 0) {
				TRACE("SpeexDecoder::Decode: decoding overflow: corrupted stream?\n");
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
	//TRACE("SpeexDecoder: fStartTime inc'd to %.6f\n", fStartTime / 1000000.0);
	*frameCount = (fOutputBufferSize - out_bytes_needed) / fFrameSize;

	if (out_buffer != buffer) {
		return B_OK;
	}
	return B_LAST_BUFFER_ERROR;
}


/*
 * SpeexDecoderPlugin
 */


Decoder *
SpeexDecoderPlugin::NewDecoder()
{
	static BLocker locker;
	static bool initdone = false;
	BAutolock lock(locker);
	if (!initdone) {
		initdone = true;
	}
	return new SpeexDecoder;
}


status_t
SpeexDecoderPlugin::RegisterDecoder()
{
	media_format_description description = speex_description();
	media_format format = speex_encoded_media_format();

	BMediaFormats formats;
	status_t result = formats.InitCheck();
	if (result != B_OK) {
		return result;
	}
	return formats.MakeFormatFor(&description, 1, &format);
}


/*
 * instantiate_plugin
 */


MediaPlugin *instantiate_plugin()
{
	return new SpeexDecoderPlugin;
}
