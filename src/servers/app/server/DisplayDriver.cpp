//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		DisplayDriver.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Mostly abstract class which handles all graphics output
//					for the server
//  
//------------------------------------------------------------------------------
#include "DisplayDriver.h"
#include "ServerCursor.h"

/*!
	\brief Sets up internal variables needed by all DisplayDriver subclasses
	
	Subclasses should follow DisplayDriver's lead and use this function mostly
	for initializing data members.
*/
DisplayDriver::DisplayDriver(void)
{
	_lock_sem=create_sem(1,"DisplayDriver Lock");

	_buffer_depth=0;
	_buffer_width=0;
	_buffer_height=0;
	_buffer_mode=-1;
	
	_is_cursor_hidden=false;
	_is_cursor_obscured=false;
	_cursor=NULL;
}


/*!
	\brief Deletes the locking semaphore
	
	Subclasses should use the destructor mostly for freeing allocated heap space.
*/
DisplayDriver::~DisplayDriver(void)
{
	delete_sem(_lock_sem);
}

/*!
	\brief Initializes the driver object.
	\return true if successful, false if not
	
	Initialize sets up the driver for display, including the initial clearing
	of the screen. If things do not go as they should, false should be returned.
*/
bool DisplayDriver::Initialize(void)
{
	return false;
}

/*!
	\brief Shuts down the driver's video subsystem
	
	Any work done by Initialize() should be undone here. Note that Shutdown() is
	called even if Initialize() was unsuccessful.
*/
void DisplayDriver::Shutdown(void)
{
}

/*!
	\brief Called for all BView::CopyBits calls
	\param Source rectangle.
	\param Destination rectangle.
	
	Bounds checking must be done in this call. If the destination is not the same size 
	as the source, the source should be scaled to fit.
*/
void DisplayDriver::CopyBits(BRect src, BRect dest)
{
}

/*!
	\brief Called for all BView::DrawBitmap calls
	\param Bitmap to be drawn. It will always be non-NULL and valid. The color 
	space is not guaranteed to match.
	\param Source rectangle
	\param Destination rectangle. Source will be scaled to fit if not the same size.
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	
	Bounds checking must be done in this call.
*/
void DisplayDriver::DrawBitmap(ServerBitmap *bmp, BRect src, BRect dest, LayerData *d)
{
}

/*!
	\brief Utilizes the font engine to draw a string to the frame buffer
	\param String to be drawn. Always non-NULL.
	\param Number of characters in the string to draw. Always greater than 0. If greater
	than the number of characters in the string, draw the entire string.
	\param Point at which the baseline starts. Characters are to be drawn 1 pixel above
	this for backwards compatibility. While the point itself is guaranteed to be inside
	the frame buffers coordinate range, the clipping of each individual glyph must be
	performed by the driver itself.
	\param Data structure containing any other data necessary for the call. Always non-NULL.
*/
void DisplayDriver::DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *delta=NULL)
{
}

/*!
	\brief Called for all BView::FillArc calls
	\param Rectangle enclosing the entire arc
	\param Starting angle for the arc in degrees
	\param Span of the arc in degrees. Ending angle = angle+span.
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	\param 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the arc may end up
	being clipped.
*/
void DisplayDriver::FillArc(BRect r, float angle, float span, LayerData *d, int8 *pat)
{
}

/*!
	\brief Called for all BView::FillBezier calls.
	\param 4-element array of BPoints in the order of start, end, and then the two control
	points. 
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	\param 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call.
*/
void DisplayDriver::FillBezier(BPoint *pts, LayerData *d, int8 *pat)
{
}

/*!
	\brief Called for all BView::FillEllipse calls
	\param BRect enclosing the ellipse to be drawn.
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	\param 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the ellipse may end up
	being clipped.
*/
void DisplayDriver::FillEllipse(BRect r, LayerData *d, int8 *pat)
{
}

/*!
	\brief Called for all BView::FillPolygon calls
	\param Array of BPoints defining the polygon.
	\param Number of points in the BPoint array.
	\param Rectangle which contains the polygon
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	\param 8-byte array containing the pattern to use. Always non-NULL.

	The points in the array are not guaranteed to be within the framebuffer's 
	coordinate range.
*/
void DisplayDriver::FillPolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat)
{
}

/*!
	\brief Called for all BView::FillRect calls
	\param BRect to be filled. Guaranteed to be in the frame buffer's coordinate space
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	\param 8-byte array containing the pattern to use. Always non-NULL.

*/
void DisplayDriver::FillRect(BRect r, LayerData *d, int8 *pat)
{
}

/*!
	\brief Called for all BView::FillRoundRect calls
	\param X radius of the corner arcs
	\param Y radius of the corner arcs
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	\param 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the roundrect may end 
	up being clipped.
*/
void DisplayDriver::FillRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat)
{
}

//void DisplayDriver::FillShape(SShape *sh, LayerData *d, int8 *pat)
//{
//}

/*!
	\brief Called for all BView::FillTriangle calls
	\param Array of 3 BPoints. Always non-NULL.
	\param BRect enclosing the triangle. While it will definitely enclose the triangle,
	it may not be within the frame buffer's bounds.
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	\param 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the triangle may end 
	up being clipped.
*/
void DisplayDriver::FillTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat)
{
}

/*!
	\brief Hides the cursor.
	
	Hide calls are not nestable, unlike that of the BApplication class. Subclasses should
	call _SetCursorHidden(true) somewhere within this function to ensure that data is
	maintained accurately.
*/
void DisplayDriver::HideCursor(void)
{
	_SetCursorHidden(true);
}

/*!
	\brief Returns whether the cursor is visible or not.
	\return true if hidden or obscured, false if not.

*/
bool DisplayDriver::IsCursorHidden(void)
{
	_Lock();

	bool value=(_is_cursor_hidden || _is_cursor_obscured);

	_Unlock();

	return value;
}

/*!
	\brief Moves the cursor to the given point.

	The coordinates passed to MoveCursorTo are guaranteed to be within the frame buffer's
	range, but the cursor data itself will need to be clipped. A check to see if the 
	cursor is obscured should be made and if so, a call to _SetCursorObscured(false) 
	should be made the cursor in addition to displaying at the passed coordinates.
*/
void DisplayDriver::MoveCursorTo(float x, float y)
{
}

/*!
	\brief Inverts the colors in the rectangle.
	\param Rectangle of the area to be inverted. Guaranteed to be within bounds.
*/
void DisplayDriver::InvertRect(BRect r)
{
}

/*!
	\brief Shows the cursor.
	
	Show calls are not nestable, unlike that of the BApplication class. Subclasses should
	call _SetCursorHidden(false) somewhere within this function to ensure that data is
	maintained accurately.
*/
void DisplayDriver::ShowCursor(void)
{
	_SetCursorHidden(false);
}

/*!
	\brief Obscures the cursor.
	
	Obscure calls are not nestable. Subclasses should call _SetCursorObscured(true) 
	somewhere within this function to ensure that data is maintained accurately. When the
	next call to MoveCursorTo() is made, the cursor will be shown again.
*/
void DisplayDriver::ObscureCursor(void)
{
	_SetCursorObscured(true);
}

/*!
	\brief Changes the cursor.
	\param The new cursor. Guaranteed to be non-NULL.
	
	The driver does not take ownership of the given cursor. Subclasses should make
	a copy of the cursor passed to it. The default version of this function hides the
	cursory, replaces it, and shows the cursor if previously visible.
*/
void DisplayDriver::SetCursor(ServerCursor *cursor)
{
	_Lock();

	bool hidden=_is_cursor_hidden;
	bool obscured=_is_cursor_obscured;
	if(_cursor)
		delete _cursor;
	_cursor=new ServerCursor(cursor);

	if(!hidden && !obscured)
		ShowCursor();

	_Unlock();
}

void DisplayDriver::StrokeArc(BRect r, float angle, float span, LayerData *d, int8 *pat)
{
}

void DisplayDriver::StrokeBezier(BPoint *pts, LayerData *d, int8 *pat)
{
}

void DisplayDriver::StrokeEllipse(BRect r, LayerData *d, int8 *pat)
{
}

void DisplayDriver::StrokeLine(BPoint start, BPoint end, LayerData *d, int8 *pat)
{
}

void DisplayDriver::StrokePolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat, bool is_closed=true)
{
}

void DisplayDriver::StrokeRect(BRect r, LayerData *d, int8 *pat)
{
}

void DisplayDriver::StrokeRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat)
{
}

//void DisplayDriver::StrokeShape(SShape *sh, LayerData *d, int8 *pat)
//{
//}

void DisplayDriver::StrokeTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat)
{
}

void DisplayDriver::StrokeLineArray(BPoint *pts, int32 numlines, RGBColor *colors, LayerData *d)
{
}

void DisplayDriver::SetMode(int32 mode)
{
}

bool DisplayDriver::DumpToFile(const char *path)
{
	return false;
}

float DisplayDriver::StringWidth(const char *string, int32 length, LayerData *d)
{
	return 0.0;
}

float DisplayDriver::StringHeight(const char *string, int32 length, LayerData *d)
{
	return 0.0;
}

void DisplayDriver::GetBoundingBoxes(const char *string, int32 count, font_metric_mode mode, escapement_delta *delta, BRect *rectarray)
{
}

void DisplayDriver::GetEscapements(const char *string, int32 charcount, escapement_delta *delta, escapement_delta *escapements, escapement_delta *offsets)
{
}

void DisplayDriver::GetEdges(const char *string, int32 charcount, edge_info *edgearray)
{
}

void DisplayDriver::GetHasGlyphs(const char *string, int32 charcount, bool *hasarray)
{
}

void DisplayDriver::GetTruncatedStrings( const char **instrings, int32 stringcount, uint32 mode, float maxwidth, char **outstrings)
{
}

uint8 DisplayDriver::GetDepth(void)
{
	return _buffer_depth;
}

uint16 DisplayDriver::GetHeight(void)
{
	return _buffer_height;
}

uint16 DisplayDriver::GetWidth(void)
{
	return _buffer_width;
}

int32 DisplayDriver::GetMode(void)
{
	return _buffer_mode;
}


// Protected Internal Functions

bool DisplayDriver::_Lock(bigtime_t timeout)
{
	if(acquire_sem_etc(_lock_sem,1,B_RELATIVE_TIMEOUT,timeout)!=B_NO_ERROR)
		return false;
	return true;
}

void DisplayDriver::_Unlock(void)
{
	release_sem(_lock_sem);
}

void DisplayDriver::_SetDepth(uint8 d)
{
	_buffer_depth=d;
}

void DisplayDriver::_SetHeight(uint16 h)
{
	_buffer_height=h;
}

void DisplayDriver::_SetWidth(uint16 w)
{
	_buffer_width=w;
}

void DisplayDriver::_SetMode(int32 m)
{
	_buffer_mode=m;
}

void DisplayDriver::_SetCursorHidden(bool state)
{
	_is_cursor_hidden=state;
}

void DisplayDriver::_SetCursorObscured(bool state)
{
	_is_cursor_obscured=state;
}

bool DisplayDriver::_IsCursorObscured(bool state)
{
	return _is_cursor_obscured;
}

ServerCursor *DisplayDriver::_GetCursor(void)
{
	return _cursor;
}

