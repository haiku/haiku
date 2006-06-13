/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef SUPPORT_SETTINGS_H
#define SUPPORT_SETTINGS_H

#include <GraphicsDefs.h>

class BMessage;

status_t load_settings(BMessage* message, const char* fileName,
					   const char* folder = NULL);

status_t save_settings(BMessage* message, const char* fileName,
					   const char* folder = NULL);


# endif // SUPPORT_SETTINGS_H
