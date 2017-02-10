/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
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
MusePackPlugin::NewDecoder(uint index)
{
	return new MusePackDecoder();
}

static media_format muse_pack_formats[1];

status_t 
MusePackPlugin::GetSupportedFormats(media_format ** formats, size_t * count)
{
	media_format_description description;
	description.family = B_MISC_FORMAT_FAMILY;
	description.u.misc.file_format = 'mpc ';
	description.u.misc.codec = 'MPC7';
		// 7 is the most recent stream version

	media_format format;
	format.type = B_MEDIA_ENCODED_AUDIO;
	format.u.encoded_audio = media_encoded_audio_format::wildcard;

	BMediaFormats mediaFormats;
	status_t result = mediaFormats.InitCheck();
	if (result != B_OK) {
		return result;
	}
	result = mediaFormats.MakeFormatFor(&description, 1, &format);
	if (result != B_OK) {
		return result;
	}
	muse_pack_formats[0] = format;

	*formats = muse_pack_formats;
	*count = 1;
	
	return B_OK;
}


//	#pragma mark -


MediaPlugin *
instantiate_plugin()
{
	return new MusePackPlugin();
}

