WinBorder class : public Layer
##############################

WinBorder objects provide window management functionality and ensure
that the border for each window is drawn.

Member Functions
================

WinBorder(BRect r, const char \*name, int32 resize, int32 flags, ServerWindow \*win)
------------------------------------------------------------------------------------

1. Pass parameters to Layer constructor
2. Instantiate a decorator
3. Initialize visible and full regions via Decorator::GetFootprint()

~WinBorder(void)
----------------

1. Delete decorator instance

void RequestDraw(void)
----------------------

Reimplements Layer::RequestDraw() because it has no corresponding BView

1. if IsDirty()==false, return
2. Iterate through each BRect in the invalid region and call Decorator::Draw(BRect) for each invalid rectangle
3. Perform recursive child calls as in Layer::RequestDraw()

void MoveBy(BPoint pt), void MoveBy(float x, float y)
-----------------------------------------------------

Moves the WinBorder's position on screen - reimplements Layer::MoveBy()

1. Call the decorator's MoveBy()
2. Call Layer::MoveBy()

void ResizeBy(BPoint pt), void ResizeBy(float x, float y)
---------------------------------------------------------

Resizes the WinBorder - reimplements Layer::MoveBy()

1. Call the decorator's ResizeBy()
2. Call Layer::ResizeBy()

void MouseDown(int8 \*buffer)
-----------------------------

Figures out what to do with B_MOUSE_DOWN messages sent to the window's border.

1. Extract data from the buffer
2. Call the decorator's Clicked() function
3. Feed return value to a switch() function (table below)
4. Call the ServerWindow's Activate() function

+-----------------------------------+-----------------------------------+
| CLICK_MOVETOBACK                  | call MoveToBack()                 |
+-----------------------------------+-----------------------------------+
| CLICK_MOVETOFRONT                 | call MoveToFront()                |
+-----------------------------------+-----------------------------------+
| CLICK_CLOSE                       | 1) call SetCloseButton(true)      |
|                                   |                                   |
|                                   | 2) call decorator->DrawClose      |
+-----------------------------------+-----------------------------------+
| CLICK_ZOOM                        | 1) call SetZoomButton(true)       |
|                                   |                                   |
|                                   | 2) call decorator->DrawZoom       |
+-----------------------------------+-----------------------------------+
| CLICK_MINIMIZE                    | 1) call SetMinimizeButton(true)   |
|                                   |                                   |
|                                   | 2) call decorator->DrawMinimize   |
+-----------------------------------+-----------------------------------+
| CLICK_DRAG                        | 1) call MoveToFront()             |
|                                   |                                   |
|                                   | 2) call set_is_win_moving(true)   |
|                                   |                                   |
|                                   | 3) Save the mouse position        |
+-----------------------------------+-----------------------------------+
| CLICK_RESIZE                      | 1) call MoveToFront()             |
|                                   |                                   |
|                                   | 2) call set_is_win_resizing(true) |
+-----------------------------------+-----------------------------------+
| CLICK_NONE                        | do nothing                        |
+-----------------------------------+-----------------------------------+
| default:                          | Spew an error to stderr and       |
|                                   | return                            |
+-----------------------------------+-----------------------------------+

void MouseUp(int8 \*buffer)
---------------------------

Figures out what to do with B_MOUSE_UP messages sent to the window's border.

1. Extract data from the buffer
2. Call the decorator's Clicked() function
3. Feed return value to a switch() function (table below)
4. if is_resizing_window, call set_is_resizing_window(false)
5. if is_moving_window, call set_is_moving_window(false)

+-----------------------------------+-----------------------------------+
| CLICK_MOVETOBACK                  | call MoveToBack()                 |
+-----------------------------------+-----------------------------------+
| CLICK_MOVETOFRONT                 | call MoveToFront()                |
+-----------------------------------+-----------------------------------+
| CLICK_CLOSE                       | 1) call SetCloseButton(false)     |
|                                   |                                   |
|                                   | 2) call decorator->DrawClose      |
|                                   |                                   |
|                                   | 3) send B_QUIT_REQUESTED to the   |
|                                   | target BWindow                    |
+-----------------------------------+-----------------------------------+
| CLICK_ZOOM                        | 1) call SetZoomButton(false)      |
|                                   |                                   |
|                                   | 2) call decorator->DrawZoom       |
|                                   |                                   |
|                                   | 3) send B_ZOOM to the target      |
|                                   | BWindow                           |
+-----------------------------------+-----------------------------------+
| CLICK_MINIMIZE                    | 1) call SetMinimizeButton(false)  |
|                                   |                                   |
|                                   | 2) call decorator->DrawMinimize   |
|                                   |                                   |
|                                   | 3) send B_MINIMIZE to the target  |
|                                   | BWindow                           |
+-----------------------------------+-----------------------------------+
| CLICK_DRAG                        | call set_is_win_moving(false)     |
+-----------------------------------+-----------------------------------+
| CLICK_RESIZE                      | call set_is_win_resizing(false)   |
+-----------------------------------+-----------------------------------+
| CLICK_NONE                        | do nothing                        |
+-----------------------------------+-----------------------------------+
| default:                          | Spew an error to stderr           |
+-----------------------------------+-----------------------------------+

void MouseMoved(int8 \*buffer)
------------------------------

Figures out what to do with B_MOUSE_MOVED messages sent to the window's border.

1. Extract data from the buffer
2. Call the decorator's Clicked() function
3. If not CLICK_CLOSE and decorator->GetClose is true, call SetClose(false) and DrawClose()
4. If not CLICK_ZOOM and decorator->GetZoom is true, call SetZoom(false) and DrawZoom()
5. If not CLICK_MINIMIZE and decorator->GetMinimize is true, call SetMinimize(false) and DrawMinimize()
6. if CLICK_RESIZE or its variants, call CursorManager::SetCursor() with the appropriate system cursor.
7. if is_moving_window() is true, calculate the amount the mouse has moved and call decorator->MoveBy() followed by Layer::MoveBy()
8. if is_resizing_window() is true, calculate the amount the mouse has moved and call decorator->ResizeBy() followed by Layer::ResizeBy()

void UpdateDecorator(void)
--------------------------

Hook function called by the WinBorder's ServerWindow when the decorator used is changed.

1. Delete the current decorator
2. Call instantiate_decorator
3. Get the new decorator's footprint region and assign it to the full and visible regions
4. Call RebuildRegions and then RequestDraw

void UpdateColors(void)
-----------------------

Hook function called by the WinBorder's ServerWindow when system colors change

1. Call the decorator's SetColors(), passing the SystemPalette's GetGUIColors() value

void UpdateFont(void)
---------------------

Hook function called by the WinBorder's ServerWindow when system fonts change

| TODO: implementation details

void UpdateScreen(void)
-----------------------

Hook function called by the WinBorder's ServerWindow when screen
attributes change

1. Call the decorator's UpdateScreen and then RequestDraw

void RebuildRegions(bool recursive=true)
----------------------------------------

Reimplementation of Layer::RebuildRegions which changes it such that
lower siblings are clipped to the footprint instead of the frame.

void Activate(bool state)
-------------------------

This function is never directly called except from within
set_active_winborder. It exists to force redraw and set the internal
state information to the proper values for when a window receives or
loses focus.

1. call the decorator's SetFocus(state)
2. set the internal is_active flag to state
3. iterate through each rectangle in the visible region and call the decorator's Draw on it.

Global Functions
================


bool is_moving_window(void), void set_is_moving_window(bool state)
------------------------------------------------------------------


These two functions set and return the variable
winborder_private::is_moving_a_window.


bool is_resizing_window(void), void set_is_resizing_window(bool state)
----------------------------------------------------------------------


These two functions set and return the variable
winborder_private::is_resizing_a_window.


void set_active_winborder(WinBorder \*win), WinBorder \* get_active_winborder(void)
-----------------------------------------------------------------------------------


These two functions set and return the variable winborder_private::active_winborder

Namespaces
==========

.. code-block:: cpp

    winborder_private {
        bool is_moving_a_window
        bool is_resizing_a_window
        WinBorder *active_winborder
    }

