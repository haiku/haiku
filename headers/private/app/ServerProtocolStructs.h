/*
 * Copyright 2009, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, <superstippi@gmx.de>
 */
#ifndef APP_SERVER_PROTOCOL_STRUCTS_H
#define APP_SERVER_PROTOCOL_STRUCTS_H


#include <Rect.h>


struct ViewSetStateInfo {
	BPoint						penLocation;
	float						penSize;
	rgb_color					highColor;
	rgb_color					lowColor;
	::pattern					pattern;
	drawing_mode				drawingMode;
	BPoint						origin;
	float						scale;
	join_mode					lineJoin;
	cap_mode					lineCap;
	float						miterLimit;
	source_alpha				alphaSourceMode;
	alpha_function				alphaFunctionMode;
	bool						fontAntialiasing;
};


struct ViewGetStateInfo {
	int32						fontID;
	float						fontSize;
	float						fontShear;
	float						fontRotation;
	float						fontFalseBoldWidth;
	int8						fontSpacing;
	int8						fontEncoding;
	int16						fontFace;
	int32						fontFlags;

	ViewSetStateInfo			viewStateInfo;
};


struct ViewDragImageInfo {
	int32						bitmapToken;
	int32						dragMode;
	BPoint						offset;
	int32						bufferSize;
};


struct ViewSetViewCursorInfo {
	int32						cursorToken;
	int32						viewToken;
	bool						sync;
};


struct ViewBeginRectTrackingInfo {
	BRect						rect;
	uint32						style;
};


struct ViewSetLineModeInfo {
	join_mode					lineJoin;
	cap_mode					lineCap;
	float						miterLimit;
};


struct ViewBlendingModeInfo {
	source_alpha				sourceAlpha;
	alpha_function				alphaFunction;
};


struct ViewDrawBitmapInfo {
	int32						bitmapToken;
	uint32						options;
	BRect						viewRect;
	BRect						bitmapRect;
};


struct ViewDrawStringInfo {
	int32						stringLength;
	BPoint						location;
	escapement_delta			delta;
};


struct ViewStrokeLineInfo {
	BPoint						startPoint;
	BPoint						endPoint;
};


struct ViewLineArrayInfo {
	BPoint						startPoint;
	BPoint						endPoint;
	rgb_color					color;
};


#endif	// APP_SERVER_PROTOCOL_STRUCTS_H
