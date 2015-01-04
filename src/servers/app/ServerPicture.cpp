/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 *		Marcus Overhagen <marcus@overhagen.de>
 */

#include "ServerPicture.h"

#include <new>
#include <stdio.h>
#include <stack>

#include "DrawingEngine.h"
#include "DrawState.h"
#include "FontManager.h"
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
	} catch (std::bad_alloc) {
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
	} catch (std::bad_alloc) {
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
	} catch (std::bad_alloc) {
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
	} catch (std::bad_alloc) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
ShapePainter::IterateClose()
{
	try {
		fOpStack.push(OP_CLOSE);
	} catch (std::bad_alloc) {
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
		fCanvas->ConvertToScreenForDrawing(&offset);
		fCanvas->GetDrawingEngine()->DrawShape(frame, opCount, opList,
			ptCount, ptList, filled, offset, fCanvas->Scale());

		delete[] opList;
		delete[] ptList;
	}
}


// #pragma mark - drawing functions


static void
get_polygon_frame(const BPoint* points, int32 numPoints, BRect* _frame)
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
nop()
{
}


static void
move_pen_by(Canvas* canvas, BPoint delta)
{
	canvas->CurrentState()->SetPenLocation(
		canvas->CurrentState()->PenLocation() + delta);
}


static void
stroke_line(Canvas* canvas, BPoint start, BPoint end)
{
	BPoint penPos = end;

	canvas->ConvertToScreenForDrawing(&start);
	canvas->ConvertToScreenForDrawing(&end);
	canvas->GetDrawingEngine()->StrokeLine(start, end);

	canvas->CurrentState()->SetPenLocation(penPos);
	// the DrawingEngine/Painter does not need to be updated, since this
	// effects only the view->screen coord conversion, which is handled
	// by the view only
}


static void
stroke_rect(Canvas* canvas, BRect rect)
{
	canvas->ConvertToScreenForDrawing(&rect);
	canvas->GetDrawingEngine()->StrokeRect(rect);
}


static void
fill_rect(Canvas* canvas, BRect rect)
{
	canvas->ConvertToScreenForDrawing(&rect);
	canvas->GetDrawingEngine()->FillRect(rect);
}


static void
draw_round_rect(Canvas* canvas, BRect rect, BPoint radii, bool fill)
{
	canvas->ConvertToScreenForDrawing(&rect);
	float scale = canvas->CurrentState()->CombinedScale();
	canvas->GetDrawingEngine()->DrawRoundRect(rect, radii.x * scale,
		radii.y * scale, fill);
}


static void
stroke_round_rect(Canvas* canvas, BRect rect, BPoint radii)
{
	draw_round_rect(canvas, rect, radii, false);
}


static void
fill_round_rect(Canvas* canvas, BRect rect, BPoint radii)
{
	draw_round_rect(canvas, rect, radii, true);
}


static void
stroke_bezier(Canvas* canvas, const BPoint* viewPoints)
{
	BPoint points[4];
	canvas->ConvertToScreenForDrawing(points, viewPoints, 4);

	canvas->GetDrawingEngine()->DrawBezier(points, false);
}


static void
fill_bezier(Canvas* canvas, const BPoint* viewPoints)
{
	BPoint points[4];
	canvas->ConvertToScreenForDrawing(points, viewPoints, 4);

	canvas->GetDrawingEngine()->DrawBezier(points, true);
}


static void
stroke_arc(Canvas* canvas, BPoint center, BPoint radii,
	float startTheta, float arcTheta)
{
	BRect rect(center.x - radii.x, center.y - radii.y,
		center.x + radii.x - 1, center.y + radii.y - 1);
	canvas->ConvertToScreenForDrawing(&rect);
	canvas->GetDrawingEngine()->DrawArc(rect, startTheta, arcTheta, false);
}


static void
fill_arc(Canvas* canvas, BPoint center, BPoint radii,
	float startTheta, float arcTheta)
{
	BRect rect(center.x - radii.x, center.y - radii.y,
		center.x + radii.x - 1, center.y + radii.y - 1);
	canvas->ConvertToScreenForDrawing(&rect);
	canvas->GetDrawingEngine()->DrawArc(rect, startTheta, arcTheta, true);
}


static void
stroke_ellipse(Canvas* canvas, BPoint center, BPoint radii)
{
	BRect rect(center.x - radii.x, center.y - radii.y,
		center.x + radii.x - 1, center.y + radii.y - 1);
	canvas->ConvertToScreenForDrawing(&rect);
	canvas->GetDrawingEngine()->DrawEllipse(rect, false);
}


static void
fill_ellipse(Canvas* canvas, BPoint center, BPoint radii)
{
	BRect rect(center.x - radii.x, center.y - radii.y,
		center.x + radii.x - 1, center.y + radii.y - 1);
	canvas->ConvertToScreenForDrawing(&rect);
	canvas->GetDrawingEngine()->DrawEllipse(rect, true);
}


static void
stroke_polygon(Canvas* canvas, int32 numPoints,
	const BPoint* viewPoints, bool isClosed)
{
	if (numPoints <= 0)
		return;

	if (numPoints <= 200) {
		// fast path: no malloc/free, also avoid
		// constructor/destructor calls
		char data[200 * sizeof(BPoint)];
		BPoint* points = (BPoint*)data;

		canvas->ConvertToScreenForDrawing(points, viewPoints, numPoints);

		BRect polyFrame;
		get_polygon_frame(points, numPoints, &polyFrame);

		canvas->GetDrawingEngine()->DrawPolygon(points, numPoints, polyFrame,
			false, isClosed && numPoints > 2);
	} else {
		 // avoid constructor/destructor calls by
		 // using malloc instead of new []
		BPoint* points = (BPoint*)malloc(numPoints * sizeof(BPoint));
		if (points == NULL)
			return;

		canvas->ConvertToScreenForDrawing(points, viewPoints, numPoints);

		BRect polyFrame;
		get_polygon_frame(points, numPoints, &polyFrame);

		canvas->GetDrawingEngine()->DrawPolygon(points, numPoints, polyFrame,
			false, isClosed && numPoints > 2);
		free(points);
	}
}


static void
fill_polygon(Canvas* canvas, int32 numPoints,
	const BPoint* viewPoints)
{
	if (numPoints <= 0)
		return;

	if (numPoints <= 200) {
		// fast path: no malloc/free, also avoid
		// constructor/destructor calls
		char data[200 * sizeof(BPoint)];
		BPoint* points = (BPoint*)data;

		canvas->ConvertToScreenForDrawing(points, viewPoints, numPoints);

		BRect polyFrame;
		get_polygon_frame(points, numPoints, &polyFrame);

		canvas->GetDrawingEngine()->DrawPolygon(points, numPoints, polyFrame,
			true, true);
	} else {
		 // avoid constructor/destructor calls by
		 // using malloc instead of new []
		BPoint* points = (BPoint*)malloc(numPoints * sizeof(BPoint));
		if (points == NULL)
			return;

		canvas->ConvertToScreenForDrawing(points, viewPoints, numPoints);

		BRect polyFrame;
		get_polygon_frame(points, numPoints, &polyFrame);

		canvas->GetDrawingEngine()->DrawPolygon(points, numPoints, polyFrame,
			true, true);
		free(points);
	}
}


static void
stroke_shape(Canvas* canvas, const BShape* shape)
{
	ShapePainter drawShape(canvas);

	drawShape.Iterate(shape);
	drawShape.Draw(shape->Bounds(), false);
}


static void
fill_shape(Canvas* canvas, const BShape* shape)
{
	ShapePainter drawShape(canvas);

	drawShape.Iterate(shape);
	drawShape.Draw(shape->Bounds(), true);
}


static void
draw_string(Canvas* canvas, const char* string, float deltaSpace,
	float deltaNonSpace)
{
	// NOTE: the picture data was recorded with a "set pen location"
	// command inserted before the "draw string" command, so we can
	// use PenLocation()
	BPoint location = canvas->CurrentState()->PenLocation();

	escapement_delta delta = { deltaSpace, deltaNonSpace };
	canvas->ConvertToScreenForDrawing(&location);
	location = canvas->GetDrawingEngine()->DrawString(string, strlen(string),
		location, &delta);

	canvas->ConvertFromScreenForDrawing(&location);
	canvas->CurrentState()->SetPenLocation(location);
	// the DrawingEngine/Painter does not need to be updated, since this
	// effects only the view->screen coord conversion, which is handled
	// by the view only
}


static void
draw_pixels(Canvas* canvas, BRect src, BRect dest, int32 width,
	int32 height, int32 bytesPerRow, int32 pixelFormat, int32 options,
	const void* data)
{
	UtilityBitmap bitmap(BRect(0, 0, width - 1, height - 1),
		(color_space)pixelFormat, 0, bytesPerRow);

	if (!bitmap.IsValid())
		return;

	memcpy(bitmap.Bits(), data, height * bytesPerRow);

	canvas->ConvertToScreenForDrawing(&dest);
	canvas->GetDrawingEngine()->DrawBitmap(&bitmap, src, dest, options);
}


static void
draw_picture(Canvas* canvas, BPoint where, int32 token)
{
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
set_clipping_rects(Canvas* canvas, const BRect* rects,
	uint32 numRects)
{
	// TODO: This might be too slow, we should copy the rects
	// directly to BRegion's internal data
	BRegion region;
	for (uint32 c = 0; c < numRects; c++)
		region.Include(rects[c]);
	canvas->SetUserClipping(&region);
	canvas->UpdateCurrentDrawingRegion();
}


static void
clip_to_picture(Canvas* canvas, BPicture* picture, BPoint pt,
	bool clipToInverse)
{
	printf("ClipToPicture(picture, BPoint(%.2f, %.2f), %s)\n",
		pt.x, pt.y, clipToInverse ? "inverse" : "");
}


static void
push_state(Canvas* canvas)
{
	canvas->PushState();
}


static void
pop_state(Canvas* canvas)
{
	canvas->PopState();

	BPoint p(0, 0);
	canvas->ConvertToScreenForDrawing(&p);
	canvas->GetDrawingEngine()->SetDrawState(canvas->CurrentState(),
		(int32)p.x, (int32)p.y);
}


// TODO: Be smart and actually take advantage of these methods:
// only apply state changes when they are called
static void
enter_state_change(Canvas* canvas)
{
}


static void
exit_state_change(Canvas* canvas)
{
	canvas->ResyncDrawState();
}


static void
enter_font_state(Canvas* canvas)
{
}


static void
exit_font_state(Canvas* canvas)
{
	canvas->GetDrawingEngine()->SetFont(canvas->CurrentState()->Font());
}


static void
set_origin(Canvas* canvas, BPoint pt)
{
	canvas->CurrentState()->SetOrigin(pt);
}


static void
set_pen_location(Canvas* canvas, BPoint pt)
{
	canvas->CurrentState()->SetPenLocation(pt);
	// the DrawingEngine/Painter does not need to be updated, since this
	// effects only the view->screen coord conversion, which is handled
	// by the view only
}


static void
set_drawing_mode(Canvas* canvas, drawing_mode mode)
{
	canvas->CurrentState()->SetDrawingMode(mode);
	canvas->GetDrawingEngine()->SetDrawingMode(mode);
}


static void
set_line_mode(Canvas* canvas, cap_mode capMode, join_mode joinMode,
	float miterLimit)
{
	DrawState* state = canvas->CurrentState();
	state->SetLineCapMode(capMode);
	state->SetLineJoinMode(joinMode);
	state->SetMiterLimit(miterLimit);
	canvas->GetDrawingEngine()->SetStrokeMode(capMode, joinMode, miterLimit);
}


static void
set_pen_size(Canvas* canvas, float size)
{
	canvas->CurrentState()->SetPenSize(size);
	canvas->GetDrawingEngine()->SetPenSize(
		canvas->CurrentState()->PenSize());
		// DrawState::PenSize() returns the scaled pen size, so we
		// need to use that value to set the drawing engine pen size.
}


static void
set_fore_color(Canvas* canvas, rgb_color color)
{
	canvas->CurrentState()->SetHighColor(color);
	canvas->GetDrawingEngine()->SetHighColor(color);
}


static void
set_back_color(Canvas* canvas, rgb_color color)
{
	canvas->CurrentState()->SetLowColor(color);
	canvas->GetDrawingEngine()->SetLowColor(color);
}


static void
set_stipple_pattern(Canvas* canvas, pattern p)
{
	canvas->CurrentState()->SetPattern(Pattern(p));
	canvas->GetDrawingEngine()->SetPattern(p);
}


static void
set_scale(Canvas* canvas, float scale)
{
	canvas->CurrentState()->SetScale(scale);
	canvas->ResyncDrawState();

	// Update the drawing engine draw state, since some stuff
	// (for example the pen size) needs to be recalculated.
}


static void
set_font_family(Canvas* canvas, const char* family)
{
	FontStyle* fontStyle = gFontManager->GetStyleByIndex(family, 0);
	ServerFont font;
	font.SetStyle(fontStyle);
	canvas->CurrentState()->SetFont(font, B_FONT_FAMILY_AND_STYLE);
}


static void
set_font_style(Canvas* canvas, const char* style)
{
	ServerFont font(canvas->CurrentState()->Font());

	FontStyle* fontStyle = gFontManager->GetStyle(font.Family(), style);

	font.SetStyle(fontStyle);
	canvas->CurrentState()->SetFont(font, B_FONT_FAMILY_AND_STYLE);
}


static void
set_font_spacing(Canvas* canvas, int32 spacing)
{
	ServerFont font;
	font.SetSpacing(spacing);
	canvas->CurrentState()->SetFont(font, B_FONT_SPACING);
}


static void
set_font_size(Canvas* canvas, float size)
{
	ServerFont font;
	font.SetSize(size);
	canvas->CurrentState()->SetFont(font, B_FONT_SIZE);
}


static void
set_font_rotate(Canvas* canvas, float rotation)
{
	ServerFont font;
	font.SetRotation(rotation);
	canvas->CurrentState()->SetFont(font, B_FONT_ROTATION);
}


static void
set_font_encoding(Canvas* canvas, int32 encoding)
{
	ServerFont font;
	font.SetEncoding(encoding);
	canvas->CurrentState()->SetFont(font, B_FONT_ENCODING);
}


static void
set_font_flags(Canvas* canvas, int32 flags)
{
	ServerFont font;
	font.SetFlags(flags);
	canvas->CurrentState()->SetFont(font, B_FONT_FLAGS);
}


static void
set_font_shear(Canvas* canvas, float shear)
{
	ServerFont font;
	font.SetShear(shear);
	canvas->CurrentState()->SetFont(font, B_FONT_SHEAR);
}


static void
set_font_face(Canvas* canvas, int32 face)
{
	ServerFont font;
	font.SetFace(face);
	canvas->CurrentState()->SetFont(font, B_FONT_FACE);
}


static void
set_blending_mode(Canvas* canvas, int16 alphaSrcMode, int16 alphaFncMode)
{
	canvas->CurrentState()->SetBlendingMode((source_alpha)alphaSrcMode,
		(alpha_function)alphaFncMode);
}


static void
reserved()
{
}


const static void* kTableEntries[] = {
	(const void*)nop,					//	0
	(const void*)move_pen_by,
	(const void*)stroke_line,
	(const void*)stroke_rect,
	(const void*)fill_rect,
	(const void*)stroke_round_rect,	//	5
	(const void*)fill_round_rect,
	(const void*)stroke_bezier,
	(const void*)fill_bezier,
	(const void*)stroke_arc,
	(const void*)fill_arc,				//	10
	(const void*)stroke_ellipse,
	(const void*)fill_ellipse,
	(const void*)stroke_polygon,
	(const void*)fill_polygon,
	(const void*)stroke_shape,			//	15
	(const void*)fill_shape,
	(const void*)draw_string,
	(const void*)draw_pixels,
	(const void*)draw_picture,
	(const void*)set_clipping_rects,	//	20
	(const void*)clip_to_picture,
	(const void*)push_state,
	(const void*)pop_state,
	(const void*)enter_state_change,
	(const void*)exit_state_change,	//	25
	(const void*)enter_font_state,
	(const void*)exit_font_state,
	(const void*)set_origin,
	(const void*)set_pen_location,
	(const void*)set_drawing_mode,		//	30
	(const void*)set_line_mode,
	(const void*)set_pen_size,
	(const void*)set_fore_color,
	(const void*)set_back_color,
	(const void*)set_stipple_pattern,	//	35
	(const void*)set_scale,
	(const void*)set_font_family,
	(const void*)set_font_style,
	(const void*)set_font_spacing,
	(const void*)set_font_size,		//	40
	(const void*)set_font_rotate,
	(const void*)set_font_encoding,
	(const void*)set_font_flags,
	(const void*)set_font_shear,
	(const void*)reserved,				//	45
	(const void*)set_font_face,
	(const void*)set_blending_mode		//	47
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
ServerPicture::SyncState(View* view)
{
	// TODO: Finish this
	EnterStateChange();

	WriteSetOrigin(view->CurrentState()->Origin());
	WriteSetPenLocation(view->CurrentState()->PenLocation());
	WriteSetPenSize(view->CurrentState()->PenSize());
	WriteSetScale(view->CurrentState()->Scale());
	WriteSetLineMode(view->CurrentState()->LineCapMode(),
		view->CurrentState()->LineJoinMode(),
		view->CurrentState()->MiterLimit());
	//WriteSetPattern(*view->CurrentState()->GetPattern().GetInt8());
	WriteSetDrawingMode(view->CurrentState()->GetDrawingMode());

	WriteSetHighColor(view->CurrentState()->HighColor());
	WriteSetLowColor(view->CurrentState()->LowColor());

	ExitStateChange();
}


void
ServerPicture::SetFontFromLink(BPrivate::LinkReceiver& link)
{
	BeginOp(B_PIC_ENTER_FONT_STATE);

	uint16 mask;
	link.Read<uint16>(&mask);

	if (mask & B_FONT_FAMILY_AND_STYLE) {
		uint32 fontID;
		link.Read<uint32>(&fontID);
		ServerFont font;
		font.SetFamilyAndStyle(fontID);
		WriteSetFontFamily(font.Family());
		WriteSetFontStyle(font.Style());
	}

	if (mask & B_FONT_SIZE) {
		float size;
		link.Read<float>(&size);
		WriteSetFontSize(size);
	}

	if (mask & B_FONT_SHEAR) {
		float shear;
		link.Read<float>(&shear);
		WriteSetFontShear(shear);
	}

	if (mask & B_FONT_ROTATION) {
		float rotation;
		link.Read<float>(&rotation);
		WriteSetFontRotation(rotation);
	}

	if (mask & B_FONT_FALSE_BOLD_WIDTH) {
		float falseBoldWidth;
		link.Read<float>(&falseBoldWidth);
		//SetFalseBoldWidth(falseBoldWidth);
	}

	if (mask & B_FONT_SPACING) {
		uint8 spacing;
		link.Read<uint8>(&spacing);
		WriteSetFontSpacing(spacing);
	}

	if (mask & B_FONT_ENCODING) {
		uint8 encoding;
		link.Read<uint8>((uint8*)&encoding);
		WriteSetFontEncoding(encoding);
	}

	if (mask & B_FONT_FACE) {
		uint16 face;
		link.Read<uint16>(&face);
		WriteSetFontFace(face);
	}

	if (mask & B_FONT_FLAGS) {
		uint32 flags;
		link.Read<uint32>(&flags);
		WriteSetFontFlags(flags);
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
	player.Play(const_cast<void**>(kTableEntries),
		sizeof(kTableEntries) / sizeof(void*), target);
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
