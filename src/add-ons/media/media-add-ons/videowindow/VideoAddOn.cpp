/*
 * Copyright (C) 2006-2008 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 * Copyright (C) 2008 Maurice Kalinowski <haiku@kaldience.com>. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#include "VideoAddOn.h"
#include "VideoNode.h"
#include "VideoView.h"
#include "debug.h"


#include <stdio.h>
#include <string.h>


VideoWindowAddOn::VideoWindowAddOn(image_id id)
		: BMediaAddOn(id)
{
	CALLED();

	fInputFormat.type = B_MEDIA_RAW_VIDEO;
	fInputFormat.u.raw_video = media_raw_video_format::wildcard;

	fInfo.internal_id = 0;
	fInfo.name = strdup("VideoWindow Consumer");
	fInfo.info = strdup("This node displays a simple video window");
	fInfo.kinds = B_BUFFER_CONSUMER;
	fInfo.flavor_flags = 0;
	fInfo.possible_count = 0;
	fInfo.in_format_count = 1;
	fInfo.in_format_flags = 0;
	fInfo.in_formats = &fInputFormat;
	fInfo.out_format_count = 0;
	fInfo.out_formats = 0;
	fInfo.out_format_flags = 0;
}


VideoWindowAddOn::~VideoWindowAddOn()
{
}


	
bool
VideoWindowAddOn::WantsAutoStart()
{
	CALLED();
	return false;
}


int32
VideoWindowAddOn::CountFlavors()
{
	CALLED();
	return 1;
}


status_t
VideoWindowAddOn::GetFlavorAt(int32 cookie, const flavor_info **flavorInfo)
{
	CALLED();
	if (cookie != 0)
		return B_BAD_INDEX;
	if (!flavorInfo || !*flavorInfo)
		return B_ERROR;

	*flavorInfo = &fInfo;
	return B_OK;
}


BMediaNode*
VideoWindowAddOn::InstantiateNodeFor(const flavor_info *info, BMessage*, status_t *outError)
{
	CALLED();
	if (!outError)
		return NULL;

	if (info->in_formats[0].type != B_MEDIA_RAW_VIDEO) {
		*outError = B_MEDIA_BAD_FORMAT;
		return NULL;
	}

	BRect size;
	if (info->in_formats[0].u.raw_video.display.line_width != 0)
		size.right = info->in_formats[0].u.raw_video.display.line_width;
	else
		size.right = 320;
	if (info->in_formats[0].u.raw_video.display.line_count != 0)
		size.bottom = info->in_formats[0].u.raw_video.display.line_count;
	else
		size.bottom = 240;

	VideoNode* node = new VideoNode("Video Node", this, info->internal_id);

	return node;
}


extern "C" BMediaAddOn *make_media_addon(image_id id)
{
	return new VideoWindowAddOn(id);
}
