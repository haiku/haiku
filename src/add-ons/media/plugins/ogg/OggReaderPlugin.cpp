#include <stdio.h>
#include <string.h>
#include <malloc.h>
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

#define OGG_BUFFER_SIZE (B_PAGE_SIZE*8)

oggReader::oggReader()
{
	TRACE("oggReader::oggReader\n");
	ogg_sync_init(&fSync);
}


oggReader::~oggReader()
{
	ogg_sync_destroy(&fSync);
}

      
const char *
oggReader::Copyright()
{
	return "ogg reader, " B_UTF8_COPYRIGHT " by Andrew Bachmann";
}

status_t
oggReader::GetPage(ogg_page * page, int read_size, bool short_page)
{
	ogg_sync_pageout(&fSync,page); // clear the buffer
	int result = 0; 
	while (result == 0) {
		char * buffer = ogg_sync_buffer(&fSync,read_size);
		ssize_t bytes = Source()->Read(buffer,read_size);
		if (bytes < 0) {
			TRACE("oggReader::GetPage: Read: error\n");
			return B_ERROR;
		}
		if (ogg_sync_wrote(&fSync,bytes) != 0) {
			TRACE("oggReader::GetPage: ogg_sync_wrote failed?: error\n");
			return B_ERROR;
		}
		result = ogg_sync_pageout(&fSync,page);
		if (short_page && (result != 0)) {
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
		ogg_stream_state stream;
		fStreams[serialno] = stream;
		if (ogg_stream_init(&fStreams[serialno],serialno) != 0) {
			TRACE("oggReader::GetPage: ogg_stream_init failed?: error\n");
			return B_ERROR;
		}
	} else if (ogg_page_bos(page) > 0) {
		TRACE("oggReader::GetPage: bos packet with duplicate serialno\n");
#ifdef STRICT_OGG
		return B_ERROR;
#else
		if (ogg_stream_destroy(&fStreams[serialno]) != 0) {
			TRACE("oggReader::GetPage: ogg_stream_destroy failed?: error\n");
			return B_ERROR;
		}
		if (ogg_stream_init(&fStreams[serialno],serialno) != 0) {
			TRACE("oggReader::GetPage: ogg_stream_init failed?: error\n");
			return B_ERROR;
		}
#endif
	}
	if (ogg_stream_pagein(&fStreams[serialno],page) != 0) {
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
		if (GetPage(&page,4096,short_page) != B_OK) {
			return B_ERROR;
		}
	}
	*streamCount = fStreams.size();
	return B_OK;
}

void
oggReader::GetFileFormatInfo(media_file_format *mff)
{
	mff->capabilities =   media_file_format::B_READABLE
						| media_file_format::B_KNOWS_ENCODED_VIDEO
						| media_file_format::B_KNOWS_ENCODED_AUDIO
						| media_file_format::B_IMPERFECTLY_SEEKABLE;
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
	*cookie = (void*)(&i->second);
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
//	ogg_stream_state * stream = static_cast<ogg_stream_state *>(cookie);
	memset(format, 0, sizeof(*format));
	*frameCount = -1; // don't know
	*duration = -1; // don't know
	// no info
	*infoBuffer = 0;
	*infoSize = 0;
	return B_OK;
}


status_t
oggReader::Seek(void *cookie,
				uint32 seekTo,
				int64 *frame, bigtime_t *time)
{
	TRACE("oggReader::Seek\n");
	BPositionIO * input = dynamic_cast<BPositionIO *>(Source());
	if (input == 0) {
		TRACE("oggReader::Seek: not a PositionIO: not seekable\n");
		return B_ERROR;
	}
//	ogg_stream_state * stream = static_cast<ogg_stream_state *>(cookie);


	return B_OK;
}


status_t
oggReader::GetNextChunk(void *cookie,
						void **chunkBuffer, int32 *chunkSize,
						media_header *mediaHeader)
{
	ogg_stream_state * stream = static_cast<ogg_stream_state *>(cookie);
	while (ogg_stream_packetpeek(stream,NULL) != 1) {
		ogg_page page;
		do {
			if (GetPage(&page) != B_OK) {
				return B_ERROR;
			}
		} while (ogg_page_serialno(&page) != stream->serialno);
	}
	if (fPackets.find(stream->serialno) == fPackets.end()) {
		ogg_packet packet;
		fPackets[stream->serialno] = packet;
	}
	if (ogg_stream_packetout(stream,&fPackets[stream->serialno]) != 1) {
		return B_ERROR;
	}
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
