#include "OggSeekable.h"
#include "OggVorbisSeekable.h"
#include <Autolock.h>
#include <stdio.h>

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...) ((void)0)
#endif

/*
 * OggSeekable codec identification and instantiation
 */

/* static */ OggSeekable *
OggSeekable::makeOggSeekable(OggReader::SeekableInterface * interface,
                         long serialno, const ogg_packet & packet)
{
	TRACE("OggSeekable::makeOggSeekable\n");
	OggSeekable * stream;
	if (OggVorbisSeekable::IsValidHeader(packet)) {
		stream = new OggVorbisSeekable(serialno);
//	} else if (OggTobiasSeekable::IsValidHeader(packet)) {
//		stream = new OggTobiasSeekable(serialno);
//	} else if (OggSpeexSeekable::IsValidHeader(packet)) {
//		stream = new OggSpeexSeekable(serialno);
//	} else if (OggTheoraSeekable::IsValidHeader(packet)) {
//		stream = new OggTheoraSeekable(serialno);
	} else {
		stream = new OggSeekable(serialno);
	}
	stream->fReaderInterface = interface;
	return stream;
}


/*
 * OggSeekable generic functions
 */

OggSeekable::OggSeekable(long serialno)
	: OggTrack(serialno)
{
	TRACE("OggSeekable::OggSeekable\n");
	ogg_sync_init(&fSync);
	ogg_stream_init(&fStreamState,serialno);
}


OggSeekable::~OggSeekable()
{
	// free internal stream state storage
	ogg_sync_clear(&fSync);
	ogg_stream_clear(&fStreamState);
}


status_t
OggSeekable::AddPage(off_t position, const ogg_page & page)
{
	TRACE("OggSeekable::AddPage\n");
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
OggSeekable::GetStreamInfo(int64 *frameCount, bigtime_t *duration,
                         media_format *format)
{
	TRACE("OggSeekable::GetStreamInfo\n");
	status_t result = B_OK;
	ogg_packet packet;
	if (GetHeaderPackets().size() < 1) {
		result = GetPacket(&packet);
		if (result != B_OK) {
			TRACE("OggSeekable::GetStreamInfo failed to get header packet\n");
			return result;
		}
		SaveHeaderPacket(packet);
	}
	packet = GetHeaderPackets()[0];
	if (!packet.b_o_s) {
		TRACE("OggSeekable::GetStreamInfo failed : not beginning of stream\n");
		return B_ERROR; // first packet was not beginning of stream
	}

	// parse header packet
	uint32 four_bytes = *(uint32*)(&packet);

	// get the format for the description
	media_format_description description;
	description.family = B_MISC_FORMAT_FAMILY;
	description.u.misc.file_format = 'OggS';
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
OggSeekable::GetNextChunk(void **chunkBuffer, int32 *chunkSize,
             media_header *mediaHeader)
{
	status_t result = GetPacket(&fChunkPacket);
	if (result != B_OK) {
		TRACE("OggSeekable::GetNextChunk failed: GetPacket = %s\n", strerror(result));
		return result;
	}
	*chunkBuffer = &fChunkPacket;
	*chunkSize = sizeof(fChunkPacket);
	return B_OK;
}


// subclass pull input function
status_t
OggSeekable::GetPacket(ogg_packet * packet)
{
	// at the end, pull the packet
	while (ogg_stream_packetpeek(&fStreamState, NULL) != 1) {
		BAutolock autolock(fSyncLock);
		int result;
		ogg_page page;
		while ((result = ogg_sync_pageout(&fSync,&page)) == 0) {
			status_t result = B_ERROR; // fReaderInterface->ReadPage();
			if (result != B_OK) {
				TRACE("OggSeekable::GetPacket: GetNextPage = %s\n", strerror(result));
				return result;
			}
		}
		if (result == -1) {
			TRACE("OggSeekable::GetPacket: ogg_sync_pageout: not synced??\n");
			return B_ERROR;
		}
		if (ogg_stream_pagein(&fStreamState,&page) != 0) {
			TRACE("OggSeekable::GetPacket: ogg_stream_pagein: failed??\n");
			return B_ERROR;
		}
	}
	if (ogg_stream_packetout(&fStreamState, packet) != 1) {
		TRACE("OggSeekable::GetPacket: ogg_stream_packetout failed\n");
		return B_ERROR;
	}
	return B_OK;
}

