/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "MusePack.h"
#include "MusePackReader.h"
#include "MusePackDecoder.h"


Reader *
MusePackPlugin::NewReader()
{
	return new MusePackReader();
}

Decoder *
MusePackPlugin::NewDecoder()
{
	return new MusePackDecoder();
}

status_t 
MusePackPlugin::RegisterDecoder()
{
	media_format_description description;
	description.family = B_MISC_FORMAT_FAMILY;
	description.u.misc.file_format = 'mpc ';
	description.u.misc.codec = 'MPC7';
		// 7 is the most recent stream version

	media_format format;
	format.type = B_MEDIA_ENCODED_AUDIO;
	format.u.encoded_audio = media_encoded_audio_format::wildcard;

	return BMediaFormats().MakeFormatFor(&description, 1, &format);
}


//	#pragma mark -


MediaPlugin *
instantiate_plugin()
{
	return new MusePackPlugin();
}

