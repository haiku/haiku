/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "cddb.h"


static int32
cddb_sum(int32 n)
{
	int32 result = 0;

	while (n > 0) {
		result = result + (n % 10);
		n = n / 10;
	}

	return result;
}


//	#pragma mark - exported functions


/*!
	Computes the CD Disc ID as specified in the FreeDB how-to (code taken from there).
*/
uint32
compute_cddb_disc_id(scsi_toc_toc &toc)
{
	int32 numTracks = toc.last_track + 1 - toc.first_track;
	uint32 n = 0;

	for (int32 i = 0; i < numTracks; i++) {
		n = n + cddb_sum((toc.tracks[i].start.time.minute * 60)
			+ toc.tracks[i].start.time.second);
	}

	int32 t = ((toc.tracks[numTracks].start.time.minute * 60)
			+ toc.tracks[numTracks].start.time.second)
		- ((toc.tracks[0].start.time.minute * 60)
			+ toc.tracks[0].start.time.second);

	return (n % 0xff) << 24 | t << 8 | numTracks;
}
