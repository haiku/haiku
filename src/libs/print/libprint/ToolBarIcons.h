/*
 * Copyright 2011-2026, The Haiku Project. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *      Philippe Houdoin
 */

#ifndef TOOL_BAR_ICONS_H
#define TOOL_BAR_ICONS_H


#include <SupportDefs.h>


class BBitmap;

enum {
	kIconGoFirst,
	kIconGoFastBackward,
	kIconGoPrevious,
	kIconGoNext,
	kIconGoFastForward,
	kIconGoLast,
	kIconZoomIn,
	kIconZoomOut
};


status_t init_tool_bar_icons();
void uninit_tool_bar_icons();
const BBitmap* tool_bar_icon(uint32 which);


#endif // TOOL_BAR_ICONS_H
