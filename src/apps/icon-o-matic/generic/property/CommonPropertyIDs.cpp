/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "CommonPropertyIDs.h"

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

		default:
			name = "<unkown property>";
			break;
	}
	return name;
}

