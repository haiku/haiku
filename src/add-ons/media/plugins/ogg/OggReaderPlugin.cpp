#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdint.h>
#include <Autolock.h>
#include <ByteOrder.h>
#include <DataIO.h>
#include <File.h>
#include <InterfaceDefs.h>
#include <MediaFormats.h>
#include "OggReaderPlugin.h"
#include "OggStream.h"
#include "OggSeekable.h"

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...) ((void)0)
#endif

OggReader::OggReader()
{
	TRACE("OggReader::OggReader\n");
	ogg_sync_init(&fSync);
	fSeekable = NULL;
	fFile = NULL;
	fPosition = 0;
}


OggReader::~OggReader()
{
	TRACE("OggReader::~OggReader\n");
	serialno_OggTrack_map::iterator i = fTracks.begin();
	while (i != fTracks.end()) {
		serialno_OggTrack_map::iterator j = i;
		i++;
		delete j->second;
	}
	ogg_sync_clear(&fSync);
}

      
const char *
OggReader::Copyright()
{
	return "ogg reader, " B_UTF8_COPYRIGHT " by Andrew Bachmann";
}


// the main page reading hook (stream friendly)
ssize_t
OggReader::ReadPage(bool first_page)
{
//	TRACE("OggReader::ReadPage\n");
	int read_size = (first_page ? 4096 : 4*B_PAGE_SIZE);
	BAutolock autolock(fSyncLock);
	ogg_page page;
retry:
	int result = ogg_sync_pageout(&fSync, &page); // first read leftovers
	while (result == 0) {
		char * buffer = ogg_sync_buffer(&fSync, read_size);
		ssize_t bytes = Source()->Read(buffer, read_size);
		if (bytes == 0) {
			TRACE("OggReader::GetPage: Read: no data\n");
			return B_LAST_BUFFER_ERROR;
		}
		if (bytes < 0) {
			TRACE("OggReader::GetPage: Read: error\n");
			return bytes;
		}
		if (ogg_sync_wrote(&fSync, bytes) != 0) {
			TRACE("OggReader::GetPage: ogg_sync_wrote failed?: error\n");
			return B_ERROR;
		}
		result = ogg_sync_pageout(&fSync, &page);
		if (first_page && (result != 1)) {
			TRACE("OggReader::GetPage: short first page not found: error\n");
			return B_ERROR;
		}
	}
	if (result == -1) {
		TRACE("OggReader::GetPage: ogg_sync_pageout: not synced: error\n");
		return B_ERROR;
	}
	if (ogg_page_version(&page) != 0) {
		TRACE("OggReader::GetPage: ogg_page_version: error in page encoding: error\n");
		return B_ERROR;
	}
	long serialno = ogg_page_serialno(&page);
	bool new_serialno = fTracks.find(serialno) == fTracks.end();
	if (new_serialno) {
		// this is an unknown serialno
		if (ogg_page_bos(&page) == 0) {
			TRACE("OggReader::GetPage: non-bos page with unknown serialno\n");
#ifdef STRICT_OGG
			return B_ERROR;
#else
			// silently discard non-bos packets with unknown serialno
			goto retry;
#endif
		}
#ifdef STRICT_OGG
		if (ogg_page_continued(&page) != 0) {
			TRACE("oggReader::GetPage: ogg_page_continued: continued page: not ogg\n");
			return B_ERROR;
		}
#endif STRICT_OGG
		// this is a beginning of stream page
		ogg_stream_state stream;
		if (ogg_stream_init(&stream, serialno) != 0) {
			TRACE("oggReader::GetPage: ogg_stream_init failed?: error\n");
			return B_ERROR;
		}
		if (ogg_stream_pagein(&stream, &page) != 0) {
			TRACE("oggReader::GetPage: ogg_stream_pagein: failed: error\n");
			return B_ERROR;
		}
		ogg_packet packet;
		if (ogg_stream_packetout(&stream, &packet) != 1) {
#ifdef STRICT_OGG
			return B_ERROR;
#endif STRICT_OGG
		}
		if (fSeekable) {
			fTracks[serialno] = OggSeekable::makeOggSeekable(fSeekable, &fSeekableLock, serialno, packet);
		} else {
			class Interface : public StreamInterface {
			private:
				OggReader * reader;
			public:
				Interface(OggReader * reader) {
					this->reader = reader;
				}
				virtual ssize_t ReadPage() {
					return reader->ReadPage();
				}
			};
			fTracks[serialno] = OggStream::makeOggStream(new Interface(this), serialno, packet);
		}
		fCookies.push_back(serialno);
	}
	// this check ensures that we only push the initial pages into OggSeekables.
	if (!fSeekable || new_serialno) {
		status_t status = fTracks[serialno]->AddPage(fPosition, page);
		if (status != B_OK) {
			return status;
		}
		fPosition += page.header_len + page.body_len;
	}
	return page.header_len + page.body_len;
}


static BPositionIO *
get_seekable(BDataIO * data)
{
	// try to cast to BPositionIO then perform a series of
	// sanity checks to ensure that seeking is reliable
	BPositionIO * seekable = dynamic_cast<BPositionIO *>(data);
	if (seekable == 0) {
		return 0;
	}
	// first try to get our current location
	off_t current = seekable->Seek(0, SEEK_CUR);
	if (current < 0) {
		return 0;
	}
	// check it against position
	if (current != seekable->Position()) {
		return 0;
	}
	// next try to seek to our current location (absolutely)
	if (seekable->Seek(current, SEEK_SET) < 0) {
		return 0;
	}
	// next try to seek to the start of the stream (absolutely)
	if (seekable->Seek(0, SEEK_SET) < 0) {
		return 0;
	}
	// then seek back to where we started
	if (seekable->Seek(current, SEEK_SET) < 0) {
		// really screwed
		return 0;
	}
	return seekable;
}


static BFile *
get_file(BDataIO * data)
{
	// try to cast to BFile then perform a series of
	// sanity checks to ensure that the file is okay
	BFile * file = dynamic_cast<BFile *>(data);
	if (file == 0) {
		return 0;
	}
	if (file->InitCheck() != B_OK) {
		return 0;
	}
	return file;
}


status_t
OggReader::Sniff(int32 *streamCount)
{
	TRACE("OggReader::Sniff\n");
#ifdef STRICT_OGG
	bool first_page = true;
#else
	bool first_page = false;
#endif
	fSeekable = get_seekable(Source());
	fFile = get_file(Source());

	ssize_t bytes = ReadPage(first_page);
	if (bytes < 0) {
		return bytes;
	}
	uint count = 1;
	while (count++ == fCookies.size()) {
		ssize_t bytes = ReadPage();
		if (bytes < 0) {
			return bytes;
		}
	}
	if (fSeekable) {
		status_t status = FindLastPages();
		if (status != B_OK) {
			return status;
		}
	}
	*streamCount = fCookies.size();
	return B_OK;
}


void
OggReader::GetFileFormatInfo(media_file_format *mff)
{
	TRACE("OggReader::GetFileFormatInfo\n");
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
OggReader::AllocateCookie(int32 streamNumber, void **cookie)
{
	TRACE("OggReader::AllocateCookie %ld\n", streamNumber);
	if (streamNumber < 0 || streamNumber > (signed)fCookies.size()) {
		TRACE("OggReader::AllocateCookie: invalid streamNumber: bail\n");
		return B_ERROR;
	}
	OggTrack * track = fTracks[fCookies[streamNumber]];
	// store the cookie
	*cookie = track;
	return B_OK;
}


status_t
OggReader::FreeCookie(void *cookie)
{
	TRACE("OggReader::FreeCookie\n");
	return B_OK;
}


status_t
OggReader::GetStreamInfo(void *cookie, int64 *frameCount, bigtime_t *duration,
						 media_format *format, void **infoBuffer, int32 *infoSize)
{
	TRACE("OggReader::GetStreamInfo\n");
	*infoBuffer = 0;
	*infoSize = 0;
	OggTrack * track = static_cast<OggTrack*>(cookie);
	status_t status = track->GetStreamInfo(frameCount, duration, format);
	TRACE("OggReader::GetStreamInfo: cookie=%x, frame count = %lld, duration = %lld.%lld seconds\n",
	      *(int*)cookie, *frameCount, (*duration)/1000000, (*duration)%1000000);
	return status;
}


status_t
OggReader::Seek(void *cookie,
				uint32 seekTo,
				int64 *frame, bigtime_t *time)
{
	TRACE("OggReader::Seek to %lld : %lld\n", *frame, *time);
	OggTrack * track = static_cast<OggTrack*>(cookie);
	return track->Seek(seekTo, frame, time);
}


status_t
OggReader::GetNextChunk(void *cookie,
						void **chunkBuffer, int32 *chunkSize,
						media_header *mediaHeader)
{
//	TRACE("OggReader::GetNextChunk\n");
	OggTrack * track = static_cast<OggTrack*>(cookie);
	return track->GetNextChunk(chunkBuffer, chunkSize, mediaHeader);
}


status_t
OggReader::FindLastPages()
{
	TRACE("OggReader::FindLastPages\n");
	bigtime_t start_time = system_time();

	status_t result = B_ERROR;

	const int read_size = 256*256;

	ogg_page page;
	ogg_sync_state sync;
	ogg_sync_init(&sync);

	off_t right = fSeekable->Seek(0, SEEK_END);
	off_t left = right;
	// we assume the common case is that the last pages are near the end
	uint serial_count = 0;
	while (serial_count < fCookies.size()) {
		int offset;
		ssize_t bytes = 0;
		while ((offset = ogg_sync_pageseek(&sync, &page)) <= 0) {
			left += -offset;
			if (offset == 0) {
				off_t pos = fSeekable->Position();
				if (pos >= right || bytes == 0) {
					if (left == 0) {
						TRACE("OggReader::FindLastPages: couldn't find some stream's page!!!\n");
						goto done;
					}
					left = max_c(0, left - read_size);
					result = fSeekable->Seek(left, SEEK_SET);
					if (result < 0) {
						goto done;
					}
					ogg_sync_reset(&sync);
				}
				char * buffer = ogg_sync_buffer(&sync, read_size);
				bytes = fSeekable->Read(buffer, read_size);
				if (bytes < 0) {
					TRACE("OggReader::FindLastPages: Read: error\n");
					result = bytes;
					goto done;
				}
				if (ogg_sync_wrote(&sync, bytes) != 0) {
					TRACE("OggReader::FindLastPages: ogg_sync_wrote failed?: error\n");
					goto done;
				}
			}
		}
		off_t current = left;
		do {
			// found a page at "current"
			long serialno = ogg_page_serialno(&page);
			OggSeekable * track = dynamic_cast<OggSeekable*>(fTracks[serialno]);
			if (track == 0) {
				TRACE("OggReader::FindLastPages: unknown serialno == TODO: chaining?\n");
			} else {
				if (track->GetLastPagePosition() == 0) {
					serial_count++;
				}
				track->SetLastPagePosition(current);
			}
			current += page.header_len + page.body_len;
		} while ((current < right) && (ogg_sync_pageout(&sync, &page) == 1));
		right = left;
		ogg_sync_reset(&sync);
	}
	result = B_OK;
done:
	ogg_sync_clear(&sync);
	TRACE("OggReader::FindLastPages took %lld microseconds\n", system_time() - start_time);

	return result;
}

/*
 * OggReaderPlugin
 */

Reader *
OggReaderPlugin::NewReader()
{
	return new OggReader;
}


MediaPlugin *
instantiate_plugin()
{
	return new OggReaderPlugin;
}
