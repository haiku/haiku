#include <stdio.h>
#include <DataIO.h>
#include <Locker.h>
#include <MediaFormats.h>
#include <MediaRoster.h>
#include "mp3DecoderPlugin.h"

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define DECODE_BUFFER_SIZE	(32 * 1024)

mp3Decoder::mp3Decoder()
{
	InitMP3(&fMpgLibPrivate);
	fResidualBytes = 0;
	fResidualBuffer = 0;
	fDecodeBuffer = new uint8 [DECODE_BUFFER_SIZE];
	fStartTime = 0;
	fFrameSize = 0;
	fFrameRate = 0;
	fBitRate = 0;
	fChannelCount = 0;
	fOutputBufferSize = 0;
}


mp3Decoder::~mp3Decoder()
{
	ExitMP3(&fMpgLibPrivate);
	delete [] fDecodeBuffer;
}


status_t
mp3Decoder::Setup(media_format *ioEncodedFormat,
				  const void *infoBuffer, int32 infoSize)
{
	// decode first chunk to initialize mpeg library
	if (B_OK != DecodeNextChunk()) {
		printf("mp3Decoder::Setup failed, can't decode first chunk\n");
		return B_ERROR;
	}

	// initialize fBitRate, fFrameRate and fChannelCount from mpg decode library values of first header
	extern int tabsel_123[2][3][16];
	extern long freqs[9];
	fBitRate = tabsel_123[fMpgLibPrivate.fr.lsf][fMpgLibPrivate.fr.lay-1][fMpgLibPrivate.fr.bitrate_index] * 1000;
	fFrameRate = freqs[fMpgLibPrivate.fr.sampling_frequency];
	fChannelCount = fMpgLibPrivate.fr.stereo;
	
	printf("mp3Decoder::Setup: channels %d, bitrate %d, framerate %d\n", fChannelCount, fBitRate, fFrameRate);
	
	// put some more useful info into the media_format describing our input format
	ioEncodedFormat->u.encoded_audio.bit_rate = fBitRate;
	
	return B_OK;
}

status_t
mp3Decoder::NegotiateOutputFormat(media_format *ioDecodedFormat)
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
	if (ioDecodedFormat->u.raw_audio.buffer_size < 512 || ioDecodedFormat->u.raw_audio.buffer_size > 65536)
		ioDecodedFormat->u.raw_audio.buffer_size = BMediaRoster::Roster()->AudioBufferSizeFor(
														fChannelCount,
														ioDecodedFormat->u.raw_audio.format,
														fFrameRate);
	if (ioDecodedFormat->u.raw_audio.channel_mask == 0)
		ioDecodedFormat->u.raw_audio.channel_mask = (fChannelCount == 1) ? B_CHANNEL_LEFT : B_CHANNEL_LEFT | B_CHANNEL_RIGHT;

	// setup rest of the needed variables
	fFrameSize = (ioDecodedFormat->u.raw_audio.format & 0xf) * fChannelCount;
	fOutputBufferSize = ioDecodedFormat->u.raw_audio.buffer_size;

	return B_OK;
}


status_t
mp3Decoder::Seek(uint32 seekTo,
				 int64 seekFrame, int64 *frame,
				 bigtime_t seekTime, bigtime_t *time)
{
	ExitMP3(&fMpgLibPrivate);
	InitMP3(&fMpgLibPrivate);
	fResidualBytes = 0;
	return B_OK;
}


status_t
mp3Decoder::Decode(void *buffer, int64 *frameCount,
				   media_header *mediaHeader, media_decode_info *info /* = 0 */)
{
	uint8 * out_buffer = static_cast<uint8 *>(buffer);
	int32	out_bytes_needed = fOutputBufferSize;
	
	mediaHeader->start_time = fStartTime;
	//TRACE("mp3Decoder: Decoding start time %.6f\n", fStartTime / 1000000.0);
	
	while (out_bytes_needed > 0) {
		if (fResidualBytes) {
			int32 bytes = min_c(fResidualBytes, out_bytes_needed);
			memcpy(out_buffer, fResidualBuffer, bytes);
			fResidualBuffer += bytes;
			fResidualBytes -= bytes;
			out_buffer += bytes;
			out_bytes_needed -= bytes;
			
			fStartTime += (1000000LL * (bytes / fFrameSize)) / fFrameRate;

			//TRACE("mp3Decoder: fStartTime inc'd to %.6f\n", fStartTime / 1000000.0);
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
mp3Decoder::DecodeNextChunk()
{
	void *chunkBuffer;
	int32 chunkSize;
	media_header mh;
	if (B_OK != GetNextChunk(&chunkBuffer, &chunkSize, &mh)) {
		TRACE("mp3Decoder::Decode: GetNextChunk failed\n");
		return B_ERROR;
	}
	
	fStartTime = mh.start_time;

	//TRACE("mp3Decoder: fStartTime reset to %.6f\n", fStartTime / 1000000.0);

	int outsize;
	int result;
	result = decodeMP3(&fMpgLibPrivate, (char *)chunkBuffer, chunkSize, (char *)fDecodeBuffer, DECODE_BUFFER_SIZE, &outsize);
	if (result == MP3_ERR) {
		TRACE("mp3Decoder::Decode: decodeMP3 returned MP3_ERR\n");
		return B_ERROR;
	}
		
	//printf("mp3Decoder::Decode: decoded %d bytes into %d bytes\n",chunkSize, outsize);
		
	fResidualBuffer = fDecodeBuffer;
	fResidualBytes = outsize;
	
	return B_OK;
}


Decoder *
mp3DecoderPlugin::NewDecoder()
{
	static BLocker locker;
	static bool initdone = false;
	locker.Lock();
	if (!initdone) {
		InitMpgLib();
		initdone = true;
	}
	locker.Unlock();
	return new mp3Decoder;
}


status_t
mp3DecoderPlugin::RegisterPlugin()
{
	PublishDecoder("audiocodec/mpeg1layer1", "mp3", "MPEG 1 audio layer 1 decoder, based on mpeg123 mpglib");
	PublishDecoder("audiocodec/mpeg1layer2", "mp3", "MPEG 1 audio layer 2 decoder, based on mpeg123 mpglib");
	PublishDecoder("audiocodec/mpeg1layer3", "mp3", "MPEG 1 audio layer 3 decoder, based on mpeg123 mpglib");
	PublishDecoder("audiocodec/mpeg2layer1", "mp3", "MPEG 2 audio layer 1 decoder, based on mpeg123 mpglib");
	PublishDecoder("audiocodec/mpeg2layer2", "mp3", "MPEG 2 audio layer 2 decoder, based on mpeg123 mpglib");
	PublishDecoder("audiocodec/mpeg2layer3", "mp3", "MPEG 2 audio layer 3 decoder, based on mpeg123 mpglib");
	PublishDecoder("audiocodec/mpeg2.5layer1", "mp3", "MPEG 2.5 audio layer 1 decoder, based on mpeg123 mpglib");
	PublishDecoder("audiocodec/mpeg2.5layer2", "mp3", "MPEG 2.5 audio layer 2 decoder, based on mpeg123 mpglib");
	PublishDecoder("audiocodec/mpeg2.5layer3", "mp3", "MPEG 2.5 audio layer 3 decoder, based on mpeg123 mpglib");
	return B_OK;
}


MediaPlugin *instantiate_plugin()
{
	return new mp3DecoderPlugin;
}
