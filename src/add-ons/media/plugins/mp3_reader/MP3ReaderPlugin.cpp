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

struct mp3data
{
	int64	position;
	char *	chunkBuffer;
	
};

mp3Reader::mp3Reader()
{
	TRACE("mp3Reader::mp3Reader\n");
	fSource = dynamic_cast<BPositionIO *>(Reader::Source());
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

	//fDataSize = Source()->Seek(0, SEEK_END);
	//if (sizeof(fRawHeader) != Source()->ReadAt(0, &fRawHeader, sizeof(fRawHeader))) {

	TRACE("mp3Reader::Sniff: data size is %Ld bytes\n", fDataSize);

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
