CursorManager class
###################

The CursorManager class handles token creation, calling the
cursor-related graphics driver functions, and freeing heap memory for
all ServerCursor instances.

Enumerated Types
================

cursor_which
------------

- CURSOR_DEFAULT
- CURSOR_TEXT
- CURSOR_MOVE
- CURSOR_DRAG
- CURSOR_RESIZE
- CURSOR_RESIZE_NW
- CURSOR_RESIZE_SE
- CURSOR_RESIZE_NS
- CURSOR_RESIZE_EW
- CURSOR_OTHER

Member Functions
================

CursorManager(void)
-------------------

1. Create the cursor list empty
2. Set the token index to 0
3. Allocate the default system cursor and pass it to AddCursor
4. Initialize the member pointer for the graphics driver
5. Create the cursorlock semaphore
6. Call SetDefaultCursor

~CursorManager(void)
--------------------

1. Empty and delete the cursor list
2. Delete the cursorlock semaphore

int32 AddCursor(ServerCursor \*sc)
----------------------------------

AddCursor() is used to register the cursor in question with the
manager, allowing for the user application to have the identifying
token, if necessary. The cursor becomes the property of the manager.
If a user application deletes a BCursor, its ServerApp will call
DeleteCursor().

1. Acquire cursor lock
2. Add \*sc to the cursor list
3. Set sc->token to the current token index value
4. Increment the token index
5. Assign sc->token to temporary variable
6. Release cursor lock
7. Return the saved token value

void DeleteCursor(int32 ctoken)
-------------------------------

1. Acquire cursor lock
2. Iterate through the cursor list, looking for ctoken
3. If any ServerCursor->token equals ctoken, remove and delete it
4. Release cursor lock

void RemoveAppCursors(ServerApp \*app)
--------------------------------------

1. Acquire cursor lock
2. Iterate through the cursor list, checking each cursor's ServerApp
   pointer
3. If any have a ServerApp pointer which matches the passed pointer,
   remove and delete them
4. Release cursor lock

void ShowCursor(void), void HideCursor(void), void ObscureCursor(void)
----------------------------------------------------------------------

Simple pass-through functions which call the graphics driver's
functions. Note that acquiring the cursor lock will be necessary for
all three calls.

void SetCursor(int32 token), void SetCursor(cursor_which cursor)
----------------------------------------------------------------

These set the current cursor for the graphics driver to the passed
cursor, either one previously added via AddCursor or a system cursor.

1. Acquire cursor lock

Token version:

2. Find the cursor in the cursor list and call the graphics driver if
   non-NULL
3. Iterate through list of system cursor tokens and see if there's a
   match. If so, set the internal cursor_which to the match.

cursor_which version:

2. determine which cursor to use via a switch statement and call the
   graphics driver with the internal pointer for the appropriate cursor
3. set the internal cursor_which to the one passed to the function

..

4. Release cursor lock

ServerCursor \*GetCursor(cursor_which which)
--------------------------------------------

GetCursor is intended for use in figuring out what cursor is in use
for a particular system cursor.

1. Acquire cursor lock
2. use a switch statement to figure which cursor to return and assign a
   temporary pointer its value
3. Release cursor lock
4. Return the temporary pointer

void ChangeCursor(cursor_which which, int32 token)
--------------------------------------------------

Calling ChangeCursor will allow a user to change a system cursor's
appearance. Note that in calling this, the cursor changes ownership
and belongs to the system. Thus, the BCursor destructor will not
ultimately cause the cursor to be deleted.

1. Acquire cursor lock
2. Call FindCursor and, if NULL, release the cursor lock and return
3. Look up the pointer for the system cursor in question and check to
   see if it is active. If active, then set the local active flag to true.
   Set the system cursor pointer to the one looked up.
4. If active flag is true, call SetCursor()
5. Release cursor lock

cursor_which GetCursorWhich(void)
---------------------------------

Returns the current cursor_which attribute which describes the
currently active cursor. If the active cursor is not a system cursor,
it will return CURSOR_OTHER.

1. Acquire cursor lock
2. Create a local cursor_which and assign it the value of the
   CursorManager's cursor_which
3. Release cursor lock
4. Return the local copy

