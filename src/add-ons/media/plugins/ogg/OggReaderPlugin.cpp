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
oggReader::Sniff(int32 *streamCount)
{
	TRACE("oggReader::Sniff\n");

	ogg_page page;
	ogg_sync_pageout(&fSync,&page); // clear the buffer

	char * buffer = ogg_sync_buffer(&fSync,4096);
	ssize_t bytes = Source()->Read(buffer,4096);
	if (bytes < 4096) {
		TRACE("oggReader::Sniff: Read: not enough: not ogg\n");
		return B_ERROR;
	}	
	if (ogg_sync_wrote(&fSync,bytes) != 0) {
		TRACE("oggReader::Sniff: ogg_sync_wrote failed?: bail\n");
		return B_ERROR;
	}
	switch ((int)ogg_sync_pageout(&fSync,&page)) {
	case -1:
		TRACE("oggReader::Sniff: ogg_sync_pageout: not synced: not ogg\n");
		return B_ERROR;
	case 0:
		TRACE("oggReader::Sniff: ogg_sync_pageout: need more data: not ogg\n");
		return B_ERROR;
	case 1:
		TRACE("oggReader::Sniff: looks like a valid ogg page\n");
		break;
	default:
		TRACE("oggReader::Sniff: ogg_sync_pageout: unexpected result: bail\n");
		return B_ERROR;
	}
	// page sanity checks
	if (ogg_page_version(&page) != 0) {
		TRACE("oggReader::Sniff: ogg_sync_version: error in page encoding: not ogg\n");
		return B_ERROR;
	}
	if (ogg_page_continued(&page) != 0) {
		TRACE("oggReader::Sniff: ogg_page_continued: continued page: not ogg\n");
		return B_ERROR;
	}
	if (ogg_page_bos(&page) == 0) {
		TRACE("oggReader::Sniff: ogg_page_bos: not beginning of a bitstream: not ogg\n");
		return B_ERROR;
	}

	// seems like ogg
	ogg_stream_state stream;
	if (ogg_stream_init(&stream,ogg_page_serialno(&page)) != 0) {
		TRACE("oggReader::Sniff: ogg_stream_init: failed: bail\n");
		return B_ERROR;
	}
	if (ogg_stream_pagein(&stream,&page) != 0) {
		TRACE("oggReader::Sniff: ogg_stream_pagein: failed: bail\n");
		return B_ERROR;
	}
	fStreams[ogg_page_serialno(&page)] = stream;
	while (true) {
		buffer = ogg_sync_buffer(&fSync,4096);
		bytes = Source()->Read(buffer,4096);
		if (bytes < 4096) {
			TRACE("oggReader::Sniff: Read: not enough: not ogg\n");
			return B_ERROR;
		}
		if (ogg_sync_wrote(&fSync,bytes) != 0) {
			TRACE("oggReader::Sniff: ogg_sync_wrote failed?: bail\n");
			return B_ERROR;
		}
		int result = ogg_sync_pageout(&fSync,&page);
		if (result == -1) {
			TRACE("oggReader::Sniff: ogg_sync_pageout: not synced: not ogg\n");
			return B_ERROR;
		} else if (result == 0) {
			// need more data to complete an ogg page
		} else if (result == 1) {
			// valid ogg page
			if (ogg_page_version(&page) != 0) {
				TRACE("oggReader::Sniff: ogg_sync_version: error in page encoding: not ogg\n");
				return B_ERROR;
			}
			if (ogg_page_bos(&page) == 0) {
				// not start of stream
				break;
			}
			// new stream
			if (ogg_stream_init(&stream,ogg_page_serialno(&page)) != 0) {
				TRACE("oggReader::Sniff: ogg_stream_init: failed: bail\n");
				return B_ERROR;
			}
			if (ogg_stream_pagein(&stream,&page) != 0) {
				TRACE("oggReader::Sniff: ogg_stream_pagein: failed: bail\n");
				return B_ERROR;
			}
			// we don't check for unique-ness of start pages.  we accept duplication.
			fStreams[ogg_page_serialno(&page)] = stream;
		} else {
			TRACE("oggReader::Sniff: ogg_sync_pageout: unexpected result: bail\n");
			return B_ERROR;
		}
	}
	// last page needs to be put into the right stream
	ogg_stream_map::iterator i = fStreams.find(ogg_page_serialno(&page));
	if (i == fStreams.end()) {
		// the next page doesn't belong to any existing stream?
		return B_ERROR;
	}
	if (ogg_stream_pagein(&i->second,&page) != 0) {
		TRACE("oggReader::Sniff: ogg_stream_pagein: failed: bail\n");
		return B_ERROR;
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
	ogg_stream_state * stream = static_cast<ogg_stream_state *>(cookie);
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
	ogg_stream_state * stream = static_cast<ogg_stream_state *>(cookie);


	return B_OK;
}


status_t
oggReader::GetNextChunk(void *cookie,
						void **chunkBuffer, int32 *chunkSize,
						media_header *mediaHeader)
{
	ogg_stream_state * stream = static_cast<ogg_stream_state *>(cookie);


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
