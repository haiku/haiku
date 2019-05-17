/*
 * Copyright 2019, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "DVDMediaIO.h"

#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "MediaDebug.h"


#define DVD_READ_CACHE 1

#define DEFAULT_LANGUAGE "en"


DVDMediaIO::DVDMediaIO(const char* path)
	:
	BAdapterIO(
		B_MEDIA_STREAMING | B_MEDIA_SEEKABLE,
		B_INFINITE_TIMEOUT),
	fPath(path),
	fLoopThread(-1),
	fExit(false)
{
	fBuffer = (uint8_t*) malloc(DVD_VIDEO_LB_LEN);
}


DVDMediaIO::~DVDMediaIO()
{
	fExit = true;

	status_t status;
	if (fLoopThread != -1)
		wait_for_thread(fLoopThread, &status);

	free(fBuffer);
}


ssize_t
DVDMediaIO::WriteAt(off_t position, const void* buffer, size_t size)
{
	return B_NOT_SUPPORTED;
}


status_t
DVDMediaIO::SetSize(off_t size)
{
	return B_NOT_SUPPORTED;
}


status_t
DVDMediaIO::Open()
{
	fInputAdapter = BuildInputAdapter();

	if (dvdnav_open(&fDvdNav, fPath) != DVDNAV_STATUS_OK) {
		TRACE("DVDMediaIO::Open() dvdnav_open error\n");
		return B_ERROR;
	}

	// read ahead cache usage
	if (dvdnav_set_readahead_flag(fDvdNav, DVD_READ_CACHE) != DVDNAV_STATUS_OK) {
		TRACE("DVDMediaIO::Open() dvdnav_set_readahead_flag error: %s\n",
			dvdnav_err_to_string(fDvdNav));
		return B_ERROR;
	}

	// set language
	if (dvdnav_menu_language_select(fDvdNav, DEFAULT_LANGUAGE) != DVDNAV_STATUS_OK ||
			dvdnav_audio_language_select(fDvdNav, DEFAULT_LANGUAGE) != DVDNAV_STATUS_OK ||
		dvdnav_spu_language_select(fDvdNav, DEFAULT_LANGUAGE) != DVDNAV_STATUS_OK) {
		TRACE("DVDMediaIO::Open() setting languages error: %s\n",
			dvdnav_err_to_string(fDvdNav));
		return B_ERROR;
	}

	// set the PGC positioning flag
	if (dvdnav_set_PGC_positioning_flag(fDvdNav, 1) != DVDNAV_STATUS_OK) {
		TRACE("DVDMediaIO::Open() dvdnav_set_PGC_positioning_flag error: %s\n",
			dvdnav_err_to_string(fDvdNav));
		return 2;
	}

	fLoopThread = spawn_thread(_LoopThread, "two and two are five",
		B_NORMAL_PRIORITY, this);

	if (fLoopThread <= 0 || resume_thread(fLoopThread) != B_OK)
		return B_ERROR;

	return B_OK;
}


int32
DVDMediaIO::_LoopThread(void* data)
{
	static_cast<DVDMediaIO *>(data)->LoopThread();
	return 0;
}


void
DVDMediaIO::LoopThread()
{
	while (!fExit) {
		int err;
		int event;
		int len;

#if DVD_READ_CACHE
		err = dvdnav_get_next_cache_block(fDvdNav, &fBuffer, &event, &len);
#else
		err = dvdnav_get_next_block(fDvdNav, fBuffer, &event, &len);
#endif

		if (err == DVDNAV_STATUS_ERR) {
			TRACE("DVDMediaIO::LoopThread(): Error getting next block: %s\n",
				dvdnav_err_to_string(fDvdNav));
			continue;
		}

		HandleDVDEvent(event, len);

#if DVD_READ_CACHE
		dvdnav_free_cache_block(fDvdNav, fBuffer);
#endif
	}

	if (dvdnav_close(fDvdNav) != DVDNAV_STATUS_OK) {
		TRACE("DVDMediaIO::LoopThread() dvdnav_close error: %s\n",
			dvdnav_err_to_string(fDvdNav));
	}
	fLoopThread = -1;
}


void
DVDMediaIO::HandleDVDEvent(int event, int len)
{
	switch (event) {
		case DVDNAV_BLOCK_OK:
			fInputAdapter->Write(fBuffer, len);
			break;

		case DVDNAV_NOP:
			break;

		case DVDNAV_STILL_FRAME:
		{
			dvdnav_still_event_t* still_event
				= (dvdnav_still_event_t*) fBuffer;
			if (still_event->length < 0xff) {
				TRACE("DVDMediaIO::HandleDVDEvent: Skipped %d "
						"seconds of still frame\n",
					still_event->length);
			} else {
				TRACE("DVDMediaIO::HandleDVDEvent: Skipped "
					"indefinite length still frame\n");
			}
			dvdnav_still_skip(fDvdNav);
			break;
		}

		case DVDNAV_WAIT:
			TRACE("DVDMediaIO::HandleDVDEvent: Skipping wait condition\n");
			dvdnav_wait_skip(fDvdNav);
			break;

		case DVDNAV_SPU_CLUT_CHANGE:
			break;

		case DVDNAV_SPU_STREAM_CHANGE:
			break;

		case DVDNAV_AUDIO_STREAM_CHANGE:
			break;

		case DVDNAV_HIGHLIGHT:
		{
			dvdnav_highlight_event_t* highlight_event
				= (dvdnav_highlight_event_t*) fBuffer;
			TRACE("DVDMediaIO::HandleDVDEvent: Button: %d\n",
				highlight_event->buttonN);
			break;
		}

		case DVDNAV_VTS_CHANGE:
			break;

		case DVDNAV_CELL_CHANGE:
		{
			int32_t title = 0, chapter = 0;
			uint32_t pos, len;

			dvdnav_current_title_info(fDvdNav, &title, &chapter);
			dvdnav_get_position(fDvdNav, &pos, &len);
			TRACE("DVDMediaIO::HandleDVDEvent: Cell: Title %d, Chapter %d\n",
				tt, ptt);
			TRACE("DVDMediaIO::HandleDVDEvent: At position %.0f%% inside "
				"the feature\n", 100 * (double)pos / (double)len);
			break;
		}

		case DVDNAV_NAV_PACKET:
			break;

		case DVDNAV_HOP_CHANNEL:
			break;

		case DVDNAV_STOP:
			fExit = true;
			break;

		default:
			TRACE("DVDMediaIO::HandleDVDEvent: unknown event (%i)\n",
				event);
			fExit = true;
			break;
	}
}


status_t
DVDMediaIO::SeekRequested(off_t position)
{
	dvdnav_sector_search(fDvdNav, position, SEEK_SET);
	return B_OK;
}


void
DVDMediaIO::MouseMoved(uint32 x, uint32 y)
{
	pci_t* pci = dvdnav_get_current_nav_pci(fDvdNav);
	dvdnav_mouse_select(fDvdNav, pci, x, y);
}


void
DVDMediaIO::MouseDown(uint32 x, uint32 y)
{
	pci_t* pci = dvdnav_get_current_nav_pci(fDvdNav);
	dvdnav_mouse_activate(fDvdNav, pci, x, y);
}
