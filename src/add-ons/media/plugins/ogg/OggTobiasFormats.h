#ifndef _OGG_TOBIAS_FORMATS_H
#define _OGG_TOBIAS_FORMATS_H

#include <MediaFormats.h>
#include <ogg/ogg.h>
#include <string.h>
#include "OggFormats.h"

/*
 * tobias descriptions/formats
 */


// video

static media_format_description
tobias_video_description()
{
	media_format_description description;
	description.family = B_AVI_FORMAT_FAMILY;
	return description;
}


static void
init_tobias_media_raw_video_format(media_raw_video_format * output)
{
	
}


static media_format
tobias_video_encoded_media_format()
{
	media_format format;
	format.type = B_MEDIA_ENCODED_VIDEO;
	init_tobias_media_raw_video_format(&format.u.encoded_video.output);
	return format;
}

// audio

static media_format_description
tobias_audio_description()
{
	media_format_description description;
	description.family = B_WAV_FORMAT_FAMILY;
	return description;
}


static void
init_tobias_media_raw_audio_format(media_raw_audio_format * output)
{
}


static media_format
tobias_audio_encoded_media_format()
{
	media_format format;
	format.type = B_MEDIA_ENCODED_AUDIO;
	init_tobias_media_raw_audio_format(&format.u.encoded_audio.output);
	return format;
}


// text

static media_format_description
tobias_text_description()
{
	media_format_description description;
	description.family = B_MISC_FORMAT_FAMILY;
	description.u.misc.file_format = OGG_FILE_FORMAT;
	description.u.misc.codec = 'text';
	return description;
}


static media_format
tobias_text_encoded_media_format()
{
	media_format format;
	format.type = B_MEDIA_HTML;
	format.user_data_type = B_CODEC_TYPE_INFO;
	strncpy((char*)format.user_data, "text", 4);
	return format;
}


#endif _OGG_TOBIAS_FORMATS_H
