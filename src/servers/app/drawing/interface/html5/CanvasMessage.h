/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		François Revol <revol@free.fr>
 */
#ifndef CANVAS_MESSAGE_H
#define CANCAS_MESSAGE_H

#include "PatternHandler.h"
#include <ViewPrivate.h>

#include "StreamingRingBuffer.h"
#include "base64.h"

#include <GraphicsDefs.h>
#include <Region.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class BBitmap;
class BFont;
class BGradient;
class BView;
class DrawState;
class Pattern;
class RemotePainter;
class ServerBitmap;
class ServerCursor;
class ServerFont;
class ViewLineArrayInfo;

enum {
	RP_INIT_CONNECTION = 1,
	RP_UPDATE_DISPLAY_MODE,
	RP_CLOSE_CONNECTION,

	RP_CREATE_STATE = 20,
	RP_DELETE_STATE,
	RP_ENABLE_SYNC_DRAWING,
	RP_DISABLE_SYNC_DRAWING,
	RP_INVALIDATE_RECT,
	RP_INVALIDATE_REGION,

	RP_SET_OFFSETS = 40,
	RP_SET_HIGH_COLOR,
	RP_SET_LOW_COLOR,
	RP_SET_PEN_SIZE,
	RP_SET_STROKE_MODE,
	RP_SET_BLENDING_MODE,
	RP_SET_PATTERN,
	RP_SET_DRAWING_MODE,
	RP_SET_FONT,

	RP_CONSTRAIN_CLIPPING_REGION = 60,
	RP_COPY_RECT_NO_CLIPPING,
	RP_INVERT_RECT,
	RP_DRAW_BITMAP,
	RP_DRAW_BITMAP_RECTS,

	RP_STROKE_ARC = 80,
	RP_STROKE_BEZIER,
	RP_STROKE_ELLIPSE,
	RP_STROKE_POLYGON,
	RP_STROKE_RECT,
	RP_STROKE_ROUND_RECT,
	RP_STROKE_SHAPE,
	RP_STROKE_TRIANGLE,
	RP_STROKE_LINE,
	RP_STROKE_LINE_ARRAY,

	RP_FILL_ARC = 100,
	RP_FILL_BEZIER,
	RP_FILL_ELLIPSE,
	RP_FILL_POLYGON,
	RP_FILL_RECT,
	RP_FILL_ROUND_RECT,
	RP_FILL_SHAPE,
	RP_FILL_TRIANGLE,
	RP_FILL_REGION,

	RP_FILL_ARC_GRADIENT = 120,
	RP_FILL_BEZIER_GRADIENT,
	RP_FILL_ELLIPSE_GRADIENT,
	RP_FILL_POLYGON_GRADIENT,
	RP_FILL_RECT_GRADIENT,
	RP_FILL_ROUND_RECT_GRADIENT,
	RP_FILL_SHAPE_GRADIENT,
	RP_FILL_TRIANGLE_GRADIENT,
	RP_FILL_REGION_GRADIENT,

	RP_STROKE_POINT_COLOR = 140,
	RP_STROKE_LINE_1PX_COLOR,
	RP_STROKE_RECT_1PX_COLOR,

	RP_FILL_RECT_COLOR = 160,
	RP_FILL_REGION_COLOR_NO_CLIPPING,

	RP_DRAW_STRING = 180,
	RP_DRAW_STRING_WITH_OFFSETS,
	RP_DRAW_STRING_RESULT,
	RP_STRING_WIDTH,
	RP_STRING_WIDTH_RESULT,
	RP_READ_BITMAP,
	RP_READ_BITMAP_RESULT,

	RP_SET_CURSOR = 200,
	RP_SET_CURSOR_VISIBLE,
	RP_MOVE_CURSOR_TO,

	RP_MOUSE_MOVED = 220,
	RP_MOUSE_DOWN,
	RP_MOUSE_UP,
	RP_MOUSE_WHEEL_CHANGED,

	RP_KEY_DOWN = 240,
	RP_KEY_UP,
	RP_UNMAPPED_KEY_DOWN,
	RP_UNMAPPED_KEY_UP,
	RP_MODIFIERS_CHANGED
};


class CanvasMessage {
public:
								CanvasMessage(StreamingRingBuffer* source,
									StreamingRingBuffer *target);
								~CanvasMessage();

		void					Start(uint16 code);
		status_t				Flush();
		void					Cancel();

		status_t				NextMessage(uint16& code);
		uint16					Code() { return fCode; }
		uint32					DataLeft() { return fDataLeft; }

		template<typename T>
		void					Add(const T& value);

		void					AddString(const char* string, size_t length);
		void					AddRegion(const BRegion& region);
		void					AddGradient(const BGradient& gradient);

		void					AddBitmap(const ServerBitmap& bitmap,
									bool minimal = false);
		void					AddFont(const ServerFont& font);
		void					AddPattern(const Pattern& pattern);
		void					AddDrawState(const DrawState& drawState);
		void					AddArrayLine(const ViewLineArrayInfo& line);
		void					AddCursor(const ServerCursor& cursor);

		template<typename T>
		void					AddList(const T* array, int32 count);

		template<typename T>
		status_t				Read(T& value);

		status_t				ReadRegion(BRegion& region);
		status_t				ReadFontState(BFont& font);
									// sets font state
		status_t				ReadViewState(BView& view, ::pattern& pattern);
									// sets viewstate and returns pattern

		status_t				ReadString(char** _string, size_t& length);
		status_t				ReadBitmap(BBitmap** _bitmap,
									bool minimal = false,
									color_space colorSpace = B_RGB32,
									uint32 flags = 0);
		status_t				ReadGradient(BGradient** _gradient);
		status_t				ReadArrayLine(BPoint& startPoint,
									BPoint& endPoint, rgb_color& color);

		template<typename T>
		status_t				ReadList(T* array, int32 count);

private:
		bool					_MakeSpace(size_t size);

		StreamingRingBuffer*	fSource;
		StreamingRingBuffer*	fTarget;

		uint8*					fBuffer;
		size_t					fAvailable;
		size_t					fWriteIndex;
		uint32					fDataLeft;
		uint16					fCode;
};


inline
CanvasMessage::CanvasMessage(StreamingRingBuffer* source,
	StreamingRingBuffer* target)
	:
	fSource(source),
	fTarget(target),
	fBuffer(NULL),
	fAvailable(0),
	fWriteIndex(0),
	fDataLeft(0)
{
}


inline
CanvasMessage::~CanvasMessage()
{
	if (fWriteIndex > 0)
		Flush();
	free(fBuffer);
}


inline void
CanvasMessage::Start(uint16 code)
{
	if (fWriteIndex > 0)
		Flush();

	Add(code);

//	uint32 sizeDummy;
//	Add(sizeDummy);
}


inline status_t
CanvasMessage::Flush()
{
	if (fWriteIndex == 0)
		return B_NO_INIT;

	// end by the multipart boundary
	static const char boundary[] = "--x\r\n\r\n";
	AddString(boundary, sizeof(boundary) - 1);

	uint32 length = fWriteIndex;
	fAvailable += fWriteIndex;
	fWriteIndex = 0;

//	memcpy(fBuffer + sizeof(uint16) * 2, &length, sizeof(uint32));
	return fTarget->Write(fBuffer, length);
}


template<typename T>
inline void
CanvasMessage::Add(const T& value)
{
	ssize_t done;

	if (!_MakeSpace(sizeof(T) * 2))	// 4/3 actually
		return;

	//memcpy(fBuffer + fWriteIndex, &value, sizeof(T));
	done = encode_base64((char *)fBuffer + fWriteIndex, (const char *)(&value),
		sizeof(T));
	fWriteIndex += done;
	fAvailable -= done;
}


inline void
CanvasMessage::AddString(const char* string, size_t length)
{
	Add(length);
	if (length > fAvailable && !_MakeSpace(length))
		return;

	memcpy(fBuffer + fWriteIndex, string, length);
	fWriteIndex += length;
	fAvailable -= length;
}


inline void
CanvasMessage::AddRegion(const BRegion& region)
{
	int32 rectCount = region.CountRects();
	Add(rectCount);

	for (int32 i = 0; i < rectCount; i++)
		Add(region.RectAt(i));
}


template<typename T>
inline void
CanvasMessage::AddList(const T* array, int32 count)
{
	for (int32 i = 0; i < count; i++)
		Add(array[i]);
}


template<typename T>
inline status_t
CanvasMessage::Read(T& value)
{
	//TODO
	if (fDataLeft < sizeof(T))
		return B_ERROR;

	int32 readSize = fSource->Read(&value, sizeof(T));
	if (readSize < 0)
		return readSize;

	if (readSize != sizeof(T))
		return B_ERROR;

	fDataLeft -= sizeof(T);
	return B_OK;
}


inline status_t
CanvasMessage::ReadRegion(BRegion& region)
{
	region.MakeEmpty();

	int32 rectCount;
	Read(rectCount);

	for (int32 i = 0; i < rectCount; i++) {
		BRect rect;
		status_t result = Read(rect);
		if (result != B_OK)
			return result;

		region.Include(rect);
	}

	return B_OK;
}


template<typename T>
inline status_t
CanvasMessage::ReadList(T* array, int32 count)
{
	for (int32 i = 0; i < count; i++) {
		status_t result = Read(array[i]);
		if (result != B_OK)
			return result;
	}

	return B_OK;
}


inline bool
CanvasMessage::_MakeSpace(size_t size)
{
	if (fAvailable >= size)
		return true;

	size_t extraSize = size + 20;
	uint8 *newBuffer = (uint8*)realloc(fBuffer, fWriteIndex + extraSize);
	if (newBuffer == NULL)
		return false;

	fAvailable = extraSize;
	fBuffer = newBuffer;
	return true;
}

#endif // CANVAS_MESSAGE_H
