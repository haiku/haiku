/*
 * Copyright (C) 2006-2008 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 * Copyright (C) 2008 Maurice Kalinowski <haiku@kaldience.com>. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef __VIDEO_ADD_ON_H
#define __VIDEO_ADD_ON_H


#include <MediaAddOn.h>


class VideoWindowAddOn : public BMediaAddOn
{
public:
								VideoWindowAddOn(image_id);
								~VideoWindowAddOn();

	bool						WantsAutoStart();
	int32						CountFlavors();
	status_t					GetFlavorAt(int32, const flavor_info**);
	BMediaNode*					InstantiateNodeFor(const flavor_info*, BMessage*, status_t*);

private:
	flavor_info					fInfo;
	media_format				fInputFormat;
};

extern "C" BMediaAddOn *make_media_addon(image_id id);

#endif
