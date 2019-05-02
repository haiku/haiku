/*
 * Copyright 2009-2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 */
#ifndef BE_CONTROL_LOOK_H
#define BE_CONTROL_LOOK_H


#include <ControlLook.h>

#include "HaikuControlLook.h"


class BBitmap;
class BControl;
class BGradientLinear;
class BView;

using BPrivate::HaikuControlLook;


class BeControlLook : public HaikuControlLook {
public:
								BeControlLook(image_id id);
	virtual						~BeControlLook();


	virtual	void				DrawMenuBarBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawMenuItemBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);
};


#endif	// BE_CONTROL_LOOK_H
