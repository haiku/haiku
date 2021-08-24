Decorator class
###############

Decorators provide the actual drawing for a window's looks.

\_ Indicates a protected member function

Member Functions
================

Decorator(BRect int32 wlook, int32 wfeel, int32 wflags)
-------------------------------------------------------

Sets up internal variables common to all decorators.

1) Assign parameters to respective data members

~Decorator(void)
----------------

Empty.

void SetColors(color_set colors)
--------------------------------

void SetDriver(DisplayDriver \*driver)
--------------------------------------

void SetClose(bool is_down)
---------------------------

void SetMinimize(bool is_down)
------------------------------

void SetZoom(bool is_down)
--------------------------

void SetFlags(int32 wflags)
---------------------------

void SetFeel(int32 wfeel)
-------------------------

void SetLook(int32 wlook)
-------------------------

bool GetClose(void)
-------------------

bool GetMinimize(void)
----------------------

bool GetZoom(void)
------------------

int32 GetLook(void)
-------------------

int32 GetFeel(void)
-------------------

int32 GetFlags(void)
--------------------

void SetTitle(const char \*string)
----------------------------------

void SetFont(SFont \*sf)
------------------------

These functions work with the internal members common to all
Decorators - assigning them and returning them. Additionally,
SetTitle() and SetFont() set the clip_font flag to true.

int32 \_ClipTitle(float width)
------------------------------

ClipTitle calculates how much of the title, in characters, can be
displayed in the given width.

1. Call StringWidth() on the title.
2. If the string's width is less thanwidth, return the string's
   character count
3. while the character count to display is > 0

   a. calculate the string's width
   b. if the string's width is less than width, return the character count
   c. decrement the character count

4. If the loop completes itself without returning a value, it can't fit,
   so return 0.

void SetFocus(bool is_active)
-----------------------------

This is for handling color states when a window receives or loses the
focus.

1. Set focus flag to whatever is_active is.
2. call hook function \_SetFocus()

bool GetFocus(void)
-------------------

Returns the focus state held by the decorator

1) Return the focus flag

int32 \_TitleWidth(void)
------------------------

Returns the character count of the title or 0 if it is NULL.

Virtual Functions
=================

Most of these functions have a default behavior which can be
overridden, but are implemented to handle the more common
implementations.

void MoveBy(float x, float y)
-----------------------------

void MoveBy(BPoint pt)
----------------------

Move all member rectangles of Decorator by the specified amount.

void ResizeBy(float x, float y)
-------------------------------

void ResizeBy(BPoint pt)
------------------------

Resize the client frame, window frame, and the tab frame (width only)
by the specified amount. Button rectangles - close, minimize, and zoom
- are not modified.

void Draw(BRect r)
------------------

void Draw(void)
---------------

Main drawing call which checks the intersection of the rectangle
passed to it and draws all items which intersect it. Draw(void) simply
performs drawing calls to draw the entire decorator's footprint area.

1. Check for intersection with BRect which encompasses the decorator's
   footprint and return if no intersection.
2. Call \_DrawFrame(intersection)
3. Call \_DrawTab(intersection)

void DrawClose(void)
--------------------

protected: void \_DrawClose(BRect r)
------------------------------------

void DrawMinimize(void)
-----------------------

protected: void \_DrawMinimize(BRect r)
---------------------------------------

void DrawZoom(void)
-------------------

protected: void \_DrawZoom(BRect r)
-----------------------------------

Each of these is designed to utilize their respective button
rectangles. The public (void) versions simply call the internal
protected ones with the button rectangle. These protected versions
are, by default, empty. The rectangle passed to them is the invalid
area to be drawn, which is not necessarily the entire button's
rectangle.

void DrawFrame(void)
--------------------

protected: void \_DrawFrame(BRect r)
------------------------------------

Draws the frame, if any. The public version amounts to
\_DrawFrame(framerect). The protected version is expected to not cover
up the client frame when drawing. Any drawing within the clientrect
member will end up being drawn over by the window's child views.

void DrawTab(void)
------------------

protected: void \_DrawTab(BRect r)
----------------------------------

Draws the window's tab, if any. DrawTab() amounts to
\_DrawTab(tabrect). If window titles are displayed, the \_DrawTitle
call is expected to be made here. Button-drawing calls, assuming that
a window's buttons are in the tab, should be made here, as well.

void DrawTitle(void)
--------------------

protected: void \_DrawTitle(BRect r)
------------------------------------

These cause the window's title to be drawn. DrawTitle() amounts to
\_DrawTitle(titlerect).

void \_SetFocus(void)
---------------------

This hook function is primarily used to change colors used when a
window changes focus states and is called immediately after the state
is changed. If, for example, a decorator does not use OpenBeOS' GUI
color set, it would change its drawing colors to reflect the change in
focus.

SRegion GetFootprint(void)
--------------------------

This returns the "footprint" of the decorator, i.e. the area which is
occupied by the window which is is the border surrounding the main
client rectangle. It is possible to have oddly-shaped window borders,
like ellipses and circles, but the resulting performance hit would
reduce the said decorator to a novelty and not something useable. All
versions are to construct an SRegion which the border occupies. This
footprint is permitted to include the client rectangle area, but this
area must not be actually drawn upon by the decorator itself. The
default version returns the frame which encompasses all other
rectangles - the "frame" member which belongs to its window border.

click_type Clicked(BPoint pt, int32 buttons, int32 modifiers)
-------------------------------------------------------------

Clicked() performs hit testing for the decorator, given input
conditions. This function is required by ALL subclasses expecting to
do more than display itself. The return type will cause the server to
take the appropriate actions, such as close the window, get ready to
move it, etc.


BRect SlideTab(float dx, dy=0)
------------------------------

SlideTab is implemented only for those decorators which allow the user
to somehow slide the tab (if there is one) along the window.
Currently, only the horizontal direction is supported. It returns the
rectangle of the invalid region which needs redrawn as a result of the
slide.

Exported C Functions
====================

extern "C" Decorator \*create_decorator(BRect frame, int32 wlook, int32 wfeel, int32 wflags)
--------------------------------------------------------------------------------------------

Required export function which simply allocates an instance of the
decorator and returns it.

extern "C" float get_decorator_version(void)
--------------------------------------------

This should, for now, return 1.00.

Enumerated Types
================

click_type
----------

- CLICK_NONE
- CLICK_ZOOM
- CLICK_CLOSE
- CLICK_MINIMIZE
- CLICK_TAB
- CLICK_MOVE
- CLICK_MOVETOBACK
- CLICK_MOVETOFRONT
- CLICK_RESIZE
- CLICK_RESIZE_L
- CLICK_RESIZE_T
- CLICK_RESIZE_R
- CLICK_RESIZE_B
- CLICK_RESIZE_LT
- CLICK_RESIZE_RT
- CLICK_RESIZE_LB
- CLICK_RESIZE_RB

