/*
 * Copyright 2001-2015 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		DarkWyrm, bpmagic@columbus.rr.com
 *		John Scipione, jscipione@gmail.com
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 *		Joseph Groover <looncraz@looncraz.net>
 */


/*!	Base class for window decorators */


#include "Decorator.h"

#include <stdio.h>

#include <Region.h>

#include "Desktop.h"
#include "DesktopSettings.h"
#include "DrawingEngine.h"


Decorator::Tab::Tab()
	:
	tabRect(),

	zoomRect(),
	closeRect(),
	minimizeRect(),

	closePressed(false),
	zoomPressed(false),
	minimizePressed(false),

	look(B_TITLED_WINDOW_LOOK),
	flags(0),
	isFocused(false),
	title(""),

	tabOffset(0),
	tabLocation(0.0f),
	textOffset(10.0f),

	truncatedTitle(""),
	truncatedTitleLength(0),

	buttonFocus(false),
	isHighlighted(false),

	minTabSize(0.0f),
	maxTabSize(0.0f)
{
	closeBitmaps[0] = closeBitmaps[1] = closeBitmaps[2] = closeBitmaps[3]
		= minimizeBitmaps[0] = minimizeBitmaps[1] = minimizeBitmaps[2]
		= minimizeBitmaps[3] = zoomBitmaps[0] = zoomBitmaps[1] = zoomBitmaps[2]
		= zoomBitmaps[3] = NULL;
}


/*!	\brief Constructor

	Does general initialization of internal data members and creates a colorset
	object.

	\param settings DesktopSettings pointer.
	\param frame Decorator frame rectangle
*/
Decorator::Decorator(DesktopSettings& settings, BRect frame,
					Desktop* desktop)
	:
	fLocker("Decorator"),

	fDrawingEngine(NULL),
	fDrawState(),

	fTitleBarRect(),
	fFrame(frame),
	fResizeRect(),
	fBorderRect(),

	fLeftBorder(),
	fTopBorder(),
	fBottomBorder(),
	fRightBorder(),

	fBorderWidth(-1),

	fTopTab(NULL),

	fDesktop(desktop),
	fFootprintValid(false)
{
	memset(&fRegionHighlights, HIGHLIGHT_NONE, sizeof(fRegionHighlights));
}


/*!	\brief Destructor

	Frees the color set and the title string
*/
Decorator::~Decorator()
{
}


Decorator::Tab*
Decorator::AddTab(DesktopSettings& settings, const char* title,
	window_look look, uint32 flags, int32 index, BRegion* updateRegion)
{
	AutoWriteLocker _(fLocker);

	Decorator::Tab* tab = _AllocateNewTab();
	if (tab == NULL)
		return NULL;
	tab->title = title;
	tab->look = look;
	tab->flags = flags;

	bool ok = false;
	if (index >= 0) {
		if (fTabList.AddItem(tab, index) == true)
			ok = true;
	} else if (fTabList.AddItem(tab) == true)
		ok = true;

	if (ok == false) {
		delete tab;
		return NULL;
	}

	Decorator::Tab* oldTop = fTopTab;
	fTopTab = tab;
	if (_AddTab(settings, index, updateRegion) == false) {
		fTabList.RemoveItem(tab);
		delete tab;
		fTopTab = oldTop;
		return NULL;
	}

	_InvalidateFootprint();
	return tab;
}


bool
Decorator::RemoveTab(int32 index, BRegion* updateRegion)
{
	AutoWriteLocker _(fLocker);

	Decorator::Tab* tab = fTabList.RemoveItemAt(index);
	if (tab == NULL)
		return false;

	_RemoveTab(index, updateRegion);

	delete tab;
	_InvalidateFootprint();
	return true;
}


bool
Decorator::MoveTab(int32 from, int32 to, bool isMoving, BRegion* updateRegion)
{
	AutoWriteLocker _(fLocker);

	if (_MoveTab(from, to, isMoving, updateRegion) == false)
		return false;
	if (fTabList.MoveItem(from, to) == false) {
		// move the tab back
		_MoveTab(from, to, isMoving, updateRegion);
		return false;
	}
	return true;
}


int32
Decorator::TabAt(const BPoint& where) const
{
	AutoReadLocker _(fLocker);

	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		if (tab->tabRect.Contains(where))
			return i;
	}

	return -1;
}


void
Decorator::SetTopTab(int32 tab)
{
	AutoWriteLocker _(fLocker);
	fTopTab = fTabList.ItemAt(tab);
}


/*!	\brief Assigns a display driver to the decorator
	\param driver A valid DrawingEngine object
*/
void
Decorator::SetDrawingEngine(DrawingEngine* engine)
{
	AutoWriteLocker _(fLocker);

	fDrawingEngine = engine;
	// lots of subclasses will depend on the driver for text support, so call
	// _DoLayout() after we have it
	if (fDrawingEngine != NULL)
		_DoLayout();
}


/*!	\brief Sets the decorator's window flags

	While this call will not update the screen, it will affect how future
	updates work and immediately affects input handling.

	\param flags New value for the flags
*/
void
Decorator::SetFlags(int32 tab, uint32 flags, BRegion* updateRegion)
{
	AutoWriteLocker _(fLocker);

	// we're nice to our subclasses - we make sure B_NOT_{H|V|}_RESIZABLE
	// are in sync (it's only a semantical simplification, not a necessity)
	if ((flags & (B_NOT_H_RESIZABLE | B_NOT_V_RESIZABLE))
			== (B_NOT_H_RESIZABLE | B_NOT_V_RESIZABLE))
		flags |= B_NOT_RESIZABLE;
	if (flags & B_NOT_RESIZABLE)
		flags |= B_NOT_H_RESIZABLE | B_NOT_V_RESIZABLE;

	Decorator::Tab* decoratorTab = fTabList.ItemAt(tab);
	if (decoratorTab == NULL)
		return;
	_SetFlags(decoratorTab, flags, updateRegion);
	_InvalidateFootprint();
		// the border might have changed (smaller/larger tab)
}


/*!	\brief Called whenever the system fonts are changed.
*/
void
Decorator::FontsChanged(DesktopSettings& settings, BRegion* updateRegion)
{
	AutoWriteLocker _(fLocker);

	_FontsChanged(settings, updateRegion);
	_InvalidateFootprint();
}


/*!	\brief Called when a system colors change.
*/
void
Decorator::ColorsChanged(DesktopSettings& settings, BRegion* updateRegion)
{
	AutoWriteLocker _(fLocker);

	UpdateColors(settings);

	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());

	_InvalidateBitmaps();
}


/*!	\brief Sets the decorator's window look
	\param look New value for the look
*/
void
Decorator::SetLook(int32 tab, DesktopSettings& settings, window_look look,
	BRegion* updateRect)
{
	AutoWriteLocker _(fLocker);

	Decorator::Tab* decoratorTab = fTabList.ItemAt(tab);
	if (decoratorTab == NULL)
		return;

	_SetLook(decoratorTab, settings, look, updateRect);
	_InvalidateFootprint();
		// the border very likely changed
}


/*!	\brief Returns the decorator's window look
	\return the decorator's window look
*/
window_look
Decorator::Look(int32 tab) const
{
	AutoReadLocker _(fLocker);
	return TabAt(tab)->look;
}


/*!	\brief Returns the decorator's window flags
	\return the decorator's window flags
*/
uint32
Decorator::Flags(int32 tab) const
{
	AutoReadLocker _(fLocker);
	return TabAt(tab)->flags;
}


/*!	\brief Returns the decorator's border rectangle
	\return the decorator's border rectangle
*/
BRect
Decorator::BorderRect() const
{
	AutoReadLocker _(fLocker);
	return fBorderRect;
}


BRect
Decorator::TitleBarRect() const
{
	AutoReadLocker _(fLocker);
	return fTitleBarRect;
}


/*!	\brief Returns the decorator's tab rectangle
	\return the decorator's tab rectangle
*/
BRect
Decorator::TabRect(int32 tab) const
{
	AutoReadLocker _(fLocker);

	Decorator::Tab* decoratorTab = fTabList.ItemAt(tab);
	if (decoratorTab == NULL)
		return BRect();
	return decoratorTab->tabRect;
}


BRect
Decorator::TabRect(Decorator::Tab* tab) const
{
	return tab->tabRect;
}


/*!	\brief Sets the close button's value.

	Note that this does not update the button's look - it just updates the
	internal button value

	\param tab The tab index
	\param pressed Whether the button is down or not
*/
void
Decorator::SetClose(int32 tab, bool pressed)
{
	AutoWriteLocker _(fLocker);

	Decorator::Tab* decoratorTab = fTabList.ItemAt(tab);
	if (decoratorTab == NULL)
		return;

	if (pressed != decoratorTab->closePressed) {
		decoratorTab->closePressed = pressed;
		DrawClose(tab);
	}
}


/*!	\brief Sets the minimize button's value.

	Note that this does not update the button's look - it just updates the
	internal button value

	\param is_down Whether the button is down or not
*/
void
Decorator::SetMinimize(int32 tab, bool pressed)
{
	AutoWriteLocker _(fLocker);

	Decorator::Tab* decoratorTab = fTabList.ItemAt(tab);
	if (decoratorTab == NULL)
		return;

	if (pressed != decoratorTab->minimizePressed) {
		decoratorTab->minimizePressed = pressed;
		DrawMinimize(tab);
	}
}

/*!	\brief Sets the zoom button's value.

	Note that this does not update the button's look - it just updates the
	internal button value

	\param is_down Whether the button is down or not
*/
void
Decorator::SetZoom(int32 tab, bool pressed)
{
	AutoWriteLocker _(fLocker);

	Decorator::Tab* decoratorTab = fTabList.ItemAt(tab);
	if (decoratorTab == NULL)
		return;

	if (pressed != decoratorTab->zoomPressed) {
		decoratorTab->zoomPressed = pressed;
		DrawZoom(tab);
	}
}


/*!	\brief Updates the value of the decorator title
	\param string New title value
*/
void
Decorator::SetTitle(int32 tab, const char* string, BRegion* updateRegion)
{
	AutoWriteLocker _(fLocker);

	Decorator::Tab* decoratorTab = fTabList.ItemAt(tab);
	if (decoratorTab == NULL)
		return;

	decoratorTab->title.SetTo(string);
	_SetTitle(decoratorTab, string, updateRegion);

	_InvalidateFootprint();
		// the border very likely changed

	// TODO: redraw?
}


/*!	\brief Returns the decorator's title
	\return the decorator's title
*/
const char*
Decorator::Title(int32 tab) const
{
	AutoReadLocker _(fLocker);

	Decorator::Tab* decoratorTab = fTabList.ItemAt(tab);
	if (decoratorTab == NULL)
		return "";

	return decoratorTab->title;
}


const char*
Decorator::Title(Decorator::Tab* tab) const
{
	AutoReadLocker _(fLocker);
	return tab->title;
}


float
Decorator::TabLocation(int32 tab) const
{
	AutoReadLocker _(fLocker);

	Decorator::Tab* decoratorTab = _TabAt(tab);
	if (decoratorTab == NULL)
		return 0.0f;

	return (float)decoratorTab->tabOffset;
}


bool
Decorator::SetTabLocation(int32 tab, float location, bool isShifting,
	BRegion* updateRegion)
{
	AutoWriteLocker _(fLocker);

	Decorator::Tab* decoratorTab = fTabList.ItemAt(tab);
	if (decoratorTab == NULL)
		return false;
	if (_SetTabLocation(decoratorTab, location, isShifting, updateRegion)) {
		_InvalidateFootprint();
		return true;
	}
	return false;
}



/*!	\brief Changes the focus value of the decorator

	While this call will not update the screen, it will affect how future
	updates work.

	\param active True if active, false if not
*/
void
Decorator::SetFocus(int32 tab, bool active)
{
	AutoWriteLocker _(fLocker);

	Decorator::Tab* decoratorTab = fTabList.ItemAt(tab);
	if (decoratorTab == NULL)
		return;
	decoratorTab->isFocused = active;
	_SetFocus(decoratorTab);
	// TODO: maybe it would be cleaner to handle the redraw here.
}


bool
Decorator::IsFocus(int32 tab) const
{
	AutoReadLocker _(fLocker);

	Decorator::Tab* decoratorTab = fTabList.ItemAt(tab);
	if (decoratorTab == NULL)
		return false;

	return decoratorTab->isFocused;
};


bool
Decorator::IsFocus(Decorator::Tab* tab) const
{
	AutoReadLocker _(fLocker);
	return tab->isFocused;
}


//	#pragma mark - virtual methods


/*!	\brief Returns a cached footprint if available otherwise recalculate it
*/
const BRegion&
Decorator::GetFootprint()
{
	AutoReadLocker _(fLocker);

	if (!fFootprintValid) {
		_GetFootprint(&fFootprint);
		fFootprintValid = true;
	}
	return fFootprint;
}


/*!	\brief Returns our Desktop object pointer
*/
::Desktop*
Decorator::GetDesktop()
{
	AutoReadLocker _(fLocker);
	return fDesktop;
}


/*!	\brief Performs hit-testing for the decorator.

	The base class provides a basic implementation, recognizing only button and
	tab hits. Derived classes must override/enhance it to handle borders and
	corners correctly.

	\param where The point to be tested.
	\return Either of the following, depending on what was hit:
		- \c REGION_NONE: None of the decorator regions.
		- \c REGION_TAB: The window tab (but none of the buttons embedded).
		- \c REGION_CLOSE_BUTTON: The close button.
		- \c REGION_ZOOM_BUTTON: The zoom button.
		- \c REGION_MINIMIZE_BUTTON: The minimize button.
		- \c REGION_LEFT_BORDER: The left border.
		- \c REGION_RIGHT_BORDER: The right border.
		- \c REGION_TOP_BORDER: The top border.
		- \c REGION_BOTTOM_BORDER: The bottom border.
		- \c REGION_LEFT_TOP_CORNER: The left-top corner.
		- \c REGION_LEFT_BOTTOM_CORNER: The left-bottom corner.
		- \c REGION_RIGHT_TOP_CORNER: The right-top corner.
		- \c REGION_RIGHT_BOTTOM_CORNER The right-bottom corner.
*/
Decorator::Region
Decorator::RegionAt(BPoint where, int32& tabIndex) const
{
	AutoReadLocker _(fLocker);

	tabIndex = -1;

	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		if (tab->closeRect.Contains(where)) {
			tabIndex = i;
			return REGION_CLOSE_BUTTON;
		}
		if (tab->zoomRect.Contains(where)) {
			tabIndex = i;
			return REGION_ZOOM_BUTTON;
		}
		if (tab->tabRect.Contains(where)) {
			tabIndex = i;
			return REGION_TAB;
		}
	}

	return REGION_NONE;
}


/*!	\brief Moves the decorator frame and all default rectangles

	If a subclass implements this method, be sure to call Decorator::MoveBy
	to ensure that internal members are also updated. All members of the
	Decorator class are automatically moved in this method

	\param x X Offset
	\param y y Offset
*/
void
Decorator::MoveBy(float x, float y)
{
	MoveBy(BPoint(x, y));
}


/*!	\brief Moves the decorator frame and all default rectangles

	If a subclass implements this method, be sure to call Decorator::MoveBy
	to ensure that internal members are also updated. All members of the
	Decorator class are automatically moved in this method

	\param offset BPoint containing the offsets
*/
void
Decorator::MoveBy(BPoint offset)
{
	AutoWriteLocker _(fLocker);

	if (fFootprintValid)
		fFootprint.OffsetBy(offset.x, offset.y);

	_MoveBy(offset);
}


/*!	\brief Resizes the decorator frame

	This is a required function for subclasses to implement - the default does
	nothing. Note that window resize flags should be followed and fFrame should
	be resized accordingly. It would also be a wise idea to ensure that the
	window's rectangles are not inverted.

	\param x x offset
	\param y y offset
*/
void
Decorator::ResizeBy(float x, float y, BRegion* dirty)
{
	ResizeBy(BPoint(x, y), dirty);
}


void
Decorator::ResizeBy(BPoint offset, BRegion* dirty)
{
	AutoWriteLocker _(fLocker);
	_ResizeBy(offset, dirty);
	_InvalidateFootprint();
}


void
Decorator::ExtendDirtyRegion(Region region, BRegion& dirty)
{
	AutoReadLocker _(fLocker);

	switch (region) {
		case REGION_TAB:
			dirty.Include(fTitleBarRect);
			break;

		case REGION_CLOSE_BUTTON:
			if ((fTopTab->flags & B_NOT_CLOSABLE) == 0) {
				for (int32 i = 0; i < fTabList.CountItems(); i++)
					dirty.Include(fTabList.ItemAt(i)->closeRect);
			}
			break;

		case REGION_MINIMIZE_BUTTON:
			if ((fTopTab->flags & B_NOT_MINIMIZABLE) == 0) {
				for (int32 i = 0; i < fTabList.CountItems(); i++)
					dirty.Include(fTabList.ItemAt(i)->minimizeRect);
			}
			break;

		case REGION_ZOOM_BUTTON:
			if ((fTopTab->flags & B_NOT_ZOOMABLE) == 0) {
				for (int32 i = 0; i < fTabList.CountItems(); i++)
					dirty.Include(fTabList.ItemAt(i)->zoomRect);
			}
			break;

		case REGION_LEFT_BORDER:
			if (fLeftBorder.IsValid()) {
				// fLeftBorder doesn't include the corners, so we have to add
				// them manually.
				BRect rect(fLeftBorder);
				rect.top = fTopBorder.top;
				rect.bottom = fBottomBorder.bottom;
				dirty.Include(rect);
			}
			break;

		case REGION_RIGHT_BORDER:
			if (fRightBorder.IsValid()) {
				// fRightBorder doesn't include the corners, so we have to add
				// them manually.
				BRect rect(fRightBorder);
				rect.top = fTopBorder.top;
				rect.bottom = fBottomBorder.bottom;
				dirty.Include(rect);
			}
			break;

		case REGION_TOP_BORDER:
			dirty.Include(fTopBorder);
			break;

		case REGION_BOTTOM_BORDER:
			dirty.Include(fBottomBorder);
			break;

		case REGION_RIGHT_BOTTOM_CORNER:
			if ((fTopTab->flags & B_NOT_RESIZABLE) == 0)
				dirty.Include(fResizeRect);
			break;

		default:
			break;
	}
}


/*!	\brief Sets a specific highlight for a decorator region.

	Can be overridden by derived classes, but the base class version must be
	called, if the highlight shall be applied.

	\param region The decorator region.
	\param highlight The value identifying the kind of highlight.
	\param dirty The dirty region to be extended, if the highlight changes. Can
		be \c NULL.
	\return \c true, if the highlight could be applied.
*/
bool
Decorator::SetRegionHighlight(Region region, uint8 highlight, BRegion* dirty,
	int32 tab)
{
	AutoWriteLocker _(fLocker);

	int32 index = (int32)region - 1;
	if (index < 0 || index >= REGION_COUNT - 1)
		return false;

	if (fRegionHighlights[index] == highlight)
		return true;
	fRegionHighlights[index] = highlight;

	if (dirty != NULL)
		ExtendDirtyRegion(region, *dirty);

	return true;
}


bool
Decorator::SetSettings(const BMessage& settings, BRegion* updateRegion)
{
	AutoWriteLocker _(fLocker);

	if (_SetSettings(settings, updateRegion)) {
		_InvalidateFootprint();
		return true;
	}
	return false;
}


bool
Decorator::GetSettings(BMessage* settings) const
{
	AutoReadLocker _(fLocker);

	if (!fTitleBarRect.IsValid())
		return false;

	if (settings->AddRect("tab frame", fTitleBarRect) != B_OK)
		return false;

	if (settings->AddFloat("border width", fBorderWidth) != B_OK)
		return false;

	// TODO only add the location of the tab of the window who requested the
	// settings
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = _TabAt(i);
		if (settings->AddFloat("tab location", (float)tab->tabOffset) != B_OK)
			return false;
	}

	return true;
}


void
Decorator::GetSizeLimits(int32* minWidth, int32* minHeight,
	int32* maxWidth, int32* maxHeight) const
{
	AutoReadLocker _(fLocker);

	float minTabSize = 0;
	if (CountTabs() > 0)
		minTabSize = _TabAt(0)->minTabSize;

	if (fTitleBarRect.IsValid()) {
		*minWidth = (int32)roundf(max_c(*minWidth,
			minTabSize - 2 * fBorderWidth));
	}
	if (fResizeRect.IsValid()) {
		*minHeight = (int32)roundf(max_c(*minHeight,
			fResizeRect.Height() - fBorderWidth));
	}
}


//! draws the tab, title, and buttons
void
Decorator::DrawTab(int32 tabIndex)
{
	AutoReadLocker _(fLocker);

	Decorator::Tab* tab = fTabList.ItemAt(tabIndex);
	if (tab == NULL)
		return;

	_DrawTab(tab, tab->tabRect);
	_DrawZoom(tab, false, tab->zoomRect);
	_DrawMinimize(tab, false, tab->minimizeRect);
	_DrawTitle(tab, tab->tabRect);
	_DrawClose(tab, false, tab->closeRect);
}


//! draws the title
void
Decorator::DrawTitle(int32 tab)
{
	AutoReadLocker _(fLocker);

	Decorator::Tab* decoratorTab = fTabList.ItemAt(tab);
	if (decoratorTab == NULL)
		return;
	_DrawTitle(decoratorTab, decoratorTab->tabRect);
}


//! Draws the close button
void
Decorator::DrawClose(int32 tab)
{
	AutoReadLocker _(fLocker);

	Decorator::Tab* decoratorTab = fTabList.ItemAt(tab);
	if (decoratorTab == NULL)
		return;

	_DrawClose(decoratorTab, true, decoratorTab->closeRect);
}


//! draws the minimize button
void
Decorator::DrawMinimize(int32 tab)
{
	AutoReadLocker _(fLocker);

	Decorator::Tab* decoratorTab = fTabList.ItemAt(tab);
	if (decoratorTab == NULL)
		return;

	_DrawTab(decoratorTab, decoratorTab->minimizeRect);
}


//! draws the zoom button
void
Decorator::DrawZoom(int32 tab)
{
	AutoReadLocker _(fLocker);

	Decorator::Tab* decoratorTab = fTabList.ItemAt(tab);
	if (decoratorTab == NULL)
		return;
	_DrawZoom(decoratorTab, true, decoratorTab->zoomRect);
}


rgb_color
Decorator::UIColor(color_which which)
{
	AutoReadLocker _(fLocker);
	DesktopSettings settings(fDesktop);
	return settings.UIColor(which);
}


float
Decorator::BorderWidth()
{
	AutoReadLocker _(fLocker);
	return fBorderWidth;
}


float
Decorator::TabHeight()
{
	AutoReadLocker _(fLocker);

	if (fTitleBarRect.IsValid())
		return fTitleBarRect.Height();

	return fBorderWidth;
}


// #pragma mark - Protected methods


Decorator::Tab*
Decorator::_AllocateNewTab()
{
	Decorator::Tab* tab = new(std::nothrow) Decorator::Tab;
	if (tab == NULL)
		return NULL;

	// Set appropriate colors based on the current focus value. In this case,
	// each decorator defaults to not having the focus.
	_SetFocus(tab);
	return tab;
}


void
Decorator::_DrawTabs(BRect rect)
{
	Decorator::Tab* focusTab = NULL;
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		if (tab->isFocused) {
			focusTab = tab;
			continue;
		}
		_DrawTab(tab, rect);
	}

	if (focusTab != NULL)
		_DrawTab(focusTab, rect);
}


//! Hook function called when the decorator changes focus
void
Decorator::_SetFocus(Decorator::Tab* tab)
{
}


bool
Decorator::_SetTabLocation(Decorator::Tab* tab, float location, bool isShifting,
	BRegion* /*updateRegion*/)
{
	return false;
}


Decorator::Tab*
Decorator::_TabAt(int32 index) const
{
	return static_cast<Decorator::Tab*>(fTabList.ItemAt(index));
}


void
Decorator::_FontsChanged(DesktopSettings& settings, BRegion* updateRegion)
{
	// get previous extent
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());

	_InvalidateBitmaps();

	_UpdateFont(settings);
	_DoLayout();

	_InvalidateFootprint();
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());
}


void
Decorator::_SetLook(Decorator::Tab* tab, DesktopSettings& settings,
	window_look look, BRegion* updateRegion)
{
	// TODO: we could be much smarter about the update region

	// get previous extent
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());

	tab->look = look;

	_UpdateFont(settings);
	_DoLayout();

	_InvalidateFootprint();
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());
}


void
Decorator::_SetFlags(Decorator::Tab* tab, uint32 flags, BRegion* updateRegion)
{
	// TODO: we could be much smarter about the update region

	// get previous extent
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());

	tab->flags = flags;
	_DoLayout();

	_InvalidateFootprint();
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());
}


void
Decorator::_MoveBy(BPoint offset)
{
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);

		tab->zoomRect.OffsetBy(offset);
		tab->closeRect.OffsetBy(offset);
		tab->minimizeRect.OffsetBy(offset);
		tab->tabRect.OffsetBy(offset);
	}
	fTitleBarRect.OffsetBy(offset);
	fFrame.OffsetBy(offset);
	fResizeRect.OffsetBy(offset);
	fBorderRect.OffsetBy(offset);
}


bool
Decorator::_SetSettings(const BMessage& settings, BRegion* updateRegion)
{
	return false;
}


/*!	\brief Returns the "footprint" of the entire window, including decorator

	This function is required by all subclasses.

	\param region Region to be changed to represent the window's screen
		footprint
*/
void
Decorator::_GetFootprint(BRegion *region)
{
}


void
Decorator::_InvalidateFootprint()
{
	fFootprintValid = false;
}


void
Decorator::_InvalidateBitmaps()
{
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = static_cast<Decorator::Tab*>(_TabAt(i));
		for (int32 index = 0; index < 4; index++) {
			tab->closeBitmaps[index] = NULL;
			tab->minimizeBitmaps[index] = NULL;
			tab->zoomBitmaps[index] = NULL;
		}
	}
}
