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
	maintained accurately. Subclasses must include a call to DisplayDriver::HideCursor
	for proper state tracking.
*/
void DisplayDriver::HideCursor(void)
{
	_is_cursor_hidden=true;
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
	maintained accurately. Subclasses must call DisplayDriver::ShowCursor at some point
	to ensure proper state tracking.
*/
void DisplayDriver::ShowCursor(void)
{
	_is_cursor_hidden=false;
	_is_cursor_obscured=false;
}

/*!
	\brief Obscures the cursor.
	
	Obscure calls are not nestable. Subclasses should call DisplayDriver::ObscureCursor
	somewhere within this function to ensure that data is maintained accurately. A check
	will be made by the system before the next MoveCursorTo call to show the cursor if
	it is obscured.
*/
void DisplayDriver::ObscureCursor(void)
{
	_is_cursor_obscured=true;
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

/*!
	\brief Called for all BView::StrokeArc calls
	\param Rectangle enclosing the entire arc
	\param Starting angle for the arc in degrees
	\param Span of the arc in degrees. Ending angle = angle+span.
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	\param 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the arc may end up
	being clipped.
*/void DisplayDriver::StrokeArc(BRect r, float angle, float span, LayerData *d, int8 *pat)
{
}

/*!
	\brief Called for all BView::StrokeBezier calls.
	\param 4-element array of BPoints in the order of start, end, and then the two control
	points. 
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	\param 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call.
*/
void DisplayDriver::StrokeBezier(BPoint *pts, LayerData *d, int8 *pat)
{
}

/*!
	\brief Called for all BView::StrokeEllipse calls
	\param BRect enclosing the ellipse to be drawn.
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	\param 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the ellipse may end up
	being clipped.
*/
void DisplayDriver::StrokeEllipse(BRect r, LayerData *d, int8 *pat)
{
}

/*!
	\brief Draws a line. Really.
	\param Starting point
	\param Ending point
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	\param 8-byte array containing the pattern to use. Always non-NULL.
	
	The endpoints themselves are guaranteed to be in bounds, but clipping for lines with
	a thickness greater than 1 will need to be done.
*/
void DisplayDriver::StrokeLine(BPoint start, BPoint end, LayerData *d, int8 *pat)
{
}

/*!
	\brief Called for all BView::StrokePolygon calls
	\param Array of BPoints defining the polygon.
	\param Number of points in the BPoint array.
	\param Rectangle which contains the polygon
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	\param 8-byte array containing the pattern to use. Always non-NULL.

	The points in the array are not guaranteed to be within the framebuffer's 
	coordinate range.
*/
void DisplayDriver::StrokePolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat, bool is_closed=true)
{
}

/*!
	\brief Called for all BView::StrokeRect calls
	\param BRect to be filled. Guaranteed to be in the frame buffer's coordinate space
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	\param 8-byte array containing the pattern to use. Always non-NULL.

*/
void DisplayDriver::StrokeRect(BRect r, LayerData *d, int8 *pat)
{
}

/*!
	\brief Called for all BView::StrokeRoundRect calls
	\param X radius of the corner arcs
	\param Y radius of the corner arcs
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	\param 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the roundrect may end 
	up being clipped.
*/
void DisplayDriver::StrokeRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat)
{
}

//void DisplayDriver::StrokeShape(SShape *sh, LayerData *d, int8 *pat)
//{
//}

/*!
	\brief Called for all BView::StrokeTriangle calls
	\param Array of 3 BPoints. Always non-NULL.
	\param BRect enclosing the triangle. While it will definitely enclose the triangle,
	it may not be within the frame buffer's bounds.
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	\param 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the triangle may end 
	up being clipped.
*/
void DisplayDriver::StrokeTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat)
{
}

/*!
	\brief Draws a series of lines - optimized for speed
	\param Array of BPoints pairs
	\param Number of lines to be drawn
	\param Array of colors for each respective line
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	
	Data for this call is passed directly from userland - this call is responsible for all
	checking. All lines are to be processed in the call using the same LayerData settings
	for each line.
*/
void DisplayDriver::StrokeLineArray(BPoint *pts, int32 numlines, RGBColor *colors, LayerData *d)
{
}

/*!
	\brief Sets the screen mode to specified resolution and color depth.
	\param constant as defined in GraphicsDefs.h
	
	Subclasses must include calls to _SetDepth, _SetHeight, _SetWidth, and _SetMode
	to update the state variables kept internally by the DisplayDriver class.
*/
void DisplayDriver::SetMode(int32 mode)
{
}

/*!
	\brief Dumps the contents of the frame buffer to a file.
	\param Path and leaf of the file to be created without an extension
	\return False if unimplemented or unsuccessful. True if otherwise.
	
	Subclasses should add an extension based on what kind of file is saved
*/
bool DisplayDriver::DumpToFile(const char *path)
{
	return false;
}

/*!
	\brief Gets the width of a string in pixels
	\param Source null-terminated string
	\param Number of characters in the string
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	\return Width of the string in pixels
	
	This corresponds to BView::StringWidth.
*/
float DisplayDriver::StringWidth(const char *string, int32 length, LayerData *d)
{
	return 0.0;
}

/*!
	\brief Gets the height of a string in pixels
	\param Source null-terminated string
	\param Number of characters in the string
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	\return Height of the string in pixels
	
	The height calculated in this function does not include any padding - just the
	precise maximum height of the characters within and does not necessarily equate
	with a font's height, i.e. the strings 'case' and 'alps' will have different values
	even when called with all other values equal.
*/
float DisplayDriver::StringHeight(const char *string, int32 length, LayerData *d)
{
	return 0.0;
}

/*!
	\brief Retrieves the bounding box each character in the string
	\param Source null-terminated string
	\param Number of characters in the string
	\param Metrics mode for either screen or printing
	\param Optional glyph padding. This value may be NULL.
	\param Array of BRect objects which will have at least count elements
	\param Data structure containing any other data necessary for the call. Always non-NULL.

	See BFont::GetBoundingBoxes for more details on this function.
*/
void DisplayDriver::GetBoundingBoxes(const char *string, int32 count, 
		font_metric_mode mode, escapement_delta *delta, BRect *rectarray, LayerData *d)
{
}

/*!
	\brief Retrieves the escapements for each character in the string
	\param Source null-terminated string
	\param Number of characters in the string
	\param Optional glyph padding. This value may be NULL.
	\param Array of escapement_delta objects which will have at least charcount elements
	\param Actual offset values when iterating over the string. This array will also 
		have at least charcount elements and the values placed therein will reflect 
		the current kerning/spacing mode.
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	
	See BFont::GetEscapements for more details on this function.
*/
void DisplayDriver::GetEscapements(const char *string, int32 charcount, 
		escapement_delta *delta, escapement_delta *escapements, escapement_delta *offsets, LayerData *d)
{
}

/*!
	\brief Retrieves the inset values of each glyph from its escapement values
	\param Source null-terminated string
	\param Number of characters in the string
	\param Array of edge_info objects which will have at least charcount elements
	\param Data structure containing any other data necessary for the call. Always non-NULL.

	See BFont::GetEdges for more details on this function.
*/
void DisplayDriver::GetEdges(const char *string, int32 charcount, edge_info *edgearray, LayerData *d)
{
}

/*!
	\brief Determines whether a font contains a certain string of characters
	\param Source null-terminated string
	\param Number of characters in the string
	\param Array of booleans which will have at least charcount elements

	See BFont::GetHasGlyphs for more details on this function.
*/
void DisplayDriver::GetHasGlyphs(const char *string, int32 charcount, bool *hasarray)
{
}

/*!
	\brief Truncates an array of strings to a certain width
	\param Array of null-terminated strings
	\param Number of strings passed to the function
	\param Truncation mode
	\param Maximum width for all strings
	\param String array provided by the caller into which the truncated strings are
		to be placed.

	See BFont::GetTruncatedStrings for more details on this function.
*/
void DisplayDriver::GetTruncatedStrings( const char **instrings, int32 stringcount, 
	uint32 mode, float maxwidth, char **outstrings)
{
}

/*!
	\brief Returns the bit depth for the current screen mode
	\return Current number of bits per pixel
*/
uint8 DisplayDriver::GetDepth(void)
{
	return _buffer_depth;
}

/*!
	\brief Returns the height for the current screen mode
	\return Height of the screen
*/
uint16 DisplayDriver::GetHeight(void)
{
	return _buffer_height;
}

/*!
	\brief Returns the width for the current screen mode
	\return Width of the screen
*/
uint16 DisplayDriver::GetWidth(void)
{
	return _buffer_width;
}

/*!
	\brief Returns the screen mode constant in use by the driver
	\return Current screen mode
*/
int32 DisplayDriver::GetMode(void)
{
	return _buffer_mode;
}

/*!
	\brief Returns whether or not the cursor is currently obscured
	\return True if obscured, false if not.
*/
bool DisplayDriver::IsCursorObscured(bool state)
{
	return _is_cursor_obscured;
}

// Protected Internal Functions

/*!
	\brief Locks the driver
	\param Optional timeout specifier
	\param True if the lock was successful, false if not.
	
	The return value need only be checked if a timeout was specified. Each public
	member function should lock the driver before doing anything else. Functions
	internal to the driver (protected/private) need not do this.
*/
bool DisplayDriver::_Lock(bigtime_t timeout)
{
	if(acquire_sem_etc(_lock_sem,1,B_RELATIVE_TIMEOUT,timeout)!=B_NO_ERROR)
		return false;
	return true;
}

/*!
	\brief Unlocks the driver
*/
void DisplayDriver::_Unlock(void)
{
	release_sem(_lock_sem);
}

/*!
	\brief Internal depth-setting function
	\param Number of bits per pixel in use
	
	_SetDepth must be called from within any implementation of SetMode
*/
void DisplayDriver::_SetDepth(uint8 d)
{
	_buffer_depth=d;
}

/*!
	\brief Internal height-setting function
	\param Height of the frame buffer
	
	_SetHeight must be called from within any implementation of SetMode
*/
void DisplayDriver::_SetHeight(uint16 h)
{
	_buffer_height=h;
}

/*!
	\brief Internal width-setting function
	\param Width of the frame buffer
	
	_SetWidth must be called from within any implementation of SetMode
*/
void DisplayDriver::_SetWidth(uint16 w)
{
	_buffer_width=w;
}

/*!
	\brief Internal mode-setting function.
	\param Screen mode in use as defined in GraphicsDefs.h
	
	_SetMode must be called from within any implementation of SetMode. Note that this
	does not actually change the screen mode; it just updates the state variable used
	to talk with the outside world.
*/
void DisplayDriver::_SetMode(int32 m)
{
	_buffer_mode=m;
}

/*!
	\brief Obtains the current cursor for the driver.
	\return Pointer to the current cursor object.
	
	Do NOT delete this pointer - change pointers via SetCursor. This call will be 
	necessary for blitting the cursor to the screen and other such tasks.
*/
ServerCursor *DisplayDriver::_GetCursor(void)
{
	return _cursor;
}

