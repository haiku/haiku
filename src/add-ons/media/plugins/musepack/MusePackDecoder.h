/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef MUSEPACK_DECODER_H
#define MUSEPACK_DECODER_H


#include "DecoderPlugin.h"
#include <MediaFormats.h>


class MusePackDecoder : public Decoder {
	public:
		MusePackDecoder();
		~MusePackDecoder();
	
		status_t Setup(media_format *ioEncodedFormat,
					const void *infoBuffer, int32 infoSize);

		status_t NegotiateOutputFormat(media_format *ioDecodedFormat);

		status_t Seek(uint32 seekTo, int64 seekFrame, int64 *frame,
					bigtime_t seekTime, bigtime_t *time);

		status_t Decode(void *buffer, int64 *frameCount,
					media_header *mediaHeader, media_decode_info *info);

	private:
};

#endif	/* MUSEPACK_DECODER_H */
