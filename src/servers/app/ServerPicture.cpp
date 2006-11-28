/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (burton666@libero.it)
 */


#include "DrawingEngine.h"
#include "ServerBitmap.h"
#include "ServerPicture.h"
#include "ServerTokenSpace.h"
#include "ViewLayer.h"
#include "WindowLayer.h"

#include <PicturePlayer.h>
#include <PictureProtocol.h>
#include <ServerProtocol.h>
#include <ShapePrivate.h>

#include <Bitmap.h>
#include <Shape.h>

#include <stdio.h>
#include <stack>

using std::stack;

class ShapePainter : public BShapeIterator {
	public:
		ShapePainter();
		virtual ~ShapePainter();

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
	// We're going to draw the currently iterated picture.
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
		}

		view->Window()->GetDrawingEngine()->DrawShape(frame, opCount, opList, ptCount, ptList,
				view->CurrentState(), filled);
	}
}

static void
nop()
{
}


static void
move_pen_by(ViewLayer *view, BPoint delta)
{
	view->CurrentState()->SetPenLocation(delta - view->CurrentState()->PenLocation());
}


static void
stroke_line(ViewLayer *view, BPoint start, BPoint end)
{
	view->ConvertToScreenForDrawing(&start);
	view->ConvertToScreenForDrawing(&end);	
	view->Window()->GetDrawingEngine()->StrokeLine(start, end, view->CurrentState());
}


static void
stroke_rect(ViewLayer *view, BRect rect)
{
	view->ConvertToScreenForDrawing(&rect);	
	view->Window()->GetDrawingEngine()->StrokeRect(rect, view->CurrentState());
}


static void
fill_rect(ViewLayer *view, BRect rect)
{
	view->ConvertToScreenForDrawing(&rect);			
	view->Window()->GetDrawingEngine()->FillRect(rect, view->CurrentState());
}


static void
stroke_round_rect(ViewLayer *view, BRect rect, BPoint radii)
{
	view->ConvertToScreenForDrawing(&rect);	
	view->Window()->GetDrawingEngine()->DrawRoundRect(rect, radii.x, radii.y, view->CurrentState(), false);
}


static void
fill_round_rect(ViewLayer *view, BRect rect, BPoint radii)
{
	view->ConvertToScreenForDrawing(&rect);	
	view->Window()->GetDrawingEngine()->DrawRoundRect(rect, radii.x, radii.y, view->CurrentState(), true);
}


static void
stroke_bezier(ViewLayer *view, BPoint *points)
{
	for (int32 i = 0; i < 4; i++)
		view->ConvertToScreenForDrawing(&points[i]);

	view->Window()->GetDrawingEngine()->DrawBezier(points, view->CurrentState(), false);
}


static void
fill_bezier(ViewLayer *view, BPoint *points)
{
	for (int32 i = 0; i < 4; i++)
		view->ConvertToScreenForDrawing(&points[i]);

	view->Window()->GetDrawingEngine()->DrawBezier(points, view->CurrentState(), true);
}


static void
stroke_arc(ViewLayer *view, BPoint center, BPoint radii, float startTheta,
			   float arcTheta)
{
	BRect rect(center.x - radii.x, center.y - radii.y, center.x + radii.x,
			center.y + radii.y);
	view->ConvertToScreenForDrawing(&rect);
	view->Window()->GetDrawingEngine()->DrawArc(rect, startTheta, arcTheta, view->CurrentState(), false);
}


static void
fill_arc(ViewLayer *view, BPoint center, BPoint radii, float startTheta,
			 float arcTheta)
{
	BRect rect(center.x - radii.x, center.y - radii.y, center.x + radii.x,
			center.y + radii.y);
	view->ConvertToScreenForDrawing(&rect);
	view->Window()->GetDrawingEngine()->DrawArc(rect, startTheta, arcTheta, view->CurrentState(), true);
}


static void
stroke_ellipse(ViewLayer *view, BPoint center, BPoint radii)
{
	BRect rect(center.x - radii.x, center.y - radii.y, center.x + radii.x,
			center.y + radii.y);
	view->ConvertToScreenForDrawing(&rect);
	view->Window()->GetDrawingEngine()->DrawEllipse(rect, view->CurrentState(), false);
}


static void
fill_ellipse(ViewLayer *view, BPoint center, BPoint radii)
{
	BRect rect(center.x - radii.x, center.y - radii.y, center.x + radii.x,
			center.y + radii.y);
	view->ConvertToScreenForDrawing(&rect);
	view->Window()->GetDrawingEngine()->DrawEllipse(rect, view->CurrentState(), true);
}


static void
stroke_polygon(ViewLayer *view, int32 numPoints, BPoint *points, bool isClosed)
{
	for (int32 i = 0; i < numPoints; i++)
		view->ConvertToScreenForDrawing(&points[i]);

	BRect polyFrame = BRect(points[0], points[0]);

	for (int32 i = 1; i < numPoints; i++) {
		if (points[i].x < polyFrame.left)
			polyFrame.left = points[i].x;
		if (points[i].y < polyFrame.top)
			polyFrame.top = points[i].y;
		if (points[i].x > polyFrame.right)
			polyFrame.right = points[i].x;
		if (points[i].y > polyFrame.bottom)
			polyFrame.bottom = points[i].y;
	}

	view->Window()->GetDrawingEngine()->DrawPolygon(points, numPoints, polyFrame, view->CurrentState(),
							false, isClosed && numPoints > 2);
}


static void
fill_polygon(ViewLayer *view, int32 numPoints, BPoint *points)
{
	for (int32 i = 0; i < numPoints; i++)
		view->ConvertToScreenForDrawing(&points[i]);

	BRect polyFrame = BRect(points[0], points[0]);

	for (int32 i = 1; i < numPoints; i++) {
		if (points[i].x < polyFrame.left)
			polyFrame.left = points[i].x;
		if (points[i].y < polyFrame.top)
			polyFrame.top = points[i].y;
		if (points[i].x > polyFrame.right)
			polyFrame.right = points[i].x;
		if (points[i].y > polyFrame.bottom)
			polyFrame.bottom = points[i].y;
	}

	view->Window()->GetDrawingEngine()->DrawPolygon(points, numPoints, polyFrame, view->CurrentState(),
							true, true);
}


static void
stroke_shape(ViewLayer *view, BShape *shape)
{
	ShapePainter drawShape;

	drawShape.Iterate(shape);
	drawShape.Draw(view, shape->Bounds(), false);
}


static void
fill_shape(ViewLayer *view, BShape *shape)
{
	ShapePainter drawShape;

	drawShape.Iterate(shape);
	drawShape.Draw(view, shape->Bounds(), true);
}


static void
draw_string(ViewLayer *view, char *string, float deltaSpace, float deltaNonSpace)
{
	BPoint location = view->CurrentState()->PenLocation();
	escapement_delta delta = {deltaSpace, deltaNonSpace };
	view->ConvertToScreenForDrawing(&location);
	view->Window()->GetDrawingEngine()->DrawString(string, strlen(string), location,
		view->CurrentState(), &delta);
	// TODO: Update pen location ?
	
}


static void
draw_pixels(ViewLayer *view, BRect src, BRect dest, int32 width, int32 height,
				 int32 bytesPerRow, int32 pixelFormat, int32 flags, void *data)
{
	// TODO: Review this
	UtilityBitmap bitmap(BRect(0, 0, width - 1, height - 1), (color_space)pixelFormat, flags, bytesPerRow);
	
	if (!bitmap.IsValid())
		return;
	
	uint8 *pixels = (uint8 *)data;
	uint8 *destPixels = (uint8 *)bitmap.Bits();
	for (int32 h = 0; h < height; h++) {
		memcpy(destPixels, pixels, bytesPerRow);
		pixels += bytesPerRow;
		destPixels += bytesPerRow;
	}

	view->ConvertToScreenForDrawing(&dest);
	
	view->Window()->GetDrawingEngine()->DrawBitmap(&bitmap, src, dest, view->CurrentState());
}


static void
set_clipping_rects(ViewLayer *view, BRect *rects, uint32 numRects)
{
	// TODO: This is too slow, we should copy the rects directly to BRegion's internal data
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
}


static void
enter_state_change(ViewLayer *view)
{
	printf("EnterStateChange\n");
}


static void
exit_state_change(ViewLayer *view)
{
	printf("ExitStateChange\n");
}


static void 
enter_font_state(ViewLayer *view)
{
	printf("EnterFontState\n");
}


static void
exit_font_state(ViewLayer *view)
{
	printf("ExitFontState\n");
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
}


static void
set_drawing_mode(ViewLayer *view, drawing_mode mode)
{
	view->CurrentState()->SetDrawingMode(mode);
}


static void
set_line_mode(ViewLayer *view, cap_mode capMode, join_mode joinMode, float miterLimit)
{
	DrawState *state = view->CurrentState();
	state->SetLineCapMode(capMode);
	state->SetLineJoinMode(joinMode);
	state->SetMiterLimit(miterLimit);
}


static void
set_pen_size(ViewLayer *view, float size)
{
	view->CurrentState()->SetPenSize(size);
}


static void
set_fore_color(ViewLayer *view, rgb_color color)
{
	view->CurrentState()->SetHighColor(RGBColor(color));
}


static void
set_back_color(ViewLayer *view, rgb_color color)
{
	view->CurrentState()->SetLowColor(RGBColor(color));
}


static void
set_stipple_pattern(ViewLayer *view, pattern p)
{
	printf("SetStipplePattern\n");
}


static void
set_scale(ViewLayer *view, float scale)
{
	view->CurrentState()->SetScale(scale);
}


static void
set_font_family(ViewLayer *view, char *family)
{
	printf("SetFontFamily(%s)\n", family);
}


static void
set_font_style(ViewLayer *view, char *style)
{
	printf("SetFontStyle(%s)\n", style);
}


static void
set_font_spacing(ViewLayer *view, int32 spacing)
{
	printf("SetFontSpacing(%ld)\n", spacing);
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
	printf("SetFontRotate(%.2f)\n", rotation);
}


static void
set_font_encoding(ViewLayer *view, int32 encoding)
{
	printf("SetFontEncoding(%ld)\n", encoding);
}


static void
set_font_flags(ViewLayer *view, int32 flags)
{
	printf("SetFontFlags(%ld)\n", flags);
}


static void
set_font_shear(ViewLayer *view, float shear)
{
	printf("SetFontShear(%.2f)\n", shear);
}


static void
set_font_face(ViewLayer *view, int32 flags)
{
	printf("SetFontFace(%ld)\n", flags);
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


const void *tableEntries[] = {
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
	(const void *)reserved,	// TODO: This is probably "draw_picture". Investigate
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
	(const void *)set_font_face, // TODO: R5 function table ends here... how is set blending mode implemented there ?
	(const void *)set_blending_mode 
};


// ServerPicture
ServerPicture::ServerPicture()
	:PictureDataWriter(&fData)
{
	fToken = gTokenSpace.NewToken(kPictureToken, this);
}


ServerPicture::ServerPicture(const ServerPicture &picture)
	:PictureDataWriter(&fData)
{
	fToken = gTokenSpace.NewToken(kPictureToken, this);

	fData.Write(picture.Data(), picture.DataLength());
}


ServerPicture::~ServerPicture()
{
}


void
ServerPicture::SyncState(ViewLayer *view)
{
	// TODO: Finish this
	BeginOp(B_PIC_ENTER_STATE_CHANGE);

//	WriteSetPenLocation(view->CurrentState()->PenLocation());
	WriteSetPenSize(view->CurrentState()->PenSize());
	WriteSetScale(view->CurrentState()->Scale());
	WriteSetLineMode(view->CurrentState()->LineCapMode(), view->CurrentState()->LineJoinMode(),
			view->CurrentState()->MiterLimit());
	
/*
	BeginOp(B_PIC_SET_STIPLE_PATTERN);
	AddData(view->CurrentState()->GetPattern().GetInt8(), sizeof(pattern));
	EndOp();
*/
	WriteSetDrawingMode(view->CurrentState()->GetDrawingMode());
	
	/*BeginOp(B_PIC_SET_BLENDING_MODE);
	AddInt16((int16)view->CurrentState()->AlphaSrcMode());
	AddInt16((int16)view->CurrentState()->AlphaFncMode());
	EndOp();
*/
	WriteSetHighColor(view->CurrentState()->HighColor().GetColor32());
	WriteSetLowColor(view->CurrentState()->LowColor().GetColor32());

	EndOp();

}


void
ServerPicture::Play(ViewLayer *view)
{
	PicturePlayer player(const_cast<void *>(fData.Buffer()), fData.BufferLength(), NULL);
	player.Play(const_cast<void **>(tableEntries), sizeof(tableEntries) / sizeof(void *), view);
}
