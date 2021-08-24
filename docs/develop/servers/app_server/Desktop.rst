Desktop module
##############

There are no globally accessible objects in this section of code, but
many function definitions to work with workspaces, screen attributes,
and other such things. These functions work with the private desktop
classes Screen and Workspace.

Global Functions
================

void InitDesktop(void)
----------------------

Sets up all the stuff necessary for the system's desktop.

1. Create a graphics module list by looping through allocation and
   initialization of display modules until failure is returned. If
   app_server exists, just create a ViewDriver module
2. Create and populate Screen list, pointing each Screen instance to its
   Driver instance
3. Create layer and workspace locks
4. Set screen 0 to active
5. Create the desktop_private::draglock semaphore

void ShutdownDesktop(void)
--------------------------

Undoes everything done in InitDesktop().

1. Delete all locks
2. Delete all screens in the list
3. Delete the desktop_private::draglock semaphore
4. If desktop_private::dragmessage is non-NULL, delete it.

void AddWorkspace(int32 index=-1)
---------------------------------

void DeleteWorkspace(int32 index)
---------------------------------

int32 CountWorkspaces(void)
---------------------------

void SetWorkspaceCount(int32 count)
-----------------------------------

int32 CurrentWorkspace(screen_id screen=B_MAIN_SCREEN_ID)
---------------------------------------------------------

void SetWorkspace(int32 workspace, screen_id screen=B_MAIN_SCREEN_ID)
---------------------------------------------------------------------

Each of these calls the appropriate method in the Screen class. Add
and Delete workspace functions operate on all screens.

void SetScreen(screen_id id)
----------------------------

int32 CountScreens(void)
------------------------

screen_id ActiveScreen(void)
----------------------------

DisplayDriver \*GetGfxDriver(screen_id screen=B_MAIN_SCREEN_ID)
---------------------------------------------------------------

status_t SetSpace(int32 index, int32 res, bool stick=true, screen_id screen=B_MAIN_SCREEN_ID)
---------------------------------------------------------------------------------------------

Each of these calls operate on the objects in the Screen list, calling
methods as appropriate.

void AddWindowToDesktop(ServerWindow \*win, int32 workspace=B_CURRENT_WORKSPACE, screen_id screen=B_MAIN_SCREEN_ID)
-------------------------------------------------------------------------------------------------------------------

void RemoveWindowFromDesktop(ServerWindow \*win)
------------------------------------------------

ServerWindow \*GetActiveWindow(void)
------------------------------------

void SetActiveWindow(ServerWindow \*win)
----------------------------------------

Layer \*GetRootLayer(int32 workspace=B_CURRENT_WORKSPACE, screen_id screen=B_MAIN_SCREEN_ID)
--------------------------------------------------------------------------------------------

These operate on the layer structure in the active Screen object,
calling methods as appropriate.

void set_drag_message(int32 size, int8 \*flattened)
---------------------------------------------------

This assigns a BMessage in its flattened state to the internal
storage. Only one message can be stored at a time. Once an assignment
is made, another cannot be made until the current one is emptied. If
additional calls are made while there is a drag message assigned, the
new assignment will not be made. Note that this merely passes a
pointer around. No copies are made and the caller is responsible for
freeing any allocated objects from the heap.

1. Acquire the draglock
2. release the lock and return if winborder_private::dragmessage is
   non-NULL
3. Assign parameters to the private variables
4. Release the draglock

int8\* get_drag_message(int32 \*size)
-------------------------------------

Retrieve the current drag message in use. This function will fail if a
NULL pointer is passed to it. Note that this does not free the current
drag message from use. The size of the flattened message is stored in
the size parameter. Note that if there is no message in use, it will
return NULL and set size to 0.


1. return NULL if size pointer is NULL
2. acquire the draglock
3. set value of size parameter to winborder_private::dragmessagesize
4. assign a temporary pointer the value of desktop_private::dragmessage
5. release the draglock
6. return the temporary pointer

void empty_drag\_ message(void)
-------------------------------

This empties the current drag message from use and allows for other
messages to be assigned.


1. acquire draglock
2. assign 0 to desktop_private::dragmessagesize and set
   desktop_private::dragmessage to NULL
3. release draglock

Namespaces
==========

desktop_private
---------------

- int8 \*dragmessage
- int32 dragmessagesize
- sem_id draglock
