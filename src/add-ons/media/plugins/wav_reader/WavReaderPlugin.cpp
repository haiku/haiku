#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <DataIO.h>
#include <ByteOrder.h>
#include <InterfaceDefs.h>
#include "WavReaderPlugin.h"

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE ((void)0)
#endif

#define FOURCC(fcc) (*reinterpret_cast<const uint32 *>(fcc))

struct wavdata
{
	uint64 position;
	uint64 datasize;

	int32 framesize;
	int32 fps;

	void *buffer;
	int32 buffersize;
	
	int64 framecount;
	bigtime_t duration;
	
	media_format format;
};

WavReader::WavReader()
{
	TRACE("WavReader::WavReader\n");
}

WavReader::~WavReader()
{
}
      
const char *
WavReader::Copyright()
{
	return "WAV reader, " B_UTF8_COPYRIGHT " by Marcus Overhagen";
}

	
status_t
WavReader::Sniff(int32 *streamCount)
{
	TRACE("WavReader::Sniff\n");

	fSource = dynamic_cast<BPositionIO *>(Reader::Source());

	fDataSize = Source()->Seek(0, SEEK_END);
	if (fDataSize < sizeof(fRawHeader)) {
		TRACE("WavReader::Sniff:  too small\n");
		return B_ERROR;
	}
	fDataSize -= sizeof(fRawHeader);
	
	if (sizeof(fRawHeader) != Source()->ReadAt(0, &fRawHeader, sizeof(fRawHeader))) {
		TRACE("WavReader::Sniff: header reading failed\n");
		return B_ERROR;
	}
	
	if (B_LENDIAN_TO_HOST_INT32(fRawHeader.riff.riff_id) != FOURCC("RIFF")
		|| B_LENDIAN_TO_HOST_INT32(fRawHeader.riff.wave_id) != FOURCC("WAVE")
		|| B_LENDIAN_TO_HOST_INT32(fRawHeader.format.fourcc) != FOURCC("fmt ")
		|| B_LENDIAN_TO_HOST_INT32(fRawHeader.data.fourcc) != FOURCC("data")) {
		TRACE("WavReader::Sniff: header not recognized\n");
		return B_ERROR;
	}

	TRACE("WavReader::Sniff: looks like we found something\n");
	
	TRACE("  format_tag        0x%04x\n", B_LENDIAN_TO_HOST_INT16(fRawHeader.common.format_tag));
	TRACE("  channels          0x%04x\n", B_LENDIAN_TO_HOST_INT16(fRawHeader.common.channels));
	TRACE("  samples_per_sec   0x%08lx\n", B_LENDIAN_TO_HOST_INT32(fRawHeader.common.samples_per_sec));
	TRACE("  avg_bytes_per_sec 0x%08lx\n", B_LENDIAN_TO_HOST_INT32(fRawHeader.common.avg_bytes_per_sec));
	TRACE("  block_align       0x%04x\n", B_LENDIAN_TO_HOST_INT16(fRawHeader.common.block_align));
	TRACE("  bits_per_sample   0x%04x\n", B_LENDIAN_TO_HOST_INT16(fRawHeader.common.bits_per_sample));
	TRACE("  format_tag        0x%04x\n", B_LENDIAN_TO_HOST_INT16(fRawHeader.common.format_tag));
	
	if (fDataSize > B_LENDIAN_TO_HOST_INT32(fRawHeader.data.len) && fDataSize < 0xffffffff) {
		TRACE("WavReader::Sniff: reducing data size from %Ld to %ld\n",
			fDataSize, B_LENDIAN_TO_HOST_INT32(fRawHeader.data.len));
		fDataSize = B_LENDIAN_TO_HOST_INT32(fRawHeader.data.len);
	}

	TRACE("WavReader::Sniff: data size is %Ld bytes\n", fDataSize);

	*streamCount = 1;
	return B_OK;
}


status_t
WavReader::AllocateCookie(int32 streamNumber, void **cookie)
{
	TRACE("WavReader::AllocateCookie\n");

	wavdata *data = new wavdata;

	data->position = 0;
	data->datasize = fDataSize;
	data->framesize = (B_LENDIAN_TO_HOST_INT16(fRawHeader.common.bits_per_sample) / 8) * B_LENDIAN_TO_HOST_INT16(fRawHeader.common.channels);
	data->fps = B_LENDIAN_TO_HOST_INT32(fRawHeader.common.samples_per_sec) / B_LENDIAN_TO_HOST_INT16(fRawHeader.common.channels);
	data->buffersize = (65536 / data->framesize) * data->framesize;
	data->buffer = malloc(data->buffersize);
	data->framecount = fDataSize / data->framesize;
	data->duration = (data->framecount * 1000000LL) / data->fps;

	TRACE(" framesize %ld\n", data->framesize);
	TRACE(" fps %ld\n", data->fps);
	TRACE(" buffersize %ld\n", data->buffersize);
	TRACE(" framecount %Ld\n", data->framecount);
	TRACE(" duration %Ld\n", data->duration);

//	BMediaFormats formats;
	media_format_description description;
	description.family = B_BEOS_FORMAT_FAMILY;
	description.u.beos.format = B_BEOS_FORMAT_RAW_AUDIO;
//	formats.GetFormatFor(description, &data->format);
	
	_get_format_for_description(&data->format, description);

	data->format.type = B_MEDIA_RAW_AUDIO;
	data->format.u.raw_audio.frame_rate = data->fps;
	data->format.u.raw_audio.channel_count = B_LENDIAN_TO_HOST_INT16(fRawHeader.common.channels);
	data->format.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT; // XXX fixme
	data->format.u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
	data->format.u.raw_audio.buffer_size = data->buffersize;
	
	// store the cookie
	*cookie = data;
	return B_OK;
}


status_t
WavReader::FreeCookie(void *cookie)
{
	TRACE("WavReader::FreeCookie\n");
	wavdata *data = reinterpret_cast<wavdata *>(cookie);

	free(data->buffer);
	delete data;
	
	return B_OK;
}


status_t
WavReader::GetStreamInfo(void *cookie, int64 *frameCount, bigtime_t *duration,
						 media_format *format, void **infoBuffer, int32 *infoSize)
{
	wavdata *data = reinterpret_cast<wavdata *>(cookie);
	
	*frameCount = data->framecount;
	*duration = data->duration;
	*format = data->format;
	*infoBuffer = 0;
	*infoSize = 0;
	return B_OK;
}


status_t
WavReader::Seek(void *cookie,
				uint32 seekTo,
				int64 *frame, bigtime_t *time)
{
	wavdata *data = reinterpret_cast<wavdata *>(cookie);
	uint64 pos;

	if (seekTo & B_MEDIA_SEEK_TO_FRAME) {
		pos = *frame * data->framesize;
		*time = (pos * 1000000LL) / data->fps;
		TRACE("WavReader::Seek to frame %Ld, pos %Ld\n", *frame, pos);
		TRACE("WavReader::Seek newtime %Ld\n", *time);
	} else if (seekTo & B_MEDIA_SEEK_TO_TIME) {
		pos = (*time * data->fps) / 1000000LL;
		TRACE("WavReader::Seek to time %Ld, pos %Ld\n", *time, pos);
		*time = (pos * 1000000LL) / data->fps;
		*frame = pos / data->framesize;
		TRACE("WavReader::Seek newtime %Ld\n", *time);
		TRACE("WavReader::Seek newframe %Ld\n", *frame);
	} else {
		return B_ERROR;
	}
	
	if (pos < 0 || pos > data->datasize) {
		TRACE("WavReader::Seek invalid position %Ld\n", pos);
		return B_ERROR;
	}
	
	data->position = pos;
	return B_OK;
}


status_t
WavReader::GetNextChunk(void *cookie,
						void **chunkBuffer, int32 *chunkSize,
						media_header *mediaHeader)
{
	wavdata *data = reinterpret_cast<wavdata *>(cookie);
	status_t result = B_OK;

	mediaHeader->start_time = ((data->position / data->framesize) * 1000000LL) / data->fps;
	mediaHeader->file_pos = 44 + data->position;

	int64 maxreadsize = data->datasize - data->position;
	int32 readsize = data->buffersize;
	if (maxreadsize < readsize) {
		readsize = maxreadsize;
		result = B_LAST_BUFFER_ERROR;
	}
		
	if (readsize != Source()->ReadAt(44 + data->position, data->buffer, readsize)) {
		TRACE("WavReader::GetNextChunk: unexpected read error\n");
		return B_ERROR;
	}
	
	data->position += readsize;
	*chunkBuffer = data->buffer;
	*chunkSize = readsize;
	return result;
}


Reader *
WavReaderPlugin::NewReader()
{
	return new WavReader;
}


MediaPlugin *instantiate_plugin()
{
	return new WavReaderPlugin;
}
