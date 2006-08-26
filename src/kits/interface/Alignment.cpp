/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <Alignment.h>

// RelativeHorizontal
float
BAlignment::RelativeHorizontal() const
{
	switch (horizontal) {
		case B_ALIGN_LEFT:
			return 0;
		case B_ALIGN_RIGHT:
			return 1;
		case B_ALIGN_HORIZONTAL_CENTER:
			return 0.5f;

		default:
			return (float)horizontal;
	}
}

// RelativeVertical
float
BAlignment::RelativeVertical() const
{
	switch (vertical) {
		case B_ALIGN_TOP:
			return 0;
		case B_ALIGN_BOTTOM:
			return 1;
		case B_ALIGN_VERTICAL_CENTER:
			return 0.5f;

		default:
			return (float)vertical;
	}
}
