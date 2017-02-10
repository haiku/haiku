/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef MUSEPACK_READER_H
#define MUSEPACK_READER_H


#include "ReaderPlugin.h"
#include <MediaFormats.h>

#include "mpc/in_mpc.h"
#include "mpc/mpc_dec.h"


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
					media_format *format, const void **infoBuffer, size_t *infoSize);

		status_t Seek(void *cookie, uint32 seekTo,
					int64 *frame, bigtime_t *time);

		status_t GetNextChunk(void *cookie, const void **chunkBuffer, size_t *chunkSize,
					media_header *mediaHeader);

	private:
		StreamInfo	fInfo;
		media_format fFormat;
		MPC_decoder *fDecoder;
};

#endif	/* MUSEPACK_READER_H */
