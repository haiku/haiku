/*
** File avcodec.h
**
** libavcodec based decoder for OpenBeOS
**
** Copyright (C) 2001 Carlos Hasan. All Rights Reserved.
** Copyright (C) 2001 François Revol. All Rights Reserved.
** Copyright (C) 2001 Axel Dörfler. All Rights Reserved.
** Copyright (C) 2004 Marcus Overhagen. All Rights Reserved.
**
** Distributed under the terms of the MIT License.
*/

#ifndef __AVCODEC_PLUGIN_H__
#define __AVCODEC_PLUGIN_H__

#include <MediaFormats.h>
#include "ReaderPlugin.h"
#include "DecoderPlugin.h"

//#define DO_PROFILING

#include "video_util.h"

struct codec_table { CodecID id; media_type type; media_format_family family; uint32 fourcc; const char *prettyname;};

extern const struct codec_table gCodecTable[];
extern const int num_codecs;
extern media_format avcodec_formats[];

class avCodec : public Decoder
{
	public:
		avCodec();
		
		virtual ~avCodec();
		
		virtual void GetCodecInfo(media_codec_info *mci);
		
		virtual status_t Setup(media_format *input_format,
							   const void *in_info, int32 in_size);
	   
		virtual status_t NegotiateOutputFormat(media_format *output_format);
		
		virtual status_t Decode(void *out_buffer, int64 *out_frameCount,
							    media_header *mh, media_decode_info *info);
	
		virtual status_t Seek(uint32 in_towhat,
							   int64 in_requiredFrame, int64 *inout_frame,
							   bigtime_t in_requiredTime, bigtime_t *inout_time);
	
	
	protected:
		media_header fHeader;
		media_decode_info fInfo;
	
		friend class avCodecInputStream;
		
	private:
		media_format fInputFormat;
		media_raw_video_format fOutputVideoFormat;

		int64 fFrame;
		bool isAudio;
	
		int ffcodec_index_in_table; // helps to find codecpretty
		
		// ffmpeg related datas
		AVCodec *fCodec;
		AVCodecContext *ffc;
		AVFrame *ffpicture, *opicture;
		
		bool 		fCodecInitDone;

		gfx_convert_func conv_func; // colorspace convert func

		void *		fExtraData;
		int			fExtraDataSize;
		int			fBlockAlign;
		
		bigtime_t	fStartTime;
		int32		fOutputFrameCount;
		float		fOutputFrameRate;
		
		char *		fChunkBuffer;
		int32		fChunkBufferOffset;
		int32		fChunkBufferSize;

		char *		fOutputBuffer;
		int32		fOutputBufferOffset;
		int32		fOutputBufferSize;

};

class avCodecPlugin : public DecoderPlugin
{
public:
	Decoder *	NewDecoder(uint index);
	status_t	GetSupportedFormats(media_format ** formats, size_t * count);
};


#endif
