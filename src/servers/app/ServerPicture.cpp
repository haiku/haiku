/*
 * Copyright 2001-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (burton666@libero.it)
 *		Marcus Overhagen <marcus@overhagen.de>
 */


#include "DrawingEngine.h"
#include "ServerApp.h"
#include "ServerBitmap.h"
#include "ServerFont.h"
#include "ServerPicture.h"
#include "ServerTokenSpace.h"
#include "ViewLayer.h"
#include "WindowLayer.h"

#include <LinkReceiver.h>
#include <OffsetFile.h>
#include <PicturePlayer.h>
#include <PictureProtocol.h>
#include <PortLink.h>
#include <ServerProtocol.h>
#include <ShapePrivate.h>

#include <Bitmap.h>
#include <List.h>
#include <Shape.h>

#include <stdio.h>
#include <stack>

using std::stack;

class ShapePainter : public BShapeIterator {
	public:
		ShapePainter();
		virtual ~ShapePainter();

		status_t Iterate(const BShape *shape);

		virtual status_t IterateMoveTo(BPoint *point);
		virtual status_t IterateLineTo(int32 lineCount, BPoint *linePts);
		virtual status_t IterateBezierTo(int32 bezierCount, BPoint *bezierPts);
		virtual status_t IterateClose();

		void Draw(ViewLayer *view, BRect frame, bool filled);

	private:
		stack<uint32> fOpStack;
		stack<BPoint> fPtStack;
};

ShapePainter::ShapePainter()
	:	BShapeIterator()
{
}

ShapePainter::~ShapePainter()
{
}

status_t
ShapePainter::Iterate(const BShape *shape)
{
	// this class doesn't modify the shape data
	return BShapeIterator::Iterate(const_cast<BShape *>(shape));
}

status_t
ShapePainter::IterateMoveTo(BPoint *point)
{
	fOpStack.push(OP_MOVETO);
	fPtStack.push(*point);

	return B_OK;
}

status_t
ShapePainter::IterateLineTo(int32 lineCount, BPoint *linePts)
{
	fOpStack.push(OP_LINETO | lineCount);
	for(int32 i = 0;i < lineCount;i++)
		fPtStack.push(linePts[i]);
	
	return B_OK;
}

status_t
ShapePainter::IterateBezierTo(int32 bezierCount, BPoint *bezierPts)
{
	bezierCount *= 3;
	fOpStack.push(OP_BEZIERTO | bezierCount);
	for(int32 i = 0;i < bezierCount;i++)
		fPtStack.push(bezierPts[i]);

	return B_OK;
}

status_t
ShapePainter::IterateClose(void)
{
	fOpStack.push(OP_CLOSE);
	
	return B_OK;
}

void
ShapePainter::Draw(ViewLayer *view, BRect frame, bool filled)
{
	// We're going to draw the currently iterated shape.
	int32 opCount, ptCount;
	opCount = fOpStack.size();
	ptCount = fPtStack.size();

	uint32 *opList;
	BPoint *ptList;
	if(opCount > 0 && ptCount > 0) {
		int32 i;
		opList = new uint32[opCount];
		ptList = new BPoint[ptCount];

		for(i = (opCount - 1);i >= 0;i--) {
			opList[i] = fOpStack.top();
			fOpStack.pop();
		}

		for(i = (ptCount - 1);i >= 0;i--) {
			ptList[i] = fPtStack.top();
			fPtStack.pop();
			view->ConvertToScreenForDrawing(&ptList[i]);
		}

		view->Window()->GetDrawingEngine()->DrawShape(frame, opCount, opList, ptCount, ptList,
				filled);
	}
}


// drawing functions
static void
get_polygon_frame(const BPoint *points, int32 numPoints, BRect *_frame)
{
	float l, t, r, b;

	ASSERT(numPoints > 0);
	
	l = r = points->x;
	t = b = points->y;

	points++;
	numPoints--;

	while (numPoints--) {
		if (points->x < l)
			l = points->x;
		if (points->x > r)
			r = points->x;
		if (points->y < t)
			t = points->y;
		if (points->y > b)
			b = points->y;
		points++;
	}

	_frame->Set(l, t, r, b);
}


static void
nop()
{
}


static void
move_pen_by(ViewLayer *view, BPoint delta)
{
	view->CurrentState()->SetPenLocation(view->CurrentState()->PenLocation() + delta);
}


static void
stroke_line(ViewLayer *view, BPoint start, BPoint end)
{
	BPoint penPos = end;

	view->ConvertToScreenForDrawing(&start);
	view->ConvertToScreenForDrawing(&end);	
	view->Window()->GetDrawingEngine()->StrokeLine(start, end);

	view->CurrentState()->SetPenLocation(penPos);
	// the DrawingEngine/Painter does not need to be updated, since this
	// effects only the view->screen coord conversion, which is handled
	// by the view only
}


static void
stroke_rect(ViewLayer *view, BRect rect)
{
	view->ConvertToScreenForDrawing(&rect);	
	view->Window()->GetDrawingEngine()->StrokeRect(rect);
}


static void
fill_rect(ViewLayer *view, BRect rect)
{
	view->ConvertToScreenForDrawing(&rect);			
	view->Window()->GetDrawingEngine()->FillRect(rect);
}


static void
stroke_round_rect(ViewLayer *view, BRect rect, BPoint radii)
{
	view->ConvertToScreenForDrawing(&rect);	
	view->Window()->GetDrawingEngine()->DrawRoundRect(rect, radii.x, radii.y,
		false);
}


static void
fill_round_rect(ViewLayer *view, BRect rect, BPoint radii)
{
	view->ConvertToScreenForDrawing(&rect);	
	view->Window()->GetDrawingEngine()->DrawRoundRect(rect, radii.x, radii.y,
		true);
}


static void
stroke_bezier(ViewLayer *view, const BPoint *viewPoints)
{
	BPoint points[4];
	view->ConvertToScreenForDrawing(points, viewPoints, 4);

	view->Window()->GetDrawingEngine()->DrawBezier(points, false);
}


static void
fill_bezier(ViewLayer *view, const BPoint *viewPoints)
{
	BPoint points[4];
	view->ConvertToScreenForDrawing(points, viewPoints, 4);

	view->Window()->GetDrawingEngine()->DrawBezier(points, true);
}


static void
stroke_arc(ViewLayer *view, BPoint center, BPoint radii, float startTheta,
			   float arcTheta)
{
	BRect rect(center.x - radii.x, center.y - radii.y, center.x + radii.x - 1,
			center.y + radii.y - 1);
	view->ConvertToScreenForDrawing(&rect);
	view->Window()->GetDrawingEngine()->DrawArc(rect, startTheta, arcTheta, false);
}


static void
fill_arc(ViewLayer *view, BPoint center, BPoint radii, float startTheta,
			 float arcTheta)
{
	BRect rect(center.x - radii.x, center.y - radii.y, center.x + radii.x - 1,
			center.y + radii.y - 1);
	view->ConvertToScreenForDrawing(&rect);
	view->Window()->GetDrawingEngine()->DrawArc(rect, startTheta, arcTheta, true);
}


static void
stroke_ellipse(ViewLayer *view, BPoint center, BPoint radii)
{
	BRect rect(center.x - radii.x, center.y - radii.y, center.x + radii.x - 1,
			center.y + radii.y - 1);
	view->ConvertToScreenForDrawing(&rect);
	view->Window()->GetDrawingEngine()->DrawEllipse(rect, false);
}


static void
fill_ellipse(ViewLayer *view, BPoint center, BPoint radii)
{
	BRect rect(center.x - radii.x, center.y - radii.y, center.x + radii.x - 1,
			center.y + radii.y - 1);
	view->ConvertToScreenForDrawing(&rect);
	view->Window()->GetDrawingEngine()->DrawEllipse(rect, true);
}


static void
stroke_polygon(ViewLayer *view, int32 numPoints, const BPoint *viewPoints, bool isClosed)
{
	if (numPoints <= 0) {
		return;
	} else if (numPoints <= 200) {
		// fast path: no malloc/free, also avoid constructor/destructor calls
		char data[200 * sizeof(BPoint)];
		BPoint *points = (BPoint *)data;

		view->ConvertToScreenForDrawing(points, viewPoints, numPoints);

		BRect polyFrame;
		get_polygon_frame(points, numPoints, &polyFrame);

		view->Window()->GetDrawingEngine()->DrawPolygon(points, numPoints, polyFrame,
			false, isClosed && numPoints > 2);
	} else {
		 // avoid constructor/destructor calls by using malloc instead of new []
		BPoint *points = (BPoint *)malloc(numPoints * sizeof(BPoint));
		if (!points)
			return;

		view->ConvertToScreenForDrawing(points, viewPoints, numPoints);

		BRect polyFrame;
		get_polygon_frame(points, numPoints, &polyFrame);

		view->Window()->GetDrawingEngine()->DrawPolygon(points, numPoints, polyFrame, 
			false, isClosed && numPoints > 2);
		free(points);
	}
}


static void
fill_polygon(ViewLayer *view, int32 numPoints, const BPoint *viewPoints)
{
	if (numPoints <= 0) {
		return;
	} else if (numPoints <= 200) {
		// fast path: no malloc/free, also avoid constructor/destructor calls
		char data[200 * sizeof(BPoint)];
		BPoint *points = (BPoint *)data;

		view->ConvertToScreenForDrawing(points, viewPoints, numPoints);

		BRect polyFrame;
		get_polygon_frame(points, numPoints, &polyFrame);

		view->Window()->GetDrawingEngine()->DrawPolygon(points, numPoints, polyFrame, 
			true, true);
	} else {
		 // avoid constructor/destructor calls by using malloc instead of new []
		BPoint *points = (BPoint *)malloc(numPoints * sizeof(BPoint)); 
		if (!points)
			return;

		view->ConvertToScreenForDrawing(points, viewPoints, numPoints);

		BRect polyFrame;
		get_polygon_frame(points, numPoints, &polyFrame);

		view->Window()->GetDrawingEngine()->DrawPolygon(points, numPoints, polyFrame, 
			true, true);
		free(points);
	}
}


static void
stroke_shape(ViewLayer *view, const BShape *shape)
{
	ShapePainter drawShape;

	drawShape.Iterate(shape);
	drawShape.Draw(view, shape->Bounds(), false);
}


static void
fill_shape(ViewLayer *view, const BShape *shape)
{
	ShapePainter drawShape;

	drawShape.Iterate(shape);
	drawShape.Draw(view, shape->Bounds(), true);
}


static void
draw_string(ViewLayer *view, const char *string, float deltaSpace,
	float deltaNonSpace)
{
	// NOTE: the picture data was recorded with a "set pen location" command
	// inserted before the "draw string" command, so we can use PenLocation()
	BPoint location = view->CurrentState()->PenLocation();

	escapement_delta delta = {deltaSpace, deltaNonSpace };
	view->ConvertToScreenForDrawing(&location);
	view->Window()->GetDrawingEngine()->DrawString(string, strlen(string),
		location, &delta);

	view->ConvertFromScreenForDrawing(&location);
	view->CurrentState()->SetPenLocation(location);
	// the DrawingEngine/Painter does not need to be updated, since this
	// effects only the view->screen coord conversion, which is handled
	// by the view only
}


static void
draw_pixels(ViewLayer *view, BRect src, BRect dest, int32 width, int32 height,
				 int32 bytesPerRow, int32 pixelFormat, int32 flags, const void *data)
{
	UtilityBitmap bitmap(BRect(0, 0, width - 1, height - 1), (color_space)pixelFormat, flags, bytesPerRow);
	
	if (!bitmap.IsValid())
		return;
	
	memcpy(bitmap.Bits(), data, height * bytesPerRow);

	view->ConvertToScreenForDrawing(&dest);
	
	view->Window()->GetDrawingEngine()->DrawBitmap(&bitmap, src, dest);
}


static void
draw_picture(ViewLayer *view, BPoint where, int32 token)
{
	ServerPicture *picture = view->Window()->ServerWindow()->App()->FindPicture(token);	
	if (picture != NULL) {
		view->CurrentState()->SetOrigin(where);
		picture->Play(view);
	}
}


static void
set_clipping_rects(ViewLayer *view, const BRect *rects, uint32 numRects)
{
	// TODO: This might be too slow, we should copy the rects
	// directly to BRegion's internal data
	BRegion region;
	for (uint32 c = 0; c < numRects; c++)
		region.Include(rects[c]);
	view->SetUserClipping(&region);	
}


static void
clip_to_picture(ViewLayer *view, BPicture *picture, BPoint pt,
				   bool clip_to_inverse_picture)
{
	printf("ClipToPicture(picture, BPoint(%.2f, %.2f), %s)\n", pt.x, pt.y,
		clip_to_inverse_picture ? "inverse" : "");
}


static void
push_state(ViewLayer *view)
{
	view->PushState();
}


static void
pop_state(ViewLayer *view)
{
	view->PopState();

	IntPoint p = view->ScrollingOffset();
	p += IntPoint(view->CurrentState()->Origin());
	view->Window()->GetDrawingEngine()->SetDrawState(
		view->CurrentState(), p.x, p.y);
}


// TODO: Be smart and actually take advantage of these methods:
// only apply state changes when they are called
static void
enter_state_change(ViewLayer *view)
{
}


static void
exit_state_change(ViewLayer *view)
{
}


static void 
enter_font_state(ViewLayer *view)
{
}


static void
exit_font_state(ViewLayer *view)
{
	view->Window()->GetDrawingEngine()->SetFont(view->CurrentState()->Font());
}


static void 
set_origin(ViewLayer *view, BPoint pt)
{
	view->CurrentState()->SetOrigin(pt);
}


static void
set_pen_location(ViewLayer *view, BPoint pt)
{
	view->CurrentState()->SetPenLocation(pt);
	// the DrawingEngine/Painter does not need to be updated, since this
	// effects only the view->screen coord conversion, which is handled
	// by the view only
}


static void
set_drawing_mode(ViewLayer *view, drawing_mode mode)
{
	view->CurrentState()->SetDrawingMode(mode);
	view->Window()->GetDrawingEngine()->SetDrawingMode(mode);
}


static void
set_line_mode(ViewLayer *view, cap_mode capMode, join_mode joinMode, float miterLimit)
{
	DrawState *state = view->CurrentState();
	state->SetLineCapMode(capMode);
	state->SetLineJoinMode(joinMode);
	state->SetMiterLimit(miterLimit);
	view->Window()->GetDrawingEngine()->SetStrokeMode(capMode, joinMode, miterLimit);
}


static void
set_pen_size(ViewLayer *view, float size)
{
	view->CurrentState()->SetPenSize(size);
	view->Window()->GetDrawingEngine()->SetPenSize(size);
}


static void
set_fore_color(ViewLayer *view, rgb_color color)
{
	view->CurrentState()->SetHighColor(color);
	view->Window()->GetDrawingEngine()->SetHighColor(color);
}


static void
set_back_color(ViewLayer *view, rgb_color color)
{
	view->CurrentState()->SetLowColor(color);
	view->Window()->GetDrawingEngine()->SetLowColor(color);
}


static void
set_stipple_pattern(ViewLayer *view, pattern p)
{
	view->CurrentState()->SetPattern(Pattern(p));
	view->Window()->GetDrawingEngine()->SetPattern(p);
}


static void
set_scale(ViewLayer *view, float scale)
{
	view->CurrentState()->SetScale(scale);
	// the DrawingEngine/Painter does not need to be updated, since this
	// effects only the view->screen coord conversion, which is handled
	// by the view only
}


static void
set_font_family(ViewLayer *view, const char *family)
{
	// TODO: Implement
	// Can we have a ServerFont::SetFamily() which accepts a string ?
}


static void
set_font_style(ViewLayer *view, const char *style)
{
	// TODO: Implement
	// Can we have a ServerFont::SetStyle() which accepts a string ?
}


static void
set_font_spacing(ViewLayer *view, int32 spacing)
{
	ServerFont font;
	font.SetSpacing(spacing);
	view->CurrentState()->SetFont(font, B_FONT_SPACING);
}


static void
set_font_size(ViewLayer *view, float size)
{
	ServerFont font;
	font.SetSize(size);
	view->CurrentState()->SetFont(font, B_FONT_SIZE);
}


static void
set_font_rotate(ViewLayer *view, float rotation)
{
	ServerFont font;
	font.SetRotation(rotation);
	view->CurrentState()->SetFont(font, B_FONT_ROTATION);
}


static void
set_font_encoding(ViewLayer *view, int32 encoding)
{
	ServerFont font;
	font.SetEncoding(encoding);
	view->CurrentState()->SetFont(font, B_FONT_ENCODING);
}


static void
set_font_flags(ViewLayer *view, int32 flags)
{
	ServerFont font;
	font.SetFlags(flags);
	view->CurrentState()->SetFont(font, B_FONT_FLAGS);
}


static void
set_font_shear(ViewLayer *view, float shear)
{
	ServerFont font;
	font.SetShear(shear);
	view->CurrentState()->SetFont(font, B_FONT_SHEAR);
}


static void
set_font_face(ViewLayer *view, int32 face)
{
	ServerFont font;
	font.SetFace(face);
	view->CurrentState()->SetFont(font, B_FONT_FACE);
}


static void
set_blending_mode(ViewLayer *view, int16 alphaSrcMode, int16 alphaFncMode)
{
	view->CurrentState()->SetBlendingMode((source_alpha)alphaSrcMode, (alpha_function)alphaFncMode);
}


static void
reserved()
{
}


const static void *kTableEntries[] = {
	(const void *)nop,
	(const void *)move_pen_by,
	(const void *)stroke_line,
	(const void *)stroke_rect,
	(const void *)fill_rect,
	(const void *)stroke_round_rect,
	(const void *)fill_round_rect,
	(const void *)stroke_bezier,
	(const void *)fill_bezier,
	(const void *)stroke_arc,
	(const void *)fill_arc,
	(const void *)stroke_ellipse,
	(const void *)fill_ellipse,
	(const void *)stroke_polygon,
	(const void *)fill_polygon,
	(const void *)stroke_shape,
	(const void *)fill_shape,
	(const void *)draw_string,
	(const void *)draw_pixels,
	(const void *)draw_picture,
	(const void *)set_clipping_rects,
	(const void *)clip_to_picture,
	(const void *)push_state,
	(const void *)pop_state,
	(const void *)enter_state_change,
	(const void *)exit_state_change,
	(const void *)enter_font_state,
	(const void *)exit_font_state,
	(const void *)set_origin,
	(const void *)set_pen_location,
	(const void *)set_drawing_mode,
	(const void *)set_line_mode,
	(const void *)set_pen_size,
	(const void *)set_fore_color,
	(const void *)set_back_color,
	(const void *)set_stipple_pattern,
	(const void *)set_scale,
	(const void *)set_font_family,
	(const void *)set_font_style,
	(const void *)set_font_spacing,
	(const void *)set_font_size,
	(const void *)set_font_rotate,
	(const void *)set_font_encoding,
	(const void *)set_font_flags,
	(const void *)set_font_shear,
	(const void *)reserved,		// TODO: Marc Flerackers calls this "set_font_bpp". Investigate
	(const void *)set_font_face,
	(const void *)set_blending_mode 
};


// ServerPicture
ServerPicture::ServerPicture()
	:
	PictureDataWriter(),
	fData(NULL),
	fPictures(NULL),
	fUsurped(NULL)
{
	fToken = gTokenSpace.NewToken(kPictureToken, this);
	fData = new (std::nothrow) BMallocIO();
	
	PictureDataWriter::SetTo(fData);
}


ServerPicture::ServerPicture(const ServerPicture &picture)
	:
	PictureDataWriter(),
	fData(NULL),
	fPictures(NULL),
	fUsurped(NULL)
{
	BMallocIO *mallocIO = new (std::nothrow) BMallocIO();
	if (mallocIO == NULL)
		return;

	fData = mallocIO;
	
	const off_t size = picture.DataLength();
	if (mallocIO->SetSize(size) < B_OK)
		return;
	
	fToken = gTokenSpace.NewToken(kPictureToken, this);

	picture.fData->ReadAt(0, const_cast<void *>(mallocIO->Buffer()), size);
		
	PictureDataWriter::SetTo(fData);
}


ServerPicture::ServerPicture(const char *fileName, const int32 &offset)
	:
	PictureDataWriter(),
	fData(NULL),
	fPictures(NULL),
	fUsurped(NULL)
{
	BPrivate::Storage::OffsetFile *file =
		new BPrivate::Storage::OffsetFile(new BFile(fileName, B_READ_WRITE), (off_t)offset);

	if (file == NULL || file->InitCheck() != B_OK)
		return;
	
	fData = file;
	fToken = gTokenSpace.NewToken(kPictureToken, this);
	
	PictureDataWriter::SetTo(fData);
}


ServerPicture::~ServerPicture()
{
	delete fData;
	gTokenSpace.RemoveToken(fToken);
	
	// We only delete the subpictures list, not the subpictures themselves,
	// since the ServerApp keeps them in a list and will delete them on quit.
	delete fPictures;
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
ServerPicture::SyncState(ViewLayer *view)
{
	// TODO: Finish this
	EnterStateChange();

	WriteSetOrigin(view->CurrentState()->Origin());
	WriteSetPenLocation(view->CurrentState()->PenLocation());
	WriteSetPenSize(view->CurrentState()->PenSize());
	WriteSetScale(view->CurrentState()->Scale());
	WriteSetLineMode(view->CurrentState()->LineCapMode(), view->CurrentState()->LineJoinMode(),
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
		// TODO: For some reason writing the font shear
		// results in missing font in Chart's BPictureButtons. Investigate.		
		//WriteSetFontShear(shear);
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
ServerPicture::Play(ViewLayer *view)
{
	// TODO: for now: then change PicturePlayer to accept a BPositionIO object
	BMallocIO *mallocIO = dynamic_cast<BMallocIO *>(fData);
	if (mallocIO == NULL)
		return;

	BPrivate::PicturePlayer player(mallocIO->Buffer(), mallocIO->BufferLength(), fPictures);
	player.Play(const_cast<void **>(kTableEntries), sizeof(kTableEntries) / sizeof(void *), view);
}


void
ServerPicture::Usurp(ServerPicture *picture)
{
	fUsurped = picture;
}


ServerPicture *
ServerPicture::StepDown()
{
	ServerPicture *old = fUsurped;
	fUsurped = NULL;
	return old;
}


bool
ServerPicture::NestPicture(ServerPicture *picture)
{
	if (fPictures == NULL)
		fPictures = new (std::nothrow) BList;
	
	if (fPictures == NULL
		|| !fPictures->AddItem(picture))
		return false;

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


#define BUFFER_SIZE 4096


status_t
ServerPicture::ImportData(BPrivate::LinkReceiver &link)
{	
	int32 size = 0;
	link.Read<int32>(&size);

	off_t oldPosition = fData->Position();
	fData->Seek(0, SEEK_SET);

	ssize_t toWrite = size;
	// TODO: For some reason, this doesn't work. Bug in LinkReceiver ?
	/*char buffer[BUFFER_SIZE];
	while (toWrite > 0) {
		ssize_t read = link.Read(buffer, toWrite > BUFFER_SIZE ? BUFFER_SIZE : toWrite);
		if (read < B_OK)
			return (status_t)read;		
		fData->Write(buffer, read);
		toWrite -= read;
	}*/

	char buffer[toWrite];
	link.Read(buffer, toWrite);
	fData->Write(buffer, toWrite);

	fData->Seek(oldPosition, SEEK_SET);
	
	return B_OK;
}


status_t
ServerPicture::ExportData(BPrivate::PortLink &link)
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
			ServerPicture *subPic = static_cast<ServerPicture *>(fPictures->ItemAtFast(i));	
			link.Attach<int32>(subPic->Token());		
		}	
	}

	off_t size = 0;
	fData->GetSize(&size);
	link.Attach<int32>((int32)size);

	ssize_t toWrite = size;
	char buffer[BUFFER_SIZE];
	while (toWrite > 0) {
		ssize_t read = fData->Read(buffer, toWrite > BUFFER_SIZE ? BUFFER_SIZE : toWrite);
		if (read < B_OK)
			return (status_t)read;		
		link.Attach(buffer, read);
		toWrite -= read;
	}
	
	fData->Seek(oldPosition, SEEK_SET);

	return B_OK;
}


