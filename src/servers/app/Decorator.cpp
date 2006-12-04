/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

/**	Base class for window decorators */

#include "Decorator.h"
#include "DrawingEngine.h"

#include <Region.h>

#include <stdio.h>


/*!
	\brief Constructor
	\param rect Size of client area
	\param wlook style of window look. See Window.h
	\param wfeel style of window feel. See Window.h
	\param wflags various window flags. See Window.h
	
	Does general initialization of internal data members and creates a colorset
	object. 
*/
Decorator::Decorator(DesktopSettings& settings, BRect rect,
		window_look look, uint32 flags)
	:
	fDrawingEngine(NULL),
	fDrawState(),

	fLook(look),
	fFlags(flags),

	fZoomRect(),
	fCloseRect(),
	fMinimizeRect(),
	fTabRect(),
	fFrame(rect),
	fResizeRect(),
	fBorderRect(),

	fClosePressed(false),
	fZoomPressed(false),
	fMinimizePressed(false),
	fIsFocused(false),
	fTitle("")
{
}


/*!
	\brief Destructor
	
	Frees the color set and the title string
*/
Decorator::~Decorator()
{
}


/*!
	\brief Assigns a display driver to the decorator
	\param driver A valid DrawingEngine object
*/
void
Decorator::SetDrawingEngine(DrawingEngine* engine)
{
	fDrawingEngine = engine;
	// lots of subclasses will depend on the driver for text support, so call
	// _DoLayout() after we have it
	if (fDrawingEngine) {
		_DoLayout();
	}
}


/*!
	\brief Sets the decorator's window flags
	\param flags New value for the flags

	While this call will not update the screen, it will affect how future
	updates work and immediately affects input handling.
*/
void
Decorator::SetFlags(uint32 flags, BRegion* updateRegion)
{
	// we're nice to our subclasses - we make sure B_NOT_{H|V|}_RESIZABLE are in sync
	// (it's only a semantical simplification, not a necessity)
	if ((flags & (B_NOT_H_RESIZABLE | B_NOT_V_RESIZABLE))
			== (B_NOT_H_RESIZABLE | B_NOT_V_RESIZABLE))
		flags |= B_NOT_RESIZABLE;
	if (flags & B_NOT_RESIZABLE)
		flags |= B_NOT_H_RESIZABLE | B_NOT_V_RESIZABLE;

	fFlags = flags;
}


/*
	\brief Sets the decorator's font
	\param font The new font object to copy from
*/
void
Decorator::SetFont(ServerFont *font)
{
	if (font)
		fDrawState.SetFont(*font);
}


/*!
	\brief Sets the decorator's window look
	\param look New value for the look
*/
void
Decorator::SetLook(DesktopSettings& settings, window_look look,
	BRegion* updateRect)
{
	fLook = look;
}


/*!
	\brief Sets the close button's value.
	\param is_down Whether the button is down or not
	
	Note that this does not update the button's look - it just updates the
	internal button value
*/
void
Decorator::SetClose(bool is_down)
{
	if (is_down != fClosePressed) {
		fClosePressed = is_down;
		DrawClose();
	}
}

/*!
	\brief Sets the minimize button's value.
	\param is_down Whether the button is down or not
	
	Note that this does not update the button's look - it just updates the
	internal button value
*/
void
Decorator::SetMinimize(bool is_down)
{
	if (is_down != fMinimizePressed) {
		fMinimizePressed = is_down;
		DrawMinimize();
	}
}

/*!
	\brief Sets the zoom button's value.
	\param is_down Whether the button is down or not
	
	Note that this does not update the button's look - it just updates the
	internal button value
*/
void
Decorator::SetZoom(bool is_down)
{
	if (is_down != fZoomPressed) {
		fZoomPressed = is_down;
		DrawZoom();
	}
}


/*!
	\brief Updates the value of the decorator title
	\param string New title value
*/
void
Decorator::SetTitle(const char* string, BRegion* updateRegion)
{
	fTitle.SetTo(string);
	_DoLayout();
	// TODO: redraw?
}


/*!
	\brief Returns the decorator's window look
	\return the decorator's window look
*/
window_look
Decorator::Look() const
{
	return fLook;
}


/*!
	\brief Returns the decorator's window flags
	\return the decorator's window flags
*/
uint32
Decorator::Flags() const
{
	return fFlags;
}


/*!
	\brief Returns the decorator's title
	\return the decorator's title
*/
const char*
Decorator::Title() const
{
	return fTitle.String();
}


/*!
	\brief Returns the decorator's border rectangle
	\return the decorator's border rectangle
*/
BRect
Decorator::BorderRect() const
{
	return fBorderRect;
}


/*!
	\brief Returns the decorator's tab rectangle
	\return the decorator's tab rectangle
*/
BRect
Decorator::TabRect() const
{
	return fTabRect;
}

/*!
	\brief Returns the value of the close button
	\return true if down, false if up
*/
bool
Decorator::GetClose()
{
	return fClosePressed;
}

/*!
	\brief Returns the value of the minimize button
	\return true if down, false if up
*/
bool
Decorator::GetMinimize()
{
	return fMinimizePressed;
}

/*!
	\brief Returns the value of the zoom button
	\return true if down, false if up
*/
bool
Decorator::GetZoom()
{
	return fZoomPressed;
}

// GetSizeLimits
void
Decorator::GetSizeLimits(int32* minWidth, int32* minHeight,
						 int32* maxWidth, int32* maxHeight) const
{
}

/*!
	\brief Changes the focus value of the decorator
	\param is_active True if active, false if not
	
	While this call will not update the screen, it will affect how future
	updates work.
*/
void
Decorator::SetFocus(bool is_active)
{
	fIsFocused = is_active;
	_SetFocus();
	// TODO: maybe it would be cleaner to handle the redraw here.
}

//-------------------------------------------------------------------------
//						Virtual Methods
//-------------------------------------------------------------------------

/*!
	\brief Returns the "footprint" of the entire window, including decorator
	\param region Region to be changed to represent the window's screen footprint
	
	This function is required by all subclasses.
*/
void
Decorator::GetFootprint(BRegion *region)
{
}

/*!
	\brief Performs hit-testing for the decorator
	\return The type of area clicked

	Clicked is called whenever it has been determined that the window has received a 
	mouse click. The default version returns DEC_NONE. A subclass may use any or all
	of them.
	
	Click type : Action taken by the server
	
	- \c DEC_NONE : Do nothing
	- \c DEC_ZOOM : Handles the zoom button (setting states, etc)
	- \c DEC_CLOSE : Handles the close button (setting states, etc)
	- \c DEC_MINIMIZE : Handles the minimize button (setting states, etc)
	- \c DEC_TAB : Currently unused
	- \c DEC_DRAG : Moves the window to the front and prepares to move the window
	- \c DEC_MOVETOBACK : Moves the window to the back of the stack
	- \c DEC_MOVETOFRONT : Moves the window to the front of the stack
	- \c DEC_SLIDETAB : Initiates tab-sliding

	- \c DEC_RESIZE : Handle window resizing as appropriate
	- \c DEC_RESIZE_L
	- \c DEC_RESIZE_T
	- \c DEC_RESIZE_R
	- \c DEC_RESIZE_B
	- \c DEC_RESIZE_LT
	- \c DEC_RESIZE_RT
	- \c DEC_RESIZE_LB
	- \c DEC_RESIZE_RB

	This function is required by all subclasses.
	
*/
click_type
Decorator::Clicked(BPoint pt, int32 buttons, int32 modifiers)
{
	return DEC_NONE;
}

/*!
	\brief Moves the decorator frame and all default rectangles
	\param x X Offset
	\param y y Offset
	
	If a subclass implements this method, be sure to call Decorator::MoveBy
	to ensure that internal members are also updated. All members of the Decorator
	class are automatically moved in this method
*/
void
Decorator::MoveBy(float x, float y)
{
	MoveBy(BPoint(x, y));
}

/*!
	\brief Moves the decorator frame and all default rectangles
	\param pt Point containing the offsets
	
	If a subclass implements this method, be sure to call Decorator::MoveBy
	to ensure that internal members are also updated. All members of the Decorator
	class are automatically moved in this method
*/
void
Decorator::MoveBy(BPoint pt)
{
	fZoomRect.OffsetBy(pt);
	fCloseRect.OffsetBy(pt);
	fMinimizeRect.OffsetBy(pt);
	fMinimizeRect.OffsetBy(pt);
	fTabRect.OffsetBy(pt);
	fFrame.OffsetBy(pt);
	fResizeRect.OffsetBy(pt);
	fBorderRect.OffsetBy(pt);
}

/*!
	\brief Resizes the decorator frame
	\param dx x offset
	\param dy y offset
	
	This is a required function for subclasses to implement - the default does nothing.
	Note that window resize flags should be followed and fFrame should be resized
	accordingly. It would also be a wise idea to ensure that the window's rectangles
	are not inverted. 
*/
void
Decorator::ResizeBy(float x, float y, BRegion* dirty)
{
	ResizeBy(BPoint(x, y), dirty);
}


bool
Decorator::SetSettings(const BMessage& settings, BRegion* updateRegion)
{
	return false;
}


bool
Decorator::GetSettings(BMessage* settings) const
{
	return false;
}


/*!
	\brief Updates the decorator's look in the area r
	\param r The area to update.
	
	The default version updates all areas which intersect the frame and tab.
*/
void
Decorator::Draw(BRect r)
{
	_DrawFrame(r & fFrame);
	_DrawTab(r & fTabRect);
}

//! Forces a complete decorator update
void
Decorator::Draw()
{
	_DrawFrame(fFrame);
	_DrawTab(fTabRect);
}

//! Draws the close button
void
Decorator::DrawClose()
{
	_DrawClose(fCloseRect);
}

//! draws the frame
void
Decorator::DrawFrame()
{
	_DrawFrame(fFrame);
}

//! draws the minimize button
void
Decorator::DrawMinimize(void)
{
	_DrawTab(fMinimizeRect);
}

//! draws the tab, title, and buttons
void
Decorator::DrawTab()
{
	_DrawTab(fTabRect);
	_DrawZoom(fZoomRect);
	_DrawMinimize(fMinimizeRect);
	_DrawTitle(fTabRect);
	_DrawClose(fCloseRect);
}

// draws the title
void
Decorator::DrawTitle()
{
	_DrawTitle(fTabRect);
}

//! draws the zoom button
void
Decorator::DrawZoom(void)
{
	_DrawZoom(fZoomRect);
}


RGBColor
Decorator::UIColor(color_which which)
{
	// TODO: for now - calling ui_color() from within the app_server
	//	will always return the default colors (as there is no be_app)
	return ui_color(which);
}


/*!
	\brief Provides the number of characters that will fit in the given width
	\param width Maximum number of pixels the title can be
	\return the number of characters that will fit in the given width
*/
int32
Decorator::_ClipTitle(float width)
{
	// TODO: eventually, use ServerFont::TruncateString()
	// when it exists (if it doesn't already)
	if (fDrawingEngine) {
		int32 strlength = fTitle.CountChars();
		float pixwidth=fDrawingEngine->StringWidth(fTitle.String(),strlength,&fDrawState);

		while (strlength >= 0) {
			if (pixwidth < width)
				return strlength;

			strlength--;
			pixwidth=fDrawingEngine->StringWidth(fTitle.String(), strlength, &fDrawState);
		}
	}
	return 0;
}

//! Function for calculating layout for the decorator
void
Decorator::_DoLayout()
{
}

/*!
	\brief Actually draws the frame
	\param r Area of the frame to update
*/
void
Decorator::_DrawFrame(BRect r)
{
}

/*!
	\brief Actually draws the tab
	\param r Area of the tab to update
	
	This function is called when the tab itself needs drawn. Other items, like the 
	window title or buttons, should not be drawn here.
*/
void
Decorator::_DrawTab(BRect r)
{
}

/*!
	\brief Actually draws the close button
	\param r Area of the button to update
	
	Unless a subclass has a particularly large button, it is probably unnecessary
	to check the update rectangle.
*/
void
Decorator::_DrawClose(BRect r)
{
}

/*!
	\brief Actually draws the title
	\param r area of the title to update
	
	The main tasks for this function are to ensure that the decorator draws the title
	only in its own area and drawing the title itself. Using B_OP_COPY for drawing
	the title is recommended because of the marked performance hit of the other 
	drawing modes, but it is not a requirement.
*/
void
Decorator::_DrawTitle(BRect r)
{
}

/*!
	\brief Actually draws the zoom button
	\param r Area of the button to update
	
	Unless a subclass has a particularly large button, it is probably unnecessary
	to check the update rectangle.
*/
void
Decorator::_DrawZoom(BRect r)
{
}

/*!
	\brief Actually draws the minimize button
	\param r Area of the button to update
	
	Unless a subclass has a particularly large button, it is probably unnecessary
	to check the update rectangle.
*/
void
Decorator::_DrawMinimize(BRect r)
{
}

//! Hook function called when the decorator changes focus
void
Decorator::_SetFocus()
{
}
