/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "MusePackReader.h"

#include <ByteOrder.h>
#include <InterfaceDefs.h>


MusePackReader::MusePackReader()
{
}


MusePackReader::~MusePackReader()
{
}


const char *
MusePackReader::Copyright()
{
}


status_t 
MusePackReader::Sniff(int32 *streamCount)
{
}


void 
MusePackReader::GetFileFormatInfo(media_file_format *mff)
{
}


status_t 
MusePackReader::AllocateCookie(int32 streamNumber, void **cookie)
{
}


status_t 
MusePackReader::FreeCookie(void *cookie)
{
}


status_t 
MusePackReader::GetStreamInfo(void *cookie, int64 *frameCount, bigtime_t *duration, media_format *format, void **infoBuffer, int32 *infoSize)
{
}


status_t 
MusePackReader::Seek(void *cookie, uint32 seekTo, int64 *frame, bigtime_t *time)
{
}


status_t 
MusePackReader::GetNextChunk(void *cookie, void **chunkBuffer, int32 *chunkSize, media_header *mediaHeader)
{
}

