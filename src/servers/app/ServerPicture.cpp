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

#include "AlphaMask.h"
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
	ShapePainter(DrawingContext* context);
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
	DrawingContext*	fContext;
	stack<uint32>	fOpStack;
	stack<BPoint>	fPtStack;
};


ShapePainter::ShapePainter(DrawingContext* context)
	:
	fContext(context)
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

		BPoint offset(fContext->CurrentState()->PenLocation());
		fContext->ConvertToScreenForDrawing(&offset);
		fContext->GetDrawingEngine()->DrawShape(frame, opCount, opList,
			ptCount, ptList, filled, offset, fContext->Scale());

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
move_pen_by(void* _context, const BPoint& delta)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	context->CurrentState()->SetPenLocation(
		context->CurrentState()->PenLocation() + delta);
}


static void
stroke_line(void* _context, const BPoint& _start, const BPoint& _end)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	BPoint start = _start;
	BPoint end = _end;

	context->ConvertToScreenForDrawing(&start);
	context->ConvertToScreenForDrawing(&end);
	context->GetDrawingEngine()->StrokeLine(start, end);

	context->CurrentState()->SetPenLocation(_end);
	// the DrawingEngine/Painter does not need to be updated, since this
	// effects only the view->screen coord conversion, which is handled
	// by the view only
}


static void
draw_rect(void* _context, const BRect& _rect, bool fill)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	BRect rect = _rect;

	context->ConvertToScreenForDrawing(&rect);
	if (fill)
		context->GetDrawingEngine()->FillRect(rect);
	else
		context->GetDrawingEngine()->StrokeRect(rect);
}


static void
draw_round_rect(void* _context, const BRect& _rect, const BPoint& radii,
	bool fill)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	BRect rect = _rect;

	context->ConvertToScreenForDrawing(&rect);
	float scale = context->CurrentState()->CombinedScale();
	context->GetDrawingEngine()->DrawRoundRect(rect, radii.x * scale,
		radii.y * scale, fill);
}


static void
draw_bezier(void* _context, size_t numPoints, const BPoint viewPoints[],
	bool fill)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);

	const size_t kSupportedPoints = 4;
	if (numPoints != kSupportedPoints)
		return;

	BPoint points[kSupportedPoints];
	context->ConvertToScreenForDrawing(points, viewPoints, kSupportedPoints);

	context->GetDrawingEngine()->DrawBezier(points, fill);
}


static void
draw_arc(void* _context, const BPoint& center, const BPoint& radii,
	float startTheta, float arcTheta, bool fill)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);

	BRect rect(center.x - radii.x, center.y - radii.y,
		center.x + radii.x - 1, center.y + radii.y - 1);
	context->ConvertToScreenForDrawing(&rect);
	context->GetDrawingEngine()->DrawArc(rect, startTheta, arcTheta, fill);
}


static void
draw_ellipse(void* _context, const BRect& _rect, bool fill)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);

	BRect rect = _rect;
	context->ConvertToScreenForDrawing(&rect);
	context->GetDrawingEngine()->DrawEllipse(rect, fill);
}


static void
draw_polygon(void* _context, size_t numPoints, const BPoint viewPoints[],
	bool isClosed, bool fill)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);

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

	context->ConvertToScreenForDrawing(points, viewPoints, numPoints);

	BRect polyFrame;
	get_polygon_frame(points, numPoints, &polyFrame);

	context->GetDrawingEngine()->DrawPolygon(points, numPoints, polyFrame,
		fill, isClosed && numPoints > 2);

	if (numPoints > kMaxStackCount)
		free(points);
}


static void
draw_shape(void* _context, const BShape& shape, bool fill)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	ShapePainter drawShape(context);

	drawShape.Iterate(&shape);
	drawShape.Draw(shape.Bounds(), fill);
}


static void
draw_string(void* _context, const char* string, size_t length, float deltaSpace,
	float deltaNonSpace)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);

	// NOTE: the picture data was recorded with a "set pen location"
	// command inserted before the "draw string" command, so we can
	// use PenLocation()
	BPoint location = context->CurrentState()->PenLocation();

	escapement_delta delta = { deltaSpace, deltaNonSpace };
	context->ConvertToScreenForDrawing(&location);
	location = context->GetDrawingEngine()->DrawString(string, length,
		location, &delta);

	context->ConvertFromScreenForDrawing(&location);
	context->CurrentState()->SetPenLocation(location);
	// the DrawingEngine/Painter does not need to be updated, since this
	// effects only the view->screen coord conversion, which is handled
	// by the view only
}


static void
draw_pixels(void* _context, const BRect& src, const BRect& _dest, uint32 width,
	uint32 height, size_t bytesPerRow, color_space pixelFormat, uint32 options,
	const void* data, size_t length)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);

	UtilityBitmap bitmap(BRect(0, 0, width - 1, height - 1),
		(color_space)pixelFormat, 0, bytesPerRow);

	if (!bitmap.IsValid())
		return;

	memcpy(bitmap.Bits(), data, std::min(height * bytesPerRow, length));

	BRect dest = _dest;
	context->ConvertToScreenForDrawing(&dest);
	context->GetDrawingEngine()->DrawBitmap(&bitmap, src, dest, options);
}


static void
draw_picture(void* _context, const BPoint& where, int32 token)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);

	ServerPicture* picture = context->GetPicture(token);
	if (picture != NULL) {
		context->PushState();
		context->SetDrawingOrigin(where);

		context->PushState();
		picture->Play(context);
		context->PopState();

		context->PopState();
		picture->ReleaseReference();
	}
}


static void
set_clipping_rects(void* _context, size_t numRects, const BRect rects[])
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);

	// TODO: This might be too slow, we should copy the rects
	// directly to BRegion's internal data
	BRegion region;
	for (uint32 c = 0; c < numRects; c++)
		region.Include(rects[c]);

	context->SetUserClipping(&region);
	context->UpdateCurrentDrawingRegion();
}


static void
clip_to_picture(void* _context, int32 pictureToken, const BPoint& where,
	bool clipToInverse)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);

	ServerPicture* picture = context->GetPicture(pictureToken);
	if (picture == NULL)
		return;

	AlphaMask* mask = new(std::nothrow) AlphaMask(
		picture, clipToInverse, where, *context->CurrentState());
	context->SetAlphaMask(mask);
	context->UpdateCurrentDrawingRegion();
	if (mask != NULL)
		mask->ReleaseReference();

	picture->ReleaseReference();
}


static void
push_state(void* _context)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	context->PushState();
}


static void
pop_state(void* _context)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	context->PopState();

	BPoint p(0, 0);
	context->ConvertToScreenForDrawing(&p);
	context->GetDrawingEngine()->SetDrawState(context->CurrentState(),
		(int32)p.x, (int32)p.y);
}


// TODO: Be smart and actually take advantage of these methods:
// only apply state changes when they are called
static void
enter_state_change(void* context)
{
}


static void
exit_state_change(void* _context)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	context->ResyncDrawState();
}


static void
enter_font_state(void* context)
{
}


static void
exit_font_state(void* _context)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	context->GetDrawingEngine()->SetFont(context->CurrentState()->Font());
}


static void
set_origin(void* _context, const BPoint& pt)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	context->CurrentState()->SetOrigin(pt);
}


static void
set_pen_location(void* _context, const BPoint& pt)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	context->CurrentState()->SetPenLocation(pt);
	// the DrawingEngine/Painter does not need to be updated, since this
	// effects only the view->screen coord conversion, which is handled
	// by the view only
}


static void
set_drawing_mode(void* _context, drawing_mode mode)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	context->CurrentState()->SetDrawingMode(mode);
	context->GetDrawingEngine()->SetDrawingMode(mode);
}


static void
set_line_mode(void* _context, cap_mode capMode, join_mode joinMode,
	float miterLimit)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	DrawState* state = context->CurrentState();
	state->SetLineCapMode(capMode);
	state->SetLineJoinMode(joinMode);
	state->SetMiterLimit(miterLimit);
	context->GetDrawingEngine()->SetStrokeMode(capMode, joinMode, miterLimit);
}


static void
set_pen_size(void* _context, float size)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	context->CurrentState()->SetPenSize(size);
	context->GetDrawingEngine()->SetPenSize(
		context->CurrentState()->PenSize());
		// DrawState::PenSize() returns the scaled pen size, so we
		// need to use that value to set the drawing engine pen size.
}


static void
set_fore_color(void* _context, const rgb_color& color)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	context->CurrentState()->SetHighColor(color);
	context->GetDrawingEngine()->SetHighColor(color);
}


static void
set_back_color(void* _context, const rgb_color& color)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	context->CurrentState()->SetLowColor(color);
	context->GetDrawingEngine()->SetLowColor(color);
}


static void
set_stipple_pattern(void* _context, const pattern& pattern)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	context->CurrentState()->SetPattern(Pattern(pattern));
	context->GetDrawingEngine()->SetPattern(pattern);
}


static void
set_scale(void* _context, float scale)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	context->CurrentState()->SetScale(scale);
	context->ResyncDrawState();

	// Update the drawing engine draw state, since some stuff
	// (for example the pen size) needs to be recalculated.
}


static void
set_font_family(void* _context, const char* _family, size_t length)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	BString family(_family, length);

	FontStyle* fontStyle = gFontManager->GetStyleByIndex(family, 0);
	ServerFont font;
	font.SetStyle(fontStyle);
	context->CurrentState()->SetFont(font, B_FONT_FAMILY_AND_STYLE);
}


static void
set_font_style(void* _context, const char* _style, size_t length)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	BString style(_style, length);

	ServerFont font(context->CurrentState()->Font());

	FontStyle* fontStyle = gFontManager->GetStyle(font.Family(), style);

	font.SetStyle(fontStyle);
	context->CurrentState()->SetFont(font, B_FONT_FAMILY_AND_STYLE);
}


static void
set_font_spacing(void* _context, uint8 spacing)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	ServerFont font;
	font.SetSpacing(spacing);
	context->CurrentState()->SetFont(font, B_FONT_SPACING);
}


static void
set_font_size(void* _context, float size)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	ServerFont font;
	font.SetSize(size);
	context->CurrentState()->SetFont(font, B_FONT_SIZE);
}


static void
set_font_rotation(void* _context, float rotation)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	ServerFont font;
	font.SetRotation(rotation);
	context->CurrentState()->SetFont(font, B_FONT_ROTATION);
}


static void
set_font_encoding(void* _context, uint8 encoding)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	ServerFont font;
	font.SetEncoding(encoding);
	context->CurrentState()->SetFont(font, B_FONT_ENCODING);
}


static void
set_font_flags(void* _context, uint32 flags)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	ServerFont font;
	font.SetFlags(flags);
	context->CurrentState()->SetFont(font, B_FONT_FLAGS);
}


static void
set_font_shear(void* _context, float shear)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	ServerFont font;
	font.SetShear(shear);
	context->CurrentState()->SetFont(font, B_FONT_SHEAR);
}


static void
set_font_face(void* _context, uint16 face)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	ServerFont font;
	font.SetFace(face);
	context->CurrentState()->SetFont(font, B_FONT_FACE);
}


static void
set_blending_mode(void* _context, source_alpha alphaSrcMode,
	alpha_function alphaFncMode)
{
	DrawingContext* context = reinterpret_cast<DrawingContext *>(_context);
	context->CurrentState()->SetBlendingMode(alphaSrcMode, alphaFncMode);
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
	set_blending_mode
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
ServerPicture::Play(DrawingContext* target)
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
