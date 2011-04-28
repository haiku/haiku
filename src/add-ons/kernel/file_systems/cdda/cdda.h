/*
 * Copyright 2007-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CDDA_H
#define CDDA_H


#include <scsi_cmds.h>


static const uint32 kFramesPerSecond = 75;
static const uint32 kFramesPerMinute = kFramesPerSecond * 60;
static const uint32 kFrameSize = 2352;
static const uint32 kDataTrackLeadGap = 11400;
static const uint8 kMaxTracks = 0x63;

struct cdtext {
	cdtext();
	~cdtext();

	char *artist;
	char *album;
	char *genre;

	char *titles[kMaxTracks];
	char *artists[kMaxTracks];
	uint8 track_count;
};


status_t read_cdtext(int fd, cdtext &text);
status_t read_table_of_contents(int fd, scsi_toc_toc *toc, size_t length);
status_t read_cdda_data(int fd, off_t endFrame, off_t offset, void *data,
	size_t length, off_t bufferOffset, void *buffer, size_t bufferSize);

#endif	// CDDA_H
