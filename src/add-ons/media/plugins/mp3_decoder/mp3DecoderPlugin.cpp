#include <stdio.h>
#include <DataIO.h>
#include <Locker.h>
#include <MediaFormats.h>
#include "mp3DecoderPlugin.h"

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define OUTPUT_BUFFER_SIZE	(8 * 1024)
#define DECODE_BUFFER_SIZE	(32 * 1024)

mp3Decoder::mp3Decoder()
{
	InitMP3(&fMpgLibPrivate);
	fResidualBytes = 0;
	fResidualBuffer = 0;
	fDecodeBuffer = new uint8 [DECODE_BUFFER_SIZE];
	fFrameSize = 0;
}


mp3Decoder::~mp3Decoder()
{
	ExitMP3(&fMpgLibPrivate);
	delete [] fDecodeBuffer;
}


status_t
mp3Decoder::Setup(media_format *ioEncodedFormat, media_format *ioDecodedFormat,
				  const void *infoBuffer, int32 infoSize)
{
	memset(ioDecodedFormat, 0, sizeof(*ioDecodedFormat));
	ioDecodedFormat->type = B_MEDIA_RAW_AUDIO;
	ioDecodedFormat->u.raw_audio.frame_rate = 44100;
	ioDecodedFormat->u.raw_audio.channel_count = 2;
	ioDecodedFormat->u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
	ioDecodedFormat->u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
	ioDecodedFormat->u.raw_audio.buffer_size = OUTPUT_BUFFER_SIZE;
	ioDecodedFormat->u.raw_audio.channel_mask = B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
	fFrameSize = 4;
	fFps = 44100;
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
	int32	out_bytes_needed = OUTPUT_BUFFER_SIZE;
	
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
			
			fStartTime += (1000000LL * (bytes / fFrameSize)) / fFps;

			//TRACE("mp3Decoder: fStartTime inc'd to %.6f\n", fStartTime / 1000000.0);
			continue;
		}
		
		if (B_OK != DecodeNextChunk())
			break;
	}

	*frameCount = (OUTPUT_BUFFER_SIZE - out_bytes_needed) / fFrameSize;

	// XXX this doesn't guarantee that we always return B_LAST_BUFFER_ERROR bofore returning B_ERROR
	return (out_bytes_needed == 0) ? B_OK : (out_bytes_needed == OUTPUT_BUFFER_SIZE) ? B_ERROR : B_LAST_BUFFER_ERROR;
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
