#ifndef _OGG_THEORA_FORMATS_H
#define _OGG_THEORA_FORMATS_H

#include <MediaFormats.h>
#include <ogg/ogg.h>
#include <string.h>
#include <OggFormats.h>

/*
 * theora descriptions/formats
 */


static media_format_description
theora_description()
{
	media_format_description description;
	description.family = B_MISC_FORMAT_FAMILY;
	description.u.misc.file_format = OGG_FILE_FORMAT;
	description.u.misc.codec = 'theo';
	return description;
}


static void
init_theora_media_raw_video_format(media_raw_video_format * output)
{
}


static media_format
theora_encoded_media_format()
{
	media_format format;
	format.type = B_MEDIA_ENCODED_AUDIO;
	format.user_data_type = B_CODEC_TYPE_INFO;
	strncpy((char*)format.user_data, "theo", 4);
	init_theora_media_raw_video_format(&format.u.encoded_video.output);
	return format;
}


#endif _OGG_THEORA_FORMATS_H
