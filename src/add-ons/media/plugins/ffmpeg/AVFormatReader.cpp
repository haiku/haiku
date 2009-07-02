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
#else
#	define TRACE(a...)
#endif

#define ERROR(a...) fprintf(stderr, a)



// #pragma mark - BPositionIO protocol


static int
position_io_open(URLContext* h, const char* filename, int flags)
{
	TRACE("position_io_open(%s)\n", filename);

	// Strip the URL prefix
	av_strstart(filename, "position_io:", &filename);

	void* pointer;
	if (sscanf(filename, "%p", &pointer) != 1) {
		TRACE("position_io_open(%s) - unable to scan BPositionIO pointer\n",
			filename);
		return AVERROR(ENOENT);
	}

	// When the pointer was placed, it was a BDataIO*. Try to convert that
	// into a BPositionIO.
	// TODO: Later we may implement two different protocols, one which
	// supports seeking (BPositionIO) and one that does not (BDataIO).
	BDataIO* dataIO = reinterpret_cast<BDataIO*>(pointer);
	BPositionIO* positionIO = dynamic_cast<BPositionIO*>(dataIO);
	if (positionIO == NULL) {
		TRACE("position_io_open(%s) - unable to cast BDataIO pointer %p\n",
			filename, dataIO);
		return AVERROR(ENOENT);
	}

	TRACE("position_io_open(%s) - success: BPositionIO: %p\n",
		filename, positionIO);

	h->priv_data = reinterpret_cast<void*>(positionIO);

	return 0;
}


static int
position_io_read(URLContext* h, unsigned char* buffer, int size)
{
	TRACE("position_io_read(%d)\n", size);

	BPositionIO* source = reinterpret_cast<BPositionIO*>(h->priv_data);
	return source->Read(buffer, size);
}


static int
position_io_write(URLContext* h, unsigned char* buffer, int size)
{
	TRACE("position_io_write(%d)\n", size);

	BPositionIO* source = reinterpret_cast<BPositionIO*>(h->priv_data);
	return source->Write(buffer, size);
}


static int64_t
position_io_seek(URLContext* h, int64_t position, int whence)
{
	TRACE("position_io_seek(%lld, %d)\n", position, whence);

	BPositionIO* source = reinterpret_cast<BPositionIO*>(h->priv_data);
	return source->Seek(position, whence);
}


static int
position_io_close(URLContext* h)
{
	TRACE("position_io_close()\n");

	// We do not close ourselves here.
	return 0;
}


URLProtocol sPositionIOProtocol = {
	"position_io",
	position_io_open,
	position_io_read,
	position_io_write,
	position_io_seek,
	position_io_close
};


// #pragma mark - AVFormatReader


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

#if 0
	// Construct an URL string that allows us to get the Source()
	// BPositionIO pointer and try to open the stream.
	char urlString[64];
	snprintf(urlString, sizeof(urlString), "position_io:%p", (void*)Source());

	if (av_open_input_file(&fContext, urlString, NULL, 0, NULL) < 0) {
		TRACE("AVFormatReader::Sniff() - av_open_input_file(%s) failed!\n",
			urlString);
		return B_ERROR;
	}

#else
	size_t bufferSize = 64 * 1024;
	uint8 buffer[bufferSize];
	AVProbeData probeData;
	probeData.filename = "";
	probeData.buf = buffer;
	probeData.buf_size = bufferSize;

	// Read a bit of the input
	_ReadPacket(Source(), buffer, bufferSize);

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

	AVFormatParameters params;
	memset(&params, 0, sizeof(params));

	if (av_open_input_stream(&fContext, &ioContext, "", inputFormat,
		&params) < 0) {
		TRACE("AVFormatReader::Sniff() - av_open_input_file() failed!\n");
		return B_ERROR;
	}
#endif

	TRACE("AVFormatReader::Sniff() - av_open_input_file() success!\n");

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
	TRACE("AVFormatReader::_ReadPacket(%p, %p, %d)\n", cookie, buffer,
		bufferSize);

	BDataIO* dataIO = reinterpret_cast<BDataIO*>(cookie);
	ssize_t read = dataIO->Read(buffer, bufferSize);

	TRACE("  read: %ld\n", read);
	return read;
}


/*static*/ off_t
AVFormatReader::_Seek(void* cookie, off_t offset, int whence)
{
	TRACE("AVFormatReader::_Seek(%p, %lld, %d)\n", cookie, offset, whence);

	BDataIO* dataIO = reinterpret_cast<BDataIO*>(cookie);
	BPositionIO* positionIO = dynamic_cast<BPositionIO*>(dataIO);
	if (positionIO == NULL) {
		TRACE("  not a BPositionIO\n");
		return -1;
	}

	off_t position = positionIO->Seek(offset, whence);

	TRACE("  position: %lld\n", position);
	return position;
}

