//------------------------------------------------------------------------------
//	Copyright (c) 2001-2003, OpenBeOS
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
//	File Name:		AccelerantDriver.cpp
//	Author:			Gabe Yoder <gyoder@stny.rr.com>
//	Description:		A display driver which works directly with the
//				accelerant for the graphics card.
//  
//------------------------------------------------------------------------------
#include "AccelerantDriver.h"
#include "ServerCursor.h"
#include "ServerBitmap.h"
#include <FindDirectory.h>
#include <graphic_driver.h>
#include <malloc.h>

/*!
	\brief Sets up internal variables needed by AccelerantDriver
	
*/
AccelerantDriver::AccelerantDriver(void) : DisplayDriver()
{
  //drawmode = DRAW_COPY;
  drawmode = 0;

  cursor=NULL;
  under_cursor=NULL;
  cursorframe.Set(0,0,0,0);

  card_fd = -1;
  accelerant_image = -1;
//  accelerant_hook = NULL;
  mode_list = NULL;
}


/*!
	\brief Deletes the heap memory used by the AccelerantDriver
	
*/
AccelerantDriver::~AccelerantDriver(void)
{
  if (cursor)
	delete cursor;
  if (under_cursor)
	delete under_cursor;
  if (mode_list)
	free(mode_list);
}

/*!
	\brief Initializes the driver object.
	\return true if successful, false if not
	
	Initialize sets up the driver for display, including the initial clearing
	of the screen. If things do not go as they should, false should be returned.
*/
bool AccelerantDriver::Initialize(void)
{
  int i;
  char signature[1024];
  char path[PATH_MAX];
  struct stat accelerant_stat;
  const static directory_which dirs[] =
  {
	B_USER_ADDONS_DIRECTORY,
	B_COMMON_ADDONS_DIRECTORY,
	B_BEOS_ADDONS_DIRECTORY
  };

  //card_fd = open("/dev/graphics/1002_4755_000400",B_READ_WRITE);
  card_fd = open("/dev/graphics/nv10_010000",B_READ_WRITE);
  //card_fd = open("/dev/graphics/stub",B_READ_WRITE);
printf("foo 1\n");
  if ( card_fd < 0 )
	return false;
printf("foo 2\n");
  if (ioctl(card_fd, B_GET_ACCELERANT_SIGNATURE, &signature, sizeof(signature)) != B_OK)
  {
	close(card_fd);
	return false;
  }
printf("signature %s\n",signature);
printf("foo 3\n");
  accelerant_image = -1;
  for (i=0; i<3; i++)
  {
printf("BAR 1\n");
	if (find_directory (dirs[i], -1, false, path, PATH_MAX) != B_OK)
	  continue;
printf("BAR 2\n");
	strcat(path,"/accelerants/");
	strcat(path,signature);
	if (stat(path, &accelerant_stat) != 0)
	  continue;
printf("BAR 3\n");
	accelerant_image = load_add_on(path);
printf("image_id %d\n",accelerant_image);
	if (accelerant_image >= 0)
	{
printf("BAR 4\n");
	  if ( get_image_symbol(accelerant_image,B_ACCELERANT_ENTRY_POINT,
			B_SYMBOL_TYPE_ANY,(void**)(&accelerant_hook)) != B_OK )
		return false;
printf("BAR 5\n");
	  init_accelerant InitAccelerant;
	  InitAccelerant = (init_accelerant)accelerant_hook(B_INIT_ACCELERANT,NULL);
	  if (!InitAccelerant || (InitAccelerant(card_fd) != B_OK))
		return false;
printf("BAR 6\n");
	  break;
	}
  }
printf("foo 4\n");
  if (accelerant_image < 0)
	return false;

  int mode_count;
  accelerant_mode_count GetModeCount = (accelerant_mode_count)accelerant_hook(B_ACCELERANT_MODE_COUNT, NULL);
printf("foo 5\n");
  if ( !GetModeCount )
	return false;
  mode_count = GetModeCount();
printf("foo 6\n");
  if ( !mode_count )
	return false;
  get_mode_list GetModeList = (get_mode_list)accelerant_hook(B_GET_MODE_LIST, NULL);
printf("foo 7\n");
  if ( !GetModeList )
	return false;
  mode_list = (display_mode *)calloc(sizeof(display_mode), mode_count);
printf("foo 8\n");
  if ( !mode_list )
	return false;
printf("foo 9\n");
  if ( GetModeList(mode_list) != B_OK )
	return false;
  set_display_mode SetDisplayMode = (set_display_mode)accelerant_hook(B_SET_DISPLAY_MODE, NULL);
printf("foo 10\n");
  if ( !SetDisplayMode )
	return false;
  /* Use the first mode in the list */
printf("foo 11\n");
  if ( SetDisplayMode(mode_list) != B_OK )
	return false;
  get_frame_buffer_config GetFrameBufferConfig = (get_frame_buffer_config)accelerant_hook(B_GET_FRAME_BUFFER_CONFIG, NULL);
printf("foo 12\n");
  if ( !GetFrameBufferConfig )
	return false;
printf("foo 13\n");
  if ( GetFrameBufferConfig(&mFrameBufferConfig) != B_OK )
	return false;
printf("foo 14\n");
  _SetDepth(mode_list[0].space);
  _SetWidth(mode_list[0].virtual_width);
  _SetHeight(mode_list[0].virtual_height);
  //_SetMode(mode_list[0].flags); ???
  memset(mFrameBufferConfig.frame_buffer,0,sizeof(mFrameBufferConfig.frame_buffer));

  return true;
}

/*!
	\brief Shuts down the driver's video subsystem
	
	Any work done by Initialize() should be undone here. Note that Shutdown() is
	called even if Initialize() was unsuccessful.
*/
void AccelerantDriver::Shutdown(void)
{
  uninit_accelerant UninitAccelerant = (uninit_accelerant)(*accelerant_hook)(B_UNINIT_ACCELERANT,NULL);
  if ( UninitAccelerant )
    UninitAccelerant();
  if (accelerant_image >= 0)
	unload_add_on(accelerant_image);
  if (card_fd >= 0)
	close(card_fd);
}

/*!
	\brief Called for all BView::CopyBits calls
	\param Source rectangle.
	\param Destination rectangle.
	
	Bounds checking must be done in this call. If the destination is not the same size 
	as the source, the source should be scaled to fit.
*/
void AccelerantDriver::CopyBits(BRect src, BRect dest)
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
void AccelerantDriver::DrawBitmap(ServerBitmap *bmp, BRect src, BRect dest, LayerData *d)
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
void AccelerantDriver::DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *delta=NULL)
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
void AccelerantDriver::FillArc(BRect r, float angle, float span, LayerData *d, int8 *pat)
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
void AccelerantDriver::FillBezier(BPoint *pts, LayerData *d, int8 *pat)
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
void AccelerantDriver::FillEllipse(BRect r, LayerData *d, int8 *pat)
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
void AccelerantDriver::FillPolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat)
{
}

/*!
	\brief Called for all BView::FillRect calls
	\param BRect to be filled. Guaranteed to be in the frame buffer's coordinate space
	\param Data structure containing any other data necessary for the call. Always non-NULL.
	\param 8-byte array containing the pattern to use. Always non-NULL.

*/
void AccelerantDriver::FillRect(BRect r, LayerData *d, int8 *pat)
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
void AccelerantDriver::FillRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat)
{
}

//void AccelerantDriver::FillShape(SShape *sh, LayerData *d, int8 *pat)
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
void AccelerantDriver::FillTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat)
{
}

/*!
	\brief Hides the cursor.
	
	Hide calls are not nestable, unlike that of the BApplication class. Subclasses should
	call _SetCursorHidden(true) somewhere within this function to ensure that data is
	maintained accurately.
*/
void AccelerantDriver::HideCursor(void)
{
	_SetCursorHidden(true);
}

/*!
	\brief Moves the cursor to the given point.

	The coordinates passed to MoveCursorTo are guaranteed to be within the frame buffer's
	range, but the cursor data itself will need to be clipped. A check to see if the 
	cursor is obscured should be made and if so, a call to _SetCursorObscured(false) 
	should be made the cursor in addition to displaying at the passed coordinates.
*/
void AccelerantDriver::MoveCursorTo(float x, float y)
{
}

/*!
	\brief Inverts the colors in the rectangle.
	\param Rectangle of the area to be inverted. Guaranteed to be within bounds.
*/
void AccelerantDriver::InvertRect(BRect r)
{
}

/*!
	\brief Shows the cursor.
	
	Show calls are not nestable, unlike that of the BApplication class. Subclasses should
	call _SetCursorHidden(false) somewhere within this function to ensure that data is
	maintained accurately.
*/
void AccelerantDriver::ShowCursor(void)
{
	_SetCursorHidden(false);
}

/*!
	\brief Obscures the cursor.
	
	Obscure calls are not nestable. Subclasses should call _SetCursorObscured(true) 
	somewhere within this function to ensure that data is maintained accurately. When the
	next call to MoveCursorTo() is made, the cursor will be shown again.
*/
void AccelerantDriver::ObscureCursor(void)
{
	_SetCursorObscured(true);
}

/*!
	\brief Changes the cursor.
	\param The new cursor. Guaranteed to be non-NULL.
	
	The driver does not take ownership of the given cursor. Subclasses should make
	a copy of the cursor passed to it. The default version of this function hides the
	cursor, replaces it, and shows the cursor if previously visible.
*/
void AccelerantDriver::SetCursor(ServerCursor *cursor)
{
}

void AccelerantDriver::StrokeArc(BRect r, float angle, float span, LayerData *d, int8 *pat)
{
}

void AccelerantDriver::StrokeBezier(BPoint *pts, LayerData *d, int8 *pat)
{
}

void AccelerantDriver::StrokeEllipse(BRect r, LayerData *d, int8 *pat)
{
}

void AccelerantDriver::StrokeLine(BPoint start, BPoint end, LayerData *d, int8 *pat)
{
}

void AccelerantDriver::StrokePolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat, bool is_closed=true)
{
}

void AccelerantDriver::StrokeRect(BRect r, LayerData *d, int8 *pat)
{
}

void AccelerantDriver::StrokeRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat)
{
}

//void AccelerantDriver::StrokeShape(SShape *sh, LayerData *d, int8 *pat)
//{
//}

void AccelerantDriver::StrokeTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat)
{
}

void AccelerantDriver::StrokeLineArray(BPoint *pts, int32 numlines, RGBColor *colors, LayerData *d)
{
}

void AccelerantDriver::SetMode(int32 mode)
{
}

bool AccelerantDriver::DumpToFile(const char *path)
{
	return false;
}

float AccelerantDriver::StringWidth(const char *string, int32 length, LayerData *d)
{
	return 0.0;
}

float AccelerantDriver::StringHeight(const char *string, int32 length, LayerData *d)
{
	return 0.0;
}

void AccelerantDriver::GetBoundingBoxes(const char *string, int32 count, font_metric_mode mode, escapement_delta *delta, BRect *rectarray)
{
}

void AccelerantDriver::GetEscapements(const char *string, int32 charcount, escapement_delta *delta, escapement_delta *escapements, escapement_delta *offsets)
{
}

void AccelerantDriver::GetEdges(const char *string, int32 charcount, edge_info *edgearray)
{
}

void AccelerantDriver::GetHasGlyphs(const char *string, int32 charcount, bool *hasarray)
{
}

void AccelerantDriver::GetTruncatedStrings( const char **instrings, int32 stringcount, uint32 mode, float maxwidth, char **outstrings)
{
}


// Protected Internal Functions

