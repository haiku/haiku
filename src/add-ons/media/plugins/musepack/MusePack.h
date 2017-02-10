/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef MUSEPACK_PLUGIN_H
#define MUSEPACK_PLUGIN_H


#include "ReaderPlugin.h"
#include "DecoderPlugin.h"


class MusePackPlugin : public ReaderPlugin, public DecoderPlugin {
	public:
		Reader *NewReader();

	Decoder *	NewDecoder(uint index);
	status_t	GetSupportedFormats(media_format ** formats, size_t * count);
};

#endif	/* MUSEPACK_PLUGIN_H */
