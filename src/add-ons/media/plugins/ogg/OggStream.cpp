#include "OggStream.h"
#include "OggSpeexStream.h"
#include "OggTheoraStream.h"
#include "OggTobiasStream.h"
#include "OggVorbisStream.h"
#include "OggFormats.h"
#include <Autolock.h>
#include <stdio.h>

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...) ((void)0)
#endif

/*
 * OggStream codec identification and instantiation
 */

/* static */ OggStream *
OggStream::makeOggStream(OggReader::StreamInterface * interface,
                         long serialno, const ogg_packet & packet)
{
	TRACE("OggStream::makeOggStream\n");
	OggStream * stream;
	if (OggVorbisStream::IsValidHeader(packet)) {
		stream = new OggVorbisStream(serialno);
	} else if (OggTobiasStream::IsValidHeader(packet)) {
		stream = new OggTobiasStream(serialno);
	} else if (OggSpeexStream::IsValidHeader(packet)) {
		stream = new OggSpeexStream(serialno);
	} else if (OggTheoraStream::IsValidHeader(packet)) {
		stream = new OggTheoraStream(serialno);
	} else {
		stream = new OggStream(serialno);
	}
	stream->fReaderInterface = interface;
	return stream;
}


/*
 * OggStream generic functions
 */

OggStream::OggStream(long serialno)
	: OggTrack(serialno)
{
	TRACE("OggStream::OggStream\n");
	fCurrentFrame = 0;
	fCurrentTime = 0;
	ogg_sync_init(&fSync);
	ogg_stream_init(&fStreamState,serialno);
}


OggStream::~OggStream()
{
	// free internal stream state storage
	ogg_sync_clear(&fSync);
	ogg_stream_clear(&fStreamState);
}


status_t
OggStream::AddPage(off_t position, const ogg_page & page)
{
//	TRACE("OggStream::AddPage\n");
	BAutolock autolock(fSyncLock);
	char * buffer;
	// copy the header to our local sync
	buffer = ogg_sync_buffer(&fSync, page.header_len);
	memcpy(buffer, page.header, page.header_len);
	ogg_sync_wrote(&fSync, page.header_len);
	// copy the body to our local sync
	buffer = ogg_sync_buffer(&fSync, page.body_len);
	memcpy(buffer, page.body, page.body_len);
	ogg_sync_wrote(&fSync, page.body_len);
	return B_OK;
}


status_t
OggStream::GetStreamInfo(int64 *frameCount, bigtime_t *duration,
                         media_format *format)
{
	TRACE("OggStream::GetStreamInfo\n");
	status_t result = B_OK;
	ogg_packet packet;
	if (GetHeaderPackets().size() < 1) {
		result = GetPacket(&packet);
		if (result != B_OK) {
			TRACE("OggStream::GetStreamInfo failed to get header packet\n");
			return result;
		}
		SaveHeaderPacket(packet);
	}
	packet = GetHeaderPackets()[0];
	if (!packet.b_o_s) {
		TRACE("OggStream::GetStreamInfo failed : not beginning of stream\n");
		return B_ERROR; // first packet was not beginning of stream
	}

	// parse header packet
	uint32 four_bytes = *(uint32*)(&packet);

	// get the format for the description
	media_format_description description;
	description.family = B_MISC_FORMAT_FAMILY;
	description.u.misc.file_format = OGG_FILE_FORMAT;
	description.u.misc.codec = four_bytes;
	BMediaFormats formats;
	result = formats.InitCheck();
	if (result == B_OK) {
		result = formats.GetFormatFor(description, format);
	}
	if (result != B_OK) {
		// ignore the error, allow the user to use ReadChunk interface
	}

	// fill out format from header packet
	format->user_data_type = B_CODEC_TYPE_INFO;
	strncpy((char*)format->user_data, (char*)(&packet), 4);

	format->SetMetaData((void*)&GetHeaderPackets(),sizeof(GetHeaderPackets()));
	*duration = 140000000;
	*frameCount = 60000;
	return B_OK;
}


// the default chunk is an ogg packet
status_t
OggStream::GetNextChunk(void **chunkBuffer, int32 *chunkSize,
             media_header *mediaHeader)
{
	status_t result = GetPacket(&fChunkPacket);
	if (result != B_OK) {
		TRACE("OggStream::GetNextChunk failed: GetPacket = %s\n", strerror(result));
		return result;
	}
	*chunkBuffer = &fChunkPacket;
	*chunkSize = sizeof(fChunkPacket);
	return B_OK;
}


// subclass pull input function
status_t
OggStream::GetPacket(ogg_packet * packet)
{
	// at the end, pull the packet
	while (ogg_stream_packetpeek(&fStreamState, NULL) != 1) {
		BAutolock autolock(fSyncLock);
		int result;
		ogg_page page;
		while ((result = ogg_sync_pageout(&fSync,&page)) == 0) {
			result = fReaderInterface->ReadPage();
			if (result < 0) {
				TRACE("OggStream::GetPacket: GetNextPage = %s\n", strerror(result));
				return result;
			}
		}
		if (result == -1) {
			TRACE("OggStream::GetPacket: ogg_sync_pageout: not synced??\n");
			return B_ERROR;
		}
		if (ogg_stream_pagein(&fStreamState,&page) != 0) {
			TRACE("OggStream::GetPacket: ogg_stream_pagein: failed??\n");
			return B_ERROR;
		}
	}
	if (ogg_stream_packetout(&fStreamState, packet) != 1) {
		TRACE("OggStream::GetPacket: ogg_stream_packetout failed\n");
		return B_ERROR;
	}
	return B_OK;
}

