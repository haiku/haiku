/*
 * Copyright 2006, 2013 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef SUPPORT_H
#define SUPPORT_H

#include <Rect.h>


class BMessage;
class BResources;
class BView;
class BWindow;


status_t load_settings(BMessage* message, const char* fileName,
	const char* folder = NULL);

status_t save_settings(const BMessage* message, const char* fileName,
	const char* folder = NULL);

status_t get_app_resources(BResources& resources);

void set_small_font(BView* view);

#endif // SUPPORT_H
