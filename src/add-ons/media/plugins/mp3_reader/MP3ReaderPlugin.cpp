#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <DataIO.h>
#include <ByteOrder.h>
#include <InterfaceDefs.h>
#include "MP3ReaderPlugin.h"

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE ((void)0)
#endif

// bit_rate_table[mpeg_version_index][layer_index][bitrate_index]
static const int bit_rate_table[4][4][16] =
{
	{ // mpeg version 2.5
		{ }, // undefined layer
		{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 }, // layer 3
		{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 }, // layer 2
		{ 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0 } // layer 1
	},
	{ // undefined version
		{ },
		{ },
		{ },
		{ }
	},
	{ // mpeg version 2
		{ },
		{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 },
		{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 },
		{ 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0 }
	},
	{ // mpeg version 1
		{ },
		{ 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0 },
		{ 0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0 },
		{ 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0}
	}
};

// sampling_rate_table[mpeg_version_index][sampling_rate_index]
static const int sampling_rate_table[4][4] =
{
	{ 11025, 12000, 8000, 0},	// mpeg version 2.5
	{ 0, 0, 0, 0 },
	{ 22050, 24000, 16000, 0},	// mpeg version 2
	{ 44100, 48000, 32000, 0}	// mpeg version 1
};


struct mp3data
{
	int64	position;
	char *	chunkBuffer;
	
};

mp3Reader::mp3Reader()
{
	TRACE("mp3Reader::mp3Reader\n");
}

mp3Reader::~mp3Reader()
{
}
      
const char *
mp3Reader::Copyright()
{
	return "mp3 reader, " B_UTF8_COPYRIGHT " by Marcus Overhagen";
}

	
status_t
mp3Reader::Sniff(int32 *streamCount)
{
	TRACE("mp3Reader::Sniff\n");
	
	fSeekableSource = dynamic_cast<BPositionIO *>(Reader::Source());
	if (!fSeekableSource) {
		TRACE("mp3Reader::Sniff: non seekable sources not supported\n");
		return B_ERROR;
	}

	fFileSize = Source()->Seek(0, SEEK_END);

	TRACE("mp3Reader::Sniff: file size is %Ld bytes\n", fFileSize);
	
	if (!IsMp3File()) {
		TRACE("mp3Reader::Sniff: non recognized as mp3 file\n");
		return B_ERROR;
	}

	TRACE("mp3Reader::Sniff: looks like an mp3 file\n");
	

	//fDataSize = Source()->Seek(0, SEEK_END);
	//if (sizeof(fRawHeader) != Source()->ReadAt(0, &fRawHeader, sizeof(fRawHeader))) {

	*streamCount = 1;
	return B_OK;
}


status_t
mp3Reader::AllocateCookie(int32 streamNumber, void **cookie)
{
	TRACE("mp3Reader::AllocateCookie\n");

	mp3data *data = new mp3data;
/*
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

	memset(&data->format, 0, sizeof(data->format));
	data->format.type = B_MEDIA_RAW_AUDIO;
	data->format.u.raw_audio.frame_rate = data->fps;
	data->format.u.raw_audio.channel_count = B_LENDIAN_TO_HOST_INT16(fRawHeader.common.channels);
	data->format.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT; // XXX fixme
	data->format.u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
	data->format.u.raw_audio.buffer_size = data->buffersize;
*/	
	// store the cookie
	*cookie = data;
	return B_OK;
}


status_t
mp3Reader::FreeCookie(void *cookie)
{
	TRACE("mp3Reader::FreeCookie\n");
	mp3data *data = reinterpret_cast<mp3data *>(cookie);
/*
	free(data->buffer);
*/
	delete data;
	
	return B_OK;
}


status_t
mp3Reader::GetStreamInfo(void *cookie, int64 *frameCount, bigtime_t *duration,
						 media_format *format, void **infoBuffer, int32 *infoSize)
{
//	mp3data *data = reinterpret_cast<mp3data *>(cookie);
/*	
	*frameCount = data->framecount;
	*duration = data->duration;
	*format = data->format;
	*infoBuffer = 0;
	*infoSize = 0;
*/
	return B_OK;
}


status_t
mp3Reader::Seek(void *cookie,
				uint32 seekTo,
				int64 *frame, bigtime_t *time)
{
	if (!fSeekableSource)
		return B_ERROR;

//	mp3data *data = reinterpret_cast<mp3data *>(cookie);
/*
	uint64 pos;
	
	if (frame) {
		pos = *frame * data->framesize;
		TRACE("mp3Reader::Seek to frame %Ld, pos %Ld\n", *frame, pos);
	} else if (time) {
		pos = (*time * data->fps) / 1000000LL;
		TRACE("mp3Reader::Seek to time %Ld, pos %Ld\n", *time, pos);
		*time = (pos * 1000000LL) / data->fps;
		TRACE("mp3Reader::Seek newtime %Ld\n", *time);
	} else {
		return B_ERROR;
	}
	
	if (pos < 0 || pos > data->datasize) {
		TRACE("mp3Reader::Seek invalid position %Ld\n", pos);
		return B_ERROR;
	}
	
	data->position = pos;
*/
	return B_OK;
}


status_t
mp3Reader::GetNextChunk(void *cookie,
						void **chunkBuffer, int32 *chunkSize,
						media_header *mediaHeader)
{
//	mp3data *data = reinterpret_cast<mp3data *>(cookie);
	status_t result = B_OK;
/*
	int32 readsize = data->datasize - data->position;
	if (readsize > data->buffersize) {
		readsize = data->buffersize;
		result = B_LAST_BUFFER_ERROR;
	}
		
	if (readsize != Source()->ReadAt(44 + data->position, data->buffer, readsize)) {
		TRACE("mp3Reader::GetNextChunk: unexpected read error\n");
		return B_ERROR;
	}
	
	if (mediaHeader) {
		// memset(mediaHeader, 0, sizeof(*mediaHeader));
		// mediaHeader->type = B_MEDIA_RAW_AUDIO;
		// mediaHeader->size_used = buffersize;
		// mediaHeader->start_time = ((data->position / data->framesize) * 1000000LL) / data->fps;
		// mediaHeader->file_pos = 44 + data->position;
	}
	
	data->position += readsize;
	*chunkBuffer = data->buffer;
	*chunkSize = readsize;
*/
	return result;
}

bool
mp3Reader::IsMp3File()
{
	// To detect an mp3 file, we seek into the middle,
	// and search for a valid sequence of 3 frame headers.
	// A mp3 frame has a maximum length of 2881 bytes, we
	// load a block of 16kB and use it to search.

	const int32 search_size = 16384;
	int64	offset;
	int32	size;
	uint8	buf[search_size];

	size = search_size;
	offset = fFileSize / 2 - search_size / 2;
	if (size > fFileSize) {
		size = fFileSize;
		offset = 0;
	}

	TRACE("searching for mp3 frame headers at %Ld in %ld bytes\n", offset, size);
	
	if (size != Source()->ReadAt(offset, buf, size)) {
		TRACE("mp3ReaderPlugin::IsMp3File reading %ld bytes at offset %Ld failed\n", size, offset);
		return false;
	}
	
	for (int32 pos = 0; pos < (size - 8); pos++) {
		int length1, length2, length3;
		length1 = GetFrameLength(&buf[pos]);
		if (length1 < 0 || (pos + length1 + 4) > size)
			continue;
		TRACE("    found a valid frame header at file position %Ld\n", offset + pos);
		length2 = GetFrameLength(&buf[pos + length1]);
		if (length2 < 0 || (pos + length1 + length2 + 4) > size)
			continue;
		TRACE("##  found second valid frame header at file position %Ld\n", offset + pos + length1);
		length3 = GetFrameLength(&buf[pos + length1 + length2]);
		if (length3 < 0)
			continue;
		TRACE("### found third valid frame header at file position %Ld\n", offset + pos + length1 + length2);
		return true;	
	}
	return false;
}

int
mp3Reader::GetFrameLength(void *header)
{
	if (((uint8 *)header)[0] != 0xff)
		return -1;
	if (((uint8 *)header)[1] & 0xe0 != 0xe)
		return -1;
	
	int mpeg_version_index = (((uint8 *)header)[1] >> 3) & 0x03;
	int layer_index = (((uint8 *)header)[1] >> 1) & 0x03;
	int bitrate_index = (((uint8 *)header)[2] >> 4) & 0x0f;
	int sampling_rate_index = (((uint8 *)header)[2] >> 2) & 0x03;
	int padding = (((uint8 *)header)[2] >> 1) & 0x01;
	
	int bitrate = bit_rate_table[mpeg_version_index][layer_index][bitrate_index];
	int samplingrate = sampling_rate_table[mpeg_version_index][sampling_rate_index];

	if (!bitrate || !samplingrate)
		return -1;	

	int length;	
	if (layer_index == 3) // layer 1
		length = ((144 * 1000 * bitrate) / samplingrate) + (padding * 4);
	else // layer 2 & 3
		length = ((144 * 1000 * bitrate) / samplingrate) + padding;
	
	TRACE("%s %s, bit rate %d, sampling rate %d, padding %d, frame length %d\n",
		mpeg_version_index == 0 ? "mpeg 2.5" : (mpeg_version_index == 2 ? "mpeg 2" : "mpeg 1"),
		layer_index == 3 ? "layer 1" : (layer_index == 2 ? "layer 2" : "layer 3"),
		bitrate, samplingrate, padding, length);
	
	return length;
}

Reader *
mp3ReaderPlugin::NewReader()
{
	return new mp3Reader;
}


MediaPlugin *
instantiate_plugin()
{
	return new mp3ReaderPlugin;
}
