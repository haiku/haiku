/*
 * Copyright 2001-2019, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 *		Marcus Overhagen <marcus@overhagen.de>
 *		Julian Harnath <julian.harnath@rwth-aachen.de>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "ServerPicture.h"

#include <new>
#include <stdio.h>
#include <stack>

#include "AlphaMask.h"
#include "DrawingEngine.h"
#include "DrawState.h"
#include "GlobalFontManager.h"
#include "Layer.h"
#include "ServerApp.h"
#include "ServerBitmap.h"
#include "ServerFont.h"
#include "ServerTokenSpace.h"
#include "View.h"
#include "Window.h"

#include <LinkReceiver.h>
#include <OffsetFile.h>
#include <ObjectListPrivate.h>
#include <PicturePlayer.h>
#include <PictureProtocol.h>
#include <PortLink.h>
#include <ServerProtocol.h>
#include <ShapePrivate.h>
#include <StackOrHeapArray.h>

#include <Bitmap.h>
#include <Debug.h>
#include <List.h>
#include <Shape.h>


using std::stack;


class ShapePainter : public BShapeIterator {
public:
	ShapePainter(Canvas* canvas, BGradient* gradient);
	virtual ~ShapePainter();

	status_t Iterate(const BShape* shape);

	virtual status_t IterateMoveTo(BPoint* point);
	virtual status_t IterateLineTo(int32 lineCount, BPoint* linePts);
	virtual status_t IterateBezierTo(int32 bezierCount, BPoint* bezierPts);
	virtual status_t IterateClose();
	virtual status_t IterateArcTo(float& rx, float& ry,
		float& angle, bool largeArc, bool counterClockWise, BPoint& point);

	void Draw(BRect frame, bool filled);

private:
	Canvas*	fCanvas;
	BGradient* fGradient;
	stack<uint32>	fOpStack;
	stack<BPoint>	fPtStack;
};


ShapePainter::ShapePainter(Canvas* canvas, BGradient* gradient)
	:
	fCanvas(canvas),
	fGradient(gradient)
{
}


ShapePainter::~ShapePainter()
{
}


status_t
ShapePainter::Iterate(const BShape* shape)
{
	// this class doesn't modify the shape data
	return BShapeIterator::Iterate(const_cast<BShape*>(shape));
}


status_t
ShapePainter::IterateMoveTo(BPoint* point)
{
	try {
		fOpStack.push(OP_MOVETO);
		fPtStack.push(*point);
	} catch (std::bad_alloc&) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
ShapePainter::IterateLineTo(int32 lineCount, BPoint* linePts)
{
	try {
		fOpStack.push(OP_LINETO | lineCount);
		for (int32 i = 0; i < lineCount; i++)
			fPtStack.push(linePts[i]);
	} catch (std::bad_alloc&) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
ShapePainter::IterateBezierTo(int32 bezierCount, BPoint* bezierPts)
{
	bezierCount *= 3;
	try {
		fOpStack.push(OP_BEZIERTO | bezierCount);
		for (int32 i = 0; i < bezierCount; i++)
			fPtStack.push(bezierPts[i]);
	} catch (std::bad_alloc&) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
ShapePainter::IterateArcTo(float& rx, float& ry,
	float& angle, bool largeArc, bool counterClockWise, BPoint& point)
{
	uint32 op;
	if (largeArc) {
		if (counterClockWise)
			op = OP_LARGE_ARC_TO_CCW;
		else
			op = OP_LARGE_ARC_TO_CW;
	} else {
		if (counterClockWise)
			op = OP_SMALL_ARC_TO_CCW;
		else
			op = OP_SMALL_ARC_TO_CW;
	}

	try {
		fOpStack.push(op | 3);
		fPtStack.push(BPoint(rx, ry));
		fPtStack.push(BPoint(angle, 0));
		fPtStack.push(point);
	} catch (std::bad_alloc&) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
ShapePainter::IterateClose()
{
	try {
		fOpStack.push(OP_CLOSE);
	} catch (std::bad_alloc&) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


void
ShapePainter::Draw(BRect frame, bool filled)
{
	// We're going to draw the currently iterated shape.
	// TODO: This can be more efficient by skipping the conversion.
	int32 opCount = fOpStack.size();
	int32 ptCount = fPtStack.size();

	if (opCount > 0 && ptCount > 0) {
		int32 i;
		uint32* opList = new(std::nothrow) uint32[opCount];
		if (opList == NULL)
			return;

		BPoint* ptList = new(std::nothrow) BPoint[ptCount];
		if (ptList == NULL) {
			delete[] opList;
			return;
		}

		for (i = opCount - 1; i >= 0; i--) {
			opList[i] = fOpStack.top();
			fOpStack.pop();
		}

		for (i = ptCount - 1; i >= 0; i--) {
			ptList[i] = fPtStack.top();
			fPtStack.pop();
		}

		// this might seem a bit weird, but under R5, the shapes
		// are always offset by the current pen location
		BPoint screenOffset = fCanvas->CurrentState()->PenLocation();
		frame.OffsetBy(screenOffset);

		const SimpleTransform transform = fCanvas->PenToScreenTransform();
		transform.Apply(&screenOffset);
		transform.Apply(&frame);

		/* stroked gradients are not yet supported */
		if (fGradient != NULL) {
			fCanvas->GetDrawingEngine()->DrawShape(frame, opCount, opList,
				ptCount, ptList, filled, *fGradient, screenOffset, fCanvas->Scale());
		} else {
			fCanvas->GetDrawingEngine()->DrawShape(frame, opCount, opList,
				ptCount, ptList, filled, screenOffset, fCanvas->Scale());
		}

		delete[] opList;
		delete[] ptList;
	}
}


// #pragma mark - drawing functions


class CanvasCallbacks: public BPrivate::PicturePlayerCallbacks {
public:
	CanvasCallbacks(Canvas* const canvas);

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
	Canvas* const fCanvas;
};


CanvasCallbacks::CanvasCallbacks(Canvas* const canvas)
	:
	fCanvas(canvas)
{
}


static void
get_polygon_frame(const BPoint* points, uint32 numPoints, BRect* _frame)
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

	_frame->Set(left, top, right, bottom);
}


void
CanvasCallbacks::MovePenBy(const BPoint& delta)
{
	fCanvas->CurrentState()->SetPenLocation(
		fCanvas->CurrentState()->PenLocation() + delta);
}


void
CanvasCallbacks::StrokeLine(const BPoint& _start, const BPoint& _end)
{
	BPoint start = _start;
	BPoint end = _end;

	const SimpleTransform transform = fCanvas->PenToScreenTransform();
	transform.Apply(&start);
	transform.Apply(&end);
	fCanvas->GetDrawingEngine()->StrokeLine(start, end);

	fCanvas->CurrentState()->SetPenLocation(_end);
	// the DrawingEngine/Painter does not need to be updated, since this
	// effects only the view->screen coord conversion, which is handled
	// by the view only
}


void
CanvasCallbacks::DrawRect(const BRect& _rect, bool fill)
{
	BRect rect = _rect;

	fCanvas->PenToScreenTransform().Apply(&rect);
	if (fill)
		fCanvas->GetDrawingEngine()->FillRect(rect);
	else
		fCanvas->GetDrawingEngine()->StrokeRect(rect);
}


void
CanvasCallbacks::DrawRoundRect(const BRect& _rect, const BPoint& radii,
	bool fill)
{
	BRect rect = _rect;

	fCanvas->PenToScreenTransform().Apply(&rect);
	float scale = fCanvas->CurrentState()->CombinedScale();
	fCanvas->GetDrawingEngine()->DrawRoundRect(rect, radii.x * scale,
		radii.y * scale, fill);
}


void
CanvasCallbacks::DrawBezier(const BPoint viewPoints[4], bool fill)
{
	const size_t kNumPoints = 4;

	BPoint points[kNumPoints];
	fCanvas->PenToScreenTransform().Apply(points, viewPoints, kNumPoints);
	fCanvas->GetDrawingEngine()->DrawBezier(points, fill);
}


void
CanvasCallbacks::DrawArc(const BPoint& center, const BPoint& radii,
	float startTheta, float arcTheta, bool fill)
{
	BRect rect(center.x - radii.x, center.y - radii.y,
		center.x + radii.x - 1, center.y + radii.y - 1);
	fCanvas->PenToScreenTransform().Apply(&rect);
	fCanvas->GetDrawingEngine()->DrawArc(rect, startTheta, arcTheta, fill);
}


void
CanvasCallbacks::DrawEllipse(const BRect& _rect, bool fill)
{
	BRect rect = _rect;
	fCanvas->PenToScreenTransform().Apply(&rect);
	fCanvas->GetDrawingEngine()->DrawEllipse(rect, fill);
}


void
CanvasCallbacks::DrawPolygon(size_t numPoints, const BPoint viewPoints[],
	bool isClosed, bool fill)
{
	if (numPoints == 0)
		return;

	BStackOrHeapArray<BPoint, 200> points(numPoints);
	if (!points.IsValid())
		return;

	fCanvas->PenToScreenTransform().Apply(points, viewPoints, numPoints);

	BRect polyFrame;
	get_polygon_frame(points, numPoints, &polyFrame);

	fCanvas->GetDrawingEngine()->DrawPolygon(points, numPoints, polyFrame,
		fill, isClosed && numPoints > 2);
}


void
CanvasCallbacks::DrawShape(const BShape& shape, bool fill)
{
	ShapePainter drawShape(fCanvas, NULL);

	drawShape.Iterate(&shape);
	drawShape.Draw(shape.Bounds(), fill);
}


void
CanvasCallbacks::DrawRectGradient(const BRect& _rect, BGradient& gradient, bool fill)
{
	BRect rect = _rect;

	const SimpleTransform transform =
		fCanvas->PenToScreenTransform();
	transform.Apply(&rect);
	transform.Apply(&gradient);

	fCanvas->GetDrawingEngine()->FillRect(rect, gradient);
}


void
CanvasCallbacks::DrawRoundRectGradient(const BRect& _rect, const BPoint& radii, BGradient& gradient,
	bool fill)
{
	BRect rect = _rect;

	const SimpleTransform transform =
		fCanvas->PenToScreenTransform();
	transform.Apply(&rect);
	transform.Apply(&gradient);
	float scale = fCanvas->CurrentState()->CombinedScale();
	fCanvas->GetDrawingEngine()->DrawRoundRect(rect, radii.x * scale,
		radii.y * scale, fill, gradient);
}


void
CanvasCallbacks::DrawBezierGradient(const BPoint viewPoints[4], BGradient& gradient, bool fill)
{
	const size_t kNumPoints = 4;

	BPoint points[kNumPoints];
	const SimpleTransform transform =
		fCanvas->PenToScreenTransform();
	transform.Apply(points, viewPoints, kNumPoints);
	transform.Apply(&gradient);
	fCanvas->GetDrawingEngine()->DrawBezier(points, fill, gradient);
}


void
CanvasCallbacks::DrawArcGradient(const BPoint& center, const BPoint& radii,
	float startTheta, float arcTheta, BGradient& gradient, bool fill)
{
	BRect rect(center.x - radii.x, center.y - radii.y,
		center.x + radii.x - 1, center.y + radii.y - 1);
	const SimpleTransform transform =
		fCanvas->PenToScreenTransform();
	transform.Apply(&rect);
	transform.Apply(&gradient);
	fCanvas->GetDrawingEngine()->DrawArc(rect, startTheta, arcTheta, fill, gradient);
}


void
CanvasCallbacks::DrawEllipseGradient(const BRect& _rect, BGradient& gradient, bool fill)
{
	BRect rect = _rect;

	const SimpleTransform transform =
		fCanvas->PenToScreenTransform();
	transform.Apply(&rect);
	transform.Apply(&gradient);
	fCanvas->GetDrawingEngine()->DrawEllipse(rect, fill, gradient);
}


void
CanvasCallbacks::DrawPolygonGradient(size_t numPoints, const BPoint viewPoints[],
	bool isClosed, BGradient& gradient, bool fill)
{
	if (numPoints == 0)
		return;

	BStackOrHeapArray<BPoint, 200> points(numPoints);
	if (!points.IsValid())
		return;

	const SimpleTransform transform =
		fCanvas->PenToScreenTransform();
	transform.Apply(points, viewPoints, numPoints);
	transform.Apply(&gradient);

	BRect polyFrame;
	get_polygon_frame(points, numPoints, &polyFrame);

	fCanvas->GetDrawingEngine()->DrawPolygon(points, numPoints, polyFrame,
		isClosed && numPoints > 2, fill, gradient);
}


void
CanvasCallbacks::DrawShapeGradient(const BShape& shape, BGradient& gradient, bool fill)
{
	ShapePainter drawShape(fCanvas, &gradient);

	drawShape.Iterate(&shape);
	drawShape.Draw(shape.Bounds(), fill);
}


void
CanvasCallbacks::StrokeLineGradient(const BPoint& _start, const BPoint& _end,
	BGradient& gradient)
{
	BPoint start = _start;
	BPoint end = _end;

	const SimpleTransform transform = fCanvas->PenToScreenTransform();
	transform.Apply(&start);
	transform.Apply(&end);
	fCanvas->GetDrawingEngine()->StrokeLine(start, end, gradient);

	fCanvas->CurrentState()->SetPenLocation(_end);
	// the DrawingEngine/Painter does not need to be updated, since this
	// effects only the view->screen coord conversion, which is handled
	// by the view only
}


void
CanvasCallbacks::DrawString(const char* string, size_t length, float deltaSpace,
	float deltaNonSpace)
{
	// NOTE: the picture data was recorded with a "set pen location"
	// command inserted before the "draw string" command, so we can
	// use PenLocation()
	BPoint location = fCanvas->CurrentState()->PenLocation();

	escapement_delta delta = { deltaSpace, deltaNonSpace };
	fCanvas->PenToScreenTransform().Apply(&location);
	location = fCanvas->GetDrawingEngine()->DrawString(string, length,
		location, &delta);

	fCanvas->PenToScreenTransform().Apply(&location);
	fCanvas->CurrentState()->SetPenLocation(location);
	// the DrawingEngine/Painter does not need to be updated, since this
	// effects only the view->screen coord conversion, which is handled
	// by the view only
}


void
CanvasCallbacks::DrawStringLocations(const char* string, size_t length,
	const BPoint* _locations, size_t locationsCount)
{
	BStackOrHeapArray<BPoint, 200> locations(locationsCount);
	if (!locations.IsValid())
		return;

	const SimpleTransform transform = fCanvas->PenToScreenTransform();
	for (size_t i = 0; i < locationsCount; i++) {
		locations[i] = _locations[i];
		transform.Apply(&locations[i]);
	}

	BPoint location = fCanvas->GetDrawingEngine()->DrawString(string, length,
		locations);

	fCanvas->PenToScreenTransform().Apply(&location);
	fCanvas->CurrentState()->SetPenLocation(location);
	// the DrawingEngine/Painter does not need to be updated, since this
	// effects only the view->screen coord conversion, which is handled
	// by the view only
}


void
CanvasCallbacks::DrawPixels(const BRect& src, const BRect& _dest, uint32 width,
	uint32 height, size_t bytesPerRow, color_space pixelFormat, uint32 options,
	const void* data, size_t length)
{
	UtilityBitmap bitmap(BRect(0, 0, width - 1, height - 1),
		(color_space)pixelFormat, 0, bytesPerRow);

	if (!bitmap.IsValid())
		return;

	memcpy(bitmap.Bits(), data, std::min(height * bytesPerRow, length));

	BRect dest = _dest;
	fCanvas->PenToScreenTransform().Apply(&dest);
	fCanvas->GetDrawingEngine()->DrawBitmap(&bitmap, src, dest, options);
}


void
CanvasCallbacks::DrawPicture(const BPoint& where, int32 token)
{
	BReference<ServerPicture> picture(fCanvas->GetPicture(token), true);
	if (picture != NULL) {
		fCanvas->PushState();
		fCanvas->SetDrawingOrigin(where);

		fCanvas->PushState();
		picture->Play(fCanvas);
		fCanvas->PopState();

		fCanvas->PopState();
	}
}


void
CanvasCallbacks::SetClippingRects(size_t numRects, const clipping_rect rects[])
{
	if (numRects == 0)
		fCanvas->SetUserClipping(NULL);
	else {
		// TODO: This might be too slow, we should copy the rects
		// directly to BRegion's internal data
		BRegion region;
		for (uint32 c = 0; c < numRects; c++)
			region.Include(rects[c]);
		fCanvas->SetUserClipping(&region);
	}
	fCanvas->UpdateCurrentDrawingRegion();
}


void
CanvasCallbacks::ClipToPicture(int32 pictureToken, const BPoint& where,
	bool clipToInverse)
{
	BReference<ServerPicture> picture(fCanvas->GetPicture(pictureToken), true);
	if (picture == NULL)
		return;
	BReference<AlphaMask> mask(new(std::nothrow) PictureAlphaMask(fCanvas->GetAlphaMask(),
		picture, *fCanvas->CurrentState(), where, clipToInverse), true);
	fCanvas->SetAlphaMask(mask);
	fCanvas->CurrentState()->GetAlphaMask()->SetCanvasGeometry(BPoint(0, 0),
		fCanvas->Bounds());
	fCanvas->ResyncDrawState();
}


void
CanvasCallbacks::PushState()
{
	fCanvas->PushState();
}


void
CanvasCallbacks::PopState()
{
	fCanvas->PopState();

	BPoint p(0, 0);
	fCanvas->PenToScreenTransform().Apply(&p);
	fCanvas->GetDrawingEngine()->SetDrawState(fCanvas->CurrentState(),
		(int32)p.x, (int32)p.y);
}


// TODO: Be smart and actually take advantage of these methods:
// only apply state changes when they are called
void
CanvasCallbacks::EnterStateChange()
{
}


void
CanvasCallbacks::ExitStateChange()
{
	fCanvas->ResyncDrawState();
}


void
CanvasCallbacks::EnterFontState()
{
}


void
CanvasCallbacks::ExitFontState()
{
	fCanvas->GetDrawingEngine()->SetFont(fCanvas->CurrentState()->Font());
}


void
CanvasCallbacks::SetOrigin(const BPoint& pt)
{
	fCanvas->CurrentState()->SetOrigin(pt);
}


void
CanvasCallbacks::SetPenLocation(const BPoint& pt)
{
	fCanvas->CurrentState()->SetPenLocation(pt);
	// the DrawingEngine/Painter does not need to be updated, since this
	// effects only the view->screen coord conversion, which is handled
	// by the view only
}


void
CanvasCallbacks::SetDrawingMode(drawing_mode mode)
{
	if (fCanvas->CurrentState()->SetDrawingMode(mode))
		fCanvas->GetDrawingEngine()->SetDrawingMode(mode);
}


void
CanvasCallbacks::SetLineMode(cap_mode capMode, join_mode joinMode,
	float miterLimit)
{
	DrawState* state = fCanvas->CurrentState();
	state->SetLineCapMode(capMode);
	state->SetLineJoinMode(joinMode);
	state->SetMiterLimit(miterLimit);
	fCanvas->GetDrawingEngine()->SetStrokeMode(capMode, joinMode, miterLimit);
}


void
CanvasCallbacks::SetPenSize(float size)
{
	fCanvas->CurrentState()->SetPenSize(size);
	fCanvas->GetDrawingEngine()->SetPenSize(
		fCanvas->CurrentState()->PenSize());
		// DrawState::PenSize() returns the scaled pen size, so we
		// need to use that value to set the drawing engine pen size.
}


void
CanvasCallbacks::SetForeColor(const rgb_color& color)
{
	fCanvas->CurrentState()->SetHighColor(color);
	fCanvas->GetDrawingEngine()->SetHighColor(color);
}


void
CanvasCallbacks::SetBackColor(const rgb_color& color)
{
	fCanvas->CurrentState()->SetLowColor(color);
	fCanvas->GetDrawingEngine()->SetLowColor(color);
}


void
CanvasCallbacks::SetStipplePattern(const pattern& pattern)
{
	fCanvas->CurrentState()->SetPattern(Pattern(pattern));
	fCanvas->GetDrawingEngine()->SetPattern(pattern);
}


void
CanvasCallbacks::SetScale(float scale)
{
	fCanvas->CurrentState()->SetScale(scale);
	fCanvas->ResyncDrawState();

	// Update the drawing engine draw state, since some stuff
	// (for example the pen size) needs to be recalculated.
}


void
CanvasCallbacks::SetFontFamily(const char* _family, size_t length)
{
	BString family(_family, length);

	gFontManager->Lock();
	FontStyle* fontStyle = gFontManager->GetStyleByIndex(family, 0);
	ServerFont font;
	font.SetStyle(fontStyle);
	gFontManager->Unlock();
	fCanvas->CurrentState()->SetFont(font, B_FONT_FAMILY_AND_STYLE);
}


void
CanvasCallbacks::SetFontStyle(const char* _style, size_t length)
{
	BString style(_style, length);

	ServerFont font(fCanvas->CurrentState()->Font());

	gFontManager->Lock();
	FontStyle* fontStyle = gFontManager->GetStyle(font.Family(), style);

	font.SetStyle(fontStyle);
	gFontManager->Unlock();
	fCanvas->CurrentState()->SetFont(font, B_FONT_FAMILY_AND_STYLE);
}


void
CanvasCallbacks::SetFontSpacing(uint8 spacing)
{
	ServerFont font;
	font.SetSpacing(spacing);
	fCanvas->CurrentState()->SetFont(font, B_FONT_SPACING);
}


void
CanvasCallbacks::SetFontSize(float size)
{
	ServerFont font;
	font.SetSize(size);
	fCanvas->CurrentState()->SetFont(font, B_FONT_SIZE);
}


void
CanvasCallbacks::SetFontRotation(float rotation)
{
	ServerFont font;
	font.SetRotation(rotation);
	fCanvas->CurrentState()->SetFont(font, B_FONT_ROTATION);
}


void
CanvasCallbacks::SetFontEncoding(uint8 encoding)
{
	ServerFont font;
	font.SetEncoding(encoding);
	fCanvas->CurrentState()->SetFont(font, B_FONT_ENCODING);
}


void
CanvasCallbacks::SetFontFlags(uint32 flags)
{
	ServerFont font;
	font.SetFlags(flags);
	fCanvas->CurrentState()->SetFont(font, B_FONT_FLAGS);
}


void
CanvasCallbacks::SetFontShear(float shear)
{
	ServerFont font;
	font.SetShear(shear);
	fCanvas->CurrentState()->SetFont(font, B_FONT_SHEAR);
}


void
CanvasCallbacks::SetFontFace(uint16 face)
{
	ServerFont font;
	font.SetFace(face);
	fCanvas->CurrentState()->SetFont(font, B_FONT_FACE);
}


void
CanvasCallbacks::SetBlendingMode(source_alpha alphaSrcMode,
	alpha_function alphaFncMode)
{
	fCanvas->CurrentState()->SetBlendingMode(alphaSrcMode, alphaFncMode);
}


void
CanvasCallbacks::SetFillRule(int32 fillRule)
{
	fCanvas->CurrentState()->SetFillRule(fillRule);
	fCanvas->GetDrawingEngine()->SetFillRule(fillRule);
}


void
CanvasCallbacks::SetTransform(const BAffineTransform& transform)
{
	BPoint leftTop(0, 0);
	fCanvas->PenToScreenTransform().Apply(&leftTop);

	fCanvas->CurrentState()->SetTransform(transform);
	fCanvas->GetDrawingEngine()->SetTransform(
		fCanvas->CurrentState()->CombinedTransform(), leftTop.x, leftTop.y);
}


void
CanvasCallbacks::TranslateBy(double x, double y)
{
	BPoint leftTop(0, 0);
	fCanvas->PenToScreenTransform().Apply(&leftTop);

	BAffineTransform transform = fCanvas->CurrentState()->Transform();
	transform.PreTranslateBy(x, y);
	fCanvas->CurrentState()->SetTransform(transform);
	fCanvas->GetDrawingEngine()->SetTransform(
		fCanvas->CurrentState()->CombinedTransform(), leftTop.x, leftTop.y);
}


void
CanvasCallbacks::ScaleBy(double x, double y)
{
	BPoint leftTop(0, 0);
	fCanvas->PenToScreenTransform().Apply(&leftTop);

	BAffineTransform transform = fCanvas->CurrentState()->Transform();
	transform.PreScaleBy(x, y);
	fCanvas->CurrentState()->SetTransform(transform);
	fCanvas->GetDrawingEngine()->SetTransform(
		fCanvas->CurrentState()->CombinedTransform(), leftTop.x, leftTop.y);
}


void
CanvasCallbacks::RotateBy(double angleRadians)
{
	BPoint leftTop(0, 0);
	fCanvas->PenToScreenTransform().Apply(&leftTop);

	BAffineTransform transform = fCanvas->CurrentState()->Transform();
	transform.PreRotateBy(angleRadians);
	fCanvas->CurrentState()->SetTransform(transform);
	fCanvas->GetDrawingEngine()->SetTransform(
		fCanvas->CurrentState()->CombinedTransform(), leftTop.x, leftTop.y);
}


void
CanvasCallbacks::BlendLayer(Layer* layer)
{
	fCanvas->BlendLayer(layer);
}


void
CanvasCallbacks::ClipToRect(const BRect& rect, bool inverse)
{
	bool needDrawStateUpdate = fCanvas->ClipToRect(rect, inverse);
	if (needDrawStateUpdate) {
		fCanvas->CurrentState()->GetAlphaMask()->SetCanvasGeometry(BPoint(0, 0),
			fCanvas->Bounds());
		fCanvas->ResyncDrawState();
	}
	fCanvas->UpdateCurrentDrawingRegion();
}


void
CanvasCallbacks::ClipToShape(int32 opCount, const uint32 opList[],
	int32 ptCount, const BPoint ptList[], bool inverse)
{
	shape_data shapeData;

	// TODO: avoid copies
	shapeData.opList = (uint32*)malloc(opCount * sizeof(uint32));
	memcpy(shapeData.opList, opList, opCount * sizeof(uint32));
	shapeData.ptList = (BPoint*)malloc(ptCount * sizeof(BPoint));
	memcpy((void*)shapeData.ptList, ptList, ptCount * sizeof(BPoint));

	shapeData.opCount = opCount;
	shapeData.opSize = opCount * sizeof(uint32);
	shapeData.ptCount = ptCount;
	shapeData.ptSize = ptCount * sizeof(BPoint);

	fCanvas->ClipToShape(&shapeData, inverse);
	fCanvas->CurrentState()->GetAlphaMask()->SetCanvasGeometry(BPoint(0, 0),
		fCanvas->Bounds());
	fCanvas->ResyncDrawState();

	free(shapeData.opList);
	free(shapeData.ptList);
}


// #pragma mark - ServerPicture


ServerPicture::ServerPicture()
	:
	fFile(NULL),
	fOwner(NULL)
{
	fToken = gTokenSpace.NewToken(kPictureToken, this);
	fData.SetTo(new(std::nothrow) BMallocIO());

	PictureDataWriter::SetTo(fData.Get());
}


ServerPicture::ServerPicture(const ServerPicture& picture)
	:
	fFile(NULL),
	fData(NULL),
	fOwner(NULL)
{
	fToken = gTokenSpace.NewToken(kPictureToken, this);

	BMallocIO* mallocIO = new(std::nothrow) BMallocIO();
	if (mallocIO == NULL)
		return;

	fData.SetTo(mallocIO);

	const off_t size = picture.DataLength();
	if (mallocIO->SetSize(size) < B_OK)
		return;

	picture.fData->ReadAt(0, const_cast<void*>(mallocIO->Buffer()),
		size);

	PictureDataWriter::SetTo(fData.Get());
}


ServerPicture::ServerPicture(const char* fileName, int32 offset)
	:
	fFile(NULL),
	fData(NULL),
	fOwner(NULL)
{
	fToken = gTokenSpace.NewToken(kPictureToken, this);

	fFile.SetTo(new(std::nothrow) BFile(fileName, B_READ_WRITE));
	if (!fFile.IsSet())
		return;

	BPrivate::Storage::OffsetFile* offsetFile
		= new(std::nothrow) BPrivate::Storage::OffsetFile(fFile.Get(), offset);
	if (offsetFile == NULL || offsetFile->InitCheck() != B_OK) {
		delete offsetFile;
		return;
	}

	fData.SetTo(offsetFile);

	PictureDataWriter::SetTo(fData.Get());
}


ServerPicture::~ServerPicture()
{
	ASSERT(fOwner == NULL);

	gTokenSpace.RemoveToken(fToken);

	if (fPictures.IsSet()) {
		for (int32 i = fPictures->CountItems(); i-- > 0;) {
			ServerPicture* picture = fPictures->ItemAt(i);
			picture->SetOwner(NULL);
			picture->ReleaseReference();
		}
	}

	if (fPushed != NULL)
		fPushed->SetOwner(NULL);
}


bool
ServerPicture::SetOwner(ServerApp* owner)
{
	if (owner == fOwner)
		return true;

	// Acquire an extra reference, since calling RemovePicture()
	// May remove the last reference and then we will self-destruct right then.
	// Setting fOwner to NULL would access free'd memory. If owner is another
	// ServerApp, it's expected to already have a reference of course.
	BReference<ServerPicture> _(this);

	if (fOwner != NULL)
		fOwner->RemovePicture(this);

	fOwner = NULL;
	if (owner == NULL)
		return true;

	if (!owner->AddPicture(this))
		return false;

	fOwner = owner;
	return true;
}


void
ServerPicture::EnterStateChange()
{
	BeginOp(B_PIC_ENTER_STATE_CHANGE);
}


void
ServerPicture::ExitStateChange()
{
	EndOp();
}


void
ServerPicture::SyncState(Canvas* canvas)
{
	// TODO: Finish this
	EnterStateChange();

	WriteSetOrigin(canvas->CurrentState()->Origin());
	WriteSetPenLocation(canvas->CurrentState()->PenLocation());
	WriteSetPenSize(canvas->CurrentState()->UnscaledPenSize());
	WriteSetScale(canvas->CurrentState()->Scale());
	WriteSetLineMode(canvas->CurrentState()->LineCapMode(),
		canvas->CurrentState()->LineJoinMode(),
		canvas->CurrentState()->MiterLimit());
	//WriteSetPattern(*canvas->CurrentState()->GetPattern().GetInt8());
	WriteSetDrawingMode(canvas->CurrentState()->GetDrawingMode());

	WriteSetHighColor(canvas->CurrentState()->HighColor());
	WriteSetLowColor(canvas->CurrentState()->LowColor());

	ExitStateChange();
}


void
ServerPicture::WriteFontState(const ServerFont& font, uint16 mask)
{
	BeginOp(B_PIC_ENTER_FONT_STATE);

	if (mask & B_FONT_FAMILY_AND_STYLE) {
		WriteSetFontFamily(font.Family());
		WriteSetFontStyle(font.Style());
	}

	if (mask & B_FONT_SIZE) {
		WriteSetFontSize(font.Size());
	}

	if (mask & B_FONT_SHEAR) {
		WriteSetFontShear(font.Shear());
	}

	if (mask & B_FONT_ROTATION) {
		WriteSetFontRotation(font.Rotation());
	}

	if (mask & B_FONT_FALSE_BOLD_WIDTH) {
		// TODO: Implement
//		WriteSetFalseBoldWidth(font.FalseBoldWidth());
	}

	if (mask & B_FONT_SPACING) {
		WriteSetFontSpacing(font.Spacing());
	}

	if (mask & B_FONT_ENCODING) {
		WriteSetFontEncoding(font.Encoding());
	}

	if (mask & B_FONT_FACE) {
		WriteSetFontFace(font.Face());
	}

	if (mask & B_FONT_FLAGS) {
		WriteSetFontFlags(font.Flags());
	}

	EndOp();
}


void
ServerPicture::Play(Canvas* target)
{
	// TODO: for now: then change PicturePlayer
	// to accept a BPositionIO object
	BMallocIO* mallocIO = dynamic_cast<BMallocIO*>(fData.Get());
	if (mallocIO == NULL)
		return;

	CanvasCallbacks callbacks(target);

	BPrivate::PicturePlayer player(mallocIO->Buffer(),
		mallocIO->BufferLength(), PictureList::Private(fPictures.Get()).AsBList());
	player.Play(callbacks);
}


/*!	Acquires a reference to the pushed picture.
*/
void
ServerPicture::PushPicture(ServerPicture* picture)
{
	if (fPushed != NULL)
		debugger("already pushed a picture");

	fPushed.SetTo(picture, false);
}


/*!	Returns a reference with the popped picture.
*/
ServerPicture*
ServerPicture::PopPicture()
{
	return fPushed.Detach();
}


void
ServerPicture::AppendPicture(ServerPicture* picture)
{
	// A pushed picture is the same as an appended one
	PushPicture(picture);
}


bool
ServerPicture::NestPicture(ServerPicture* picture)
{
	if (!fPictures.IsSet())
		fPictures.SetTo(new(std::nothrow) PictureList);

	if (!fPictures.IsSet() || !fPictures->AddItem(picture))
		return false;

	picture->AcquireReference();
	return true;
}


off_t
ServerPicture::DataLength() const
{
	if (!fData.IsSet())
		return 0;
	off_t size;
	fData->GetSize(&size);
	return size;
}


status_t
ServerPicture::ImportData(BPrivate::LinkReceiver& link)
{
	int32 size = 0;
	link.Read<int32>(&size);

	off_t oldPosition = fData->Position();
	fData->Seek(0, SEEK_SET);

	status_t status = B_NO_MEMORY;
	char* buffer = new(std::nothrow) char[size];
	if (buffer) {
		status = B_OK;
		ssize_t read = link.Read(buffer, size);
		if (read < B_OK || fData->Write(buffer, size) < B_OK)
			status = B_ERROR;
		delete [] buffer;
	}

	fData->Seek(oldPosition, SEEK_SET);
	return status;
}


status_t
ServerPicture::ExportData(BPrivate::PortLink& link)
{
	link.StartMessage(B_OK);

	off_t oldPosition = fData->Position();
	fData->Seek(0, SEEK_SET);

	int32 subPicturesCount = 0;
	if (fPictures.IsSet())
		subPicturesCount = fPictures->CountItems();
	link.Attach<int32>(subPicturesCount);
	if (subPicturesCount > 0) {
		for (int32 i = 0; i < subPicturesCount; i++) {
			ServerPicture* subPicture = fPictures->ItemAt(i);
			link.Attach<int32>(subPicture->Token());
		}
	}

	off_t size = 0;
	fData->GetSize(&size);
	link.Attach<int32>((int32)size);

	status_t status = B_NO_MEMORY;
	char* buffer = new(std::nothrow) char[size];
	if (buffer) {
		status = B_OK;
		ssize_t read = fData->Read(buffer, size);
		if (read < B_OK || link.Attach(buffer, read) < B_OK)
			status = B_ERROR;
		delete [] buffer;
	}

	if (status != B_OK) {
		link.CancelMessage();
		link.StartMessage(B_ERROR);
	}

	fData->Seek(oldPosition, SEEK_SET);
	return status;
}
