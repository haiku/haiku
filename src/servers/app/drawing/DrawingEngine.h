/*
 * Copyright 2001-2018, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Gabe Yoder <gyoder@stny.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Julian Harnath <julian.harnath@rwth-aachen.de>
 */
#ifndef DRAWING_ENGINE_H_
#define DRAWING_ENGINE_H_


#include <AutoDeleter.h>
#include <Accelerant.h>
#include <Font.h>
#include <Locker.h>
#include <Point.h>
#include <Gradient.h>
#include <ServerProtocolStructs.h>

#include "HWInterface.h"


class BPoint;
class BRect;
class BRegion;

class DrawState;
class Painter;
class ServerBitmap;
class ServerCursor;
class ServerFont;


class DrawingEngine : public HWInterfaceListener {
public:
							DrawingEngine(HWInterface* interface = NULL);
	virtual					~DrawingEngine();

	// HWInterfaceListener interface
	virtual	void			FrameBufferChanged();

	// for "changing" hardware
			void			SetHWInterface(HWInterface* interface);

	virtual	void			SetCopyToFrontEnabled(bool enable);
			bool			CopyToFrontEnabled() const
								{ return fCopyToFront; }
	virtual	void			CopyToFront(/*const*/ BRegion& region);

	// locking
			bool			LockParallelAccess();
#if DEBUG
	virtual	bool			IsParallelAccessLocked() const;
#endif
			void			UnlockParallelAccess();

			bool			LockExclusiveAccess();
	virtual	bool			IsExclusiveAccessLocked() const;
			void			UnlockExclusiveAccess();

	// for screen shots
			ServerBitmap*	DumpToBitmap();
	virtual	status_t		ReadBitmap(ServerBitmap *bitmap, bool drawCursor,
								BRect bounds);

	// clipping for all drawing functions, passing a NULL region
	// will remove any clipping (drawing allowed everywhere)
	virtual	void			ConstrainClippingRegion(const BRegion* region);

	virtual	void			SetDrawState(const DrawState* state,
								int32 xOffset = 0, int32 yOffset = 0);

	virtual	void			SetHighColor(const rgb_color& color);
	virtual	void			SetLowColor(const rgb_color& color);
	virtual	void			SetPenSize(float size);
	virtual	void			SetStrokeMode(cap_mode lineCap, join_mode joinMode,
								float miterLimit);
	virtual void			SetFillRule(int32 fillRule);
	virtual	void			SetPattern(const struct pattern& pattern);
	virtual	void			SetDrawingMode(drawing_mode mode);
	virtual	void			SetDrawingMode(drawing_mode mode,
								drawing_mode& oldMode);
	virtual	void			SetBlendingMode(source_alpha srcAlpha,
								alpha_function alphaFunc);
	virtual	void			SetFont(const ServerFont& font);
	virtual	void			SetFont(const DrawState* state);
	virtual	void			SetTransform(const BAffineTransform& transform,
								int32 xOffset, int32 yOffset);

			void			SuspendAutoSync();
			void			Sync();

	// drawing functions
	virtual	void			CopyRegion(/*const*/ BRegion* region,
								int32 xOffset, int32 yOffset);

	virtual	void			InvertRect(BRect r);

	virtual	void			DrawBitmap(ServerBitmap* bitmap,
								const BRect& bitmapRect, const BRect& viewRect,
								uint32 options = 0);
	// drawing primitives
	virtual	void			DrawArc(BRect r, const float& angle,
								const float& span, bool filled);
	virtual	void			FillArc(BRect r, const float& angle,
								const float& span, const BGradient& gradient);

	virtual	void			DrawBezier(BPoint* pts, bool filled);
	virtual	void			FillBezier(BPoint* pts, const BGradient& gradient);

	virtual	void			DrawEllipse(BRect r, bool filled);
	virtual	void			FillEllipse(BRect r, const BGradient& gradient);

	virtual	void			DrawPolygon(BPoint* ptlist, int32 numpts,
								BRect bounds, bool filled, bool closed);
	virtual	void			FillPolygon(BPoint* ptlist, int32 numpts,
								BRect bounds, const BGradient& gradient,
								bool closed);

	// these rgb_color versions are used internally by the server
	virtual	void			StrokePoint(const BPoint& point,
								const rgb_color& color);
	virtual	void			StrokeRect(BRect rect, const rgb_color &color);
	virtual	void			FillRect(BRect rect, const rgb_color &color);
	virtual	void			FillRegion(BRegion& region, const rgb_color& color);

	virtual	void			StrokeRect(BRect rect);
	virtual	void			FillRect(BRect rect);
	virtual	void			FillRect(BRect rect, const BGradient& gradient);

	virtual	void			FillRegion(BRegion& region);
	virtual	void			FillRegion(BRegion& region,
								const BGradient& gradient);

	virtual	void			DrawRoundRect(BRect rect, float xrad,
								float yrad, bool filled);
	virtual	void			FillRoundRect(BRect rect, float xrad,
								float yrad, const BGradient& gradient);

	virtual	void			DrawShape(const BRect& bounds,
								int32 opcount, const uint32* oplist,
								int32 ptcount, const BPoint* ptlist,
								bool filled, const BPoint& viewToScreenOffset,
								float viewScale);
	virtual	void			FillShape(const BRect& bounds,
								int32 opcount, const uint32* oplist,
							 	int32 ptcount, const BPoint* ptlist,
							 	const BGradient& gradient,
							 	const BPoint& viewToScreenOffset,
								float viewScale);

	virtual	void			DrawTriangle(BPoint* points, const BRect& bounds,
								bool filled);
	virtual	void			FillTriangle(BPoint* points, const BRect& bounds,
								const BGradient& gradient);

	// these versions are used by the Decorator
	virtual	void			StrokeLine(const BPoint& start,
								const BPoint& end, const rgb_color& color);

	virtual	void			StrokeLine(const BPoint& start,
								const BPoint& end);

	virtual	void			StrokeLineArray(int32 numlines,
								const ViewLineArrayInfo* data);

	// -------- text related calls

	// returns the pen position behind the (virtually) drawn
	// string
	virtual	BPoint			DrawString(const char* string, int32 length,
								const BPoint& pt,
								escapement_delta* delta = NULL);
	virtual	BPoint			DrawString(const char* string, int32 length,
								const BPoint* offsets);

			float			StringWidth(const char* string, int32 length,
								escapement_delta* delta = NULL);

	// convenience function which is independent of graphics
	// state (to be used by Decorator or ServerApp etc)
			float			StringWidth(const char* string,
								int32 length, const ServerFont& font,
								escapement_delta* delta = NULL);

			BPoint			DrawStringDry(const char* string, int32 length,
								const BPoint& pt,
								escapement_delta* delta = NULL);
			BPoint			DrawStringDry(const char* string, int32 length,
								const BPoint* offsets);


	// software rendering backend invoked by CopyRegion() for the sorted
	// individual rects
	virtual	BRect			CopyRect(BRect rect, int32 xOffset,
								int32 yOffset) const;

			void			SetRendererOffset(int32 offsetX, int32 offsetY);

private:
	friend class DrawTransaction;

			void			_CopyRect(uint8* bits, uint32 width,
								uint32 height, uint32 bytesPerRow,
								int32 xOffset, int32 yOffset) const;

			ObjectDeleter<Painter>
							fPainter;
			HWInterface*	fGraphicsCard;
			uint32			fAvailableHWAccleration;
			int32			fSuspendSyncLevel;
			bool			fCopyToFront;
};

#endif // DRAWING_ENGINE_H_
