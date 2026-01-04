/*
 * Copyright 2001-2015, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 *		Marcus Overhagen <marcus@overhagen.de>
 *      Julian Harnath <julian.harnath@rwth-aachen.de>
 */

#include "PictureBoundingBoxPlayer.h"

#include <new>
#include <stdio.h>

#include "DrawState.h"
#include "GlobalFontManager.h"
#include "Layer.h"
#include "ServerApp.h"
#include "ServerBitmap.h"
#include "ServerFont.h"
#include "ServerPicture.h"
#include "ServerTokenSpace.h"
#include "View.h"
#include "Window.h"

#include <AutoDeleter.h>
#include <Bitmap.h>
#include <Debug.h>
#include <List.h>
#include <ObjectListPrivate.h>
#include <PicturePlayer.h>
#include <PictureProtocol.h>
#include <Shape.h>
#include <StackOrHeapArray.h>


//#define DEBUG_TRACE_BB
#ifdef DEBUG_TRACE_BB
#	define TRACE_BB(text, ...) debug_printf("PBBP: " text, ##__VA_ARGS__)
#else
#	define TRACE_BB(text, ...)
#endif


typedef PictureBoundingBoxPlayer::State BoundingBoxState;


// #pragma mark - PictureBoundingBoxPlayer::State


class PictureBoundingBoxPlayer::State {
public:
	State(const DrawState* drawState, BRect* boundingBox)
		:
		fDrawState(drawState->Squash()),
		fBoundingBox(boundingBox)
	{
		fBoundingBox->Set(INT_MAX, INT_MAX, INT_MIN, INT_MIN);
	}

	~State()
	{
	}

	DrawState* GetDrawState()
	{
		return fDrawState.Get();
	}

	void PushDrawState()
	{
		DrawState* previousState = fDrawState.Detach();
		DrawState* newState = previousState->PushState();
		if (newState == NULL)
			newState = previousState;

		fDrawState.SetTo(newState);
	}

	void PopDrawState()
	{
		if (fDrawState->PreviousState() != NULL)
			fDrawState.SetTo(fDrawState->PopState());
	}

	SimpleTransform PenToLocalTransform() const
	{
		SimpleTransform transform;
		fDrawState->Transform(transform);
		return transform;
	}

	void IncludeRect(BRect& rect)
	{
		_AffineTransformRect(rect);
		*fBoundingBox = (*fBoundingBox) | rect;
	}

private:
	void _AffineTransformRect(BRect& rect)
	{
		BAffineTransform transform = fDrawState->CombinedTransform();
		if (transform.IsIdentity())
			return;

		BPoint transformedShape[4];
		transformedShape[0] = rect.LeftTop();
		transformedShape[1] = rect.LeftBottom();
		transformedShape[2] = rect.RightTop();
		transformedShape[3] = rect.RightBottom();

		transform.Apply(&transformedShape[0], 4);

		float minX = INT_MAX;
		float minY = INT_MAX;
		float maxX = INT_MIN;
		float maxY = INT_MIN;

		for (uint32 i = 0; i < 4; i++) {
			if (transformedShape[i].x < minX)
				minX = transformedShape[i].x;
			else if (transformedShape[i].x > maxX)
				maxX = transformedShape[i].x;
			if (transformedShape[i].y < minY)
				minY = transformedShape[i].y;
			else if (transformedShape[i].y > maxY)
				maxY = transformedShape[i].y;
		}

		rect.Set(minX, minY, maxX, maxY);
	}


private:
	ObjectDeleter<DrawState>
				fDrawState;
	BRect*		fBoundingBox;
};


// #pragma mark - Picture playback hooks


class BoundingBoxCallbacks: public BPrivate::PicturePlayerCallbacks {
public:
	BoundingBoxCallbacks(BoundingBoxState* const state);

	virtual void MovePenBy(const BPoint& where);
	virtual void StrokeLine(const BPoint& start, const BPoint& end);
	virtual void DrawRect(const BRect& rect, bool fill);
	virtual void DrawRoundRect(const BRect& rect, const BPoint& radii, bool fill);
	virtual void DrawBezier(const BPoint controlPoints[4], bool fill);
	virtual void DrawArc(const BPoint& center, const BPoint& radii, float startTheta,
		float arcTheta, bool fill);
	virtual void DrawEllipse(const BRect& rect, bool fill);
	virtual void DrawPolygon(size_t numPoints, const BPoint points[], bool isClosed, bool fill);
	virtual void DrawShape(const BShape& shape, bool fill);
	virtual void DrawString(const char* string, size_t length, float spaceEscapement,
		float nonSpaceEscapement);
	virtual void DrawPixels(const BRect& source, const BRect& destination, uint32 width,
		uint32 height, size_t bytesPerRow, color_space pixelFormat, uint32 flags, const void* data,
		size_t length);
	virtual void DrawPicture(const BPoint& where, int32 token);
	virtual void SetClippingRects(size_t numRects, const clipping_rect rects[]);
	virtual void ClipToPicture(int32 token, const BPoint& where, bool clipToInverse);
	virtual void PushState();
	virtual void PopState();
	virtual void EnterStateChange();
	virtual void ExitStateChange();
	virtual void EnterFontState();
	virtual void ExitFontState();
	virtual void SetOrigin(const BPoint& origin);
	virtual void SetPenLocation(const BPoint& location);
	virtual void SetDrawingMode(drawing_mode mode);
	virtual void SetLineMode(cap_mode capMode, join_mode joinMode, float miterLimit);
	virtual void SetPenSize(float size);
	virtual void SetForeColor(const rgb_color& color);
	virtual void SetBackColor(const rgb_color& color);
	virtual void SetStipplePattern(const pattern& patter);
	virtual void SetScale(float scale);
	virtual void SetFontFamily(const char* familyName, size_t length);
	virtual void SetFontStyle(const char* styleName, size_t length);
	virtual void SetFontSpacing(uint8 spacing);
	virtual void SetFontSize(float size);
	virtual void SetFontRotation(float rotation);
	virtual void SetFontEncoding(uint8 encoding);
	virtual void SetFontFlags(uint32 flags);
	virtual void SetFontShear(float shear);
	virtual void SetFontFace(uint16 face);
	virtual void SetBlendingMode(source_alpha alphaSourceMode, alpha_function alphaFunctionMode);
	virtual void SetTransform(const BAffineTransform& transform);
	virtual void TranslateBy(double x, double y);
	virtual void ScaleBy(double x, double y);
	virtual void RotateBy(double angleRadians);
	virtual void BlendLayer(Layer* layer);
	virtual void ClipToRect(const BRect& rect, bool inverse);
	virtual void ClipToShape(int32 opCount, const uint32 opList[], int32 ptCount,
		const BPoint ptList[], bool inverse);
	virtual void DrawStringLocations(const char* string, size_t length, const BPoint locations[],
		size_t locationCount);
	virtual void DrawRectGradient(const BRect& rect, BGradient& gradient, bool fill);
	virtual void DrawRoundRectGradient(const BRect& rect, const BPoint& radii, BGradient& gradient,
		bool fill);
	virtual void DrawBezierGradient(const BPoint controlPoints[4], BGradient& gradient, bool fill);
	virtual void DrawArcGradient(const BPoint& center, const BPoint& radii, float startTheta,
		float arcTheta, BGradient& gradient, bool fill);
	virtual void DrawEllipseGradient(const BRect& rect, BGradient& gradient, bool fill);
	virtual void DrawPolygonGradient(size_t numPoints, const BPoint points[], bool isClosed,
		BGradient& gradient, bool fill);
	virtual void DrawShapeGradient(const BShape& shape, BGradient& gradient, bool fill);
	virtual void SetFillRule(int32 fillRule);
	virtual void StrokeLineGradient(const BPoint& start, const BPoint& end, BGradient& gradient);

private:
	BoundingBoxState* const fState;
};


BoundingBoxCallbacks::BoundingBoxCallbacks(BoundingBoxState* const state)
	:
	fState(state)
{
}


static void
get_polygon_frame(const BPoint* points, int32 numPoints, BRect* frame)
{
	ASSERT(numPoints > 0);

	float left = points->x;
	float top = points->y;
	float right = left;
	float bottom = top;

	points++;
	numPoints--;

	while (numPoints--) {
		if (points->x < left)
			left = points->x;
		if (points->x > right)
			right = points->x;
		if (points->y < top)
			top = points->y;
		if (points->y > bottom)
			bottom = points->y;
		points++;
	}

	frame->Set(left, top, right, bottom);
}


template<class RectType>
static void
expand_rect_for_pen_size(BoundingBoxState* state, RectType& rect)
{
	float penInset = -((state->GetDrawState()->PenSize() / 2.0f) + 1.0f);
	rect.InsetBy(penInset, penInset);
}


void
BoundingBoxCallbacks::MovePenBy(const BPoint& delta)
{
	TRACE_BB("%p move pen by %.2f %.2f\n", fState, delta.x, delta.y);

	fState->GetDrawState()->SetPenLocation(
		fState->GetDrawState()->PenLocation() + delta);
}


void
BoundingBoxCallbacks::StrokeLine(const BPoint& _start,
	const BPoint& _end)
{
	TRACE_BB("%p stroke line %.2f %.2f -> %.2f %.2f\n", fState,
		_start.x, _start.y, _end.x, _end.y);

	BPoint start = _start;
	BPoint end = _end;

	const SimpleTransform transform = fState->PenToLocalTransform();
	transform.Apply(&start);
	transform.Apply(&end);

	BRect rect;
	if (start.x <= end.x) {
		rect.left = start.x;
		rect.right = end.x;
	} else {
		rect.left = end.x;
		rect.right = start.x;
	}
	if (start.y <= end.y) {
		rect.top = start.y;
		rect.bottom = end.y;
	} else {
		rect.top = end.y;
		rect.bottom = start.y;
	}

	expand_rect_for_pen_size(fState, rect);
	fState->IncludeRect(rect);

	fState->GetDrawState()->SetPenLocation(_end);
}


void
BoundingBoxCallbacks::DrawRect(const BRect& _rect, bool fill)
{
	TRACE_BB("%p draw rect fill=%d %.2f %.2f %.2f %.2f\n", fState, fill,
		_rect.left, _rect.top, _rect.right, _rect.bottom);

	BRect rect = _rect;
	fState->PenToLocalTransform().Apply(&rect);
	if (!fill)
		expand_rect_for_pen_size(fState, rect);
	fState->IncludeRect(rect);
}


void
BoundingBoxCallbacks::DrawRoundRect(const BRect& _rect,
	const BPoint&, bool fill)
{
	DrawRect(_rect, fill);
}


static void
determine_bounds_bezier(BoundingBoxState* state, const BPoint viewPoints[4],
	BRect& outRect)
{
	// Note: this is an approximation which results in a rectangle which
	// encloses all four control points. That will always enclose the curve,
	// although not necessarily tightly, but it's good enough for the purpose.
	// The exact bounding box of a bezier curve is not trivial to determine,
	// (need to calculate derivative of the curve) and we're going for
	// performance here.
	BPoint points[4];
	state->PenToLocalTransform().Apply(points, viewPoints, 4);
	BPoint topLeft = points[0];
	BPoint bottomRight = points[0];
	for (uint32 index = 1; index < 4; index++) {
		if (points[index].x < topLeft.x || points[index].y < topLeft.y)
			topLeft = points[index];
		if (points[index].x > topLeft.x || points[index].y > topLeft.y)
			bottomRight = points[index];
	}
	outRect.SetLeftTop(topLeft);
	outRect.SetRightBottom(bottomRight);
}


void
BoundingBoxCallbacks::DrawBezier(const BPoint viewPoints[4], bool fill)
{
	TRACE_BB("%p draw bezier fill=%d (%.2f %.2f) (%.2f %.2f) "
		"(%.2f %.2f) (%.2f %.2f)\n",
		fState,
		fill,
		viewPoints[0].x, viewPoints[0].y,
		viewPoints[1].x, viewPoints[1].y,
		viewPoints[2].x, viewPoints[2].y,
		viewPoints[3].x, viewPoints[3].y);

	BRect rect;
	determine_bounds_bezier(fState, viewPoints, rect);
	if (!fill)
		expand_rect_for_pen_size(fState, rect);
	fState->IncludeRect(rect);
}


void
BoundingBoxCallbacks::DrawEllipse(const BRect& _rect, bool fill)
{
	TRACE_BB("%p draw ellipse fill=%d (%.2f %.2f) (%.2f %.2f)\n", fState, fill,
		_rect.left, _rect.top, _rect.right, _rect.bottom);

	BRect rect = _rect;
	fState->PenToLocalTransform().Apply(&rect);
	if (!fill)
		expand_rect_for_pen_size(fState, rect);
	fState->IncludeRect(rect);
}


void
BoundingBoxCallbacks::DrawArc(const BPoint& center,
	const BPoint& radii, float, float, bool fill)
{
	BRect rect(center.x - radii.x, center.y - radii.y,
		center.x + radii.x - 1, center.y + radii.y - 1);
	DrawEllipse(rect, fill);
}


static void
determine_bounds_polygon(BoundingBoxState* state, int32 numPoints,
	const BPoint* viewPoints, BRect& outRect)
{
	if (numPoints <= 0)
		return;

	BStackOrHeapArray<BPoint, 200> points(numPoints);
	state->PenToLocalTransform().Apply(points, viewPoints, numPoints);
	get_polygon_frame(points, numPoints, &outRect);
}


void
BoundingBoxCallbacks::DrawPolygon(size_t numPoints,
	const BPoint viewPoints[], bool, bool fill)
{
	TRACE_BB("%p draw polygon fill=%d (%ld points)\n", fState, fill, numPoints);

	BRect rect;
	determine_bounds_polygon(fState, numPoints, viewPoints, rect);
	if (!fill)
		expand_rect_for_pen_size(fState, rect);
	fState->IncludeRect(rect);
}


void
BoundingBoxCallbacks::DrawShape(const BShape& shape, bool fill)
{
	BRect rect = shape.Bounds();

	TRACE_BB("%p stroke shape (bounds %.2f %.2f %.2f %.2f)\n", fState,
		rect.left, rect.top, rect.right, rect.bottom);

	fState->PenToLocalTransform().Apply(&rect);
	if (!fill)
		expand_rect_for_pen_size(fState, rect);
	fState->IncludeRect(rect);
}


void
BoundingBoxCallbacks::DrawString(const char* string, size_t length,
	float deltaSpace, float deltaNonSpace)
{
	TRACE_BB("%p string '%s'\n", fState, string);

	ServerFont font = fState->GetDrawState()->Font();

	escapement_delta delta = { deltaSpace, deltaNonSpace };
	BRect rect;
	font.GetBoundingBoxesForStrings((char**)&string, &length, 1, &rect,
		B_SCREEN_METRIC, &delta);

	BPoint location = fState->GetDrawState()->PenLocation();

	fState->PenToLocalTransform().Apply(&location);
	rect.OffsetBy(location);
	fState->IncludeRect(rect);

	fState->PenToLocalTransform().Apply(&location);
	fState->GetDrawState()->SetPenLocation(location);
}


void
BoundingBoxCallbacks::DrawPixels(const BRect&, const BRect& _dest,
	uint32, uint32, size_t, color_space, uint32, const void*, size_t)
{
	TRACE_BB("%p pixels (dest %.2f %.2f %.2f %.2f)\n", fState,
		_dest.left, _dest.top, _dest.right, _dest.bottom);

	BRect dest = _dest;
	fState->PenToLocalTransform().Apply(&dest);
	fState->IncludeRect(dest);
}


void
BoundingBoxCallbacks::DrawPicture(const BPoint& where, int32 token)
{
	TRACE_BB("%p picture (unimplemented)\n", fState);

	// TODO
	(void)where;
	(void)token;
}


void
BoundingBoxCallbacks::SetClippingRects(size_t numRects, const clipping_rect rects[])
{
	TRACE_BB("%p cliping rects (%ld rects)\n", fState, numRects);

	// TODO
	(void)rects;
	(void)numRects;
}


void
BoundingBoxCallbacks::ClipToPicture(int32 pictureToken, const BPoint& where,
	bool clipToInverse)
{
	TRACE_BB("%p clip to picture (unimplemented)\n", fState);

	// TODO
}


void
BoundingBoxCallbacks::PushState()
{
	TRACE_BB("%p push state\n", fState);

	fState->PushDrawState();
}


void
BoundingBoxCallbacks::PopState()
{
	TRACE_BB("%p pop state\n", fState);

	fState->PopDrawState();
}


void
BoundingBoxCallbacks::EnterStateChange()
{
}


void
BoundingBoxCallbacks::ExitStateChange()
{
}


void
BoundingBoxCallbacks::EnterFontState()
{
}


void
BoundingBoxCallbacks::ExitFontState()
{
}


void
BoundingBoxCallbacks::SetOrigin(const BPoint& pt)
{
	TRACE_BB("%p set origin %.2f %.2f\n", , pt.x, pt.y);
	fState->GetDrawState()->SetOrigin(pt);
}


void
BoundingBoxCallbacks::SetPenLocation(const BPoint& pt)
{
	TRACE_BB("%p set pen location %.2f %.2f\n", fState, pt.x, pt.y);
	fState->GetDrawState()->SetPenLocation(pt);
}


void
BoundingBoxCallbacks::SetDrawingMode(drawing_mode)
{
}


void
BoundingBoxCallbacks::SetLineMode(cap_mode capMode, join_mode joinMode,
	float miterLimit)
{
	DrawState* drawState = fState->GetDrawState();
	drawState->SetLineCapMode(capMode);
	drawState->SetLineJoinMode(joinMode);
	drawState->SetMiterLimit(miterLimit);
}


void
BoundingBoxCallbacks::SetPenSize(float size)
{
	TRACE_BB("%p set pen size %.2f\n", fState, size);

	fState->GetDrawState()->SetPenSize(size);
}


void
BoundingBoxCallbacks::SetForeColor(const rgb_color& color)
{
	fState->GetDrawState()->SetHighColor(color);
}


void
BoundingBoxCallbacks::SetBackColor(const rgb_color& color)
{
	fState->GetDrawState()->SetLowColor(color);
}


void
BoundingBoxCallbacks::SetStipplePattern(const pattern& _pattern)
{
	fState->GetDrawState()->SetPattern(Pattern(_pattern));
}


void
BoundingBoxCallbacks::SetScale(float scale)
{
	fState->GetDrawState()->SetScale(scale);
}


void
BoundingBoxCallbacks::SetFontFamily(const char* _family, size_t length)
{
	BString family(_family, length);
	gFontManager->Lock();
	FontStyle* fontStyle = gFontManager->GetStyleByIndex(family, 0);
	ServerFont font;
	font.SetStyle(fontStyle);
	gFontManager->Unlock();
	fState->GetDrawState()->SetFont(font, B_FONT_FAMILY_AND_STYLE);
}


void
BoundingBoxCallbacks::SetFontStyle(const char* _style, size_t length)
{
	BString style(_style, length);
	ServerFont font(fState->GetDrawState()->Font());
	gFontManager->Lock();
	FontStyle* fontStyle = gFontManager->GetStyle(font.Family(), style);
	font.SetStyle(fontStyle);
	gFontManager->Unlock();
	fState->GetDrawState()->SetFont(font, B_FONT_FAMILY_AND_STYLE);
}


void
BoundingBoxCallbacks::SetFontSpacing(uint8 spacing)
{
	ServerFont font;
	font.SetSpacing(spacing);
	fState->GetDrawState()->SetFont(font, B_FONT_SPACING);
}


void
BoundingBoxCallbacks::SetFontSize(float size)
{
	ServerFont font;
	font.SetSize(size);
	fState->GetDrawState()->SetFont(font, B_FONT_SIZE);
}


void
BoundingBoxCallbacks::SetFontRotation(float rotation)
{
	ServerFont font;
	font.SetRotation(rotation);
	fState->GetDrawState()->SetFont(font, B_FONT_ROTATION);
}


void
BoundingBoxCallbacks::SetFontEncoding(uint8 encoding)
{
	ServerFont font;
	font.SetEncoding(encoding);
	fState->GetDrawState()->SetFont(font, B_FONT_ENCODING);
}


void
BoundingBoxCallbacks::SetFontFlags(uint32 flags)
{
	ServerFont font;
	font.SetFlags(flags);
	fState->GetDrawState()->SetFont(font, B_FONT_FLAGS);
}


void
BoundingBoxCallbacks::SetFontShear(float shear)
{
	ServerFont font;
	font.SetShear(shear);
	fState->GetDrawState()->SetFont(font, B_FONT_SHEAR);
}


void
BoundingBoxCallbacks::SetFontFace(uint16 face)
{
	ServerFont font;
	font.SetFace(face);
	fState->GetDrawState()->SetFont(font, B_FONT_FACE);
}


void
BoundingBoxCallbacks::SetBlendingMode(source_alpha, alpha_function)
{
}


void
BoundingBoxCallbacks::SetTransform(const BAffineTransform& transform)
{
	TRACE_BB("%p transform\n", fState);
	fState->GetDrawState()->SetTransform(transform);
}


void
BoundingBoxCallbacks::TranslateBy(double x, double y)
{
	TRACE_BB("%p translate\n", fState);
	BAffineTransform transform = fState->GetDrawState()->Transform();
	transform.PreTranslateBy(x, y);
	fState->GetDrawState()->SetTransform(transform);
}


void
BoundingBoxCallbacks::ScaleBy(double x, double y)
{
	TRACE_BB("%p scale\n", fState);
	BAffineTransform transform = fState->GetDrawState()->Transform();
	transform.PreScaleBy(x, y);
	fState->GetDrawState()->SetTransform(transform);
}


void
BoundingBoxCallbacks::RotateBy(double angleRadians)
{
	TRACE_BB("%p rotate\n", fState);
	BAffineTransform transform = fState->GetDrawState()->Transform();
	transform.PreRotateBy(angleRadians);
	fState->GetDrawState()->SetTransform(transform);
}


void
BoundingBoxCallbacks::BlendLayer(Layer* layer)
{
	TRACE_BB("%p nested layer\n", fState);

	BRect boundingBox;
	PictureBoundingBoxPlayer::Play(layer, fState->GetDrawState(), &boundingBox);
	if (boundingBox.IsValid())
		fState->IncludeRect(boundingBox);
}


void
BoundingBoxCallbacks::DrawRectGradient(const BRect& _rect, BGradient& gradient, bool fill)
{
	TRACE_BB("%p draw rect gradient fill=%d %.2f %.2f %.2f %.2f\n", fState, fill,
		_rect.left, _rect.top, _rect.right, _rect.bottom);

	BRect rect = _rect;
	const SimpleTransform transform = fState->PenToLocalTransform();
	transform.Apply(&rect);
	transform.Apply(&gradient);
	if (!fill)
		expand_rect_for_pen_size(fState, rect);
	fState->IncludeRect(rect);
}


void
BoundingBoxCallbacks::DrawRoundRectGradient(const BRect& rect, const BPoint& radii,
	BGradient& gradient, bool fill)
{
	TRACE_BB("%p draw round rect gradient fill=%d %.2f %.2f %.2f %.2f\n", fState, fill,
		rect.left, rect.top, rect.right, rect.bottom);
	DrawRectGradient(rect, gradient, fill);
}


void
BoundingBoxCallbacks::DrawBezierGradient(const BPoint controlPoints[4], BGradient& gradient,
	bool fill)
{
	TRACE_BB("%p draw bezier gradient fill=%d (%.2f %.2f) (%.2f %.2f) "
		"(%.2f %.2f) (%.2f %.2f)\n",
		fState,
		fill,
		viewPoints[0].x, viewPoints[0].y,
		viewPoints[1].x, viewPoints[1].y,
		viewPoints[2].x, viewPoints[2].y,
		viewPoints[3].x, viewPoints[3].y);

	BRect rect;
	determine_bounds_bezier(fState, controlPoints, rect);
	fState->PenToLocalTransform().Apply(&gradient);
	if (!fill)
		expand_rect_for_pen_size(fState, rect);
	fState->IncludeRect(rect);
}


void
BoundingBoxCallbacks::DrawEllipseGradient(const BRect& _rect, BGradient& gradient, bool fill)
{
	TRACE_BB("%p draw ellipse gradient fill=%d (%.2f %.2f) (%.2f %.2f)\n", fState, fill,
		_rect.left, _rect.top, _rect.right, _rect.bottom);

	BRect rect = _rect;
	const SimpleTransform transform = fState->PenToLocalTransform();
	transform.Apply(&rect);
	transform.Apply(&gradient);
	if (!fill)
		expand_rect_for_pen_size(fState, rect);
	fState->IncludeRect(rect);
}


void
BoundingBoxCallbacks::DrawArcGradient(const BPoint& center, const BPoint& radii, float startTheta,
	float arcTheta, BGradient& gradient, bool fill)
{
	BRect rect(center.x - radii.x, center.y - radii.y,
		center.x + radii.x - 1, center.y + radii.y - 1);
	DrawEllipseGradient(rect, gradient, fill);
}


void
BoundingBoxCallbacks::DrawPolygonGradient(size_t numPoints, const BPoint points[], bool isClosed,
	BGradient& gradient, bool fill)
{
	TRACE_BB("%p draw polygon fill=%d (%ld points)\n", fState, fill, numPoints);
	if (numPoints <= 0)
		return;

	BRect rect;
	determine_bounds_polygon(fState, numPoints, points, rect);
	fState->PenToLocalTransform().Apply(&gradient);
	if (!fill)
		expand_rect_for_pen_size(fState, rect);
	fState->IncludeRect(rect);
}


void
BoundingBoxCallbacks::DrawShapeGradient(const BShape& shape, BGradient& gradient, bool fill)
{
	BRect rect = shape.Bounds();

	// FIXME: this doesn't work - ShapePainter uses ScreenTransform unconditionally
	TRACE_BB("%p stroke shape gradient (bounds %.2f %.2f %.2f %.2f)\n", fState,
		rect.left, rect.top, rect.right, rect.bottom);

	const SimpleTransform transform = fState->PenToLocalTransform();
	transform.Apply(&rect);
	transform.Apply(&gradient);
	if (!fill)
		expand_rect_for_pen_size(fState, rect);
	fState->IncludeRect(rect);
}


void
BoundingBoxCallbacks::ClipToRect(const BRect& rect, bool inverse)
{
	// TODO
}


void
BoundingBoxCallbacks::ClipToShape(int32 opCount, const uint32 opList[], int32 ptCount,
	const BPoint ptList[], bool inverse)
{
	// TODO
}


void
BoundingBoxCallbacks::DrawStringLocations(const char* string, size_t length,
	const BPoint locations[], size_t locationCount)
{
	// TODO
}


void
BoundingBoxCallbacks::SetFillRule(int32 fillRule)
{
	// TODO
}


void
BoundingBoxCallbacks::StrokeLineGradient(const BPoint& start, const BPoint& end,
	BGradient& gradient)
{
	TRACE_BB("%p stroke line gradient %.2f %.2f -> %.2f %.2f\n", fState,
		start.x, start.y, end.x, end.y);
	const SimpleTransform transform = fState->PenToLocalTransform();
	transform.Apply(&gradient);
	StrokeLine(start, end);
}


/* static */ void
PictureBoundingBoxPlayer::Play(ServerPicture* picture,
	const DrawState* drawState, BRect* outBoundingBox)
{
	State state(drawState, outBoundingBox);

	BMallocIO* mallocIO = dynamic_cast<BMallocIO*>(picture->fData.Get());
	if (mallocIO == NULL)
		return;

	BoundingBoxCallbacks callbacks(&state);

	BPrivate::PicturePlayer player(mallocIO->Buffer(),
		mallocIO->BufferLength(), ServerPicture::PictureList::Private(
			picture->fPictures.Get()).AsBList());
	player.Play(callbacks);
}
