/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include "MusePackReader.h"

#include <InterfaceDefs.h>
#include <stdio.h>
#include <string.h>


//#define TRACE_MUSEPACK_READER
#ifdef TRACE_MUSEPACK_READER
  #define TRACE printf
#else
  #define TRACE(a...)
#endif


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
		TRACE("MusePackReader: ReadStreamInfo() engine error %d\n", error);
		return B_ERROR;
	} else if (error < B_OK)
		return error;

	TRACE("MusePackReader: recognized MPC file\n");
	*_streamCount = 1;
	return B_OK;
}


void 
MusePackReader::GetFileFormatInfo(media_file_format *mff)
{

	mff->capabilities =   media_file_format::B_READABLE
						| media_file_format::B_KNOWS_ENCODED_AUDIO
						| media_file_format::B_IMPERFECTLY_SEEKABLE;
	mff->family = B_MISC_FORMAT_FAMILY;
	mff->version = 100;
	strcpy(mff->mime_type, "audio/x-mpc");
	strcpy(mff->file_extension, "mpc");
	strcpy(mff->short_name, "MusePack");
	strcpy(mff->pretty_name, "MusePack");
}


status_t 
MusePackReader::AllocateCookie(int32 streamNumber, void **_cookie)
{
	// we don't need a cookie - we only know one single stream
	*_cookie = NULL;
	
	media_format_description description;
	description.family = B_MISC_FORMAT_FAMILY;
	description.u.misc.file_format = 'mpc ';
	description.u.misc.codec = 'MPC7';
		// 7 is the most recent stream version

	status_t status = BMediaFormats().GetFormatFor(description, &fFormat);
	if (status < B_OK)
		return status;
		
	// allocate and initialize internal decoder
	fDecoder = new MPC_decoder(static_cast<BPositionIO *>(Source()));
	fDecoder->RESET_Globals();
	fDecoder->RESET_Synthesis();
	fDecoder->SetStreamInfo(&fInfo);
	if (!fDecoder->FileInit()) {
		delete fDecoder;
		fDecoder = 0;
		return B_ERROR;
	}

#if 0 // not required to report
	fFormat.u.encoded_audio.output.frame_rate = fInfo.simple.SampleFreq;
	fFormat.u.encoded_audio.output.channel_count = fInfo.simple.Channels;
	fFormat.u.encoded_audio.output.format = media_raw_audio_format::B_AUDIO_FLOAT;
	fFormat.u.encoded_audio.output.byte_order = B_MEDIA_HOST_ENDIAN;
	fFormat.u.encoded_audio.output.buffer_size = sizeof(MPC_SAMPLE_FORMAT) * FRAMELEN * 2;
#endif

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
	media_format *format, const void **_infoBuffer, size_t *_infoSize)
{
	if (cookie != NULL)
		return B_BAD_VALUE;

	*_frameCount = FRAMELEN * (int64)fInfo.simple.Frames;
	*_duration = bigtime_t(1000.0 * fInfo.simple.Frames * FRAMELEN
		/ (fInfo.simple.SampleFreq / 1000.0) + 0.5);

	*format = fFormat;

	// we provide the stream info in this place
	*_infoBuffer = (void *)fDecoder;
	*_infoSize = sizeof(MPC_decoder);

	return B_OK;
}


status_t 
MusePackReader::Seek(void *cookie, uint32 seekTo, int64 *frame, bigtime_t *time)
{
	// we don't care, let the decoder do the work...
	// (MPC not really differentiates between decoder and container)
	return B_OK;
}


status_t 
MusePackReader::GetNextChunk(void *cookie, const void **chunkBuffer, size_t *chunkSize,
	media_header *mediaHeader)
{
	// this one will never be called by our decoder
	return B_ERROR;
}

