/*
	DisplayDriver.cpp
		Modular class to allow the server to not care what the ultimate output is
		for graphics calls.
*/
#include "DisplayDriver.h"
#include "ColorUtils.h"
#include "SystemPalette.h"
#include "ServerCursor.h"

DisplayDriver::DisplayDriver(void)
{
	is_initialized=false;
	cursor_visible=true;
	show_on_move=false;
	locker=new BLocker();

	penpos.Set(0,0);
	pensize=0.0;

	// initialize to BView defaults
	highcol.red=0;
	highcol.green=0;
	highcol.blue=0;
	highcol.alpha=0;
	lowcol.red=255;
	lowcol.green=255;
	lowcol.blue=255;
	lowcol.alpha=255;
}

DisplayDriver::~DisplayDriver(void)
{
	delete locker;
}

void DisplayDriver::Initialize(void)
{	// Loading loop to find proper driver or other setup for the driver
}

bool DisplayDriver::IsInitialized(void)
{	// Let us know whether things worked out ok
	return is_initialized;
}

void DisplayDriver::Shutdown(void)
{	// For use by subclasses
}

void DisplayDriver::SafeMode(void)
{	// Set video mode to 640x480x256 mode
}

void DisplayDriver::Reset(void)
{	// Intended to reload and restart driver. Defaults to a safe mode
}

void DisplayDriver::Clear(uint8 red,uint8 green,uint8 blue)
{
}

void DisplayDriver::Clear(rgb_color col)
{
}

void DisplayDriver::SetScreen(uint32 space)
{	// Set the screen to a particular mode
}

int32 DisplayDriver::GetHeight(void)
{	// Gets the height of the current mode
	return ginfo->height;
}

int32 DisplayDriver::GetWidth(void)
{	// Gets the width of the current mode
	return ginfo->width;
}

int DisplayDriver::GetDepth(void)
{	// Gets the color depth of the current mode
	return ginfo->bytes_per_row;
}

void DisplayDriver::AddLine(BPoint pt1, BPoint pt2, rgb_color col)
{
}

void DisplayDriver::BeginLineArray(int32 count)
{
}

void DisplayDriver::Blit(BRect src, BRect dest)
{	// Screen-to-screen bitmap copying
}

void DisplayDriver::DrawBitmap(ServerBitmap *bitmap, BRect source, BRect dest)
{
}

void DisplayDriver::DrawChar(char c, BPoint point)
{
}

void DisplayDriver::DrawLineArray(int32 count,BPoint *start, BPoint *end, rgb_color *color)
{
}

void DisplayDriver::DrawString(char *string, int length, BPoint point)
{
}

void DisplayDriver::EndLineArray(void)
{
}

void DisplayDriver::FillArc(int centerx, int centery, int xradius, int yradius, float angle, float span, uint8 *pattern)
{
}

void DisplayDriver::FillBezier(BPoint *points, uint8 *pattern)
{
}

void DisplayDriver::FillEllipse(float centerx, float centery, float x_radius, float y_radius,uint8 *pattern)
{
}

void DisplayDriver::FillPolygon(int *x, int *y, int numpoints, bool is_closed)
{
}

void DisplayDriver::FillRect(BRect rect, uint8 *pattern)
{
}

void DisplayDriver::FillRect(BRect rect, rgb_color col)
{
}

void DisplayDriver::FillRegion(BRegion *region)
{
}

void DisplayDriver::FillRoundRect(BRect rect,float xradius, float yradius, uint8 *pattern)
{
}

void DisplayDriver::FillShape(BShape *shape)
{
}

void DisplayDriver::FillTriangle(BPoint first, BPoint second, BPoint third, BRect rect, uint8 *pattern)
{
}

void DisplayDriver::FillTriangle(BPoint first, BPoint second, BPoint third, BRect rect, rgb_color col)
{
}

drawing_mode DisplayDriver::GetDrawingMode(void)
{
	return B_OP_COPY;
}

void DisplayDriver::HideCursor(void)
{
}

rgb_color DisplayDriver::HighColor(void)
{
	locker->Lock();
	rgb_color col=highcol;
	locker->Unlock();
	return col;
}

bool DisplayDriver::IsCursorHidden(void)
{
	return false;
}

rgb_color DisplayDriver::LowColor(void)
{
	locker->Lock();
	rgb_color col=lowcol;
	locker->Unlock();
	return col;
}

void DisplayDriver::ObscureCursor(void)
{	// Hides cursor until mouse is moved
}

void DisplayDriver::MoveCursorTo(float x, float y)
{
}

void DisplayDriver::MovePenTo(BPoint pt)
{	// Moves the graphics pen to this position
	penpos=pt;
}

BPoint DisplayDriver::PenPosition(void)
{
	return penpos;
}

float DisplayDriver::PenSize(void)
{
	return pensize;
}

void DisplayDriver::SetCursor(ServerCursor *cursor)
{
}

void DisplayDriver::SetDrawingMode(drawing_mode mode)
{
}

void DisplayDriver::SetHighColor(uint8 r,uint8 g,uint8 b,uint8 a=255)
{
	SetRGBColor(&highcol,r,g,b,a);
	high16=FindClosestColor16(highcol);
	high8=FindClosestColor(system_palette,highcol);
}

void DisplayDriver::SetHighColor(rgb_color col)
{
	highcol=col;
	high16=FindClosestColor16(highcol);
	high8=FindClosestColor(system_palette,highcol);
}

void DisplayDriver::SetLowColor(uint8 r,uint8 g,uint8 b,uint8 a=255)
{
	SetRGBColor(&lowcol,r,g,b,a);
	low16=FindClosestColor16(lowcol);
	low8=FindClosestColor(system_palette,lowcol);
}

void DisplayDriver::SetLowColor(rgb_color col)
{
	lowcol=col;
	low16=FindClosestColor16(lowcol);
	low8=FindClosestColor(system_palette,lowcol);
}

void DisplayDriver::SetPenSize(float size)
{
	pensize=(size>0)?size:1;
}

void DisplayDriver::SetPixel(int x, int y, uint8 *pattern)
{	// Internal function utilized by other functions to draw to the buffer
	// Will eventually be an inline function
}

void DisplayDriver::ShowCursor(void)
{
}

float DisplayDriver::StringWidth(const char *string, int32 length)
{
	return -1.0;
}

void DisplayDriver::StrokeArc(int centerx, int centery, int xradius, int yradius, float angle, float span, uint8 *pattern)
{
}

void DisplayDriver::StrokeBezier(BPoint *points, uint8 *pattern)
{
}

void DisplayDriver::StrokeEllipse(float centerx, float centery, float x_radius, float y_radius,uint8 *pattern)
{
}

void DisplayDriver::StrokeLine(BPoint point, uint8 *pattern)
{
}

void DisplayDriver::StrokeLine(BPoint pt1, BPoint pt2, rgb_color col)
{
}

void DisplayDriver::StrokePolygon(int *x, int *y, int numpoints, bool is_closed)
{
}

void DisplayDriver::StrokeRect(BRect rect, uint8 *pattern)
{
}

void DisplayDriver::StrokeRect(BRect rect, rgb_color col)
{
}

void DisplayDriver::StrokeRoundRect(BRect rect,float xradius, float yradius, uint8 *pattern)
{
}

void DisplayDriver::StrokeShape(BShape *shape)
{
}

void DisplayDriver::StrokeTriangle(BPoint first, BPoint second, BPoint third, BRect rect, uint8 *pattern)
{
}

void DisplayDriver::StrokeTriangle(BPoint first, BPoint second, BPoint third, BRect rect, rgb_color col)
{
}

void DisplayDriver::Line32(BPoint pt, BPoint pt2, uint8 *pattern)
{
}

void DisplayDriver::Line16(BPoint pt, BPoint pt2, uint8 *pattern)
{
}

void DisplayDriver::Line8(BPoint pt, BPoint pt2, uint8 *pattern)
{
}
