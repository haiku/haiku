/*
 * Copyright 2007-2008, Haiku. Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "ColorSpaceToString.h"


const char*
color_space_to_string(color_space format)
{
	const char* name = "<unkown format>";
	switch (format) {
		case B_RGB32:
			name = "B_RGB32";
			break;
		case B_RGBA32:
			name = "B_RGBA32";
			break;
		case B_RGB32_BIG:
			name = "B_RGB32_BIG";
			break;
		case B_RGBA32_BIG:
			name = "B_RGBA32_BIG";
			break;
		case B_RGB24:
			name = "B_RGB24";
			break;
		case B_RGB24_BIG:
			name = "B_RGB24_BIG";
			break;
		case B_CMAP8:
			name = "B_CMAP8";
			break;
		case B_GRAY8:
			name = "B_GRAY8";
			break;
		case B_GRAY1:
			name = "B_GRAY1";
			break;

		// YCbCr
		case B_YCbCr422:
			name = "B_YCbCr422";
			break;
		case B_YCbCr411:
			name = "B_YCbCr411";
			break;
		case B_YCbCr444:
			name = "B_YCbCr444";
			break;
		case B_YCbCr420:
			name = "B_YCbCr420";
			break;

		// YUV
		case B_YUV422:
			name = "B_YUV422";
			break;
		case B_YUV411:
			name = "B_YUV411";
			break;
		case B_YUV444:
			name = "B_YUV444";
			break;
		case B_YUV420:
			name = "B_YUV420";
			break;

		case B_YUV9:
			name = "B_YUV9";
			break;
		case B_YUV12:
			name = "B_YUV12";
			break;

		default:
			break;
	}
	return name;
}
