/*
 * Copyright 2010 Stephan Aßmus <superstippi@gmx.de>. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "CodecTable.h"

extern "C" {
	#include "avcodec.h"
	#include "avformat.h"
}

//XXX: newer versions have it in libavformat/internal.h
typedef struct AVCodecTag {
    enum CodecID id;
    unsigned int tag;
} AVCodecTag;


struct AVInputFamily gAVInputFamilies[] = {
	{ B_AIFF_FORMAT_FAMILY, "aiff" },
	{ B_AVI_FORMAT_FAMILY, "avi" },
	{ B_MPEG_FORMAT_FAMILY, "mpeg" },
	{ B_QUICKTIME_FORMAT_FAMILY, "mov" },
	{ B_ANY_FORMAT_FAMILY, NULL}
};

static const int32 sMaxFormatCount = 1024;
media_format gAVCodecFormats[sMaxFormatCount];


status_t
register_avcodec_tags(media_format_family family, const char *avname, int &index)
{
	AVInputFormat *inputFormat = av_find_input_format(avname);
	if (inputFormat == NULL)
		return B_MEDIA_NO_HANDLER;

	BMediaFormats mediaFormats;
	if (mediaFormats.InitCheck() != B_OK)
		return B_ERROR;

	for (int tagSet = 0; inputFormat->codec_tag[tagSet]; tagSet++) {
		const AVCodecTag *tags = inputFormat->codec_tag[tagSet];
		if (tags == NULL)
			continue;

		for (; tags->id != CODEC_ID_NONE; tags++) {
			// XXX: we might want to keep some strange PCM codecs too...
			// skip unwanted codec tags
			if (tags->tag == CODEC_ID_RAWVIDEO
				|| (tags->tag >= CODEC_ID_PCM_S16LE
					&& tags->tag < CODEC_ID_ADPCM_IMA_QT)
				|| tags->tag >= CODEC_ID_DVD_SUBTITLE)
				continue;

			if (index >= sMaxFormatCount) {
				fprintf(stderr, "Maximum format count reached for auto-generated "
					"AVCodec to media_format mapping, but there are still more "
					"AVCodecs compiled into libavcodec!\n");
				break;
			}

			media_format format;
			// Determine media type
			if (tags->tag < CODEC_ID_PCM_S16LE)
				format.type = B_MEDIA_ENCODED_VIDEO;
			else
				format.type = B_MEDIA_ENCODED_AUDIO;

			media_format_description description;
			memset(&description, 0, sizeof(description));

			// Hard-code everything to B_MISC_FORMAT_FAMILY to ease matching
			// later on.
			description.family = family;
			switch (family) {
				case B_AIFF_FORMAT_FAMILY:
					description.u.aiff.codec = tags->tag;
					break;
				case B_AVI_FORMAT_FAMILY:
					description.u.avi.codec = tags->tag;
					break;
				case B_MPEG_FORMAT_FAMILY:
					description.u.mpeg.id = tags->tag;
					break;
				case B_QUICKTIME_FORMAT_FAMILY:
					description.u.quicktime.codec = tags->tag;
					break;
				case B_WAV_FORMAT_FAMILY:
					description.u.wav.codec = tags->tag;
					break;
				default:
					break;
			}

			format.require_flags = 0;
			format.deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;

			if (mediaFormats.MakeFormatFor(&description, 1, &format) != B_OK)
				return B_ERROR;

			gAVCodecFormats[index] = format;

			index++;
		}
	}
	return B_OK;
}


status_t
build_decoder_formats(media_format** _formats, size_t* _count)
{
	BMediaFormats mediaFormats;
	if (mediaFormats.InitCheck() != B_OK)
		return B_ERROR;

	int32 index = 0;
	AVCodec* codec = NULL;
	while ((codec = av_codec_next(codec)) != NULL) {
		if (index >= sMaxFormatCount) {
			fprintf(stderr, "Maximum format count reached for auto-generated "
				"AVCodec to media_format mapping, but there are still more "
				"AVCodecs compiled into libavcodec!\n");
			break;
		}
		media_format format;
		// Determine media type
		switch (codec->type) {
			case AVMEDIA_TYPE_VIDEO:
				format.type = B_MEDIA_ENCODED_VIDEO;
				break;
			case AVMEDIA_TYPE_AUDIO:
				format.type = B_MEDIA_ENCODED_AUDIO;
				break;
			default:
				// ignore this AVCodec
				continue;
		}

		media_format_description description;
		memset(&description, 0, sizeof(description));

		// Hard-code everything to B_MISC_FORMAT_FAMILY to ease matching
		// later on.
		description.family = B_MISC_FORMAT_FAMILY;
		description.u.misc.file_format = 'ffmp';
		description.u.misc.codec = codec->id;

		format.require_flags = 0;
		format.deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;

		if (mediaFormats.MakeFormatFor(&description, 1, &format) != B_OK)
			return B_ERROR;

		gAVCodecFormats[index] = format;

		index++;
	}

	*_formats = gAVCodecFormats;
	*_count = index;

	return B_OK;
}

