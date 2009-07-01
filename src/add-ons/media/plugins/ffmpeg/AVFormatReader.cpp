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
}

//#include "RawFormats.h"


//#define TRACE_AVFORMAT_READER
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
	// always successful, priv_data is set elsewhere
	return 0;
}


static int
position_io_read(URLContext* h, unsigned char* buffer, int size)
{
	BPositionIO* source = reinterpret_cast<BPositionIO*>(h->priv_data);
	return source->Read(buffer, size);
}


static int
position_io_write(URLContext* h, unsigned char* buffer, int size)
{
	BPositionIO* source = reinterpret_cast<BPositionIO*>(h->priv_data);
	return source->Write(buffer, size);
}


static int64_t
position_io_seek(URLContext *h, int64_t position, int whence)
{
	BPositionIO* source = reinterpret_cast<BPositionIO*>(h->priv_data);
	return source->Seek(position, whence);
}


static int
position_io_close(URLContext *h)
{
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


