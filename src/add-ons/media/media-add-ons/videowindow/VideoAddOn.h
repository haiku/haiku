/*
 * VideoAddOn.h - "Video Window" media add-on.
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
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
