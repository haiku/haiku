#include "ServerBitmap.h"
#include "ServerPicture.h"
#include "ServerTokenSpace.h"
#include "ViewLayer.h"

#include <ServerProtocol.h>
#include <TPicture.h>

#include <stdio.h>

static void
nop()
{
	printf("nop\n");
}


static void
move_pen_by(ViewLayer *view, BPoint delta)
{
	//view->MovePenBy(delta)
	printf("MovePenBy(%.2f, %.2f)\n", delta.x, delta.y);
}


static void
stroke_line(ViewLayer *view, BPoint start, BPoint end)
{
	//view->MovePenTo(start);
	//view->StrokeLine(end);
	printf("StrokeLine(BPoint(%.2f, %.2f), BPoint(%.2f, %.2f))\n", start.x, start.y, 
		end.x, end.y);
}


static void
stroke_rect(ViewLayer *view, BRect rect)
{
	//view->StrokeRect(rect);

	printf("StrokeRect(BRect(%.2f, %.2f, %.2f, %.2f))\n", rect.left, rect.top,
		rect.right, rect.bottom);
}


static void
fill_rect(ViewLayer *view, BRect rect)
{
	//view->FillRect(rect);

	printf("FillRect(BRect(%.2f, %.2f, %.2f, %.2f))\n", rect.left, rect.top,
		rect.right, rect.bottom);
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
draw_string(ViewLayer *view, char *string, float deltax, float deltay)
{
	//int32 len = string ? strlen(string) : 0;

	//view->DrawString(string, len, NULL);

	printf("DrawString(\"%s\", %.2f, %.2f)\n", string, deltax, deltay);
}


static void
draw_pixels(ViewLayer *view, BRect src, BRect dest, int32 width, int32 height,
				 int32 bytesPerRow, int32 pixelFormat, int32 flags, void *data)
{
	//ServerBitmap bitmap(BRect(0, 0, width - 1, height - 1),
	//	(color_space)pixelFormat);

	//bitmap.SetBits(data, bytesPerRow * height, 0, (color_space)pixelFormat);

	//view->DrawBitmap(&bitmap, src, dest);

	printf("DrawPixels(BRect(%.2f, %.2f, %.2f, %.2f),\n"
		"BRect(%.2f, %.2f, %.2f, %.2f),\n"
		"%ld, %ld, %ld, %ld, %ld)\n",
		src.left, src.top, src.right, src.bottom,
		dest.left, dest.top, dest.right, dest.bottom,
		width, height, bytesPerRow, pixelFormat, flags);
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
push_state(void *user)
{
	printf("PushState\n");
}


static void
pop_state(void *user)
{
	printf("PopState\n");
}


static void
enter_state_change(void *user)
{
	printf("EnterStateChange\n");
}


static void
exit_state_change(void *user)
{
	printf("ExitStateChange\n");
}


static void 
enter_font_state(void *user)
{
	printf("EnterFontState\n");
}


static void
exit_font_state(void *user)
{
	printf("ExitFontState\n");
}


static void 
set_origin(void *user, BPoint pt)
{
	printf("SetOrigin(%.2f, %.2f)\n", pt.x, pt.y);
}


static void
set_pen_location(ViewLayer *view, BPoint pt)
{
	//view->MovePenTo(pt);

	printf("SetPenLocation(%.2f, %.2f)\n", pt.x, pt.y);
}


static void
set_drawing_mode(void *user, drawing_mode mode)
{
	printf("SetDrawingMode(%d)\n", mode);
}


static void
set_line_mode(ViewLayer *view, cap_mode capMode, join_mode joinMode,
				 float miterLimit)
{
	//view->SetLineMode(capMode, joinMode, miterLimit);
	printf("SetLineMode(%d, %d, %.2f)\n", capMode, joinMode, miterLimit);
}


static void
set_pen_size(ViewLayer *view, float size)
{
	//view->SetPenSize(size);
	printf("SetPenSize(%.2f)\n", size);
}


static void
set_fore_color(ViewLayer *view, rgb_color color)
{
	//view->SetHighColor(color);

	printf("SetForeColor(%d, %d, %d, %d)\n", color.red, color.green,
		color.blue, color.alpha);
}


static void
set_back_color(ViewLayer *view, rgb_color color)
{
	//view->SetLowColor(color);

	printf("SetBackColor(%d, %d, %d, %d)\n", color.red, color.green,
		color.blue, color.alpha);
}


static void
set_stipple_pattern(void *user, pattern p)
{
	printf("SetStipplePattern\n");
}


static void
set_scale(void *user, float scale)
{
	printf("SetScale(%.2f)\n", scale);
}


static void
set_font_family(void *user, char *family)
{
	printf("SetFontFamily(%s)\n", family);
}


static void
set_font_style(void *user, char *style)
{
	printf("SetFontStyle(%s)\n", style);
}


static void
set_font_spacing(void *user, int32 spacing)
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
set_font_rotate(void *user, float rotation)
{
	printf("SetFontRotate(%.2f)\n", rotation);
}


static void
set_font_encoding(void *user, int32 encoding)
{
	printf("SetFontEncoding(%ld)\n", encoding);
}


static void
set_font_flags(void *user, int32 flags)
{
	printf("SetFontFlags(%ld)\n", flags);
}


static void
set_font_shear(void *user, float shear)
{
	printf("SetFontShear(%.2f)\n", shear);
}


static void
set_font_face(void *user, int32 flags)
{
	printf("SetFontFace(%ld)\n", flags);
}


static void
reserved()
{
	printf("reserved\n");
}


const void *tableEntries[] = {
	nop,
	move_pen_by,
	stroke_line,
	stroke_rect,
	fill_rect,
	stroke_round_rect,
	fill_round_rect,
	stroke_bezier,
	fill_bezier,
	stroke_arc,
	fill_arc,
	stroke_ellipse,
	fill_ellipse,
	stroke_polygon,
	fill_polygon,
	stroke_shape,
	fill_shape,
	draw_string,
	draw_pixels,
	reserved,
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
	set_font_rotate,
	set_font_encoding,
	set_font_flags,
	set_font_shear,
	reserved,
	set_font_face,
};


ServerPicture::ServerPicture()
{
	fToken = gTokenSpace.NewToken(kPictureToken, this);
}


ServerPicture::~ServerPicture()
{
}


void
ServerPicture::BeginOp(int16 op)
{
	int32 size = 0;
	fStack.AddItem((void *)fData.Position());
	fData.Write(&op, sizeof(op));
	fData.Write(&size, sizeof(size));
}


void
ServerPicture::EndOp()
{
	off_t curPos = fData.Position();
	off_t stackPos = (off_t)fStack.RemoveItem(fStack.CountItems() - 1);
	size_t size = curPos - stackPos - 6;

	fData.Seek(stackPos + 2, SEEK_SET);
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
	//TPicture picture((void *)fData.Buffer(), fData.BufferLength(), NULL);
	//picture.Play((void *)tableEntries, sizeof(tableEntries) / sizeof(void*), view);
}
