#include <stdio.h>
#include <DataIO.h>
#include <Locker.h>
#include <MediaFormats.h>
#include <MediaRoster.h>
#include "vorbisCodecPlugin.h"

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define DECODE_BUFFER_SIZE	(32 * 1024)

vorbisDecoder::vorbisDecoder()
{
	vorbis_info_init(&fVorbisInfo);

	fResidualBytes = 0;
	fResidualBuffer = 0;
	fDecodeBuffer = new uint8 [DECODE_BUFFER_SIZE];
	fStartTime = 0;
	fFrameSize = 0;
	fBitRate = 0;
	fOutputBufferSize = 0;
}


vorbisDecoder::~vorbisDecoder()
{
	delete [] fDecodeBuffer;
}


status_t
vorbisDecoder::Setup(media_format *ioEncodedFormat,
				  const void *infoBuffer, int32 infoSize)
{
	if (ioEncodedFormat->type != B_MEDIA_ENCODED_AUDIO) {
		TRACE("vorbisDecoder::Setup not called with encoded video");
		return B_ERROR;
	}

	// save the extractor information for future reference
	fOutput = ioEncodedFormat->u.encoded_audio.output;
	fBitRate = ioEncodedFormat->u.encoded_audio.bit_rate;
	fFrameSize = ioEncodedFormat->u.encoded_audio.frame_size;
	
	printf("vorbisDecoder::Setup: bitrate %d\n", fBitRate);

	
	return B_OK;
}

status_t
vorbisDecoder::NegotiateOutputFormat(media_format *ioDecodedFormat)
{
	// fFrameRate and fChannelCount are already valid here
	
	// BeBook says: The codec will find and return in ioFormat its best matching format
	// => This means, we never return an error, and always change the format values
	//    that we don't support to something more applicable

	ioDecodedFormat->type = B_MEDIA_RAW_AUDIO;
	ioDecodedFormat->u.raw_audio.frame_rate = fFrameRate;
	ioDecodedFormat->u.raw_audio.channel_count = fChannelCount;
	ioDecodedFormat->u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT; // XXX should support other formats, too
	ioDecodedFormat->u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN; // XXX should support other endain, too
	if (ioDecodedFormat->u.raw_audio.buffer_size < 512
     || ioDecodedFormat->u.raw_audio.buffer_size > 65536)
		ioDecodedFormat->u.raw_audio.buffer_size
	      = BMediaRoster::Roster()->AudioBufferSizeFor(fChannelCount,
													   ioDecodedFormat->u.raw_audio.format,
													   fFrameRate);
	if (ioDecodedFormat->u.raw_audio.channel_mask == 0)
		ioDecodedFormat->u.raw_audio.channel_mask
		  = (fChannelCount == 1) ? B_CHANNEL_LEFT : B_CHANNEL_LEFT | B_CHANNEL_RIGHT;

	// setup rest of the needed variables
	fFrameSize = (ioDecodedFormat->u.raw_audio.format & 0xf) * fChannelCount;
	fOutputBufferSize = ioDecodedFormat->u.raw_audio.buffer_size;

	return B_OK;
}


status_t
vorbisDecoder::Seek(uint32 seekTo,
				 int64 seekFrame, int64 *frame,
				 bigtime_t seekTime, bigtime_t *time)
{
	ExitMP3(&fMpgLibPrivate);
	InitMP3(&fMpgLibPrivate);
	fResidualBytes = 0;
	return B_OK;
}


status_t
vorbisDecoder::Decode(void *buffer, int64 *frameCount,
				   media_header *mediaHeader, media_decode_info *info /* = 0 */)
{
	uint8 * out_buffer = static_cast<uint8 *>(buffer);
	int32	out_bytes_needed = fOutputBufferSize;
	
	mediaHeader->start_time = fStartTime;
	//TRACE("vorbisDecoder: Decoding start time %.6f\n", fStartTime / 1000000.0);
	
	while (out_bytes_needed > 0) {
		if (fResidualBytes) {
			int32 bytes = min_c(fResidualBytes, out_bytes_needed);
			memcpy(out_buffer, fResidualBuffer, bytes);
			fResidualBuffer += bytes;
			fResidualBytes -= bytes;
			out_buffer += bytes;
			out_bytes_needed -= bytes;
			
			fStartTime += (1000000LL * (bytes / fFrameSize)) / fFrameRate;

			//TRACE("vorbisDecoder: fStartTime inc'd to %.6f\n", fStartTime / 1000000.0);
			continue;
		}
		
		if (B_OK != DecodeNextChunk())
			break;
	}

	*frameCount = (fOutputBufferSize - out_bytes_needed) / fFrameSize;

	// XXX this doesn't guarantee that we always return B_LAST_BUFFER_ERROR bofore returning B_ERROR
	return (out_bytes_needed == 0) ? B_OK : (out_bytes_needed == fOutputBufferSize) ? B_ERROR : B_LAST_BUFFER_ERROR;
}

status_t
vorbisDecoder::DecodeNextChunk()
{
	void *chunkBuffer;
	int32 chunkSize;
	media_header mh;
	if (B_OK != GetNextChunk(&chunkBuffer, &chunkSize, &mh)) {
		TRACE("vorbisDecoder::Decode: GetNextChunk failed\n");
		return B_ERROR;
	}
	
	fStartTime = mh.start_time;

	//TRACE("vorbisDecoder: fStartTime reset to %.6f\n", fStartTime / 1000000.0);

	int outsize;
	int result;
	result = decodeMP3(&fMpgLibPrivate, (char *)chunkBuffer, chunkSize, (char *)fDecodeBuffer, DECODE_BUFFER_SIZE, &outsize);
	if (result == MP3_ERR) {
		TRACE("vorbisDecoder::Decode: decodeMP3 returned MP3_ERR\n");
		return B_ERROR;
	}
		
	//printf("vorbisDecoder::Decode: decoded %d bytes into %d bytes\n",chunkSize, outsize);
		
	fResidualBuffer = fDecodeBuffer;
	fResidualBytes = outsize;
	
	return B_OK;
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
vorbisDecoderPlugin::RegisterPlugin()
{
	PublishDecoder("audiocodec/vorbis", "vorbis", "vorbis decoder, based on libvorbis");
	return B_OK;
}


MediaPlugin *instantiate_plugin()
{
	return new vorbisDecoderPlugin;
}
