/*
 * Copyright 2001-2008, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Gabe Yoder <gyoder@stny.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef DRAWING_ENGINE_H_
#define DRAWING_ENGINE_H_


#include <Accelerant.h>
#include <Font.h>
#include <Locker.h>
#include <Point.h>
#include <Gradient.h>

#include "HWInterface.h"

class BPoint;
class BRect;
class BRegion;

class DrawState;
class Painter;
class ServerBitmap;
class ServerCursor;
class ServerFont;

typedef struct {
	BPoint pt1;
	BPoint pt2;
	rgb_color color;
} LineArrayData;

class DrawingEngine : public HWInterfaceListener {
public:
							DrawingEngine(HWInterface* interface = NULL);
	virtual					~DrawingEngine();

	// HWInterfaceListener interface
	virtual	void			FrameBufferChanged();

			// for "changing" hardware
			void			SetHWInterface(HWInterface* interface);

			void			SetCopyToFrontEnabled(bool enable);
			bool			CopyToFrontEnabled() const
								{ return fCopyToFront; }
			void			CopyToFront(/*const*/ BRegion& region);

			// locking
			bool			LockParallelAccess();
			bool			IsParallelAccessLocked();
			void			UnlockParallelAccess();

			bool			LockExclusiveAccess();
			bool			IsExclusiveAccessLocked();
			void			UnlockExclusiveAccess();

			// for screen shots
			ServerBitmap*	DumpToBitmap();
			status_t		ReadBitmap(ServerBitmap *bitmap, bool drawCursor,
								BRect bounds);

			// clipping for all drawing functions, passing a NULL region
			// will remove any clipping (drawing allowed everywhere)
			void			ConstrainClippingRegion(const BRegion* region);

			void			SetDrawState(const DrawState* state,
								int32 xOffset = 0, int32 yOffset = 0);

			void			SetHighColor(const rgb_color& color);
			void			SetLowColor(const rgb_color& color);
			void			SetPenSize(float size);
			void			SetStrokeMode(cap_mode lineCap, join_mode joinMode,
								float miterLimit);
			void			SetPattern(const struct pattern& pattern);
			void			SetDrawingMode(drawing_mode mode);
			void			SetBlendingMode(source_alpha srcAlpha,
								alpha_function alphaFunc);
			void			SetFont(const ServerFont& font);
			void			SetFont(const DrawState* state);

			void			SuspendAutoSync();
			void			Sync();

			// drawing functions
			void			CopyRegion(/*const*/ BRegion* region,
								int32 xOffset, int32 yOffset);

			void			InvertRect(BRect r);

			void			DrawBitmap(ServerBitmap* bitmap,
								const BRect& bitmapRect, const BRect& viewRect,
								uint32 options = 0);
			// drawing primitives

			void			DrawArc(BRect r, const float& angle,
								const float& span, bool filled);
			void			FillArcGradient(BRect r, const float& angle,
								const float& span, const BGradient& gradient);

			void			DrawBezier(BPoint* pts, bool filled);
			void			FillBezierGradient(BPoint* pts,
								const BGradient& gradient);

			void			DrawEllipse(BRect r, bool filled);
			void			FillEllipseGradient(BRect r,
								const BGradient& gradient);

			void			DrawPolygon(BPoint* ptlist, int32 numpts,
								BRect bounds, bool filled, bool closed);
			void			FillPolygonGradient(BPoint* ptlist, int32 numpts,
								BRect bounds, const BGradient& gradient,
								bool closed);

			// these rgb_color versions are used internally by the server
			void			StrokePoint(const BPoint& pt,
								const rgb_color& color);
			void			StrokeRect(BRect r, const rgb_color &color);
			void			FillRect(BRect r, const rgb_color &color);
			void			FillRegion(BRegion& r, const rgb_color& color);

			void			StrokeRect(BRect r);
			void			FillRect(BRect r);
			void			FillRectGradient(BRect r, const BGradient& gradient);

			void			FillRegion(BRegion& r);
			void			FillRegionGradient(BRegion& r,
								const BGradient& gradient);

			void			DrawRoundRect(BRect r, float xrad,
								float yrad, bool filled);
			void			FillRoundRectGradient(BRect r, float xrad,
								float yrad, const BGradient& gradient);

			void			DrawShape(const BRect& bounds,
								int32 opcount, const uint32* oplist, 
								int32 ptcount, const BPoint* ptlist,
								bool filled);
			void			FillShapeGradient(const BRect& bounds,
							  int32 opcount, const uint32* oplist, 
							  int32 ptcount, const BPoint* ptlist,
							  const BGradient& gradient);
	
			void			DrawTriangle(BPoint* pts, const BRect& bounds,
								bool filled);
			void			FillTriangleGradient(BPoint* pts,
								const BRect& bounds, const BGradient& gradient);

			// this version used by Decorator
			void			StrokeLine(const BPoint& start,
								const BPoint& end, const rgb_color& color);

			void			StrokeLine(const BPoint& start,
								const BPoint& end);

			void			StrokeLineArray(int32 numlines,
								const LineArrayData* data);

			// -------- text related calls

			// returns the pen position behind the (virtually) drawn
			// string
			BPoint			DrawString(const char* string, int32 length,
								const BPoint& pt,
								escapement_delta* delta = NULL);

			float			StringWidth(const char* string, int32 length,
								escapement_delta* delta = NULL);

			// convenience function which is independent of graphics
			// state (to be used by Decorator or ServerApp etc)
			float			StringWidth(const char* string,
								int32 length, const ServerFont& font,
								escapement_delta* delta = NULL);

 private:
			BRect			_CopyRect(BRect r, int32 xOffset,
								int32 yOffset) const;

			void			_CopyRect(uint8* bits, uint32 width,
								uint32 height, uint32 bytesPerRow,
								int32 xOffset, int32 yOffset) const;

	inline	void			_CopyToFront(const BRect& frame);

			Painter*		fPainter;
			HWInterface*	fGraphicsCard;
			uint32			fAvailableHWAccleration;
			int32			fSuspendSyncLevel;
			bool			fCopyToFront;
};

#endif // DRAWING_ENGINE_H_
