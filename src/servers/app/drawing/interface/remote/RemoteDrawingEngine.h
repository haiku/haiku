/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */
#ifndef REMOTE_DRAWING_ENGINE_H
#define REMOTE_DRAWING_ENGINE_H

#include "DrawingEngine.h"
#include "DrawState.h"
#include "RemoteHWInterface.h"
#include "ServerFont.h"

#include <AutoDeleter.h>

class BPoint;
class BRect;
class BRegion;

class BitmapDrawingEngine;
class ServerBitmap;

class RemoteDrawingEngine : public DrawingEngine {
public:
								RemoteDrawingEngine(
									RemoteHWInterface* interface);
	virtual						~RemoteDrawingEngine();

	// HWInterfaceListener interface
	virtual	void				FrameBufferChanged();

	virtual	void				SetCopyToFrontEnabled(bool enabled);

	// for screen shots
	virtual	status_t			ReadBitmap(ServerBitmap* bitmap,
									bool drawCursor, BRect bounds);

	// clipping for all drawing functions, passing a NULL region
	// will remove any clipping (drawing allowed everywhere)
	virtual	void				ConstrainClippingRegion(const BRegion* region);

	virtual	void				SetDrawState(const DrawState* state,
									int32 xOffset = 0, int32 yOffset = 0);

	virtual	void				SetHighColor(const rgb_color& color);
	virtual	void				SetLowColor(const rgb_color& color);
	virtual	void				SetPenSize(float size);
	virtual	void				SetStrokeMode(cap_mode lineCap,
									join_mode joinMode, float miterLimit);
	virtual	void				SetPattern(const struct pattern& pattern);
	virtual	void				SetDrawingMode(drawing_mode mode);
	virtual	void				SetDrawingMode(drawing_mode mode,
									drawing_mode& oldMode);
	virtual	void				SetBlendingMode(source_alpha srcAlpha,
									alpha_function alphaFunc);
	virtual	void				SetFont(const ServerFont& font);
	virtual	void				SetFont(const DrawState* state);
	virtual	void				SetTransform(const BAffineTransform& transform,
									int32 xOffset, int32 yOffset);

	// drawing functions
	virtual	void				InvertRect(BRect rect);

	virtual	void				DrawBitmap(ServerBitmap* bitmap,
									const BRect& bitmapRect,
									const BRect& viewRect, uint32 options = 0);

	// drawing primitives
	virtual	void				DrawArc(BRect rect, const float& angle,
									const float& span, bool filled);
	virtual	void				FillArc(BRect rect, const float& angle,
									const float& span,
									const BGradient& gradient);

	virtual	void				DrawBezier(BPoint* points, bool filled);
	virtual	void				FillBezier(BPoint* points,
									const BGradient& gradient);

	virtual	void				DrawEllipse(BRect rect, bool filled);
	virtual	void				FillEllipse(BRect rect,
									const BGradient& gradient);

	virtual	void				DrawPolygon(BPoint* pointList, int32 numPoints,
									BRect bounds, bool filled, bool closed);
	virtual	void				FillPolygon(BPoint* pointList, int32 numPoints,
									BRect bounds, const BGradient& gradient,
									bool closed);

	// these rgb_color versions are used internally by the server
	virtual	void				StrokePoint(const BPoint& point,
									const rgb_color& color);
	virtual	void				StrokeRect(BRect rect, const rgb_color &color);
	virtual	void				FillRect(BRect rect, const rgb_color &color);
	virtual	void				FillRegion(BRegion& region,
									const rgb_color& color);

	virtual	void				StrokeRect(BRect rect);
	virtual	void				FillRect(BRect rect);
	virtual	void				FillRect(BRect rect, const BGradient& gradient);

	virtual	void				FillRegion(BRegion& region);
	virtual	void				FillRegion(BRegion& region,
									const BGradient& gradient);

	virtual	void				DrawRoundRect(BRect rect, float xRadius,
									float yRadius, bool filled);
	virtual	void				FillRoundRect(BRect rect, float xRadius,
									float yRadius, const BGradient& gradient);

	virtual	void				DrawShape(const BRect& bounds,
									int32 opCount, const uint32* opList,
									int32 pointCount, const BPoint* pointList,
									bool filled,
									const BPoint& viewToScreenOffset,
									float viewScale);
	virtual	void				FillShape(const BRect& bounds,
									int32 opCount, const uint32* opList,
									int32 pointCount, const BPoint* pointList,
									const BGradient& gradient,
									const BPoint& viewToScreenOffset,
									float viewScale);

	virtual	void				DrawTriangle(BPoint* points,
									const BRect& bounds, bool filled);
	virtual	void				FillTriangle(BPoint* points,
									const BRect& bounds,
									const BGradient& gradient);

	// these versions are used by the Decorator
	virtual	void				StrokeLine(const BPoint& start,
									const BPoint& end, const rgb_color& color);
	virtual	void				StrokeLine(const BPoint& start,
									const BPoint& end);
	virtual	void				StrokeLineArray(int32 numlines,
									const ViewLineArrayInfo* data);

	// returns the pen position behind the (virtually) drawn string
	virtual	BPoint				DrawString(const char* string, int32 length,
									const BPoint& point,
									escapement_delta* delta = NULL);
	virtual	BPoint				DrawString(const char* string, int32 length,
									const BPoint* offsets);

	virtual	float				StringWidth(const char* string, int32 length,
									escapement_delta* delta = NULL);

	// software rendering backend invoked by CopyRegion() for the sorted
	// individual rects
	virtual	BRect				CopyRect(BRect rect, int32 xOffset,
									int32 yOffset) const;

private:
			status_t			_AddCallback();

	static	bool				_DrawingEngineResult(void* cookie,
									RemoteMessage& message);

			BRect				_BuildBounds(BPoint* points, int32 pointCount);
			status_t			_ExtractBitmapRegions(ServerBitmap& bitmap,
									uint32 options, const BRect& bitmapRect,
									const BRect& viewRect, double xScale,
									double yScale, BRegion& region,
									UtilityBitmap**& bitmaps);

			RemoteHWInterface*	fHWInterface;
			int32				fToken;

			DrawState			fState;
			BRegion				fClippingRegion;
			float				fExtendWidth;

			bool				fCallbackAdded;
			sem_id				fResultNotify;
			BPoint				fDrawStringResult;
			float				fStringWidthResult;
			BBitmap*			fReadBitmapResult;

			ObjectDeleter<BitmapDrawingEngine>
								fBitmapDrawingEngine;
};

#endif // REMOTE_DRAWING_ENGINE_H
