/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef FLAT_ICON_FORMAT_H
#define FLAT_ICON_FORMAT_H


#include <SupportDefs.h>

#include "IconBuild.h"


_BEGIN_ICON_NAMESPACE


extern const uint32 FLAT_ICON_MAGIC;

extern const char* kVectorAttrNodeName;
extern const char* kVectorAttrMimeName;

enum {
	STYLE_TYPE_SOLID_COLOR			= 1,
	STYLE_TYPE_GRADIENT				= 2,
	STYLE_TYPE_SOLID_COLOR_NO_ALPHA	= 3,
	STYLE_TYPE_SOLID_GRAY			= 4,
	STYLE_TYPE_SOLID_GRAY_NO_ALPHA	= 5,

	SHAPE_TYPE_PATH_SOURCE			= 10,

	TRANSFORMER_TYPE_AFFINE			= 20,
	TRANSFORMER_TYPE_CONTOUR		= 21,
	TRANSFORMER_TYPE_PERSPECTIVE	= 22,
	TRANSFORMER_TYPE_STROKE			= 23,
};

enum {
	GRADIENT_FLAG_TRANSFORM			= 1 << 1,
	GRADIENT_FLAG_NO_ALPHA			= 1 << 2,
	GRADIENT_FLAG_16_BIT_COLORS		= 1 << 3, // not yet used
	GRADIENT_FLAG_GRAYS				= 1 << 4,
};

enum {
	PATH_FLAG_CLOSED				= 1 << 1,
	PATH_FLAG_USES_COMMANDS			= 1 << 2,
	PATH_FLAG_NO_CURVES				= 1 << 3,
};

enum {
	PATH_COMMAND_H_LINE				= 0,
	PATH_COMMAND_V_LINE				= 1,
	PATH_COMMAND_LINE				= 2,
	PATH_COMMAND_CURVE				= 3,
};

enum {
	SHAPE_FLAG_TRANSFORM			= 1 << 1,
	SHAPE_FLAG_HINTING				= 1 << 2,
	SHAPE_FLAG_LOD_SCALE			= 1 << 3,
	SHAPE_FLAG_HAS_TRANSFORMERS		= 1 << 4,
	SHAPE_FLAG_TRANSLATION			= 1 << 5,
};

// utility functions

class LittleEndianBuffer;

bool read_coord(LittleEndianBuffer& buffer, float& coord);
bool write_coord(LittleEndianBuffer& buffer, float coord);

bool read_float_24(LittleEndianBuffer& buffer, float& value);
bool write_float_24(LittleEndianBuffer& buffer, float value);


_END_ICON_NAMESPACE


#endif	// FLAT_ICON_FORMAT_H
