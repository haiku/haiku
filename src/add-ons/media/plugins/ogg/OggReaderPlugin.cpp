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
	ogg_stream_init(&fOggStreamState,(int)this);
	ogg_sync_init(&fOggSyncState);
}


oggReader::~oggReader()
{
	ogg_stream_destroy(&fOggStreamState);
	ogg_sync_destroy(&fOggSyncState);
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
	
	fSeekableSource = dynamic_cast<BPositionIO *>(Reader::Source());
	if (!fSeekableSource) {
		TRACE("oggReader::Sniff: not a BPositionIO\n");
		return B_ERROR;
	}
	fSeekableSource->Seek(0,SEEK_SET);

	ogg_sync_state sync;
    ogg_sync_init(&sync);
	char * buffer = 0;
	ssize_t bytes = 0;
	ogg_page page;
	buffer = ogg_sync_buffer(&sync,4096);
   	bytes = fSeekableSource->Read(buffer,4096);
	if (bytes < 4096) {
		TRACE("oggReader::Sniff: Read: not enough: not ogg\n");
		ogg_sync_destroy(&sync);
		return B_ERROR;
	}
	ogg_sync_wrote(&sync,bytes);
	if (bytesogg_sync_pageout(&sync,&page) != 1) {
		TRACE("oggReader::Sniff: ogg_sync_pageout error: not ogg\n");
		ogg_sync_destroy(&sync);
		return B_ERROR;
	}
	// seems like ogg
	// TODO: count the streams
	*streamCount = 1;

/*	ogg_stream_state stream;
	ogg_stream_init(&stream,ogg_page_serial_no(&page);

	ogg_stream_pagein(&stream,&page);
	ogg_packet packet;
	ogg_stream_packetout(&stream,&packet);*/

	return B_OK;
}


void
oggReader::GetFileFormatInfo(media_file_format *mff)
{
	mff->capabilities =   media_file_format::B_READABLE
						| media_file_format::B_KNOWS_ENCODED_VIDEO
						| media_file_format::B_KNOWS_ENCODED_AUDIO
						| media_file_format::B_IMPERFECTLY_SEEKABLE;
	mff->family = B_OGG_FORMAT_FAMILY;
	mff->version = 100;
	strcpy(mff->mime_type, "application/ogg");
	strcpy(mff->file_extension, "ogg");

//	uint8 header[4];
//	Source()->ReadAt(fDataStart, header, sizeof(header));
	strcpy(mff->short_name,  "Ogg");
	strcpy(mff->pretty_name, "Ogg bitstream");
}

	
status_t
oggReader::AllocateCookie(int32 streamNumber, void **cookie)
{
	TRACE("oggReader::AllocateCookie\n");

	// store the cookie
	*cookie = data;
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
	return B_OK;
}


status_t
oggReader::Seek(void *cookie,
				uint32 seekTo,
				int64 *frame, bigtime_t *time)
{
	if (!fSeekableSource)
		return B_ERROR;

	ogg_sync_reset
	return B_OK;
}


status_t
oggReader::GetNextChunk(void *cookie,
						void **chunkBuffer, int32 *chunkSize,
						media_header *mediaHeader)
{
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
