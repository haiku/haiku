/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef SUPPORT_H
#define SUPPORT_H

#include <GraphicsDefs.h>
#include <Rect.h>

class BMessage;
class BView;
class BWindow;

status_t load_settings(BMessage* message, const char* fileName,
					   const char* folder = NULL);

status_t save_settings(BMessage* message, const char* fileName,
					   const char* folder = NULL);

// looper of view must be locked!
void stroke_frame(BView* view, BRect frame,
				  rgb_color left, rgb_color top,
				  rgb_color right, rgb_color bottom);

bool make_sure_frame_is_on_screen(BRect& frame, BWindow* window = NULL);

#endif // SUPPORT_H
