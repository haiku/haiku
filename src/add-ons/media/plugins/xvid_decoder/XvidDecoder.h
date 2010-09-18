/*
 * XvidDecoder.h - XviD plugin for the Haiku Operating System
 *
 * Copyright (C) 2007 Stephan Aßmus <superstippi@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#ifndef XVID_DECODER_H
#define XVID_DECODER_H


#include <DataIO.h>
#include <MediaFormats.h>

#include "DecoderPlugin.h"
#include "xvid.h"


class XvidDecoder : public Decoder {
 public:
								XvidDecoder();
		
	virtual						~XvidDecoder();
		
	virtual	void				GetCodecInfo(media_codec_info *mci);
	
	virtual	status_t			Setup(media_format* inputFormat,
									const void* inInfo, size_t inSize);
   
	virtual	status_t			NegotiateOutputFormat(
									media_format *outputFormat);
	
	virtual	status_t			Decode(void* outBuffer, int64* outFrameCount,
									media_header* mh, media_decode_info* info);

	virtual	status_t			SeekedTo(int64 frame, bigtime_t time);

 private:
			int					_XvidInit();
			int					_XvidUninit();

			int					_XvidDecode(uchar *istream, uchar *ostream,
									int inStreamSize,
									xvid_dec_stats_t* xvidDecoderStats,
									bool hurryUp);

	media_format				fInputFormat;
	media_raw_video_format		fOutputVideoFormat;

	void*						fXvidDecoderHandle;
	int							fXvidColorspace;

	int64						fFrame;
	int32						fIndexInCodecTable;

	const void*					fChunkBuffer;
	uint8*						fWrappedChunkBuffer;
	const char*					fChunkBufferHandle;
	size_t						fChunkBufferSize;
	int32						fLeftInChunkBuffer;

	bool						fDiscontinuity;
};


class XvidPlugin : public DecoderPlugin {
 public:
			Decoder*			NewDecoder(uint index);
			status_t			GetSupportedFormats(
									media_format** formats, size_t* count);
};


#endif // XVID_DECODER_H
