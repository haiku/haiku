#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdint.h>
#include <DataIO.h>
#include <ByteOrder.h>
#include <InterfaceDefs.h>
#include <MediaFormats.h>
#include "OggReaderPlugin.h"

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...) ((void)0)
#endif

oggReader::oggReader()
{
	TRACE("oggReader::oggReader\n");
	ogg_sync_init(&fSync);
	fSeekable = 0;
}


oggReader::~oggReader()
{
	TRACE("oggReader::~oggReader\n");
	ogg_stream_map::iterator i = fStreams.begin();
	while (i != fStreams.end()) {
		ogg_stream_map::iterator j = i;
		i++;
		delete j->second;
	}
	ogg_sync_clear(&fSync);
}

      
const char *
oggReader::Copyright()
{
	return "ogg reader, " B_UTF8_COPYRIGHT " by Andrew Bachmann";
}

status_t
oggReader::GetPage(ogg_page * page, int read_size, bool short_page)
{
	TRACE("oggReader::GetPage\n");
	int result = ogg_sync_pageout(&fSync,page); // first read leftovers
	while (result == 0) {
		char * buffer = ogg_sync_buffer(&fSync,read_size);
		ssize_t bytes = Source()->Read(buffer,read_size);
		if (bytes == 0) {
			TRACE("oggReader::GetPage: Read: no data\n");
			return B_LAST_BUFFER_ERROR;
		}
		if (bytes < 0) {
			TRACE("oggReader::GetPage: Read: error\n");
			return bytes;
		}
		if (ogg_sync_wrote(&fSync,bytes) != 0) {
			TRACE("oggReader::GetPage: ogg_sync_wrote failed?: error\n");
			return B_ERROR;
		}
		result = ogg_sync_pageout(&fSync,page);
		if (short_page && (result != 1)) {
			TRACE("oggReader::GetPage: short page not found: error\n");
			return B_ERROR;
		}
	}
	if (result == -1) {
		TRACE("oggReader::GetPage: ogg_sync_pageout: not synced: error\n");
		return B_ERROR;
	}
#ifdef STRICT_OGG
	if (ogg_page_version(page) != 0) {
		TRACE("oggReader::GetPage: ogg_page_version: error in page encoding: error\n");
		return B_ERROR;
	}
#endif
	long serialno = ogg_page_serialno(page);
	if (fStreams.find(serialno) == fStreams.end()) {
		// this is an unknown serialno
		if (ogg_page_bos(page) == 0) {
			TRACE("oggReader::GetPage: non-bos packet with unknown serialno\n");
#ifdef STRICT_OGG
			return B_ERROR;
#endif
		}
		ogg_stream_state * stream = new ogg_stream_state;
		if (ogg_stream_init(stream,serialno) != 0) {
			TRACE("oggReader::GetPage: ogg_stream_init failed?: error\n");
			return B_ERROR;
		}
		fStreams[serialno] = stream;
	} else if (ogg_page_bos(page) > 0) {
		TRACE("oggReader::GetPage: bos packet with duplicate serialno\n");
#ifdef STRICT_OGG
		return B_ERROR;
#else
		if (ogg_stream_clear(fStreams[serialno]) != 0) {
			TRACE("oggReader::GetPage: ogg_stream_destroy failed?: error\n");
			return B_ERROR;
		}
		if (ogg_stream_init(fStreams[serialno],serialno) != 0) {
			TRACE("oggReader::GetPage: ogg_stream_init failed?: error\n");
			return B_ERROR;
		}
#endif
	}
	if (ogg_stream_pagein(fStreams[serialno],page) != 0) {
		TRACE("oggReader::Sniff: ogg_stream_pagein: failed: error\n");
		return B_ERROR;
	}
	return B_OK;
}

status_t
oggReader::Sniff(int32 *streamCount)
{
	TRACE("oggReader::Sniff\n");
#ifdef STRICT_OGG
	bool short_page = true;
#else
	bool short_page = false;
#endif
	fSeekable = dynamic_cast<BPositionIO*>(Source());

	off_t current_position = 0;
	if (fSeekable) {
		current_position = fSeekable->Seek(0,SEEK_CUR);
		fSeekable->Seek(0,SEEK_SET);
	}

	ogg_page page;
	if (GetPage(&page,4096,short_page) != B_OK) {
		return B_ERROR;
	}

	// page sanity checks
	if (ogg_page_version(&page) != 0) {
		TRACE("oggReader::Sniff: ogg_page_version: error in page encoding: not ogg\n");
		return B_ERROR;
	}
#ifdef STRICT_OGG
	if (ogg_page_bos(&page) == 0) {
		TRACE("oggReader::Sniff: ogg_page_bos: not beginning of a bitstream: not ogg\n");
		return B_ERROR;
	}
	if (ogg_page_continued(&page) != 0) {
		TRACE("oggReader::Sniff: ogg_page_continued: continued page: not ogg\n");
		return B_ERROR;
	}
#endif STRICT_OGG

	while (ogg_page_bos(&page) > 0) {
		int serialno = ogg_page_serialno(&page);
		ogg_stream_state * stream = fStreams[serialno];
		ogg_packet packet;
		if (ogg_stream_packetout(stream,&packet) != 1) {
#ifdef STRICT_OGG
			return B_ERROR;
#endif STRICT_OGG
		}
		unsigned char * buffer = new unsigned char[packet.bytes];
		fInitialHeaderPackets[serialno] = packet;
		memcpy(buffer,packet.packet,packet.bytes);
		fInitialHeaderPackets[serialno].packet = buffer;
		if (GetPage(&page,4096,short_page) != B_OK) {
			return B_ERROR;
		}
	}
	*streamCount = fStreams.size();
	if (fSeekable) {
		fPostSniffPosition = fSeekable->Seek(0,SEEK_CUR);
		fSeekable->Seek(current_position,SEEK_SET);
	}
	return B_OK;
}

void
oggReader::GetFileFormatInfo(media_file_format *mff)
{
	TRACE("oggReader::GetFileFormatInfo\n");
	mff->capabilities =   media_file_format::B_READABLE
						| media_file_format::B_IMPERFECTLY_SEEKABLE
						| media_file_format::B_KNOWS_ENCODED_VIDEO
						| media_file_format::B_KNOWS_ENCODED_AUDIO
						| media_file_format::B_KNOWS_OTHER;
	mff->family = B_MISC_FORMAT_FAMILY;
	mff->version = 100;
	strcpy(mff->mime_type, "application/ogg");
	strcpy(mff->file_extension, "ogg");
	strcpy(mff->short_name, "Ogg");
	strcpy(mff->pretty_name, "Ogg bitstream");
}

	
status_t
oggReader::AllocateCookie(int32 streamNumber, void **cookie)
{
	TRACE("oggReader::AllocateCookie\n");
	ogg_stream_map::iterator i = fStreams.begin();
	while (streamNumber > 0) {
		if (i == fStreams.end()) {
			TRACE("oggReader::AllocateCookie: invalid streamNumber: bail\n");
			return B_ERROR;
		}
		i++;
		streamNumber--;
	}
	// store the cookie
	*cookie = (void*)(i->second);
	return B_OK;
}


status_t
oggReader::FreeCookie(void *cookie)
{
	TRACE("oggReader::FreeCookie\n");
	return B_OK;
}


status_t
oggReader::GetStreamInfo(void *cookie, int64 *frameCount, bigtime_t *duration,
						 media_format *format, void **infoBuffer, int32 *infoSize)
{
	TRACE("oggReader::GetStreamInfo\n");
	ogg_stream_state * stream = static_cast<ogg_stream_state *>(cookie);
	memset(format, 0, sizeof(*format));
	if (fSeekable) {
		off_t input_length;
		{
			off_t current_position = fSeekable->Seek(0,SEEK_CUR);
			input_length = fSeekable->Seek(0,SEEK_END);
			fSeekable->Seek(current_position,SEEK_SET);
		}
		*frameCount = input_length/256;  // estimate: 1 frame = 256 bytes
		*duration = input_length*1000/8; // estimate: 1 second = 8 bytes
	} else {
		assert(sizeof(bigtime_t)==sizeof(uint64_t));
		// if the input is not seekable, we return really large sizes
		*frameCount = INT64_MAX;
		*duration = INT64_MAX;
	}
	ogg_packet * packet = &fInitialHeaderPackets[stream->serialno];
	*infoBuffer = (void*)packet;
	*infoSize = sizeof(ogg_packet);
	return B_OK;
}


status_t
oggReader::Seek(void *cookie,
				uint32 seekTo,
				int64 *frame, bigtime_t *time)
{
	TRACE("oggReader::Seek\n");
	return B_OK;
	debugger("oggReader::Seek");
	if (!fSeekable) {
		TRACE("oggReader::Seek: not a PositionIO: not seekable\n");
		return B_ERROR;
	}
	ogg_stream_state * stream = static_cast<ogg_stream_state *>(cookie);
	int position;
	if (seekTo & B_MEDIA_SEEK_TO_FRAME) {
		position = *frame;
	} else if (seekTo & B_MEDIA_SEEK_TO_TIME) {
		position = *time;
	} else {
		return B_ERROR;
	}
	// TODO: use B_MEDIA_SEEK_CLOSEST_BACKWARD, B_MEDIA_SEEK_CLOSEST_FORWARD to find key frame
	int serialno = stream->serialno;
	if (ogg_stream_clear(stream) != 0) {
		TRACE("oggReader::GetPage: ogg_stream_destroy failed?: error\n");
		return B_ERROR;
	}
	if (ogg_stream_init(stream,serialno) != 0) {
		TRACE("oggReader::GetPage: ogg_stream_init failed?: error\n");
		return B_ERROR;
	}
	fSeekable->Seek(position,SEEK_SET);
	
	return B_OK;
}


status_t
oggReader::GetNextChunk(void *cookie,
						void **chunkBuffer, int32 *chunkSize,
						media_header *mediaHeader)
{
	TRACE("oggReader::GetNextChunk\n");
	if (fSeekable && (fSeekable->Seek(0,SEEK_CUR) < fPostSniffPosition)) {
		fSeekable->Seek(fPostSniffPosition,SEEK_SET);
	}	
	ogg_stream_state * stream = static_cast<ogg_stream_state *>(cookie);
	while (ogg_stream_packetpeek(stream,NULL) != 1) {
		ogg_page page;
		do {
			status_t status = GetPage(&page);
			if (status != B_OK) {
				return status;
			}
		} while (ogg_page_serialno(&page) != stream->serialno);
	}
	ogg_packet packet;
	if (ogg_stream_packetout(stream,&packet) != 1) {
		return B_ERROR;
	}
	fPackets[stream->serialno] = packet;
	*chunkBuffer = (void*)&fPackets[stream->serialno];
	*chunkSize = sizeof(ogg_packet);

	return B_OK;
}


Reader *
oggReaderPlugin::NewReader()
{
	return new oggReader;
}


MediaPlugin *
instantiate_plugin()
{
	return new oggReaderPlugin;
}
