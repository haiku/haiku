#ifndef _OGG_VORBIS_FORMATS_H
#define _OGG_VORBIS_FORMATS_H

#include <MediaFormats.h>
#include <ogg/ogg.h>
#include <string.h>

/*
 * vorbis descriptions/formats
 */


static media_format_description
vorbis_description()
{
	media_format_description description;
	description.family = B_MISC_FORMAT_FAMILY;
	description.u.misc.file_format = 'OggS';
	description.u.misc.codec = 'vorb';
	return description;
}


static void
init_vorbis_media_raw_audio_format(media_raw_audio_format * output)
{
	output->format = media_raw_audio_format::B_AUDIO_FLOAT;
	output->byte_order = B_MEDIA_HOST_ENDIAN;
}


static media_format
vorbis_encoded_media_format()
{
	media_format format;
	format.type = B_MEDIA_ENCODED_AUDIO;
	format.user_data_type = B_CODEC_TYPE_INFO;
	strncpy((char*)format.user_data, "vorb", 4);
	format.u.encoded_audio.frame_size = sizeof(ogg_packet);
	init_vorbis_media_raw_audio_format(&format.u.encoded_audio.output);
	return format;
}


#endif _OGG_VORBIS_FORMATS_H
