/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "CommonPropertyIDs.h"

#include <stdio.h>

#include <debugger.h>

#include <Catalog.h>
#include <Locale.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-PropertyNames"


// name_for_id
const char*
name_for_id(int32 id)
{
	const char* name = NULL;
	switch (id) {
		case PROPERTY_NAME:
			name = B_TRANSLATE("Name");
			break;

		case PROPERTY_OPACITY:
			name = B_TRANSLATE("Opacity");
			break;
		case PROPERTY_COLOR:
			name = B_TRANSLATE("Color");
			break;

		case PROPERTY_WIDTH:
			name = B_TRANSLATE("Width");
			break;
		case PROPERTY_HEIGHT:
			name = B_TRANSLATE("Height");
			break;

		case PROPERTY_CAP_MODE:
			name = B_TRANSLATE("Caps");
			break;
		case PROPERTY_JOIN_MODE:
			name = B_TRANSLATE("Joins");
			break;
		case PROPERTY_MITER_LIMIT:
			name = B_TRANSLATE("Miter Limit");
			break;
		case PROPERTY_STROKE_SHORTEN:
			name = B_TRANSLATE("Shorten");
			break;

		case PROPERTY_CLOSED:
			name = B_TRANSLATE("Closed");
			break;
		case PROPERTY_PATH:
			name = B_TRANSLATE("Path");
			break;

		case PROPERTY_HINTING:
			name = B_TRANSLATE("Rounding");
			break;
		case PROPERTY_MIN_VISIBILITY_SCALE:
			name = B_TRANSLATE("Min LOD");
			break;
		case PROPERTY_MAX_VISIBILITY_SCALE:
			name = B_TRANSLATE("Max LOD");
			break;

		case PROPERTY_TRANSLATION_X:
			name = B_TRANSLATE("Translation X");
			break;
		case PROPERTY_TRANSLATION_Y:
			name = B_TRANSLATE("Translation Y");
			break;
		case PROPERTY_ROTATION:
			name = B_TRANSLATE("Rotation");
			break;
		case PROPERTY_SCALE_X:
			name = B_TRANSLATE("Scale X");
			break;
		case PROPERTY_SCALE_Y:
			name = B_TRANSLATE("Scale Y");
			break;

		case PROPERTY_DETECT_ORIENTATION:
			name = B_TRANSLATE("Detect Orient.");
			break;

		default:
			name = B_TRANSLATE("<unkown property>");
			break;
	}
	return name;
}

