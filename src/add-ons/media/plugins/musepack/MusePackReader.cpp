/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "MusePackReader.h"
#include "mpc/mpc_dec.h"

#include <InterfaceDefs.h>
#include <stdio.h>
#include <string.h>


MusePackReader::MusePackReader()
{
}


MusePackReader::~MusePackReader()
{
}


const char *
MusePackReader::Copyright()
{
	return "MusePack reader, " B_UTF8_COPYRIGHT " by Axel Dörfler";
}


status_t 
MusePackReader::Sniff(int32 *_streamCount)
{
	BPositionIO *file = dynamic_cast<BPositionIO *>(Source());
	if (file == NULL)
		// we cannot handle non seekable files for now
		return B_ERROR;

	file->Seek(0, SEEK_SET);
	int error = fInfo.ReadStreamInfo(file);
	if (error > B_OK) {
		// error came from engine
		printf("MusePack: ReadStreamInfo() engine error %d\n", error);
		return B_ERROR;
	} else if (error < B_OK)
		return error;

	*_streamCount = 1;
	return B_OK;
}


void 
MusePackReader::GetFileFormatInfo(media_file_format *mff)
{
}


status_t 
MusePackReader::AllocateCookie(int32 streamNumber, void **_cookie)
{
	// we don't need a cookie - we only know one single stream
	*_cookie = NULL;
	return B_OK;
}


status_t 
MusePackReader::FreeCookie(void *cookie)
{
	// nothing to do here
	return B_OK;
}


status_t 
MusePackReader::GetStreamInfo(void *cookie, int64 *_frameCount, bigtime_t *_duration,
	media_format *format, void **_infoBuffer, int32 *_infoSize)
{
	if (cookie != NULL)
		return B_BAD_VALUE;

	*_frameCount = fInfo.simple.Frames;
	*_duration = bigtime_t(1000.0 * (fInfo.simple.Frames - 0.5) * FRAMELEN
		/ (fInfo.simple.SampleFreq / 1000) + 0.5);

	media_format_description description;
	description.family = B_MISC_FORMAT_FAMILY;
	description.u.misc.codec = 'MPC7';
		// ToDo: does this make any sense? BTW '7' is the most recent stream version...
	BMediaFormats formats;
	formats.GetFormatFor(description, format);

	*_infoBuffer = NULL;
	*_infoSize = 0;

	return B_OK;
}


status_t 
MusePackReader::Seek(void *cookie, uint32 seekTo, int64 *frame, bigtime_t *time)
{
	return B_ERROR;
}


status_t 
MusePackReader::GetNextChunk(void *cookie, void **chunkBuffer, int32 *chunkSize,
	media_header *mediaHeader)
{
	return B_ERROR;
}

