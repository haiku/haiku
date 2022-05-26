Screen class
############

The Screen class handles all infrastructure needed for each
monitor/video card pair. The Workspace class holds data and supplies
the infrastructure for drawing the screen.

Member Functions
================

Screen(DisplayDriver \*gfxmodule, uint8 workspaces)
---------------------------------------------------

1. Set driver pointer to gfxmodule
2. If driver pointer is non-NULL and driver->Initialize() is true, set
   initialized flag to true
3. If initialized, get the appropriate display driver info and save it
   internally
4. Create and populate workspace list
5. Clear all workspaces


~Screen(void)
-------------

1. Remove all workspaces from the list and delete them
2. Delete the workspace list

void AddWorkspace(int32 index=-1)
---------------------------------

Adds a workspace to the screen object, setting its settings to the
default, and adding it to the list in the specified index or the end
of the list if the index is -1


1. Create a workspace object
2. Add it to the workspace list - to the end if the index is -1, and to
   the particular index if not

void DeleteWorkspace(int32 index)
---------------------------------

Deletes the workspace at the specified index.


1. Remove the item at the specified index and if non-NULL, delete it.

int32 CountWorkspaces(void)
---------------------------

Returns the number of workspaces kept by the particular Screen objects

1. Return the workspace list's CountItems() value

void SetWorkspaceCount(int32 count)
-----------------------------------

Sets the number of workspaces available to count. If count is less
than the number of current workspaces, the last workspaces are deleted
first. Workspaces are added to the end of the list. If a delete
workspace should include the active workspace, then the workspace with
the index count-1 is activated. There must be at least one workspace.

1. if count equals the workspace list's CountItems value, return
2. If count is less than 1, set count to 1. If greater than 32, set to 32.
3. If active workspace index is greater than count-1, call SetWorkspace(count-1)
4. If count is greater than the workspace list's CountItems value, call
   AddWorkspace the appropriate number of times.
5. If count is less than the workspace list's CountItems, call
   RemoveWorkspace the appropriate number of times

int32 CurrentWorkspace(void)
----------------------------

Returns the active workspace index

void SetWorkspace(int32 workspace)
----------------------------------

void Activate(bool active=true)
-------------------------------

DisplayDriver \*GetDriver(void)
-------------------------------

status_t SetSpace(int32 index, int32 res, bool stick=true)
----------------------------------------------------------

void AddWindow(ServerWindow \*win, int32 workspace=B_CURRENT_WORKSPACE)
-----------------------------------------------------------------------

void RemoveWindow(ServerWindow \*win)
-------------------------------------

ServerWindow \*ActiveWindow(void)
---------------------------------

void SetActiveWindow(ServerWindow \*win)
----------------------------------------

Layer \*GetRootLayer(int32 workspace=B_CURRENT_WORKSPACE)
---------------------------------------------------------

bool IsInitialized(void)
------------------------

Workspace \*GetActiveWorkspace(void)
------------------------------------

Workspace class members
=======================

Workspace(void)
---------------

1. Set background color to RGB(51,102,160)
2. Copy frame_buffer_info and graphics_card_info structure values
3. Create a RootLayer object using the values from the two structures

~Workspace(void)
----------------

1. Call the RootLayer object's PruneTree method and delete it

void SetBGColor(const RGBColor &c)
----------------------------------

Sets the particular color for the workspace. Note that this does not
refresh the display.

1. Set the internal background RGBColor object to the color parameter

RGBColor BGColor(void)
----------------------

Returns the internal background color


RootLayer \*GetRoot(void)
-------------------------

Returns the pointer to the RootLayer object


void SetData(graphics_card_info \*gcinfo, frame_buffer_info \*fbinfo)
---------------------------------------------------------------------

Changes the graphics data and resizes the RootLayer accordingly.


1. Copy the two structures to the internal one
2. Resize the RootLayer
3. If the RootLayer was resized larger, Invalidate the new areas


void GetData(graphics_card_info \*gcinfo, frame_buffer_info \*fbinfo)
---------------------------------------------------------------------

Copies the two data structures into the parameters passed.

RootLayer class members
=======================

RootLayer(BRect frame, const char \*name)


1. passes B_FOLLOW_NONE to Layer constructor
2. set level to 0
3. set the background color to the color for the workspace set in the
   system preferences


~RootLayer(void)
----------------

Does nothing.


void RequestDraw(const BRect &r)
--------------------------------

Requests that the layer be drawn on screen. The rectangle passed is in
the layer's own coordinates.


1. call the display driver's FillRect on the rectangle, filling with
   the layer's background color
2. recurse through each child and call its RequestDraw() function if it
   intersects the child's frame

void MoveBy(BPoint pt), void MoveBy(float x, float y)
-----------------------------------------------------

Made empty so that the root layer cannot be moved

void SetDriver(DisplayDriver \*d)
---------------------------------

Assigns a particular display driver object to the root layer if
non-NULL.


void RebuildRegions(bool recursive=false)
-----------------------------------------

Rebuilds the visible and invalid layers based on the layer hierarchy.
Used to update the regions after a call to remove or add a child layer
is made or when a layer is hidden or shown.


1. get the frame
2. set full and visible regions to frame
3. iterate through each child and exclude its full region from the
   visible region if the child is visible.

