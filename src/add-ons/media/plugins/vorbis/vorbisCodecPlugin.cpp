#include <stdio.h>
#include <Autolock.h>
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
	TRACE("vorbisDecoder::vorbisDecoder\n");
	vorbis_info_init(&fInfo);
	vorbis_comment_init(&fComment);

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
	TRACE("vorbisDecoder::~vorbisDecoder\n");
	delete [] fDecodeBuffer;
}


status_t
vorbisDecoder::Setup(media_format *ioEncodedFormat,
				  const void *infoBuffer, int32 infoSize)
{
	if ((ioEncodedFormat->type != B_MEDIA_UNKNOWN_TYPE)
	    && (ioEncodedFormat->type != B_MEDIA_ENCODED_AUDIO)) {
		TRACE("vorbisDecoder::Setup not called with audio/unknown stream: not vorbis");
		return B_ERROR;
	}
	if (infoSize != sizeof(ogg_packet)) {
		TRACE("vorbisDecoder::Setup not called with ogg_packet info: not vorbis");
		return B_ERROR;
	}
	ogg_packet * packet = (ogg_packet*)infoBuffer;
	// parse header packet
	if (vorbis_synthesis_headerin(&fInfo,&fComment,packet) != 0) {
		TRACE("vorbisDecoder::Setup: vorbis_synthesis_headerin failed: not vorbis header");
		return B_ERROR;
	}
	// get comment packet
	int32 size;
	media_header mh;
	if (GetNextChunk((void**)&packet, &size, &mh) != B_OK) {
		TRACE("vorbisDecoder::Setup: GetNextChunk failed to get comment\n");
		return B_ERROR;
	}
	// parse comment packet
	if (vorbis_synthesis_headerin(&fInfo,&fComment,packet) != 0) {
		TRACE("vorbiseDecoder::Setup: vorbis_synthesis_headerin failed: not vorbis comment");
		return B_ERROR;
	}
	// get codebook packet
	if (GetNextChunk((void**)&packet, &size, &mh) != B_OK) {
		TRACE("vorbisDecoder::Setup: GetNextChunk failed to get codebook\n");
		return B_ERROR;
	}
	// parse codebook packet
	if (vorbis_synthesis_headerin(&fInfo,&fComment,packet) != 0) {
		TRACE("vorbiseDecoder::Setup: vorbis_synthesis_headerin failed: not vorbis codebook");
		return B_ERROR;
	}
	// initialize decoder
	vorbis_synthesis_init(&fDspState,&fInfo);
	vorbis_block_init(&fDspState,&fBlock);
	ioEncodedFormat->type = B_MEDIA_ENCODED_AUDIO;
	return B_OK;
}

size_t get_audio_buffer_size(const media_raw_audio_format & raf) {
	BMediaRoster * roster = BMediaRoster::Roster();
	return roster->AudioBufferSizeFor(raf.channel_count,raf.format,raf.frame_rate);
}

status_t
vorbisDecoder::NegotiateOutputFormat(media_format *ioDecodedFormat)
{
	TRACE("vorbisDecoder::NegotiateOutputFormat\n");
	// BeBook says: The codec will find and return in ioFormat its best matching format
	// => This means, we never return an error, and always change the format values
	//    that we don't support to something more applicable
	ioDecodedFormat->type = B_MEDIA_RAW_AUDIO;
	ioDecodedFormat->u.raw_audio.frame_rate = (float)fInfo.rate; // XXX long->float ??
	ioDecodedFormat->u.raw_audio.channel_count = fInfo.channels;
	ioDecodedFormat->u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT; // XXX verify: support others?
	ioDecodedFormat->u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN; // XXX should support other endain, too

	if (ioDecodedFormat->u.raw_audio.buffer_size < 512
     || ioDecodedFormat->u.raw_audio.buffer_size > 65536)
		ioDecodedFormat->u.raw_audio.buffer_size
		 = get_audio_buffer_size(ioDecodedFormat->u.raw_audio);
	if (ioDecodedFormat->u.raw_audio.channel_mask == 0)
		ioDecodedFormat->u.raw_audio.channel_mask
		  = (fInfo.channels == 1) ? B_CHANNEL_LEFT : B_CHANNEL_LEFT | B_CHANNEL_RIGHT;

	// setup rest of the needed variables
	fFrameSize = (ioDecodedFormat->u.raw_audio.format & 0xf) * fInfo.channels;
	fOutputBufferSize = ioDecodedFormat->u.raw_audio.buffer_size;

	return B_OK;
}


status_t
vorbisDecoder::Seek(uint32 seekTo,
				 int64 seekFrame, int64 *frame,
				 bigtime_t seekTime, bigtime_t *time)
{
	debugger("vorbisDecoder::Seek");
	TRACE("vorbisDecoder::Seek\n");
	fResidualBytes = 0;
	return B_OK;
}


status_t
vorbisDecoder::Decode(void *buffer, int64 *frameCount,
				   media_header *mediaHeader, media_decode_info *info /* = 0 */)
{
	debugger("vorbisDecoder::Decode");
	TRACE("vorbisDecoder::Decode\n");
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
			
			fStartTime += (1000000LL * (bytes / fFrameSize)) / fInfo.rate;

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
	debugger("vorbisDecoder::DecodeNextChunk");
	TRACE("vorbisDecoder::DecodeNextChunk\n");
	void *chunkBuffer;
	int32 chunkSize;
	media_header mh;
	if (B_OK != GetNextChunk(&chunkBuffer, &chunkSize, &mh)) {
		TRACE("vorbisDecoder::Decode: GetNextChunk failed\n");
		return B_ERROR;
	}
	if (chunkSize != sizeof(ogg_packet)) {
		TRACE("vorbisDecoder::Decode: chunk not ogg_packet-sized\n");
		return B_ERROR;
	}
	ogg_packet * packet = static_cast<ogg_packet*>(chunkBuffer);
	
	//fStartTime = mh.start_time;

	//TRACE("vorbisDecoder: fStartTime reset to %.6f\n", fStartTime / 1000000.0);

	int outsize = 0;
	if (vorbis_synthesis(&fBlock,packet)==0) {
		vorbis_synthesis_blockin(&fDspState,&fBlock);
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
	PublishDecoder("unknown", "vorbis", "vorbis decoder, based on libvorbis");
	PublishDecoder("audiocodec/vorbis", "vorbis", "vorbis decoder, based on libvorbis");
	return B_OK;
}


MediaPlugin *instantiate_plugin()
{
	return new vorbisDecoderPlugin;
}
