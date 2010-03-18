/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include "RemoteDrawingEngine.h"
#include "RemoteMessage.h"

#include "DrawState.h"

#include <Bitmap.h>
#include <utf8_functions.h>

#include <new>


RemoteDrawingEngine::RemoteDrawingEngine(RemoteHWInterface* interface)
	:
	DrawingEngine(interface),
	fHWInterface(interface),
	fToken((uint32)this), // TODO: need to redo that for 64 bit
	fExtendWidth(0),
	fCallbackAdded(false),
	fResultNotify(-1)
{
	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_CREATE_STATE);
	message.Add(fToken);
}


RemoteDrawingEngine::~RemoteDrawingEngine()
{
	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_DELETE_STATE);
	message.Add(fToken);
	message.Flush();

	if (fCallbackAdded)
		fHWInterface->RemoveCallback(fToken);
	if (fResultNotify >= 0)
		delete_sem(fResultNotify);
}


// #pragma mark -


void
RemoteDrawingEngine::FrameBufferChanged()
{
	// Not allowed
}


// #pragma mark -


void
RemoteDrawingEngine::SetCopyToFrontEnabled(bool enabled)
{
	DrawingEngine::SetCopyToFrontEnabled(enabled);

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(enabled ? RP_ENABLE_SYNC_DRAWING : RP_DISABLE_SYNC_DRAWING);
	message.Add(fToken);
}


// #pragma mark -


//! the RemoteDrawingEngine needs to be locked!
void
RemoteDrawingEngine::ConstrainClippingRegion(const BRegion* region)
{
	if (fClippingRegion == *region)
		return;

	fClippingRegion = *region;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_CONSTRAIN_CLIPPING_REGION);
	message.Add(fToken);
	message.AddRegion(*region);
}


void
RemoteDrawingEngine::SetDrawState(const DrawState* state, int32 xOffset,
	int32 yOffset)
{
	SetPenSize(state->PenSize());
	SetDrawingMode(state->GetDrawingMode());
	SetBlendingMode(state->AlphaSrcMode(), state->AlphaFncMode());
	SetPattern(state->GetPattern().GetPattern());
	SetStrokeMode(state->LineCapMode(), state->LineJoinMode(),
		state->MiterLimit());
	SetHighColor(state->HighColor());
	SetLowColor(state->LowColor());
	SetFont(state->Font());

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_SET_OFFSETS);
	message.Add(fToken);
	message.Add(xOffset);
	message.Add(yOffset);
}


void
RemoteDrawingEngine::SetHighColor(const rgb_color& color)
{
	if (fState.HighColor() == color)
		return;

	fState.SetHighColor(color);

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_SET_HIGH_COLOR);
	message.Add(fToken);
	message.Add(color);
}


void
RemoteDrawingEngine::SetLowColor(const rgb_color& color)
{
	if (fState.LowColor() == color)
		return;

	fState.SetLowColor(color);

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_SET_LOW_COLOR);
	message.Add(fToken);
	message.Add(color);
}


void
RemoteDrawingEngine::SetPenSize(float size)
{
	if (fState.PenSize() == size)
		return;

	fState.SetPenSize(size);
	fExtendWidth = -(size / 2);

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_SET_PEN_SIZE);
	message.Add(fToken);
	message.Add(size);
}


void
RemoteDrawingEngine::SetStrokeMode(cap_mode lineCap, join_mode joinMode,
	float miterLimit)
{
	if (fState.LineCapMode() == lineCap && fState.LineJoinMode() == joinMode
		&& fState.MiterLimit() == miterLimit)
		return;

	fState.SetLineCapMode(lineCap);
	fState.SetLineJoinMode(joinMode);
	fState.SetMiterLimit(miterLimit);

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_SET_STROKE_MODE);
	message.Add(fToken);
	message.Add(lineCap);
	message.Add(joinMode);
	message.Add(miterLimit);
}


void
RemoteDrawingEngine::SetBlendingMode(source_alpha sourceAlpha,
	alpha_function alphaFunc)
{
	if (fState.AlphaSrcMode() == sourceAlpha
		&& fState.AlphaFncMode() == alphaFunc)
		return;

	fState.SetBlendingMode(sourceAlpha, alphaFunc);

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_SET_BLENDING_MODE);
	message.Add(fToken);
	message.Add(sourceAlpha);
	message.Add(alphaFunc);
}


void
RemoteDrawingEngine::SetPattern(const struct pattern& pattern)
{
	if (fState.GetPattern() == pattern)
		return;

	fState.SetPattern(pattern);

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_SET_PATTERN);
	message.Add(fToken);
	message.Add(pattern);
}


void
RemoteDrawingEngine::SetDrawingMode(drawing_mode mode)
{
	if (fState.GetDrawingMode() == mode)
		return;

	fState.SetDrawingMode(mode);

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_SET_DRAWING_MODE);
	message.Add(fToken);
	message.Add(mode);
}


void
RemoteDrawingEngine::SetDrawingMode(drawing_mode mode, drawing_mode& oldMode)
{
	oldMode = fState.GetDrawingMode();
	SetDrawingMode(mode);
}


void
RemoteDrawingEngine::SetFont(const ServerFont& font)
{
	if (fState.Font() == font)
		return;

	fState.SetFont(font);

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_SET_FONT);
	message.Add(fToken);
	message.AddFont(font);
}


void
RemoteDrawingEngine::SetFont(const DrawState* state)
{
	SetFont(state->Font());
}


// #pragma mark -


BRect
RemoteDrawingEngine::CopyRect(BRect rect, int32 xOffset, int32 yOffset) const
{
	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_COPY_RECT_NO_CLIPPING);
	message.Add(xOffset);
	message.Add(yOffset);
	message.Add(rect);
	return rect.OffsetBySelf(xOffset, yOffset);
}


void
RemoteDrawingEngine::InvertRect(BRect rect)
{
	if (!fClippingRegion.Intersects(rect))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_INVERT_RECT);
	message.Add(fToken);
	message.Add(rect);
}


void
RemoteDrawingEngine::DrawBitmap(ServerBitmap* bitmap, const BRect& _bitmapRect,
	const BRect& _viewRect, uint32 options)
{
	if (!fClippingRegion.Intersects(_viewRect))
		return;

	BRect viewRect = _viewRect;
	BRect bitmapRect = _bitmapRect;
	if (bitmapRect.IntegerWidth() == viewRect.IntegerWidth()
		&& bitmapRect.IntegerHeight() == viewRect.IntegerHeight()) {
		// unscaled bitmap we can chop off stuff we don't need
		BRegion target(viewRect);
		target.IntersectWith(&fClippingRegion);
		BRect frame = target.Frame();

		if (frame != viewRect) {
			BPoint offset = frame.LeftTop() - viewRect.LeftTop();
			viewRect = frame;
			bitmapRect = viewRect.OffsetToCopy(bitmapRect.LeftTop() + offset);
		}
	}

	UtilityBitmap* other = NULL;
	BRect bounds = bitmap->Bounds();
	BRect newBounds;
	newBounds.right
		= min_c(bounds.IntegerWidth(), bitmapRect.IntegerWidth());
	newBounds.bottom	
		= min_c(bounds.IntegerHeight(), bitmapRect.IntegerHeight());

	if (newBounds.IntegerWidth() < bounds.IntegerWidth()
		|| newBounds.IntegerHeight() < bounds.IntegerHeight()) {

		other = new(std::nothrow) UtilityBitmap(newBounds, bitmap->ColorSpace(),
			bitmap->Flags());

		if (other != NULL && other->ImportBits(bitmap->Bits(),
				bitmap->BitsLength(), bitmap->BytesPerRow(),
				bitmap->ColorSpace(), bitmapRect.LeftTop(), BPoint(0, 0),
				newBounds.IntegerWidth() + 1,
				newBounds.IntegerHeight() + 1) == B_OK) {
			bitmapRect.OffsetTo(0, 0);
			bitmap = other;
		}
	}

	// TODO: we may want to cache/checksum bitmaps
	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_DRAW_BITMAP);
	message.Add(fToken);
	message.Add(bitmapRect);
	message.Add(viewRect);
	message.Add(options);
	message.AddBitmap(*bitmap);

	if (other != NULL)
		delete other;
}


void
RemoteDrawingEngine::DrawArc(BRect rect, const float& angle, const float& span,
	bool filled)
{
	BRect bounds = rect;
	if (!filled)
		bounds.InsetBy(fExtendWidth, fExtendWidth);

	if (!fClippingRegion.Intersects(bounds))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(filled ? RP_FILL_ARC : RP_STROKE_ARC);
	message.Add(fToken);
	message.Add(rect);
	message.Add(angle);
	message.Add(span);
}

void
RemoteDrawingEngine::FillArc(BRect rect, const float& angle, const float& span,
	const BGradient& gradient)
{
	if (!fClippingRegion.Intersects(rect))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_FILL_ARC_GRADIENT);
	message.Add(fToken);
	message.Add(rect);
	message.Add(angle);
	message.Add(span);
	message.AddGradient(gradient);
}


void
RemoteDrawingEngine::DrawBezier(BPoint* points, bool filled)
{
	BRect bounds = _BuildBounds(points, 4);
	if (!filled)
		bounds.InsetBy(fExtendWidth, fExtendWidth);

	if (!fClippingRegion.Intersects(bounds))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(filled ? RP_FILL_BEZIER : RP_STROKE_BEZIER);
	message.Add(fToken);
	message.AddList(points, 4);
}


void
RemoteDrawingEngine::FillBezier(BPoint* points, const BGradient& gradient)
{
	BRect bounds = _BuildBounds(points, 4);
	if (!fClippingRegion.Intersects(bounds))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_FILL_BEZIER_GRADIENT);
	message.Add(fToken);
	message.AddList(points, 4);
	message.AddGradient(gradient);
}


void
RemoteDrawingEngine::DrawEllipse(BRect rect, bool filled)
{
	BRect bounds = rect;
	if (!filled)
		bounds.InsetBy(fExtendWidth, fExtendWidth);

	if (!fClippingRegion.Intersects(bounds))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(filled ? RP_FILL_ELLIPSE : RP_STROKE_ELLIPSE);
	message.Add(fToken);
	message.Add(rect);
}


void
RemoteDrawingEngine::FillEllipse(BRect rect, const BGradient& gradient)
{
	if (!fClippingRegion.Intersects(rect))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_FILL_ELLIPSE_GRADIENT);
	message.Add(fToken);
	message.Add(rect);
	message.AddGradient(gradient);
}


void
RemoteDrawingEngine::DrawPolygon(BPoint* pointList, int32 numPoints,
	BRect bounds, bool filled, bool closed)
{
	BRect clipBounds = bounds;
	if (!filled)
		clipBounds.InsetBy(fExtendWidth, fExtendWidth);

	if (!fClippingRegion.Intersects(clipBounds))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(filled ? RP_FILL_POLYGON : RP_STROKE_POLYGON);
	message.Add(fToken);
	message.Add(bounds);
	message.Add(closed);
	message.Add(numPoints);
	for (int32 i = 0; i < numPoints; i++)
		message.Add(pointList[i]);
}


void
RemoteDrawingEngine::FillPolygon(BPoint* pointList, int32 numPoints,
	BRect bounds, const BGradient& gradient, bool closed)
{
	if (!fClippingRegion.Intersects(bounds))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_FILL_POLYGON_GRADIENT);
	message.Add(fToken);
	message.Add(bounds);
	message.Add(closed);
	message.Add(numPoints);
	for (int32 i = 0; i < numPoints; i++)
		message.Add(pointList[i]);
	message.AddGradient(gradient);
}


// #pragma mark - rgb_color versions


void
RemoteDrawingEngine::StrokePoint(const BPoint& point, const rgb_color& color)
{
	BRect bounds(point, point);
	bounds.InsetBy(fExtendWidth, fExtendWidth);

	if (!fClippingRegion.Intersects(bounds))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_STROKE_POINT_COLOR);
	message.Add(fToken);
	message.Add(point);
	message.Add(color);
}


void
RemoteDrawingEngine::StrokeLine(const BPoint& start, const BPoint& end,
	const rgb_color& color)
{
	BPoint points[2] = { start, end };
	BRect bounds = _BuildBounds(points, 2);

	if (!fClippingRegion.Intersects(bounds))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_STROKE_LINE_1PX_COLOR);
	message.Add(fToken);
	message.AddList(points, 2);
	message.Add(color);
}


void
RemoteDrawingEngine::StrokeRect(BRect rect, const rgb_color &color)
{
	BRect bounds = rect;
	bounds.InsetBy(fExtendWidth, fExtendWidth);

	if (!fClippingRegion.Intersects(bounds))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_STROKE_RECT_1PX_COLOR);
	message.Add(fToken);
	message.Add(rect);
	message.Add(color);
}


void
RemoteDrawingEngine::FillRect(BRect rect, const rgb_color& color)
{
	if (!fClippingRegion.Intersects(rect))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_FILL_RECT_COLOR);
	message.Add(fToken);
	message.Add(rect);
	message.Add(color);
}


void
RemoteDrawingEngine::FillRegion(BRegion& region, const rgb_color& color)
{
	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_FILL_REGION_COLOR_NO_CLIPPING);
	message.AddRegion(region);
	message.Add(color);
}


// #pragma mark - DrawState versions


void
RemoteDrawingEngine::StrokeRect(BRect rect)
{
	BRect bounds = rect;
	bounds.InsetBy(fExtendWidth, fExtendWidth);

	if (!fClippingRegion.Intersects(bounds))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_STROKE_RECT);
	message.Add(fToken);
	message.Add(rect);
}


void
RemoteDrawingEngine::FillRect(BRect rect)
{
	if (!fClippingRegion.Intersects(rect))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_FILL_RECT);
	message.Add(fToken);
	message.Add(rect);
}


void
RemoteDrawingEngine::FillRect(BRect rect, const BGradient& gradient)
{
	if (!fClippingRegion.Intersects(rect))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_FILL_RECT_GRADIENT);
	message.Add(fToken);
	message.Add(rect);
	message.AddGradient(gradient);
}


void
RemoteDrawingEngine::FillRegion(BRegion& region)
{
	BRegion clippedRegion = region;
	clippedRegion.IntersectWith(&fClippingRegion);
	if (clippedRegion.CountRects() == 0)
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_FILL_REGION);
	message.Add(fToken);
	message.AddRegion(clippedRegion.CountRects() < region.CountRects()
		? clippedRegion : region);
}


void
RemoteDrawingEngine::FillRegion(BRegion& region, const BGradient& gradient)
{
	BRegion clippedRegion = region;
	clippedRegion.IntersectWith(&fClippingRegion);
	if (clippedRegion.CountRects() == 0)
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_FILL_REGION_GRADIENT);
	message.Add(fToken);
	message.AddRegion(clippedRegion.CountRects() < region.CountRects()
		? clippedRegion : region);
	message.AddGradient(gradient);
}


void
RemoteDrawingEngine::DrawRoundRect(BRect rect, float xRadius, float yRadius,
	bool filled)
{
	BRect bounds = rect;
	if (!filled)
		bounds.InsetBy(fExtendWidth, fExtendWidth);

	if (!fClippingRegion.Intersects(bounds))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(filled ? RP_FILL_ROUND_RECT : RP_STROKE_ROUND_RECT);
	message.Add(fToken);
	message.Add(rect);
	message.Add(xRadius);
	message.Add(yRadius);
}


void
RemoteDrawingEngine::FillRoundRect(BRect rect, float xRadius, float yRadius,
	const BGradient& gradient)
{
	if (!fClippingRegion.Intersects(rect))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_FILL_ROUND_RECT_GRADIENT);
	message.Add(fToken);
	message.Add(rect);
	message.Add(xRadius);
	message.Add(yRadius);
	message.AddGradient(gradient);
}


void
RemoteDrawingEngine::DrawShape(const BRect& bounds, int32 opCount,
	const uint32* opList, int32 pointCount, const BPoint* pointList,
	bool filled, const BPoint& viewToScreenOffset, float viewScale)
{
	BRect clipBounds = bounds;
	if (!filled)
		clipBounds.InsetBy(fExtendWidth, fExtendWidth);

	if (!fClippingRegion.Intersects(clipBounds))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(filled ? RP_FILL_SHAPE : RP_STROKE_SHAPE);
	message.Add(fToken);
	message.Add(bounds);
	message.Add(opCount);
	message.AddList(opList, opCount);
	message.Add(pointCount);
	message.AddList(pointList, pointCount);
	// TODO: viewToScreenOffset and viewScale
}


void
RemoteDrawingEngine::FillShape(const BRect& bounds, int32 opCount,
	const uint32* opList, int32 pointCount, const BPoint* pointList,
	const BGradient& gradient, const BPoint& viewToScreenOffset,
	float viewScale)
{
	if (!fClippingRegion.Intersects(bounds))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_FILL_SHAPE_GRADIENT);
	message.Add(fToken);
	message.Add(bounds);
	message.Add(opCount);
	message.AddList(opList, opCount);
	message.Add(pointCount);
	message.AddList(pointList, pointCount);
	message.AddGradient(gradient);
	// TODO: viewToScreenOffset and viewScale
}


void
RemoteDrawingEngine::DrawTriangle(BPoint* points, const BRect& bounds,
	bool filled)
{
	BRect clipBounds = bounds;
	if (!filled)
		clipBounds.InsetBy(fExtendWidth, fExtendWidth);

	if (!fClippingRegion.Intersects(clipBounds))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(filled ? RP_FILL_TRIANGLE : RP_STROKE_TRIANGLE);
	message.Add(fToken);
	message.AddList(points, 3);
	message.Add(bounds);
}


void
RemoteDrawingEngine::FillTriangle(BPoint* points, const BRect& bounds,
	const BGradient& gradient)
{
	if (!fClippingRegion.Intersects(bounds))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_FILL_TRIANGLE_GRADIENT);
	message.Add(fToken);
	message.Add(points[0]);
	message.Add(points[1]);
	message.Add(points[2]);
	message.Add(bounds);
	message.AddGradient(gradient);
}


void
RemoteDrawingEngine::StrokeLine(const BPoint &start, const BPoint &end)
{
	BPoint points[2] = { start, end };
	BRect bounds = _BuildBounds(points, 2);

	if (!fClippingRegion.Intersects(bounds))
		return;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_STROKE_LINE);
	message.Add(fToken);
	message.AddList(points, 2);
}


void
RemoteDrawingEngine::StrokeLineArray(int32 numLines,
	const ViewLineArrayInfo *lineData)
{
	RemoteMessage message(NULL, fHWInterface->SendBuffer());
	message.Start(RP_STROKE_LINE_ARRAY);
	message.Add(fToken);
	message.Add(numLines);
	for (int32 i = 0; i < numLines; i++)
		message.AddArrayLine(lineData[i]);
}


// #pragma mark - string functions


BPoint
RemoteDrawingEngine::DrawString(const char* string, int32 length,
	const BPoint& point, escapement_delta* delta)
{
	RemoteMessage message(NULL, fHWInterface->SendBuffer());

	message.Start(RP_DRAW_STRING);
	message.Add(fToken);
	message.Add(point);
	message.AddString(string, length);
	message.Add(delta != NULL);
	if (delta != NULL)
		message.AddList(delta, length);

	status_t result = _AddCallback();
	if (message.Flush() != B_OK)
		return point;

	if (result != B_OK)
		return point;

	do {
		result = acquire_sem_etc(fResultNotify, 1, B_RELATIVE_TIMEOUT,
			1 * 1000 * 1000);
	} while (result == B_INTERRUPTED);

	if (result != B_OK)
		return point;

	return fDrawStringResult;
}


BPoint
RemoteDrawingEngine::DrawString(const char* string, int32 length,
	const BPoint* offsets)
{
	// Guaranteed to have at least one point.
	RemoteMessage message(NULL, fHWInterface->SendBuffer());

	message.Start(RP_DRAW_STRING_WITH_OFFSETS);
	message.Add(fToken);
	message.AddString(string, length);
	message.AddList(offsets, UTF8CountChars(string, length));

	status_t result = _AddCallback();
	if (message.Flush() != B_OK)
		return offsets[0];

	if (result != B_OK)
		return offsets[0];

	do {
		result = acquire_sem_etc(fResultNotify, 1, B_RELATIVE_TIMEOUT,
			1 * 1000 * 1000);
	} while (result == B_INTERRUPTED);

	if (result != B_OK)
		return offsets[0];

	return fDrawStringResult;
}


float
RemoteDrawingEngine::StringWidth(const char* string, int32 length,
	escapement_delta* delta)
{
	// TODO: decide if really needed and use callback if so
	return fState.Font().StringWidth(string, length, delta);
}


// #pragma mark -


status_t
RemoteDrawingEngine::ReadBitmap(ServerBitmap* bitmap, bool drawCursor,
	BRect bounds)
{
	if (_AddCallback() != B_OK)
		return B_UNSUPPORTED;

	RemoteMessage message(NULL, fHWInterface->SendBuffer());

	message.Start(RP_READ_BITMAP);
	message.Add(fToken);
	message.Add(bounds);
	message.Add(drawCursor);
	if (message.Flush() != B_OK)
		return B_UNSUPPORTED;

	status_t result;
	do {
		result = acquire_sem_etc(fResultNotify, 1, B_RELATIVE_TIMEOUT,
			100 * 1000 * 1000);
	} while (result == B_INTERRUPTED);

	if (result != B_OK)
		return result;

	BBitmap* read = fReadBitmapResult;
	if (read == NULL)
		return B_UNSUPPORTED;

	result = bitmap->ImportBits(read->Bits(), read->BitsLength(),
		read->BytesPerRow(), read->ColorSpace());
	delete read;
	return result;
}


// #pragma mark -


status_t
RemoteDrawingEngine::_AddCallback()
{
	if (fCallbackAdded)
		return B_OK;

	if (fResultNotify < 0)
		fResultNotify = create_sem(0, "drawing engine result");
	if (fResultNotify < 0)
		return fResultNotify;

	status_t result = fHWInterface->AddCallback(fToken, &_DrawingEngineResult,
		this);

	fCallbackAdded = result == B_OK;
	return result;
}


bool
RemoteDrawingEngine::_DrawingEngineResult(void* cookie, RemoteMessage& message)
{
	RemoteDrawingEngine* engine = (RemoteDrawingEngine*)cookie;

	switch (message.Code()) {
		case RP_DRAW_STRING_RESULT:
			if (message.Read(engine->fDrawStringResult) != B_OK)
				return false;
			break;

		case RP_STRING_WIDTH_RESULT:
			if (message.Read(engine->fStringWidthResult) != B_OK)
				return false;
			break;

		case RP_READ_BITMAP_RESULT:
			if (message.ReadBitmap(&engine->fReadBitmapResult) != B_OK)
				return false;
			break;

		default:
			return false;
	}

	release_sem(engine->fResultNotify);
	return true;
}


BRect
RemoteDrawingEngine::_BuildBounds(BPoint* points, int32 pointCount)
{
	BRect bounds(1000000, 1000000, 0, 0);
	for (int32 i = 0; i < pointCount; i++) {
		bounds.left = min_c(bounds.left, points[i].x);
		bounds.top = min_c(bounds.top, points[i].y);
		bounds.right = max_c(bounds.right, points[i].x);
		bounds.bottom = max_c(bounds.bottom, points[i].y);
	}

	return bounds;
}
