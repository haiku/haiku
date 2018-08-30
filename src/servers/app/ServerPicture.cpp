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
#include "FontManager.h"
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

#include <Bitmap.h>
#include <Debug.h>
#include <List.h>
#include <Shape.h>


using std::stack;


class ShapePainter : public BShapeIterator {
public:
	ShapePainter(Canvas* canvas);
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
	stack<uint32>	fOpStack;
	stack<BPoint>	fPtStack;
};


ShapePainter::ShapePainter(Canvas* canvas)
	:
	fCanvas(canvas)
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

		BPoint offset(fCanvas->CurrentState()->PenLocation());
		fCanvas->PenToScreenTransform().Apply(&offset);
		fCanvas->GetDrawingEngine()->DrawShape(frame, opCount, opList,
			ptCount, ptList, filled, offset, fCanvas->Scale());

		delete[] opList;
		delete[] ptList;
	}
}


// #pragma mark - drawing functions


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


static void
move_pen_by(void* _canvas, const BPoint& delta)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	canvas->CurrentState()->SetPenLocation(
		canvas->CurrentState()->PenLocation() + delta);
}


static void
stroke_line(void* _canvas, const BPoint& _start, const BPoint& _end)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	BPoint start = _start;
	BPoint end = _end;

	const SimpleTransform transform = canvas->PenToScreenTransform();
	transform.Apply(&start);
	transform.Apply(&end);
	canvas->GetDrawingEngine()->StrokeLine(start, end);

	canvas->CurrentState()->SetPenLocation(_end);
	// the DrawingEngine/Painter does not need to be updated, since this
	// effects only the view->screen coord conversion, which is handled
	// by the view only
}


static void
draw_rect(void* _canvas, const BRect& _rect, bool fill)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	BRect rect = _rect;

	canvas->PenToScreenTransform().Apply(&rect);
	if (fill)
		canvas->GetDrawingEngine()->FillRect(rect);
	else
		canvas->GetDrawingEngine()->StrokeRect(rect);
}


static void
draw_round_rect(void* _canvas, const BRect& _rect, const BPoint& radii,
	bool fill)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	BRect rect = _rect;

	canvas->PenToScreenTransform().Apply(&rect);
	float scale = canvas->CurrentState()->CombinedScale();
	canvas->GetDrawingEngine()->DrawRoundRect(rect, radii.x * scale,
		radii.y * scale, fill);
}


static void
draw_bezier(void* _canvas, size_t numPoints, const BPoint viewPoints[],
	bool fill)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);

	const size_t kSupportedPoints = 4;
	if (numPoints != kSupportedPoints)
		return;

	BPoint points[kSupportedPoints];
	canvas->PenToScreenTransform().Apply(points, viewPoints, kSupportedPoints);
	canvas->GetDrawingEngine()->DrawBezier(points, fill);
}


static void
draw_arc(void* _canvas, const BPoint& center, const BPoint& radii,
	float startTheta, float arcTheta, bool fill)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);

	BRect rect(center.x - radii.x, center.y - radii.y,
		center.x + radii.x - 1, center.y + radii.y - 1);
	canvas->PenToScreenTransform().Apply(&rect);
	canvas->GetDrawingEngine()->DrawArc(rect, startTheta, arcTheta, fill);
}


static void
draw_ellipse(void* _canvas, const BRect& _rect, bool fill)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);

	BRect rect = _rect;
	canvas->PenToScreenTransform().Apply(&rect);
	canvas->GetDrawingEngine()->DrawEllipse(rect, fill);
}


static void
draw_polygon(void* _canvas, size_t numPoints, const BPoint viewPoints[],
	bool isClosed, bool fill)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);

	if (numPoints == 0)
		return;

	const size_t kMaxStackCount = 200;
	char stackData[kMaxStackCount * sizeof(BPoint)];
	BPoint* points = (BPoint*)stackData;
	if (numPoints > kMaxStackCount) {
		points = (BPoint*)malloc(numPoints * sizeof(BPoint));
		if (points == NULL)
			return;
	}

	canvas->PenToScreenTransform().Apply(points, viewPoints, numPoints);

	BRect polyFrame;
	get_polygon_frame(points, numPoints, &polyFrame);

	canvas->GetDrawingEngine()->DrawPolygon(points, numPoints, polyFrame,
		fill, isClosed && numPoints > 2);

	if (numPoints > kMaxStackCount)
		free(points);
}


static void
draw_shape(void* _canvas, const BShape& shape, bool fill)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	ShapePainter drawShape(canvas);

	drawShape.Iterate(&shape);
	drawShape.Draw(shape.Bounds(), fill);
}


static void
draw_string(void* _canvas, const char* string, size_t length, float deltaSpace,
	float deltaNonSpace)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);

	// NOTE: the picture data was recorded with a "set pen location"
	// command inserted before the "draw string" command, so we can
	// use PenLocation()
	BPoint location = canvas->CurrentState()->PenLocation();

	escapement_delta delta = { deltaSpace, deltaNonSpace };
	canvas->PenToScreenTransform().Apply(&location);
	location = canvas->GetDrawingEngine()->DrawString(string, length,
		location, &delta);

	canvas->PenToScreenTransform().Apply(&location);
	canvas->CurrentState()->SetPenLocation(location);
	// the DrawingEngine/Painter does not need to be updated, since this
	// effects only the view->screen coord conversion, which is handled
	// by the view only
}


static void
draw_string_locations(void* _canvas, const char* string, size_t length,
	const BPoint* locations, size_t locationsCount)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);

	BPoint location = canvas->GetDrawingEngine()->DrawString(string, length,
		locations);

	canvas->PenToScreenTransform().Apply(&location);
	canvas->CurrentState()->SetPenLocation(location);
	// the DrawingEngine/Painter does not need to be updated, since this
	// effects only the view->screen coord conversion, which is handled
	// by the view only
}


static void
draw_pixels(void* _canvas, const BRect& src, const BRect& _dest, uint32 width,
	uint32 height, size_t bytesPerRow, color_space pixelFormat, uint32 options,
	const void* data, size_t length)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);

	UtilityBitmap bitmap(BRect(0, 0, width - 1, height - 1),
		(color_space)pixelFormat, 0, bytesPerRow);

	if (!bitmap.IsValid())
		return;

	memcpy(bitmap.Bits(), data, std::min(height * bytesPerRow, length));

	BRect dest = _dest;
	canvas->PenToScreenTransform().Apply(&dest);
	canvas->GetDrawingEngine()->DrawBitmap(&bitmap, src, dest, options);
}


static void
draw_picture(void* _canvas, const BPoint& where, int32 token)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);

	ServerPicture* picture = canvas->GetPicture(token);
	if (picture != NULL) {
		canvas->PushState();
		canvas->SetDrawingOrigin(where);

		canvas->PushState();
		picture->Play(canvas);
		canvas->PopState();

		canvas->PopState();
		picture->ReleaseReference();
	}
}


static void
set_clipping_rects(void* _canvas, size_t numRects, const BRect rects[])
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);

	if (numRects == 0)
		canvas->SetUserClipping(NULL);
	else {
		// TODO: This might be too slow, we should copy the rects
		// directly to BRegion's internal data
		BRegion region;
		for (uint32 c = 0; c < numRects; c++)
			region.Include(rects[c]);
		canvas->SetUserClipping(&region);
	}
	canvas->UpdateCurrentDrawingRegion();
}


static void
clip_to_picture(void* _canvas, int32 pictureToken, const BPoint& where,
	bool clipToInverse)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);

	ServerPicture* picture = canvas->GetPicture(pictureToken);
	if (picture == NULL)
		return;
	AlphaMask* mask = new(std::nothrow) PictureAlphaMask(canvas->GetAlphaMask(), 
		picture, *canvas->CurrentState(), where, clipToInverse);
	canvas->SetAlphaMask(mask);
	canvas->CurrentState()->GetAlphaMask()->SetCanvasGeometry(BPoint(0, 0),
		canvas->Bounds());
	canvas->ResyncDrawState();
	if (mask != NULL)
		mask->ReleaseReference();

	picture->ReleaseReference();
}


static void
push_state(void* _canvas)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	canvas->PushState();
}


static void
pop_state(void* _canvas)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	canvas->PopState();

	BPoint p(0, 0);
	canvas->PenToScreenTransform().Apply(&p);
	canvas->GetDrawingEngine()->SetDrawState(canvas->CurrentState(),
		(int32)p.x, (int32)p.y);
}


// TODO: Be smart and actually take advantage of these methods:
// only apply state changes when they are called
static void
enter_state_change(void* _canvas)
{
}


static void
exit_state_change(void* _canvas)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	canvas->ResyncDrawState();
}


static void
enter_font_state(void* _canvas)
{
}


static void
exit_font_state(void* _canvas)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	canvas->GetDrawingEngine()->SetFont(canvas->CurrentState()->Font());
}


static void
set_origin(void* _canvas, const BPoint& pt)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	canvas->CurrentState()->SetOrigin(pt);
}


static void
set_pen_location(void* _canvas, const BPoint& pt)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	canvas->CurrentState()->SetPenLocation(pt);
	// the DrawingEngine/Painter does not need to be updated, since this
	// effects only the view->screen coord conversion, which is handled
	// by the view only
}


static void
set_drawing_mode(void* _canvas, drawing_mode mode)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	if (canvas->CurrentState()->SetDrawingMode(mode))
		canvas->GetDrawingEngine()->SetDrawingMode(mode);
}


static void
set_line_mode(void* _canvas, cap_mode capMode, join_mode joinMode,
	float miterLimit)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	DrawState* state = canvas->CurrentState();
	state->SetLineCapMode(capMode);
	state->SetLineJoinMode(joinMode);
	state->SetMiterLimit(miterLimit);
	canvas->GetDrawingEngine()->SetStrokeMode(capMode, joinMode, miterLimit);
}


static void
set_pen_size(void* _canvas, float size)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	canvas->CurrentState()->SetPenSize(size);
	canvas->GetDrawingEngine()->SetPenSize(
		canvas->CurrentState()->PenSize());
		// DrawState::PenSize() returns the scaled pen size, so we
		// need to use that value to set the drawing engine pen size.
}


static void
set_fore_color(void* _canvas, const rgb_color& color)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	canvas->CurrentState()->SetHighColor(color);
	canvas->GetDrawingEngine()->SetHighColor(color);
}


static void
set_back_color(void* _canvas, const rgb_color& color)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	canvas->CurrentState()->SetLowColor(color);
	canvas->GetDrawingEngine()->SetLowColor(color);
}


static void
set_stipple_pattern(void* _canvas, const pattern& pattern)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	canvas->CurrentState()->SetPattern(Pattern(pattern));
	canvas->GetDrawingEngine()->SetPattern(pattern);
}


static void
set_scale(void* _canvas, float scale)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	canvas->CurrentState()->SetScale(scale);
	canvas->ResyncDrawState();

	// Update the drawing engine draw state, since some stuff
	// (for example the pen size) needs to be recalculated.
}


static void
set_font_family(void* _canvas, const char* _family, size_t length)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	BString family(_family, length);

	FontStyle* fontStyle = gFontManager->GetStyleByIndex(family, 0);
	ServerFont font;
	font.SetStyle(fontStyle);
	canvas->CurrentState()->SetFont(font, B_FONT_FAMILY_AND_STYLE);
}


static void
set_font_style(void* _canvas, const char* _style, size_t length)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	BString style(_style, length);

	ServerFont font(canvas->CurrentState()->Font());

	FontStyle* fontStyle = gFontManager->GetStyle(font.Family(), style);

	font.SetStyle(fontStyle);
	canvas->CurrentState()->SetFont(font, B_FONT_FAMILY_AND_STYLE);
}


static void
set_font_spacing(void* _canvas, uint8 spacing)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	ServerFont font;
	font.SetSpacing(spacing);
	canvas->CurrentState()->SetFont(font, B_FONT_SPACING);
}


static void
set_font_size(void* _canvas, float size)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	ServerFont font;
	font.SetSize(size);
	canvas->CurrentState()->SetFont(font, B_FONT_SIZE);
}


static void
set_font_rotation(void* _canvas, float rotation)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	ServerFont font;
	font.SetRotation(rotation);
	canvas->CurrentState()->SetFont(font, B_FONT_ROTATION);
}


static void
set_font_encoding(void* _canvas, uint8 encoding)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	ServerFont font;
	font.SetEncoding(encoding);
	canvas->CurrentState()->SetFont(font, B_FONT_ENCODING);
}


static void
set_font_flags(void* _canvas, uint32 flags)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	ServerFont font;
	font.SetFlags(flags);
	canvas->CurrentState()->SetFont(font, B_FONT_FLAGS);
}


static void
set_font_shear(void* _canvas, float shear)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	ServerFont font;
	font.SetShear(shear);
	canvas->CurrentState()->SetFont(font, B_FONT_SHEAR);
}


static void
set_font_face(void* _canvas, uint16 face)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	ServerFont font;
	font.SetFace(face);
	canvas->CurrentState()->SetFont(font, B_FONT_FACE);
}


static void
set_blending_mode(void* _canvas, source_alpha alphaSrcMode,
	alpha_function alphaFncMode)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	canvas->CurrentState()->SetBlendingMode(alphaSrcMode, alphaFncMode);
}


static void
set_transform(void* _canvas, const BAffineTransform& transform)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	canvas->CurrentState()->SetTransform(transform);
	canvas->GetDrawingEngine()->SetTransform(transform);
}


static void
translate_by(void* _canvas, double x, double y)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	BAffineTransform transform = canvas->CurrentState()->Transform();
	transform.PreTranslateBy(x, y);
	canvas->CurrentState()->SetTransform(transform);
	canvas->GetDrawingEngine()->SetTransform(transform);
}


static void
scale_by(void* _canvas, double x, double y)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	BAffineTransform transform = canvas->CurrentState()->Transform();
	transform.PreScaleBy(x, y);
	canvas->CurrentState()->SetTransform(transform);
	canvas->GetDrawingEngine()->SetTransform(transform);
}


static void
rotate_by(void* _canvas, double angleRadians)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	BAffineTransform transform = canvas->CurrentState()->Transform();
	transform.PreRotateBy(angleRadians);
	canvas->CurrentState()->SetTransform(transform);
	canvas->GetDrawingEngine()->SetTransform(transform);
}


static void
blend_layer(void* _canvas, Layer* layer)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	canvas->BlendLayer(layer);
}


static void
clip_to_rect(void* _canvas, const BRect& rect, bool inverse)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	bool needDrawStateUpdate = canvas->ClipToRect(rect, inverse);
	if (needDrawStateUpdate) {
		canvas->CurrentState()->GetAlphaMask()->SetCanvasGeometry(BPoint(0, 0),
			canvas->Bounds());
		canvas->ResyncDrawState();
	}
	canvas->UpdateCurrentDrawingRegion();
}


static void
clip_to_shape(void* _canvas, int32 opCount, const uint32 opList[],
	int32 ptCount, const BPoint ptList[], bool inverse)
{
	Canvas* const canvas = reinterpret_cast<Canvas*>(_canvas);
	shape_data shapeData;

	// TODO: avoid copies
	shapeData.opList = (uint32*)malloc(opCount * sizeof(uint32));
	memcpy(shapeData.opList, opList, opCount * sizeof(uint32));
	shapeData.ptList = (BPoint*)malloc(ptCount * sizeof(BPoint));
	memcpy(shapeData.ptList, ptList, ptCount * sizeof(BPoint));

	shapeData.opCount = opCount;
	shapeData.opSize = opCount * sizeof(uint32);
	shapeData.ptCount = ptCount;
	shapeData.ptSize = ptCount * sizeof(BPoint);

	canvas->ClipToShape(&shapeData, inverse);
	canvas->CurrentState()->GetAlphaMask()->SetCanvasGeometry(BPoint(0, 0),
		canvas->Bounds());
	canvas->ResyncDrawState();

	free(shapeData.opList);
	free(shapeData.ptList);
}


static const BPrivate::picture_player_callbacks kPicturePlayerCallbacks = {
	move_pen_by,
	stroke_line,
	draw_rect,
	draw_round_rect,
	draw_bezier,
	draw_arc,
	draw_ellipse,
	draw_polygon,
	draw_shape,
	draw_string,
	draw_pixels,
	draw_picture,
	set_clipping_rects,
	clip_to_picture,
	push_state,
	pop_state,
	enter_state_change,
	exit_state_change,
	enter_font_state,
	exit_font_state,
	set_origin,
	set_pen_location,
	set_drawing_mode,
	set_line_mode,
	set_pen_size,
	set_fore_color,
	set_back_color,
	set_stipple_pattern,
	set_scale,
	set_font_family,
	set_font_style,
	set_font_spacing,
	set_font_size,
	set_font_rotation,
	set_font_encoding,
	set_font_flags,
	set_font_shear,
	set_font_face,
	set_blending_mode,
	set_transform,
	translate_by,
	scale_by,
	rotate_by,
	blend_layer,
	clip_to_rect,
	clip_to_shape,
	draw_string_locations
};


// #pragma mark - ServerPicture


ServerPicture::ServerPicture()
	:
	fFile(NULL),
	fPictures(NULL),
	fPushed(NULL),
	fOwner(NULL)
{
	fToken = gTokenSpace.NewToken(kPictureToken, this);
	fData = new(std::nothrow) BMallocIO();

	PictureDataWriter::SetTo(fData);
}


ServerPicture::ServerPicture(const ServerPicture& picture)
	:
	fFile(NULL),
	fData(NULL),
	fPictures(NULL),
	fPushed(NULL),
	fOwner(NULL)
{
	fToken = gTokenSpace.NewToken(kPictureToken, this);

	BMallocIO* mallocIO = new(std::nothrow) BMallocIO();
	if (mallocIO == NULL)
		return;

	fData = mallocIO;

	const off_t size = picture.DataLength();
	if (mallocIO->SetSize(size) < B_OK)
		return;

	picture.fData->ReadAt(0, const_cast<void*>(mallocIO->Buffer()),
		size);

	PictureDataWriter::SetTo(fData);
}


ServerPicture::ServerPicture(const char* fileName, int32 offset)
	:
	fFile(NULL),
	fData(NULL),
	fPictures(NULL),
	fPushed(NULL),
	fOwner(NULL)
{
	fToken = gTokenSpace.NewToken(kPictureToken, this);

	fFile = new(std::nothrow) BFile(fileName, B_READ_WRITE);
	if (fFile == NULL)
		return;

	BPrivate::Storage::OffsetFile* offsetFile
		= new(std::nothrow) BPrivate::Storage::OffsetFile(fFile, offset);
	if (offsetFile == NULL || offsetFile->InitCheck() != B_OK) {
		delete offsetFile;
		return;
	}

	fData = offsetFile;

	PictureDataWriter::SetTo(fData);
}


ServerPicture::~ServerPicture()
{
	ASSERT(fOwner == NULL);

	delete fData;
	delete fFile;
	gTokenSpace.RemoveToken(fToken);

	if (fPictures != NULL) {
		for (int32 i = fPictures->CountItems(); i-- > 0;) {
			ServerPicture* picture = fPictures->ItemAt(i);
			picture->SetOwner(NULL);
			picture->ReleaseReference();
		}

		delete fPictures;
	}

	if (fPushed != NULL) {
		fPushed->SetOwner(NULL);
		fPushed->ReleaseReference();
	}
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
	BMallocIO* mallocIO = dynamic_cast<BMallocIO*>(fData);
	if (mallocIO == NULL)
		return;

	BPrivate::PicturePlayer player(mallocIO->Buffer(),
		mallocIO->BufferLength(), PictureList::Private(fPictures).AsBList());
	player.Play(kPicturePlayerCallbacks, sizeof(kPicturePlayerCallbacks),
		target);
}


/*!	Acquires a reference to the pushed picture.
*/
void
ServerPicture::PushPicture(ServerPicture* picture)
{
	if (fPushed != NULL)
		debugger("already pushed a picture");

	fPushed = picture;
	fPushed->AcquireReference();
}


/*!	Returns a reference with the popped picture.
*/
ServerPicture*
ServerPicture::PopPicture()
{
	ServerPicture* old = fPushed;
	fPushed = NULL;
	return old;
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
	if (fPictures == NULL)
		fPictures = new(std::nothrow) PictureList;

	if (fPictures == NULL || !fPictures->AddItem(picture))
		return false;

	picture->AcquireReference();
	return true;
}


off_t
ServerPicture::DataLength() const
{
	if (fData == NULL)
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
	if (fPictures != NULL)
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
