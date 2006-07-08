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

// name_for_id
const char*
name_for_id(int32 id)
{
	const char* name = NULL;
	switch (id) {
		case PROPERTY_NAME:
			name = "Name";
			break;

		case PROPERTY_OPACITY:
			name = "Opacity";
			break;
		case PROPERTY_COLOR:
			name = "Color";
			break;

		case PROPERTY_WIDTH:
			name = "Width";
			break;
		case PROPERTY_HEIGHT:
			name = "Height";
			break;

		case PROPERTY_CAP_MODE:
			name = "Caps";
			break;
		case PROPERTY_JOIN_MODE:
			name = "Joins";
			break;
		case PROPERTY_MITER_LIMIT:
			name = "Miter Limit";
			break;

		case PROPERTY_CLOSED:
			name = "Closed";
			break;

		case PROPERTY_TRANSLATION_X:
			name = "Translation X";
			break;
		case PROPERTY_TRANSLATION_Y:
			name = "Translation Y";
			break;
		case PROPERTY_ROTATION:
			name = "Rotation";
			break;
		case PROPERTY_SCALE_X:
			name = "Scale X";
			break;
		case PROPERTY_SCALE_Y:
			name = "Scale Y";
			break;

		case PROPERTY_DETECT_ORIENTATION:
			name = "Detect Orient.";
			break;

		default:
			name = "<unkown property>";
			break;
	}
	return name;
}

