#ifndef	_OBWINDOW_H
#define	_OBWINDOW_H

#include <BeBuild.h>
#include <StorageDefs.h>
#include <InterfaceDefs.h>
#include <List.h>
#include <Looper.h>
#include <Rect.h>
#include <Window.h>
#include <String.h>

class BButton;
class BMenuBar;
class BMenuItem;
class BMessage;
class BMessageRunner;
class BMessenger;
class OBView;
class PortLink;

struct message;
struct _cmd_key_;
struct _view_attr_;

#define ADDWINDOW '_adw'
#define REMOVEWINDOW '_rmw'

class OBWindow : public BLooper 
{
public:
OBWindow(BRect frame,const char *title, window_type type,
uint32 flags,uint32 workspace = B_CURRENT_WORKSPACE);

OBWindow(BRect frame,const char *title, window_look look,window_feel feel,
uint32 flags,uint32 workspace = B_CURRENT_WORKSPACE);

virtual					~OBWindow();

virtual	void			Quit();
void			Close();

void			AddChild(OBView *child, OBView *before = NULL);
bool			RemoveChild(OBView *child);
int32			CountChildren() const;
OBView			*ChildAt(int32 index) const;

virtual	void			DispatchMessage(BMessage *message, BHandler *handler);
virtual	void			MessageReceived(BMessage *message);
virtual	void			FrameMoved(BPoint new_position);
virtual void			WorkspacesChanged(uint32 old_ws, uint32 new_ws);
virtual void			WorkspaceActivated(int32 ws, bool state);
virtual	void			FrameResized(float new_width, float new_height);
virtual void			Minimize(bool minimize);
virtual void			Zoom(	BPoint rec_position,
								float rec_width,
								float rec_height);
void			Zoom();
virtual void			ScreenChanged(BRect screen_size, color_space depth);
bool			NeedsUpdate() const;
void			UpdateIfNeeded();
OBView			*FindView(const char *view_name) const;
OBView			*FindView(BPoint) const;
OBView			*CurrentFocus() const;
void			Activate(bool = true);
virtual	void			WindowActivated(bool state);
void			ConvertToScreen(BPoint *pt) const;
BPoint			ConvertToScreen(BPoint pt) const;
void			ConvertFromScreen(BPoint *pt) const;
BPoint			ConvertFromScreen(BPoint pt) const;
void			ConvertToScreen(BRect *rect) const;
BRect			ConvertToScreen(BRect rect) const;
void			ConvertFromScreen(BRect *rect) const;
BRect			ConvertFromScreen(BRect rect) const;
void			MoveBy(float dx, float dy);
void			MoveTo(BPoint);
void			MoveTo(float x, float y);
void			ResizeBy(float dx, float dy);
void			ResizeTo(float width, float height);
virtual	void			Show();
virtual	void			Hide();
bool			IsHidden() const;
bool			IsMinimized() const;

void			Flush() const;
void			Sync() const;

status_t		SendBehind(const OBWindow *window);

void			DisableUpdates();
void			EnableUpdates();

void			BeginViewTransaction();
void			EndViewTransaction();

BRect			Bounds() const;
BRect			Frame() const;
const char		*Title() const;
void			SetTitle(const char *title);
bool			IsFront() const;
bool			IsActive() const;
uint32			Workspaces() const;
void			SetWorkspaces(uint32);
OBView			*LastMouseMovedView() const;


status_t		AddToSubset(OBWindow *window);
status_t		RemoveFromSubset(OBWindow *window);

virtual	bool			QuitRequested();
virtual thread_id		Run();

protected:
friend OBView;

uint32			WindowLookToInteger(window_look wl);
uint32			WindowFeelToInteger(window_feel wf);
window_look		WindowLookFromType(window_type t);
window_feel		WindowFeelFromType(window_type t);

BString	*wtitle;
int32 ID;
int32 topview_ID;
uint8 hidelevel;
uint32 wflags;
			
PortLink	*drawmsglink, *serverlink;
port_id		inport;
port_id		outport;
			
OBView *top_view;
OBView *focusedview;
OBView *fLastMouseMovedView;
BRect wframe;

window_look	wlook;
window_feel	wfeel;

bool startlocked;
bool in_update;
bool is_active;
int32 wkspace;
};

#endif