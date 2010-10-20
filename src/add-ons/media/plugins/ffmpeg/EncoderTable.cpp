/*
 * Copyright 2009-2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "EncoderTable.h"


const EncoderDescription gEncoderTable[] = {
	{
		{
			"MPEG-4 video",
			"mpeg4",
			0,
			CODEC_ID_MPEG4,
			{ 0 }
		},
		B_ANY_FORMAT_FAMILY, // TODO: Hm, actually not really /any/ family...
		B_MEDIA_RAW_VIDEO,
		B_MEDIA_ENCODED_VIDEO,
		23
	},
	{
		{
			"MPEG-1 video",
			"mpeg1video",
			0,
			CODEC_ID_MPEG1VIDEO,
			{ 0 }
		},
		B_MPEG_FORMAT_FAMILY,
		B_MEDIA_RAW_VIDEO,
		B_MEDIA_ENCODED_VIDEO,
		10
	},
	{
		{
			"MPEG-2 video",
			"mpeg2video",
			0,
			CODEC_ID_MPEG2VIDEO,
			{ 0 }
		},
		B_MPEG_FORMAT_FAMILY,
		B_MEDIA_RAW_VIDEO,
		B_MEDIA_ENCODED_VIDEO,
		15
	},
	{
		{
			"Theora video",
			"theora",
			0,
			CODEC_ID_THEORA,
			{ 0 }
		},
		B_ANY_FORMAT_FAMILY,
		B_MEDIA_RAW_VIDEO,
		B_MEDIA_ENCODED_VIDEO,
		18
	},
	{
		{
			"DV (Digital Video)",
			"dvvideo",
			0,
			CODEC_ID_DVVIDEO,
			{ 0 }
		},
		B_MISC_FORMAT_FAMILY,
		B_MEDIA_RAW_VIDEO,
		B_MEDIA_ENCODED_VIDEO,
		5
	},
	{
		{
			"MJPEG (Motion JPEG)",
			"mjpeg",
			0,
			CODEC_ID_MJPEG,
			{ 0 }
		},
		B_ANY_FORMAT_FAMILY,
		B_MEDIA_RAW_VIDEO,
		B_MEDIA_ENCODED_VIDEO,
		5
	},
	{
		{
			"Raw audio",
			"pcm",
			0,
			0,
			{ 0 }
		},
		B_ANY_FORMAT_FAMILY,
		B_MEDIA_RAW_AUDIO,
		B_MEDIA_ENCODED_AUDIO,
		1
	},
	{
		{
			"Dolby Digital (AC-3)",
			"ac3",
			0,
			CODEC_ID_AC3,
			{ 0 }
		},
		B_ANY_FORMAT_FAMILY,
		B_MEDIA_RAW_AUDIO,
		B_MEDIA_ENCODED_AUDIO,
		10
	},
	{
		{
			"Vorbis audio",
			"vorbis",
			0,
			CODEC_ID_VORBIS,
			{ 0 }
		},
		B_ANY_FORMAT_FAMILY,
		B_MEDIA_RAW_AUDIO,
		B_MEDIA_ENCODED_AUDIO,
		10
	}
};

const size_t gEncoderCount = sizeof(gEncoderTable) / sizeof(EncoderDescription);


/*static*/ CodecID
raw_audio_codec_id_for(const media_format& format)
{
	if (format.type != B_MEDIA_RAW_AUDIO)
		return CODEC_ID_NONE;

	if (format.u.raw_audio.byte_order == B_MEDIA_LITTLE_ENDIAN) {
		switch (format.u.raw_audio.format) {
			case media_raw_audio_format::B_AUDIO_FLOAT:
				return CODEC_ID_PCM_F32LE;
			case media_raw_audio_format::B_AUDIO_DOUBLE:
				return CODEC_ID_PCM_F64LE;
			case media_raw_audio_format::B_AUDIO_INT:
				return CODEC_ID_PCM_S32LE;
			case media_raw_audio_format::B_AUDIO_SHORT:
				return CODEC_ID_PCM_S16LE;
			case media_raw_audio_format::B_AUDIO_UCHAR:
				return CODEC_ID_PCM_U8;
			default:
				return CODEC_ID_NONE;
		}
	} else {
		switch (format.u.raw_audio.format) {
			case media_raw_audio_format::B_AUDIO_FLOAT:
				return CODEC_ID_PCM_F32BE;
			case media_raw_audio_format::B_AUDIO_DOUBLE:
				return CODEC_ID_PCM_F64BE;
			case media_raw_audio_format::B_AUDIO_INT:
				return CODEC_ID_PCM_S32BE;
			case media_raw_audio_format::B_AUDIO_SHORT:
				return CODEC_ID_PCM_S16BE;
			case media_raw_audio_format::B_AUDIO_UCHAR:
				return CODEC_ID_PCM_U8;
			default:
				return CODEC_ID_NONE;
		}
	}
}



