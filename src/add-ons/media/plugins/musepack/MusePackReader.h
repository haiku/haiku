/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef MUSEPACK_READER_H
#define MUSEPACK_READER_H


#include "ReaderPlugin.h"
#include <MediaFormats.h>


class MusePackReader : public Reader {
	public:
		MusePackReader();
		~MusePackReader();

		const char *Copyright();

		status_t Sniff(int32 *streamCount);

		void GetFileFormatInfo(media_file_format *mff);

		status_t AllocateCookie(int32 streamNumber, void **cookie);
		status_t FreeCookie(void *cookie);

		status_t GetStreamInfo(void *cookie, int64 *frameCount, bigtime_t *duration,
					media_format *format, void **infoBuffer, int32 *infoSize);

		status_t Seek(void *cookie, uint32 seekTo,
					int64 *frame, bigtime_t *time);

		status_t GetNextChunk(void *cookie, void **chunkBuffer, int32 *chunkSize,
					media_header *mediaHeader);

	private:
};

#endif	/* MUSEPACK_READER_H */
