#ifndef _OGG_TOBIAS_FORMATS_H
#define _OGG_TOBIAS_FORMATS_H

#include <MediaFormats.h>
#include <ogg/ogg.h>

/*
 * tobias descriptions/formats
 */


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


#endif _OGG_TOBIAS_FORMATS_H
