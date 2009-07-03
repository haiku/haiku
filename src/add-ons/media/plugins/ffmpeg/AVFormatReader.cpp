/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU L-GPL license.
 */

#include "AVFormatReader.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <new>

#include <ByteOrder.h>
#include <DataIO.h>
//#include <InterfaceDefs.h>
#include <MediaFormats.h>

extern "C" {
	#include "avformat.h"
	#include "libavutil/avstring.h"
}

//#include "RawFormats.h"


#define TRACE_AVFORMAT_READER
#ifdef TRACE_AVFORMAT_READER
#	define TRACE printf
#	define TRACE_IO(a...)
#else
#	define TRACE(a...)
#	define TRACE_IO(a...)
#endif

#define ERROR(a...) fprintf(stderr, a)


AVFormatReader::AVFormatReader()
	:
	fContext(NULL)
{
	TRACE("AVFormatReader::AVFormatReader\n");
}


AVFormatReader::~AVFormatReader()
{
	TRACE("AVFormatReader::~AVFormatReader\n");

	// TODO: Deallocate fContext
}


// #pragma mark -


const char*
AVFormatReader::Copyright()
{
	// TODO: Return copyright of the file instead!
	return "Copyright 2009, Stephan Aßmus";
}

	
status_t
AVFormatReader::Sniff(int32* streamCount)
{
	TRACE("AVFormatReader::Sniff\n");

	size_t bufferSize = 64 * 1024;
	size_t probeSize = 1024;
	uint8 buffer[bufferSize];
	AVProbeData probeData;
	probeData.filename = "";
	probeData.buf = buffer;
	probeData.buf_size = probeSize;

	// Read a bit of the input...
	if (_ReadPacket(Source(), buffer, probeSize) != (ssize_t)probeSize)
		return B_IO_ERROR;
	// ...and seek back to the beginning of the file.
	_Seek(Source(), 0, SEEK_SET);

	// Probe the input format
	AVInputFormat* inputFormat = av_probe_input_format(&probeData, 1);

	if (inputFormat == NULL) {
		TRACE("AVFormatReader::Sniff() - av_probe_input_format() failed!\n");
		return B_ERROR;
	}

	TRACE("AVFormatReader::Sniff() - av_probe_input_format(): %s\n",
		inputFormat->name);

	ByteIOContext ioContext;

	// Init io module for input
	if (init_put_byte(&ioContext, buffer, bufferSize, 0, Source(),
		_ReadPacket, 0, _Seek) != 0) {
		TRACE("AVFormatReader::Sniff() - init_put_byte() failed!\n");
		return B_ERROR;
	}

	AVFormatParameters formatParameters;
	memset(&formatParameters, 0, sizeof(formatParameters));

	if (av_open_input_stream(&fContext, &ioContext, "", inputFormat,
		&formatParameters) < 0) {
		TRACE("AVFormatReader::Sniff() - av_open_input_stream() failed!\n");
		return B_ERROR;
	}

	// Retrieve stream information
	if (av_find_stream_info(fContext) < 0) {
		TRACE("AVFormatReader::Sniff() - av_find_stream_info() failed!\n");
		return B_ERROR;
	}

	TRACE("AVFormatReader::Sniff() - av_find_stream_info() success!\n");

	// Dump information about stream onto standard error
	dump_format(fContext, 0, "", 0);

	return B_ERROR;
}


void
AVFormatReader::GetFileFormatInfo(media_file_format* mff)
{
	TRACE("AVFormatReader::GetFileFormatInfo\n");

	mff->capabilities = media_file_format::B_READABLE
		| media_file_format::B_KNOWS_ENCODED_VIDEO
		| media_file_format::B_KNOWS_ENCODED_AUDIO
		| media_file_format::B_IMPERFECTLY_SEEKABLE;
	mff->family = B_MISC_FORMAT_FAMILY;
	mff->version = 100;
	strcpy(mff->mime_type, "video/mpg");
	strcpy(mff->file_extension, "mpg");
	strcpy(mff->short_name,  "MPEG");
	strcpy(mff->pretty_name, "MPEG (Motion Picture Experts Group)");
}


// #pragma mark -


status_t
AVFormatReader::AllocateCookie(int32 streamNumber, void** _cookie)
{
	TRACE("AVFormatReader::AllocateCookie(%ld)\n", streamNumber);

	return B_ERROR;
}


status_t
AVFormatReader::FreeCookie(void *_cookie)
{
	return B_ERROR;
}


// #pragma mark -


status_t
AVFormatReader::GetStreamInfo(void* _cookie, int64* frameCount,
	bigtime_t* duration, media_format* format, const void** infoBuffer,
	size_t* infoSize)
{
	return B_ERROR;
}


status_t
AVFormatReader::Seek(void* _cookie, uint32 seekTo, int64* frame,
	bigtime_t* time)
{
	return B_ERROR;
}


status_t
AVFormatReader::FindKeyFrame(void* _cookie, uint32 flags, int64* frame,
	bigtime_t* time)
{
	return B_ERROR;
}


status_t
AVFormatReader::GetNextChunk(void* _cookie, const void** chunkBuffer,
	size_t* chunkSize, media_header* mediaHeader)
{
	return B_ERROR;
}


// #pragma mark -


/*static*/ int
AVFormatReader::_ReadPacket(void* cookie, uint8* buffer, int bufferSize)
{
	TRACE_IO("AVFormatReader::_ReadPacket(%p, %p, %d)\n", cookie, buffer,
		bufferSize);

	BDataIO* dataIO = reinterpret_cast<BDataIO*>(cookie);
	ssize_t read = dataIO->Read(buffer, bufferSize);

	TRACE_IO("  read: %ld\n", read);
	return (int)read;
}


/*static*/ off_t
AVFormatReader::_Seek(void* cookie, off_t offset, int whence)
{
	TRACE_IO("AVFormatReader::_Seek(%p, %lld, %d)\n", cookie, offset, whence);

	BDataIO* dataIO = reinterpret_cast<BDataIO*>(cookie);
	BPositionIO* positionIO = dynamic_cast<BPositionIO*>(dataIO);
	if (positionIO == NULL) {
		TRACE("  not a BPositionIO\n");
		return -1;
	}

	// Support for special file size retrieval API without seeking anywhere:
	if (whence == AVSEEK_SIZE) {
		off_t size;
		if (positionIO->GetSize(&size) == B_OK)
			return size;
		return -1;
	}

	off_t position = positionIO->Seek(offset, whence);

	TRACE_IO("  position: %lld\n", position);
	return position;
}

