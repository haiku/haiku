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


static media_format
vorbis_encoded_media_format()
{
	media_format format;
	format.type = B_MEDIA_ENCODED_AUDIO;
	format.user_data_type = B_CODEC_TYPE_INFO;
	strncpy((char*)format.user_data, "vorb", 4);
	format.u.encoded_audio.frame_size = sizeof(ogg_packet);
	format.u.encoded_audio.output.byte_order = B_MEDIA_HOST_ENDIAN;
	format.u.encoded_audio.output.format = media_raw_audio_format::B_AUDIO_FLOAT;
	return format;
}


#endif _OGG_VORBIS_FORMATS_H
