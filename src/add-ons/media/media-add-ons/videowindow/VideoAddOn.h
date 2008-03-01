/*
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef __VIDEO_ADD_ON_H
#define __VIDEO_ADD_ON_H

#include <MediaAddOn.h>

class MediaAddOn : public BMediaAddOn
{
public:

private:
};

extern "C" BMediaAddOn *make_media_addon(image_id id);

#endif
