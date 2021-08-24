DisplayDriver class
###################

The DisplayDriver class is not a useful class unto itself. It is to
provide a consistent interface for the rest of the app_server to
whatever rendering context it is utilizing, whether it be a remote
screen, a ServerBitmap, installed graphics hardware, or whatever.
Documentation below will describe the role of each function.

Member Functions
================

DisplayDriver(void)
-------------------

~DisplayDriver(void)
--------------------

bool Initialize(void)
---------------------

void Shutdown(void)
-------------------

These four are for general start and stop procedures. The constructor
and destructor concern themselves with the internal members common to
all drivers, such as the current cursor and the access semaphore.
Subclasses will probably end up using these to handle memory
allocation-related issues, but likely not much else. Initialize() and
Shutdown() are for general setup internal to the module. Note that if
Initialize() returns false, the server will not use the module, call
Shutdown(), and then delete it as accordingly.

void CopyBits(BRect src, BRect dest)
------------------------------------

void InvertRect(BRect r)
------------------------

void DrawBitmap(ServerBitmap \*bmp, BRect src, BRect dest, render_mode mode)
----------------------------------------------------------------------------

void DrawPicture(SPicture \*pic, BPoint pt)
-------------------------------------------

void DrawChar(char c, BPoint pt)
--------------------------------

void DrawString(const char \*string, int32 length, BPoint pt, escapement_delta \*delta=NULL)
--------------------------------------------------------------------------------------------

void StrokeArc(BRect r, float angle, float span, layerdata \*d, int8 \*pattern)
-------------------------------------------------------------------------------

void FillArc(BRect r, float angle, float span, layerdata \*d, int8 \*pattern)
-----------------------------------------------------------------------------

void StrokeBezier(BPoint \*pts, layerdata \*d, int8 \*pat)
----------------------------------------------------------

void FillBezier(BPoint \*pts, layerdata \*d, int8 \*pat)
--------------------------------------------------------

void StrokeEllipse(BRect r, layerdata \*d, int8 \*pattern)
----------------------------------------------------------

void FillEllipse(BRect r, layerdata \*d, int8 \*pattern)
--------------------------------------------------------

void StrokeLine(BPoint start, BPoint end, layerdata \*d, int8 \*pattern)
------------------------------------------------------------------------

void StrokeLineArray(BPoint \*pts, int32 numlines, rgb_color \*colors, layerdata \*d)
-------------------------------------------------------------------------------------

void StrokePolygon(BPoint \*ptlist, int32 numpts, BRect rect, layerdata \*d, int8 \*pattern, bool is_closed=true)
-----------------------------------------------------------------------------------------------------------------

void FillPolygon(BPoint \*ptlist, int32 numpts, BRect rect, layerdata \*d, int8 \*pattern)
------------------------------------------------------------------------------------------

void StrokeRect(BRect r, layerdata \*d, int8 \*pattern)
-------------------------------------------------------

void FillRect(BRect r, layerdata \*d, int8 \*pattern)
-----------------------------------------------------

void StrokeRoundRect(BRect r, float xrad, float yrad, layerdata \*d, int8 \*pattern)
------------------------------------------------------------------------------------

void FillRoundRect(BRect r, float xrad, float yrad, layerdata \*d, int8 \*pattern)
----------------------------------------------------------------------------------

void StrokeShape(SShape \*sh, layerdata \*d, int8 \*pattern)
------------------------------------------------------------

void FillShape(SShape \*sh, layerdata \*d, int8 \*pattern)
----------------------------------------------------------

void StrokeTriangle(BPoints \*pts, BRect r, layerdata \*d, int8 \*pattern)
--------------------------------------------------------------------------

void FillTriangle(BPoints \*pts, BRect r, layerdata \*d, int8 \*pattern)
------------------------------------------------------------------------

void ShowCursor(void)
---------------------

void HideCursor(void)
---------------------

void ObscureCursor(void)
------------------------

bool IsCursorHidden(void)
-------------------------

void SetCursor(ServerCursor \*csr)
----------------------------------

float StringWidth(const char \*string, int32 length, LayerData \*d)
-------------------------------------------------------------------

float StringHeight(const char \*string, int32 length, LayerData \*d)
--------------------------------------------------------------------


These drawing functions are the meat and potatoes of the graphics
module. Defining any or all of them is completely optional. However,
the default versions of these functions will do nothing. Thus,
implementing them is likely a good idea, even if not required.



Protected Functions
===================

uint8 GetDepth(void)
--------------------

uint16 GetHeight(void)
----------------------

uint16 GetWidth(void)
---------------------

screen_mode GetMode(void)
-------------------------

void SetMode(screen_mode mode)
------------------------------


These five functions are called internally in order to get information
about the current state of the buffer in the module. GetDepth should
return 8, 16, or 32, in any event because the server handles RGB color
spaces of these depths only.


bool DumpToFile(const char \*path)
----------------------------------

DumpToFile is completely optional, providing a hook which allows
screenshots to be taken. The default version does nothing but return
false. If a screenshot is successful, return true.


void Lock(void)
---------------

void Unlock(void)
-----------------


These two functions provide a locking scheme for the driver in order
to easily make it thread safe. Note that it is not publicly callable.


void SetDepthInternal(uint8 d)
------------------------------

void SetHeightInternal(uint16 h)
--------------------------------

void SetWidthInternal(uint16 w)
-------------------------------

void SetModeInternal(int32 m)
-----------------------------


These four functions set the internal state variables for height,
width, etc. If the driver reimplements the public members SetDepth(),
etc, be sure to call the respective internal call so that calls to
GetDepth(), etc. return the proper values.


void SetCursorHidden(bool state)
--------------------------------

void SetCursorObscured(bool state)
----------------------------------

bool IsCursorObscured(void)
---------------------------


These calls handle internal state tracking so that subclasses don't
have to.

