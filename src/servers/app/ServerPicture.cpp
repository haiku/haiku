#include "DrawingEngine.h"
#include "ServerBitmap.h"
#include "ServerPicture.h"
#include "ServerTokenSpace.h"
#include "ViewLayer.h"
#include "WindowLayer.h"

#include <ServerBitmap.h>
#include <ServerProtocol.h>
#include <TPicture.h>

#include <Bitmap.h>

#include <stdio.h>


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
	view->Window()->GetDrawingEngine()->StrokeLine(start, end, view->CurrentState()->HighColor());
}


static void
stroke_rect(ViewLayer *view, BRect rect)
{
	view->ConvertToScreenForDrawing(&rect);	
	view->Window()->GetDrawingEngine()->StrokeRect(rect, view->CurrentState()->HighColor());
}


static void
fill_rect(ViewLayer *view, BRect rect)
{
	view->ConvertToScreenForDrawing(&rect);			
	view->Window()->GetDrawingEngine()->FillRect(rect, view->CurrentState()->HighColor());
}


static void
stroke_round_rect(void *user, BRect rect, BPoint radii)
{
	printf("StrokeRoundRect(BRect(%.2f, %.2f, %.2f, %.2f), BPoint(%.2f, %.2f))\n",
		rect.left, rect.top, rect.right, rect.bottom, radii.x, radii.y);
}


static void
fill_round_rect(void *user, BRect rect, BPoint radii)
{
	printf("FillRoundRect(BRect(%.2f, %.2f, %.2f, %.2f), BPoint(%.2f, %.2f))\n",
		rect.left, rect.top, rect.right, rect.bottom, radii.x, radii.y);
}


static void
stroke_bezier(void *user, BPoint *control)
{
	printf("StrokeBezier\n");
	for (int32 i = 0; i < 4; i++)
		control[i].PrintToStream();
}


static void
fill_bezier(void *user, BPoint *control)
{
	printf("FillBezier\n");
	for (int32 i = 0; i < 4; i++)
		control[i].PrintToStream();
}


static void
stroke_arc(void *user, BPoint center, BPoint radii, float startTheta,
			   float arcTheta)
{
	printf("StrokeArc(BPoint(%.2f, %.2f), BPoint(%.2f, %.2f), %.2f, %.2f)\n",
		center.x, center.y, radii.x, radii.y, startTheta, arcTheta);
}


static void
fill_arc(void *user, BPoint center, BPoint radii, float startTheta,
			 float arcTheta)
{
	printf("FillArc(BPoint(%.2f, %.2f), BPoint(%.2f, %.2f), %.2f, %.2f)\n",
		center.x, center.y, radii.x, radii.y, startTheta, arcTheta);
}


static void
stroke_ellipse(ViewLayer *view, BPoint center, BPoint radii)
{
	//view->StrokeEllipse(BRect(BPoint(center.x - radii.x, center.y - radii.y),
	//	BPoint(center.x + radii.x, center.y + radii.y)));

	printf("StrokeEllipse(BPoint(%.2f, %.2f), BPoint(%.2f, %.2f))\n",
		center.x, center.y, radii.x, radii.y);
}


static void
fill_ellipse(ViewLayer *view, BPoint center, BPoint radii)
{
	//view->FillEllipse(BRect(BPoint(center.x - radii.x, center.y - radii.y),
	//	BPoint(center.x + radii.x, center.y + radii.y)));

	printf("FillEllipse(BPoint(%.2f, %.2f), BPoint(%.2f, %.2f))\n", center.x, center.y,
		radii.x, radii.y);
}


static void
stroke_polygon(void *user, int32 numPoints, BPoint *points, bool isClosed)
{
	printf("StrokePolygon %ld %s\n", numPoints, isClosed ? "closed" : "");
	for (int32 i = 0; i < numPoints; i++)
		points[i].PrintToStream();
}


static void
fill_polygon(void *user, int32 numPoints, BPoint *points)
{
	printf("FillPolygon %ld\n", numPoints);
	for (int32 i = 0; i < numPoints; i++)
		points[i].PrintToStream();
}


static void
stroke_shape(ViewLayer *view, BShape *shape)
{
	//view->StrokeShape(shape);

	printf("StrokeShape\n");
}


static void
fill_shape(ViewLayer *view, BShape *shape)
{
	//view->FillShape(shape);

	printf("FillShape\n");
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
set_clipping_rects(void *user, BRect *rects, uint32 numRects)
{
	printf("SetClippingRects %ld\n", numRects);
	for (uint32 i = 0; i < numRects; i++)
		rects[i].PrintToStream();
}


static void
clip_to_picture(void *user, BPicture *picture, BPoint pt,
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
	/*
	BFont font;
	view->GetFont(&font);
	font.SetSize(size);
	view->SetFont(&font, B_FONT_SIZE);
	*/
	printf("SetFontSize(%.2f)\n", size);
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
	(const void *)reserved,
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
	(const void *)reserved,
	(const void *)set_font_face,
};


ServerPicture::ServerPicture()
{
	fToken = gTokenSpace.NewToken(kPictureToken, this);
}


ServerPicture::ServerPicture(const ServerPicture &picture)
{
	fToken = gTokenSpace.NewToken(kPictureToken, this);

	AddData(picture.Data(), picture.DataLength());
}


ServerPicture::~ServerPicture()
{
}


void
ServerPicture::BeginOp(int16 op)
{
	fStack.push(fData.Position());
	fData.Write(&op, sizeof(op));
	
	// Init the size of the opcode block to 0
	size_t size = 0;
	fData.Write(&size, sizeof(size));
}


void
ServerPicture::EndOp()
{
	off_t curPos = fData.Position();
	off_t stackPos = fStack.top();
	fStack.pop();
	
	// The size of the op is calculated like this:
	// current position on the stream minus the position on the stack,
	// minus the space occupied by the op code itself (int16)
	// and the space occupied by the size field (size_t) 
	size_t size = curPos - stackPos - sizeof(size_t) - sizeof(int16);

	// Size was set to 0 in BeginOp(). Now we overwrite it with the correct value
	fData.Seek(stackPos + sizeof(int16), SEEK_SET);
	fData.Write(&size, sizeof(size));
	fData.Seek(curPos, SEEK_SET);
}


void
ServerPicture::EnterStateChange()
{
	// BeginOp(B_PIC_ENTER_STATE_CHANGE);
}


void
ServerPicture::ExitStateChange()
{
	// EndOp();
}


void
ServerPicture::EnterFontChange()
{
	// BeginOp(B_PIC_ENTER_FONT_STATE);
}


void
ServerPicture::ExitFontChange()
{
	// EndOp();
}


void
ServerPicture::AddInt8(int8 data)
{
	fData.Write(&data, sizeof(data));
}


void
ServerPicture::AddInt16(int16 data)
{
	fData.Write(&data, sizeof(data));
}


void
ServerPicture::AddInt32(int32 data)
{
	fData.Write(&data, sizeof(data));
}


void
ServerPicture::AddInt64(int64 data)
{
	fData.Write(&data, sizeof(data));
}


void
ServerPicture::AddFloat(float data)
{
	fData.Write(&data, sizeof(data));
}


void
ServerPicture::AddCoord(BPoint data)
{
	fData.Write(&data, sizeof(data));
}


void
ServerPicture::AddRect(BRect data)
{
	fData.Write(&data, sizeof(data));
}


void
ServerPicture::AddColor(rgb_color data)
{
	fData.Write(&data, sizeof(data));
}


void
ServerPicture::AddString(const char *data)
{
	int32 len = data ? strlen(data) : 0;
	fData.Write(&len, sizeof(int32));
	fData.Write(data, len);
}


void
ServerPicture::AddData(const void *data, int32 size)
{
	fData.Write(data, size);
}


void
ServerPicture::SyncState(ViewLayer *view)
{
	// TODO:
}


void
ServerPicture::Play(ViewLayer *view)
{
	PicturePlayer player(const_cast<void *>(fData.Buffer()), fData.BufferLength(), NULL);
	player.Play(const_cast<void **>(tableEntries), sizeof(tableEntries) / sizeof(void *), view);
}
