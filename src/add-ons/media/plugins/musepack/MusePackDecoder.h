/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef MUSEPACK_DECODER_H
#define MUSEPACK_DECODER_H


#include "DecoderPlugin.h"
#include <MediaFormats.h>

#include "mpc/mpc_dec.h"


class MusePackDecoder : public Decoder {
	public:
		MusePackDecoder();
		~MusePackDecoder();

		void GetCodecInfo(media_codec_info *info);

		status_t Setup(media_format *ioEncodedFormat,
					const void *infoBuffer, size_t infoSize);

		status_t NegotiateOutputFormat(media_format *ioDecodedFormat);

		status_t SeekedTo(int64 frame, bigtime_t time);

		status_t Decode(void *buffer, int64 *frameCount,
					media_header *mediaHeader, media_decode_info *info);

	private:
		MPC_decoder	*fDecoder;
		StreamInfo	*fInfo;
		float		fFrameRate;
		int64		fFramePosition;
};

#endif	/* MUSEPACK_DECODER_H */
