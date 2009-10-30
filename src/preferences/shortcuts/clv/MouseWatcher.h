/****DOCUMENTATION
Once started, MouseWatcher will watch the mouse until the mouse buttons are all released, sending
messages to the target BView (TargetView is specified as the target handler in the BMessenger used to
send the messages.  The BLooper == window of the target view is determined automatically by the
BMessenger)

If the mouse moves, a MW_MOUSE_MOVED message is sent.
If the mouse buttons are changed, but not released, a MW_MOUSE_DOWN message is sent.
If the mouse button(s) are released, a MW_MOUSE_UP message is sent.

These messages will have three data entries:

"where" (B_POINT_TYPE)		- The position of the mouse in TargetView's coordinate system.
"buttons" (B_INT32_TYPE)	- The mouse buttons.  See BView::GetMouse().
"modifiers" (B_INT32_TYPE)	- The modifier keys held down at the time.  See modifiers().

Once it is started, you can't stop it, but that shouldn't matter - the user will most likely release
the buttons soon, and you can interpret the events however you want.

StartMouseWatcher returns a valid thread ID, or it returns an error code:
B_NO_MORE_THREADS. all thread_id numbers are currently in use.
B_NO_MEMORY. Not enough memory to allocate the resources for another thread.
****/
#ifndef MouseWatcher_h
#define MouseWatcher_h

#include <SupportDefs.h>
#include <OS.h>

class BView;

const uint32 MW_MOUSE_DOWN = 'Mw-D';
const uint32 MW_MOUSE_UP = 'Mw-U';
const uint32 MW_MOUSE_MOVED = 'Mw-M';

thread_id StartMouseWatcher(BView* TargetView);

#endif

