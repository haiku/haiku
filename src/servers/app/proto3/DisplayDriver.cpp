/*
	DisplayDriver.cpp
		Modular class to allow the server to not care what the ultimate output is
		for graphics calls.
*/
#include "DisplayDriver.h"
#include "ServerCursor.h"

DisplayDriver::DisplayDriver(void)
{
	is_initialized=false;
	cursor_visible=false;
	show_on_move=false;
	locker=new BLocker();
}

DisplayDriver::~DisplayDriver(void)
{
	delete locker;
}

void DisplayDriver::Initialize(void)
{	// Loading loop to find proper driver or other setup for the driver
}

void DisplayDriver::Shutdown(void)
{	// For use by subclasses
}

bool DisplayDriver::IsInitialized(void)
{	// Let us know whether things worked out ok
	return is_initialized;
}

void DisplayDriver::SafeMode(void)
{	// Set video mode to 640x480x256 mode
}

void DisplayDriver::Reset(void)
{	// Intended to reload and restart driver. Defaults to a safe mode
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

void DisplayDriver::Clear(uint8 red,uint8 green,uint8 blue)
{
}

void DisplayDriver::StrokeRect(BRect rect, rgb_color color)
{
}

void DisplayDriver::FillRect(BRect rect, rgb_color color)
{
}

void DisplayDriver::StrokeEllipse(BRect rect, rgb_color color)
{
}

void DisplayDriver::FillEllipse(BRect rect,rgb_color color)
{
}

void DisplayDriver::SetPixel(int x, int y, rgb_color color)
{	// Internal function utilized by other functions to draw to the buffer
	// Will eventually be an inline function
}

void DisplayDriver::SetCursor(ServerCursor *cursor)
{
}

void DisplayDriver::SetCursor(int32 value)
{	// This is used just to provide an easy way of setting one of the default cursors
	// Then again, I may just get rid of this thing.
}

void DisplayDriver::ShowCursor(void)
{
}

void DisplayDriver::HideCursor(void)
{
}

void DisplayDriver::ObscureCursor(void)
{	// Hides cursor until mouse is moved
}

bool DisplayDriver::IsCursorHidden(void)
{
	return false;
}

void DisplayDriver::MoveCursorTo(int x, int y)
{
}

void DisplayDriver::Blit(BPoint dest, ServerBitmap *sourcebmp, ServerBitmap *destbmp)
{	// Screen-to-screen bitmap copying
}

void DisplayDriver::DrawBitmap(ServerBitmap *bitmap)
{
}

void DisplayDriver::DrawChar(char c, int x, int y)
{
}

void DisplayDriver::DrawString(char *string, int length, int x, int y)
{
}

void DisplayDriver::StrokeArc(int centerx, int centery, int xradius, int yradius)
{
}

void DisplayDriver::FillArc(int centerx, int centery, int xradius, int yradius)
{
}

void DisplayDriver::StrokePolygon(int *x, int *y, int numpoints, bool is_closed)
{
}

void DisplayDriver::FillPolygon(int *x, int *y, int numpoints, bool is_closed)
{
}

void DisplayDriver::StrokeRegion(ServerRegion *region)
{
}

void DisplayDriver::FillRegion(ServerRegion *region)
{
}

void DisplayDriver::StrokeRoundRect(ServerRect *rect)
{
}

void DisplayDriver::FillRoundRect(ServerRect *rect)
{
}

void DisplayDriver::StrokeShape(ServerShape *shape)
{
}

void DisplayDriver::FillShape(ServerShape *shape)
{
}

void DisplayDriver::StrokeTriangle(int *x, int *y)
{
}

void DisplayDriver::FillTriangle(int *x, int *y)
{
}

void DisplayDriver::MoveTo(int x, int y)
{	// Moves the graphics pen to this position
}
