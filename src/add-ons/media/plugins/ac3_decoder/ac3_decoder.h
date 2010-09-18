/*
 *  ac3_decoder.h
 *  Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>
 *
 *  This file is part of an AC-3 decoder plugin for the OpenBeOS
 *  media kit. OpenBeOS can be found at http://www.openbeos.org
 *
 *  This file is distributed under the terms of the MIT license.
 *  You can also use it under the terms of the GPL version 2.
 *
 *  Since this file is linked against a GPL licensed library,
 *  the combined work of this file and the liba52 library, the
 *  ac3_decoder plugin must be distributed as GPL licensed.
 */

#ifndef _AC3_DECODER_H
#define _AC3_DECODER_H

#include <inttypes.h>
#include "DecoderPlugin.h"

extern "C" {
	#include "liba52/config.h"
	#include "liba52/a52.h"
}


class AC3Decoder : public Decoder
{
public:
				AC3Decoder();
				~AC3Decoder();
	
	void		GetCodecInfo(media_codec_info *info);

	status_t	Setup(media_format *ioEncodedFormat,
					  const void *infoBuffer, size_t infoSize);

	status_t	NegotiateOutputFormat(media_format *ioDecodedFormat);

	status_t	SeekedTo(int64 frame, bigtime_t time);

	bool		GetStreamInfo();
							 
	status_t	Decode(void *buffer, int64 *frameCount,
					   media_header *mediaHeader, media_decode_info *info);

	bool		InputGetData(void **buffer, int size);
	void		InputRemoveData(int size);
	
	bool		DecodeNext();
	
private:
	enum {		INPUT_BUFFER_MAX_SIZE = 4000 }; // must be >= 3840
	void *		fInputBuffer;
	int			fInputBufferSize;
	
	const void *fInputChunk;
	size_t		fInputChunkSize;
	int			fInputChunkOffset;
	
	bigtime_t	fStartTime;
	bool		fHasStreamInfo;
	bool		fDisableDynamicCompression;
	
	float *		fSamples;
	
	a52_state_t *fState;
	
	int 		fFlags;
	int			fFrameRate;
	int			fBitRate;
	int			fFrameSize;
	int			fChannelCount;
	int			fChannelMask;
	int			*fInterleaveOffset;
	
	char		fChannelInfo[120];
};


class AC3DecoderPlugin : public DecoderPlugin
{
public:
	Decoder *	NewDecoder(uint index);
	status_t	GetSupportedFormats(media_format ** formats, size_t * count);
};


#endif // _AC3_DECODER_H
