/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus, <superstippi@gmx.de>
 */


#include <Application.h>
#include <Autolock.h>
#include <Button.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MessageQueue.h>
#include <MessageRunner.h>
#include <PropertyInfo.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>
#include <Window.h>

#include <AppMisc.h>
#include <ApplicationPrivate.h>
#include <MessagePrivate.h>
#include <PortLink.h>
#include <ServerProtocol.h>
#include <TokenSpace.h>

#include <ctype.h>
#include <stdio.h>
#include <math.h>


//#define DEBUG_WIN
#ifdef DEBUG_WIN
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif


struct BWindow::unpack_cookie {
	unpack_cookie();

	BMessage*	message;
	int32		index;
	BHandler*	focus;
	int32		focus_token;
	int32		last_view_token;
	bool		found_focus;
	bool		tokens_scanned;
};

class BWindow::Shortcut {
	public:
		Shortcut(uint32 key, uint32 modifiers, BMenuItem* item);
		Shortcut(uint32 key, uint32 modifiers, BMessage* message, BHandler* target);
		~Shortcut();

		bool Matches(uint32 key, uint32 modifiers) const;

		BMenuItem* MenuItem() const { return fMenuItem; }
		BMessage* Message() const { return fMessage; }
		BHandler* Target() const { return fTarget; }

		static uint32 AllowedModifiers();
		static uint32 PrepareKey(uint32 key);
		static uint32 PrepareModifiers(uint32 modifiers);

	private:
		uint32		fKey;
		uint32		fModifiers;
		BMenuItem*	fMenuItem;
		BMessage*	fMessage;
		BHandler*	fTarget;
};


using BPrivate::gDefaultTokens;

static property_info sWindowPropInfo[] = {
	{
		"Feel", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER }, NULL, 0, { B_INT32_TYPE } 
	},

	{
		"Flags", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER }, NULL, 0, { B_INT32_TYPE }
	},

	{
		"Frame", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER }, NULL, 0, { B_RECT_TYPE }
	},

	{
		"Hidden", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER }, NULL, 0, { B_BOOL_TYPE }
	},

	{
		"Look", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER }, NULL, 0, { B_INT32_TYPE }
	},

	{
		"Title", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER }, NULL, 0, { B_STRING_TYPE }
	},

	{
		"Workspaces", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER }, NULL, 0, { B_INT32_TYPE}
	},

	{
		"MenuBar", {},
		{ B_DIRECT_SPECIFIER }, NULL, 0, {}
	},

	{
		"View", {}, {}, NULL, 0, {}
	},

	{
		"Minimize", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER }, NULL, 0, { B_BOOL_TYPE }
	},

	{}
};


void 
_set_menu_sem_(BWindow *window, sem_id sem)
{
	if (window != NULL)
		window->fMenuSem = sem;
}


//	#pragma mark -


BWindow::unpack_cookie::unpack_cookie()
	:
	message((BMessage*)~0UL),
		// message == NULL is our exit condition
	index(0),
	focus_token(B_NULL_TOKEN),
	last_view_token(B_NULL_TOKEN),
	found_focus(false),
	tokens_scanned(false)
{
}


//	#pragma mark -


BWindow::Shortcut::Shortcut(uint32 key, uint32 modifiers, BMenuItem* item)
	:
	fKey(PrepareKey(key)),
	fModifiers(PrepareModifiers(modifiers)),
	fMenuItem(item),
	fMessage(NULL),
	fTarget(NULL)
{
}


BWindow::Shortcut::Shortcut(uint32 key, uint32 modifiers, BMessage* message,
	BHandler* target)
	:
	fKey(PrepareKey(key)),
	fModifiers(PrepareModifiers(modifiers)),
	fMenuItem(NULL),
	fMessage(message),
	fTarget(target)
{
}


BWindow::Shortcut::~Shortcut()
{
	// we own the message, if any
	delete fMessage;
}


bool
BWindow::Shortcut::Matches(uint32 key, uint32 modifiers) const
{
	return fKey == key && fModifiers == modifiers;
}


/*static*/
uint32
BWindow::Shortcut::AllowedModifiers()
{
	return B_COMMAND_KEY | B_OPTION_KEY | B_SHIFT_KEY
		| B_CONTROL_KEY | B_MENU_KEY;
}


/*static*/
uint32
BWindow::Shortcut::PrepareModifiers(uint32 modifiers)
{
	return (modifiers & AllowedModifiers()) | B_COMMAND_KEY;
}


/*static*/
uint32
BWindow::Shortcut::PrepareKey(uint32 key)
{
	return tolower(key);
		// TODO: support unicode and/or more intelligent key mapping
}


//	#pragma mark -


BWindow::BWindow(BRect frame, const char* title, window_type type,
	uint32 flags, uint32 workspace)
	: BLooper(title)
{
	window_look look;
	window_feel feel;
	_DecomposeType(type, &look, &feel);

	_InitData(frame, title, look, feel, flags, workspace);
}


BWindow::BWindow(BRect frame, const char* title, window_look look, window_feel feel,
	uint32 flags, uint32 workspace)
	: BLooper(title)
{
	_InitData(frame, title, look, feel, flags, workspace);
}


BWindow::BWindow(BMessage* data)
	: BLooper(data)
{
	data->FindRect("_frame", &fFrame);

	const char *title;
	data->FindString("_title", &title);

	window_look look;
	data->FindInt32("_wlook", (int32 *)&look);

	window_feel feel;
	data->FindInt32("_wfeel", (int32 *)&feel);

	if (data->FindInt32("_flags", (int32 *)&fFlags) != B_OK)
		fFlags = 0;

	uint32 workspaces;
	data->FindInt32("_wspace", (int32 *)&workspaces);

	uint32 type;
	if (data->FindInt32("_type", (int32*)&type) == B_OK)
		_DecomposeType((window_type)type, &fLook, &fFeel);

		// connect to app_server and initialize data
	_InitData(fFrame, title, look, feel, fFlags, workspaces);

	if (data->FindFloat("_zoom", 0, &fMaxZoomWidth) == B_OK
		&& data->FindFloat("_zoom", 1, &fMaxZoomHeight) == B_OK)
		SetZoomLimits(fMaxZoomWidth, fMaxZoomHeight);

	if (data->FindFloat("_sizel", 0, &fMinWidth) == B_OK
		&& data->FindFloat("_sizel", 1, &fMinHeight) == B_OK
		&& data->FindFloat("_sizel", 2, &fMaxWidth) == B_OK
		&& data->FindFloat("_sizel", 3, &fMaxHeight) == B_OK)
		SetSizeLimits(fMinWidth, fMaxWidth,
			fMinHeight, fMaxHeight);

	if (data->FindInt64("_pulse", &fPulseRate) == B_OK)
		SetPulseRate(fPulseRate);

	BMessage msg;
	int32 i = 0;
	while ( data->FindMessage("_views", i++, &msg) == B_OK){ 
		BArchivable *obj = instantiate_object(&msg);
		BView *child = dynamic_cast<BView *>(obj);
		if (child)
			AddChild(child);
	}
}


BWindow::BWindow(BRect frame, int32 bitmapToken)
	: BLooper("offscreen bitmap")
{
	// TODO: Implement for real
	_DecomposeType(B_UNTYPED_WINDOW, &fLook, &fFeel);
	_InitData(frame, "offscreen", fLook, fFeel, 0, 0, bitmapToken);
}


BWindow::~BWindow()
{
	fTopView->RemoveSelf();
	delete fTopView;

	// remove all remaining shortcuts
	int32 shortCutCount = fShortcuts.CountItems();
	for (int32 i = 0; i < shortCutCount; i++) {
		delete (Shortcut*)fShortcuts.ItemAtFast(i);
	}

	// TODO: release other dynamically-allocated objects

	// Deleting this semaphore will tell open menus to quit.
	if (fMenuSem > 0)
		delete_sem(fMenuSem);

	// disable pulsing
	SetPulseRate(0);

	// tell app_server about our demise
	fLink->StartMessage(AS_DELETE_WINDOW);
	// sync with the server so that for example
	// a BBitmap can be sure that there are no
	// more pending messages that are executed
	// after the bitmap is deleted (which uses
	// a different link and server side thread)
	int32 code;
	fLink->FlushWithReply(code);

	// the sender port belongs to the app_server
	delete_port(fLink->ReceiverPort());
	delete fLink;
}


BArchivable *
BWindow::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data , "BWindow")) 
		return NULL; 

	return new BWindow(data); 
}


status_t
BWindow::Archive(BMessage* data, bool deep) const
{
	status_t retval = BLooper::Archive(data, deep);
	if (retval != B_OK)
		return retval;

	data->AddRect("_frame", fFrame);
	data->AddString("_title", fTitle);
	data->AddInt32("_wlook", fLook);
	data->AddInt32("_wfeel", fFeel);
	if (fFlags)
		data->AddInt32("_flags", fFlags);
	data->AddInt32("_wspace", (uint32)Workspaces());

	if (!_ComposeType(fLook, fFeel))
		data->AddInt32("_type", (uint32)Type());

	if (fMaxZoomWidth != 32768.0 || fMaxZoomHeight != 32768.0) {
		data->AddFloat("_zoom", fMaxZoomWidth);
		data->AddFloat("_zoom", fMaxZoomHeight);
	}

	if (fMinWidth != 0.0 || fMinHeight != 0.0 
		|| fMaxWidth != 32768.0 || fMaxHeight != 32768.0) {
		data->AddFloat("_sizel", fMinWidth);
		data->AddFloat("_sizel", fMinHeight);
		data->AddFloat("_sizel", fMaxWidth);
		data->AddFloat("_sizel", fMaxHeight);
	}

	if (fPulseRate != 500000)
		data->AddInt64("_pulse", fPulseRate);

	if (deep) {
		int32 noOfViews = CountChildren();
		for (int32 i = 0; i < noOfViews; i++){
			BMessage childArchive;
			if (ChildAt(i)->Archive(&childArchive, deep) == B_OK)
				data->AddMessage("_views", &childArchive);
		}
	}

	return B_OK;
}


void
BWindow::Quit()
{
	if (!IsLocked()) {
		const char *name = Name();
		if (!name)
			name = "no-name";

		printf("ERROR - you must Lock a looper before calling Quit(), "
			   "team=%ld, looper=%s\n", Team(), name);
	}

	// Try to lock
	if (!Lock()){
		// We're toast already
		return;
	}

	while (!IsHidden())	{ 
		Hide(); 
	}

	if (fFlags & B_QUIT_ON_WINDOW_CLOSE)
		be_app->PostMessage(B_QUIT_REQUESTED);

	BLooper::Quit();
}


void
BWindow::AddChild(BView *child, BView *before)
{
	fTopView->AddChild(child, before);
}


bool
BWindow::RemoveChild(BView *child)
{
	return fTopView->RemoveChild(child);
}


int32
BWindow::CountChildren() const
{
	return fTopView->CountChildren();
}


BView *
BWindow::ChildAt(int32 index) const
{
	return fTopView->ChildAt(index);
}


void
BWindow::Minimize(bool minimize)
{
	if (IsModal())
		return;

	if (IsFloating())
		return;		

	if (fMinimized == minimize)
		return;

	fMinimized = minimize;

	Lock();
	fLink->StartMessage(AS_WINDOW_MINIMIZE);
	fLink->Attach<bool>(minimize);
	fLink->Flush();
	Unlock();
}


status_t
BWindow::SendBehind(const BWindow *window)
{
	if (!window)
		return B_ERROR;

	Lock();
	fLink->StartMessage(AS_SEND_BEHIND);
	fLink->Attach<int32>(_get_object_token_(window));
	fLink->Attach<team_id>(Team());

	status_t status = B_ERROR;
	fLink->FlushWithReply(status);

	Unlock();

	return status;
}


void
BWindow::Flush() const
{
	const_cast<BWindow *>(this)->Lock();
	fLink->Flush();
	const_cast<BWindow *>(this)->Unlock();
}


void
BWindow::Sync() const
{
	const_cast<BWindow*>(this)->Lock();
	fLink->StartMessage(AS_SYNC);

	// waiting for the reply is the actual syncing
	int32 code;
	fLink->FlushWithReply(code);

	const_cast<BWindow*>(this)->Unlock();
}


void
BWindow::DisableUpdates()
{
	Lock();
	fLink->StartMessage(AS_DISABLE_UPDATES);
	fLink->Flush();
	Unlock();
}


void
BWindow::EnableUpdates()
{
	Lock();
	fLink->StartMessage(AS_ENABLE_UPDATES);
	fLink->Flush();
	Unlock();
}


void
BWindow::BeginViewTransaction()
{
	if (Lock()) {
		if (fInTransaction) {
			Unlock();
			return;
		}
		fLink->StartMessage(AS_BEGIN_TRANSACTION);
		fInTransaction = true;

		Unlock();
	}
}


void
BWindow::EndViewTransaction()
{
	if (Lock()) {
		if (!fInTransaction) {
			Unlock();
			return;
		}
		fLink->StartMessage(AS_END_TRANSACTION);
		fLink->Flush();
		fInTransaction = false;		

		Unlock();
	}
}


bool
BWindow::IsFront() const
{
	if (IsActive())
		return true;

	if (IsModal())
		return true;

	return false;
}


void 
BWindow::MessageReceived(BMessage *msg)
{
	if (!msg->HasSpecifiers())
		return BLooper::MessageReceived(msg);

	BMessage replyMsg(B_REPLY);
	bool handled = false;

	switch (msg->what) {
		case B_GET_PROPERTY:
		case B_SET_PROPERTY: {
			BMessage specifier;
			int32 what;
			const char *prop;
			int32 index;

			if (msg->GetCurrentSpecifier(&index, &specifier, &what, &prop) != B_OK)
				break;

			if (!strcmp(prop, "Feel")) {
				if (msg->what == B_GET_PROPERTY) {
					replyMsg.AddInt32("result", (uint32)Feel());
					handled = true;
				} else {
					uint32 newFeel;
					if (msg->FindInt32("data", (int32 *)&newFeel) == B_OK) {
						SetFeel((window_feel)newFeel);
						handled = true;
					}
				}
			} else if (!strcmp(prop, "Flags")) {
				if (msg->what == B_GET_PROPERTY) {
					replyMsg.AddInt32("result", Flags());
					handled = true;
				} else {
					uint32 newFlags;
					if (msg->FindInt32("data", (int32 *)&newFlags) == B_OK) {
						SetFlags(newFlags);
						handled = true;
					}
				}
			} else if (!strcmp(prop, "Frame")) {
				if (msg->what == B_GET_PROPERTY) {
					replyMsg.AddRect("result", Frame());
					handled = true;
				} else {
					BRect newFrame;
					if (msg->FindRect("data", &newFrame) == B_OK) {
						MoveTo(newFrame.LeftTop());
						ResizeTo(newFrame.Width(), newFrame.Height());
						handled = true;
					}
				}
			} else if (!strcmp(prop, "Hidden")) {
				if (msg->what == B_GET_PROPERTY) {
					replyMsg.AddBool("result", IsHidden());
					handled = true;
				} else {
					bool hide;
					if (msg->FindBool("data", &hide) == B_OK) {
						if (hide) {
							if (!IsHidden())
								Hide();
						} else if (IsHidden())
							Show();

						handled = true;
					}
				}
			} else if (!strcmp(prop, "Look")) {
				if (msg->what == B_GET_PROPERTY) {
					replyMsg.AddInt32("result", (uint32)Look());
					handled = true;
				} else {
					uint32 newLook;
					if (msg->FindInt32("data", (int32 *)&newLook) == B_OK) {
						SetLook((window_look)newLook);
						handled = true;
					}
				}
			} else if (!strcmp(prop, "Title")) {
				if (msg->what == B_GET_PROPERTY) {
					replyMsg.AddString("result", Title());
					handled = true;
				} else {
					const char *newTitle = NULL;
					if (msg->FindString("data", &newTitle) == B_OK) {
						SetTitle(newTitle);
						handled = true;
					}
				}
			} else if (!strcmp(prop, "Workspaces")) {
				if (msg->what == B_GET_PROPERTY) {
					replyMsg.AddInt32( "result", Workspaces());
					handled = true;
				} else {
					uint32 newWorkspaces;
					if (msg->FindInt32("data", (int32 *)&newWorkspaces) == B_OK) {
						SetWorkspaces(newWorkspaces);
						handled = true;
					}
				}
			} else if (!strcmp(prop, "Minimize")) {
				if (msg->what == B_GET_PROPERTY) {
					replyMsg.AddBool("result", IsMinimized());
					handled = true;
				} else {
					bool minimize;
					if (msg->FindBool("data", &minimize) == B_OK) {
						Minimize(minimize);
						handled = true;
					}
				}
			}
			break;
		}
	}

	if (handled) {
		if (msg->what == B_SET_PROPERTY)
			replyMsg.AddInt32("error", B_OK);
	} else {
		replyMsg.what = B_MESSAGE_NOT_UNDERSTOOD;
		replyMsg.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
		replyMsg.AddString("message", "Didn't understand the specifier(s)");
	}
	msg->SendReply(&replyMsg);
} 


void 
BWindow::DispatchMessage(BMessage *msg, BHandler *target) 
{
	if (!msg)
		return;

	switch (msg->what) {
		case B_ZOOM:
			Zoom();
			break;

		case B_MINIMIZE:
		{
			bool minimize;
			if (msg->FindBool("minimize", &minimize) == B_OK)
				Minimize(minimize);
			break;
		}

		case B_WINDOW_RESIZED:
		{
			int32 width, height;
//			if (msg->FindInt32("width", &width) == B_OK
//				&& msg->FindInt32("height", &height) == B_OK) {
//				fFrame.right = fFrame.left + width;
//				fFrame.bottom = fFrame.top + height;
//
//				_AdoptResize();
//				FrameResized(width, height);
//			}
			if (msg->FindInt32("width", &width) == B_OK
				&& msg->FindInt32("height", &height) == B_OK) {

				// combine with pending resize notifications
				BMessage* pendingMessage;
				while ((pendingMessage = MessageQueue()->FindMessage(B_WINDOW_RESIZED, 0))) {
					if (pendingMessage != msg) {
						int32 nextWidth;
						if (pendingMessage->FindInt32("width", &nextWidth) == B_OK)
							width = nextWidth;
	
						int32 nextHeight;
						if (pendingMessage->FindInt32("height", &nextHeight) == B_OK) 
							height = nextHeight;
	
						MessageQueue()->RemoveMessage(pendingMessage);
						// TODO: the BeBook says that MessageQueue::RemoveMessage() deletes the message!
						delete pendingMessage;
						// this deletes the first *additional* message
						// fCurrentMessage is safe
					} else {
						MessageQueue()->RemoveMessage(pendingMessage);
					}
				}
				fFrame.right = fFrame.left + width;
				fFrame.bottom = fFrame.top + height;

				_AdoptResize();
				FrameResized(width, height);
			}
			break;
		}

		case B_WINDOW_MOVED:
		{
			BPoint origin;
			if (msg->FindPoint("where", &origin) == B_OK) {
				fFrame.OffsetTo(origin);

				FrameMoved(origin);
			}
			break;
		}

		case B_WINDOW_ACTIVATED:
			if (target == this) {
				bool active;
				if (msg->FindBool("active", &active) == B_OK
					&& active != fActive) {
					fActive = active;
					WindowActivated(active);
	
					// call hook function 'WindowActivated(bool)' for all
					// views attached to this window.
					fTopView->_Activate(active);
				}
			} else
				target->MessageReceived(msg);
			break;

		case B_SCREEN_CHANGED:
			if (target == this) {
				BRect frame;
				uint32 mode;
				if (msg->FindRect("frame", &frame) == B_OK
					&& msg->FindInt32("mode", (int32 *)&mode) == B_OK)
					ScreenChanged(frame, (color_space)mode);
			} else
				target->MessageReceived(msg);
			break;

		case B_WORKSPACE_ACTIVATED:
			if (target == this) {
				uint32 workspace;
				bool active;
				if (msg->FindInt32("workspace", (int32 *)&workspace) == B_OK
					&& msg->FindBool("active", &active) == B_OK)
					WorkspaceActivated(workspace, active);
			} else
				target->MessageReceived(msg);
			break;

		case B_WORKSPACES_CHANGED:
			if (target == this) {
				uint32 oldWorkspace, newWorkspace;
				if (msg->FindInt32("old", (int32 *)&oldWorkspace) == B_OK
					&& msg->FindInt32("new", (int32 *)&newWorkspace) == B_OK)
					WorkspacesChanged(oldWorkspace, newWorkspace);
			} else
				target->MessageReceived(msg);
			break;

		case B_KEY_DOWN:
		{
			uint32 modifiers;
			int32 rawChar;
			const char *string = NULL;
			msg->FindInt32("modifiers", (int32*)&modifiers);
			msg->FindInt32("raw_char", &rawChar);
			msg->FindString("bytes", &string);

			// TODO: cannot use "string" here if we support having different
			//	font encoding per view (it's supposed to be converted by
			//	_HandleKeyDown() one day)
			if (!_HandleKeyDown(string[0], (uint32)modifiers)) {
				if (BView* view = dynamic_cast<BView*>(target))
					view->KeyDown(string, strlen(string));
				else
					target->MessageReceived(msg);
			}
			break;
		}

		case B_KEY_UP:
		{
			const char *string = NULL;
			msg->FindString("bytes", &string);

			// TODO: same as above
			if (BView* view = dynamic_cast<BView*>(target))
				view->KeyUp(string, strlen(string));
			else
				target->MessageReceived(msg);
			break;
		}

		case B_MOUSE_DOWN:
		{
			BPoint where;
			msg->FindPoint("be:view_where", &where);

			if (BView *view = dynamic_cast<BView *>(target))
				view->MouseDown(where);
			else
				target->MessageReceived(msg);
			break;
		}

		case B_MOUSE_UP:
		{
			BPoint where;
			msg->FindPoint("be:view_where", &where);

			if (BView *view = dynamic_cast<BView *>(target))
				view->MouseUp(where);
			else
				target->MessageReceived(msg);
			break;
		}

		case B_MOUSE_MOVED:
		{
			BPoint where;
			uint32 buttons;
			uint32 transit;
			msg->FindPoint("be:view_where", &where);
			msg->FindInt32("buttons", (int32*)&buttons);
			msg->FindInt32("be:transit", (int32*)&transit);
//			bigtime_t when;
//			if (msg->FindInt64("when", (int64*)&when) < B_OK)
//				printf("BWindow B_MOUSE_MOVED no when\n");
//			else if (system_time() - when > 5000) {
//				printf("BWindow B_MOUSE_MOVED lagging behind\n");
//			}
			BMessage* dragMessage = NULL;
			if (msg->HasMessage("be:drag_message")) {
				dragMessage = new BMessage();
				if (msg->FindMessage("be:drag_message", dragMessage) != B_OK) {
					delete dragMessage;
					dragMessage = NULL;
				}
			}

			if (BView *view = dynamic_cast<BView *>(target))
				view->MouseMoved(where, transit, dragMessage);
			else
				target->MessageReceived(msg);

			delete dragMessage;
			break;
		}

		case B_PULSE:
			if (target == this && fPulseEnabled) {
				fTopView->_Pulse();
				fLink->Flush();
			} else
				target->MessageReceived(msg);
			break;

		case _UPDATE_:
		{
			STRACE(("info:BWindow handling _UPDATE_.\n"));
			BRect updateRect;

			fLink->StartMessage(AS_BEGIN_UPDATE);
			fInTransaction = true;

			int32 code;
			if (fLink->FlushWithReply(code) == B_OK
				&& code == B_OK) {
				// read culmulated update rect
				fLink->Read<BRect>(&updateRect);

				// read tokens for views that need to be drawn
				// NOTE: we need to read the tokens completely
				// first, or other calls would likely mess up the
				// data in the link.
				BList tokens(20);
				int32 token;
				status_t error = fLink->Read<int32>(&token);
				while (error >= B_OK && token != B_NULL_TOKEN) {
					tokens.AddItem((void*)token);
					error = fLink->Read<int32>(&token);
				}
				// draw
				int32 count = tokens.CountItems();
				for (int32 i = 0; i < count; i++) {
					if (BView* view = _FindView((int32)tokens.ItemAtFast(i)))
						view->_Draw(updateRect);
				}
				// TODO: the tokens are actually hirachically sorted,
				// so traversing the list in revers and calling
				// child->DrawAfterChildren would actually work correctly,
				// only that drawing outside a view is not yet supported
				// in the app_server.
			}

			fLink->StartMessage(AS_END_UPDATE);
			fLink->Flush();
			fInTransaction = false;

			break;
		}

		case B_VIEW_RESIZED:
		case B_VIEW_MOVED:
		{
			// NOTE: The problem with this implementation is that BView::Window()->CurrentMessage()
			// will show this message, and not what it used to be on R5. This might break apps and
			// we need to fix this here or change the way this feature is implemented. However, this
			// implementation shows what has to be done when Layers are moved or resized inside the
			// app_server. This message is generated from Layer::MoveByHook() and ResizeByHook() in
			// Layer::_AddToViewsWithInvalidCoords().
			int32 token;
			BPoint frameLeftTop;
			float width;
			float height;
			BView *view;
			for (int32 i = 0; CurrentMessage() && msg->FindInt32("_token", i, &token) >= B_OK; i++) {
				if (token >= 0) {
					msg->FindPoint("where", i, &frameLeftTop);
					msg->FindFloat("width", i, &width);
					msg->FindFloat("height", i, &height);
					if ((view = _FindView(token)) != NULL) {
						// update the views offset in parent
						if (view->LeftTop() != frameLeftTop) {
							view->fParentOffset = frameLeftTop;

							// optionally call FrameMoved
							if (view->fFlags & B_FRAME_EVENTS) {
								STRACE(("Calling BView(%s)::FrameMoved( %.1f, %.1f )\n", view->Name(),
										frameLeftTop.x, frameLeftTop.y));
								view->FrameMoved(frameLeftTop);
							}
						}
						// update the views width and height
						if (view->fBounds.Width() != width || view->fBounds.Height() != height) {
							// TODO: does this work when a views left/top side is resized?
							view->fBounds.right = view->fBounds.left + width;
							view->fBounds.bottom = view->fBounds.top + height;
							// optionally call FrameResized
							if (view->fFlags & B_FRAME_EVENTS) {
								STRACE(("Calling BView(%s)::FrameResized( %f, %f )\n", view->Name(), width, height));
								view->FrameResized(width, height);
							}
						}
					} else {
						fprintf(stderr, "B_VIEW_RESIZED ***PANIC: BW: Can't find view with ID: %ld !***\n", token);
					}
				}
			}
			break;
		}

		case _MENUS_DONE_:
			MenusEnded();
			break;

		// These two are obviously some kind of old scripting messages
		// this is NOT an app_server message and we have to be cautious
		case B_WINDOW_MOVE_BY:
		{
			BPoint offset;
			if (msg->FindPoint("data", &offset) == B_OK)
				MoveBy(offset.x, offset.y);
			else
				msg->SendReply(B_MESSAGE_NOT_UNDERSTOOD);
			break;
		}

		// this is NOT an app_server message and we have to be cautious
		case B_WINDOW_MOVE_TO:
		{
			BPoint origin;
			if (msg->FindPoint("data", &origin) == B_OK)
				MoveTo(origin);
			else
				msg->SendReply(B_MESSAGE_NOT_UNDERSTOOD);
			break;
		}
		case _MESSAGE_DROPPED_:
		{
			if (fLastMouseMovedView)
				target = fLastMouseMovedView;

			uint32 originalWhat;
			if (msg->FindInt32("_original_what", (int32*)&originalWhat) == B_OK) {
				msg->what = originalWhat;
				msg->RemoveName("_original_what");
			}

			BLooper::DispatchMessage(msg, target); 
			break;
		}

		default:
			BLooper::DispatchMessage(msg, target); 
			break;
	}
}


void
BWindow::FrameMoved(BPoint new_position)
{
	// does nothing
	// Hook function
}


void
BWindow::FrameResized(float new_width, float new_height)
{
	// does nothing
	// Hook function
}


void
BWindow::WorkspacesChanged(uint32 old_ws, uint32 new_ws)
{
	// does nothing
	// Hook function
}


void
BWindow::WorkspaceActivated(int32 ws, bool state)
{
	// does nothing
	// Hook function
}


void
BWindow::MenusBeginning()
{
	// does nothing
	// Hook function
}


void
BWindow::MenusEnded()
{
	// does nothing
	// Hook function
}


void
BWindow::SetSizeLimits(float minWidth, float maxWidth, 
	float minHeight, float maxHeight)
{
	if (minWidth > maxWidth || minHeight > maxHeight)
		return;

	if (Lock()) {
		fLink->StartMessage(AS_SET_SIZE_LIMITS);
		fLink->Attach<float>(minWidth);
		fLink->Attach<float>(maxWidth);
		fLink->Attach<float>(minHeight);
		fLink->Attach<float>(maxHeight);

		int32 code;
		if (fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			// read the values that were really enforced on
			// the server side (the window frame could have
			// been changed, too)
			fLink->Read<BRect>(&fFrame);
			fLink->Read<float>(&fMinWidth);
			fLink->Read<float>(&fMaxWidth);
			fLink->Read<float>(&fMinHeight);
			fLink->Read<float>(&fMaxHeight);

			_AdoptResize();
				// TODO: the same has to be done for SetLook() (that can alter
				//		the size limits, and hence, the size of the window
		}
		Unlock();
	}
}


void
BWindow::GetSizeLimits(float *minWidth, float *maxWidth, 
	float *minHeight, float *maxHeight)
{
	// TODO: What about locking?!?
	*minHeight = fMinHeight;
	*minWidth = fMinWidth;
	*maxHeight = fMaxHeight;
	*maxWidth = fMaxWidth;
}


void
BWindow::SetZoomLimits(float maxWidth, float maxHeight)
{
	// TODO: What about locking?!?
	if (maxWidth > fMaxWidth)
		maxWidth = fMaxWidth;
	else
		fMaxZoomWidth = maxWidth;

	if (maxHeight > fMaxHeight)
		maxHeight = fMaxHeight;
	else
		fMaxZoomHeight = maxHeight;
}


void
BWindow::Zoom(BPoint rec_position, float rec_width, float rec_height)
{
	// this is also a Hook function!

	MoveTo(rec_position);
	ResizeTo(rec_width, rec_height);
}


void
BWindow::Zoom()
{
	// TODO: broken.
	// TODO: What about locking?!?
	float minWidth, minHeight;
	BScreen screen;

	/*
		from BeBook:
		However, if the window's rectangle already matches these "zoom" dimensions
		(give or take a few pixels), Zoom() passes the window's previous
		("non-zoomed") size and location. (??????)
	*/

	if (Frame().Width() == fMaxZoomWidth && Frame().Height() == fMaxZoomHeight) {
		BPoint position(Frame().left, Frame().top);
		Zoom(position, fMaxZoomWidth, fMaxZoomHeight);
		return;
	}

	/* From BeBook:
		The dimensions that non-virtual Zoom() passes to hook Zoom() are deduced from
		the smallest of three rectangles:
	*/

	// 1) the rectangle defined by SetZoomLimits(), 
	minHeight = fMaxZoomHeight;
	minWidth = fMaxZoomWidth;

	// 2) the rectangle defined by SetSizeLimits()
	if (fMaxHeight < minHeight)
		minHeight = fMaxHeight;
	if (fMaxWidth < minWidth)
		minWidth = fMaxWidth;

	// 3) the screen rectangle
	if (screen.Frame().Width() < minWidth)
		minWidth = screen.Frame().Width();
	if (screen.Frame().Height() < minHeight)
		minHeight = screen.Frame().Height();

	Zoom(Frame().LeftTop(), minWidth, minHeight);
}


void
BWindow::ScreenChanged(BRect screen_size, color_space depth)
{
	// Hook function
	// does nothing
}


void
BWindow::SetPulseRate(bigtime_t rate)
{
	// TODO: What about locking?!?
	if (rate < 0)
		return;

	// ToDo: isn't fPulseRunner enough? Why fPulseEnabled?
	if (fPulseRate == 0 && !fPulseEnabled) {
		fPulseRunner = new BMessageRunner(BMessenger(this),
			new BMessage(B_PULSE), rate);
		fPulseRate = rate;
		fPulseEnabled = true;
		return;
	}

	if (rate == 0 && fPulseEnabled) {
		delete fPulseRunner;
		fPulseRunner = NULL;

		fPulseRate = rate;
		fPulseEnabled = false;
		return;
	}

	fPulseRunner->SetInterval(rate);
}


bigtime_t
BWindow::PulseRate() const
{
	// TODO: What about locking?!?
	return fPulseRate;
}


void
BWindow::AddShortcut(uint32 key, uint32 modifiers, BMenuItem *item)
{
	Shortcut* shortcut = new Shortcut(key, modifiers, item);

	// removes the shortcut if it already exists!
	RemoveShortcut(key, modifiers);

	fShortcuts.AddItem(shortcut);
}


void
BWindow::AddShortcut(uint32 key, uint32 modifiers, BMessage *message)
{
	AddShortcut(key, modifiers, message, this);
}


void
BWindow::AddShortcut(uint32 key, uint32 modifiers, BMessage *message, BHandler *target)
{
	if (message == NULL)
		return;

	Shortcut* shortcut = new Shortcut(key, modifiers, message, target);

	// removes the shortcut if it already exists!
	RemoveShortcut(key, modifiers);

	fShortcuts.AddItem(shortcut);
}


void
BWindow::RemoveShortcut(uint32 key, uint32 modifiers)
{
	Shortcut* shortcut = _FindShortcut(key, modifiers);
	if (shortcut != NULL) {
		fShortcuts.RemoveItem(shortcut);
		delete shortcut;
	}
}


BButton *
BWindow::DefaultButton() const
{
	// TODO: What about locking?!?
	return fDefaultButton;
}


void
BWindow::SetDefaultButton(BButton *button)
{
	// TODO: What about locking?!?
	if (fDefaultButton == button)
		return;

	if (fDefaultButton != NULL) {
		// tell old button it's no longer the default one
		BButton *oldDefault = fDefaultButton;
		oldDefault->MakeDefault(false);
		oldDefault->Invalidate();
	}

	fDefaultButton = button;

	if (button != NULL) {
		// notify new default button
		fDefaultButton->MakeDefault(true);
		fDefaultButton->Invalidate();
	}
}


bool
BWindow::NeedsUpdate() const
{
	// TODO: What about locking?!?

	const_cast<BWindow *>(this)->Lock();	
	fLink->StartMessage(AS_NEEDS_UPDATE);

	int32 code = B_ERROR;
	fLink->FlushWithReply(code);

	const_cast<BWindow *>(this)->Unlock();

	return code == B_OK;
}


void
BWindow::UpdateIfNeeded()
{
	// TODO: What about locking?!?
	// works only from this thread
	if (find_thread(NULL) != Thread())
		return;

	// Since we're blocking the event loop, we need to retrieve 
	// all messages that are pending on the port.
	_DequeueAll();

	BMessageQueue *queue = MessageQueue();
	queue->Lock();

	// First process and remove any _UPDATE_ message in the queue
	// According to Adi, there can only be one at a time

	BMessage *msg;
	for (int32 i = 0; (msg = queue->FindMessage(i)) != NULL; i++) {
		if (msg->what == _UPDATE_) {
			BWindow::DispatchMessage(msg, this);
				// we need to make sure that no overridden method is called 
				// here; for BWindow::DispatchMessage() we now exactly what
				// will happen
			queue->RemoveMessage(msg);
			delete msg;
			break;
		}
	}

	queue->Unlock();
}


BView *
BWindow::FindView(const char *viewName) const
{
	BAutolock _(const_cast<BWindow*>(this));
	return fTopView->FindView(viewName);
}


BView *
BWindow::FindView(BPoint point) const
{
	BAutolock _(const_cast<BWindow*>(this));
	return _FindView(fTopView, point);
}


BView *BWindow::CurrentFocus() const
{
	return fFocus;
}


void
BWindow::Activate(bool active)
{
	// TODO: What about locking?!?
	if (IsHidden())
		return;

	Lock();
	fLink->StartMessage(AS_ACTIVATE_WINDOW);
	fLink->Attach<bool>(active);
	fLink->Flush();
	Unlock();
}


void
BWindow::WindowActivated(bool state)
{
	// hook function
	// does nothing
}


void
BWindow::ConvertToScreen(BPoint *point) const
{
	point->x += fFrame.left;
	point->y += fFrame.top;
}


BPoint
BWindow::ConvertToScreen(BPoint point) const
{
	return point + fFrame.LeftTop();
}


void
BWindow::ConvertFromScreen(BPoint *point) const
{
	point->x -= fFrame.left;
	point->y -= fFrame.top;
}


BPoint
BWindow::ConvertFromScreen(BPoint point) const
{
	return point - fFrame.LeftTop();
}


void
BWindow::ConvertToScreen(BRect *rect) const
{
	rect->OffsetBy(fFrame.LeftTop());
}


BRect
BWindow::ConvertToScreen(BRect rect) const
{
	return rect.OffsetByCopy(fFrame.LeftTop());
}


void
BWindow::ConvertFromScreen(BRect* rect) const
{
	rect->OffsetBy(-fFrame.left, -fFrame.top);
}


BRect
BWindow::ConvertFromScreen(BRect rect) const
{
	return rect.OffsetByCopy(-fFrame.left, -fFrame.top);
}


bool 
BWindow::IsMinimized() const
{
	// Hiding takes precendence over minimization!!!
	if (IsHidden())
		return false;

	return fMinimized;
}


BRect
BWindow::Bounds() const
{
	return BRect(0, 0, fFrame.Width(), fFrame.Height());
}


BRect
BWindow::Frame() const
{
	return fFrame;
}


const char *
BWindow::Title() const
{
	return fTitle;
}


void
BWindow::SetTitle(const char *title)
{
	if (title == NULL)
		title = "";

	free(fTitle);
	fTitle = strdup(title);

	// we will change BWindow's thread name to "w>window title"	

	char threadName[B_OS_NAME_LENGTH];
	strcpy(threadName, "w>");
#ifdef __HAIKU__
	strlcat(threadName, title, B_OS_NAME_LENGTH);
#else
	int32 length = strlen(title);
	length = min_c(length, B_OS_NAME_LENGTH - 3);
	memcpy(threadName + 2, title, length);
	threadName[length + 2] = '\0';
#endif

	// change the handler's name
	SetName(threadName);

	// if the message loop has been started...
	if (Thread() >= B_OK)
		rename_thread(Thread(), threadName);

	// we notify the app_server so we can actually see the change
	if (Lock()) {
		fLink->StartMessage(AS_SET_WINDOW_TITLE);
		fLink->AttachString(fTitle);
		fLink->Flush();
		Unlock();
	}
}


bool
BWindow::IsActive() const
{
	return fActive;
}


void
BWindow::SetKeyMenuBar(BMenuBar *bar)
{
	fKeyMenuBar = bar;
}


BMenuBar *
BWindow::KeyMenuBar() const
{
	return fKeyMenuBar;
}


bool
BWindow::IsModal() const
{
	return fFeel == B_MODAL_SUBSET_WINDOW_FEEL
		|| fFeel == B_MODAL_APP_WINDOW_FEEL
		|| fFeel == B_MODAL_ALL_WINDOW_FEEL;
}


bool
BWindow::IsFloating() const
{
	return fFeel == B_FLOATING_SUBSET_WINDOW_FEEL
		|| fFeel == B_FLOATING_APP_WINDOW_FEEL
		|| fFeel == B_FLOATING_ALL_WINDOW_FEEL;
}


status_t
BWindow::AddToSubset(BWindow *window)
{
	if (window == NULL || window->Feel() != B_NORMAL_WINDOW_FEEL
		|| (fFeel != B_MODAL_SUBSET_WINDOW_FEEL
			&& fFeel != B_FLOATING_SUBSET_WINDOW_FEEL))
		return B_BAD_VALUE;

	Lock();

	status_t status = B_ERROR;
	fLink->StartMessage(AS_ADD_TO_SUBSET);
	fLink->Attach<int32>(_get_object_token_(window));
	fLink->FlushWithReply(status);

	Unlock();

	return status;
}


status_t
BWindow::RemoveFromSubset(BWindow *window)
{
	if (window == NULL || window->Feel() != B_NORMAL_WINDOW_FEEL
		|| (fFeel != B_MODAL_SUBSET_WINDOW_FEEL
			&& fFeel != B_FLOATING_SUBSET_WINDOW_FEEL))
		return B_BAD_VALUE;

	Lock();

	status_t status = B_ERROR;
	fLink->StartMessage(AS_REMOVE_FROM_SUBSET);
	fLink->Attach<int32>(_get_object_token_(window));
	fLink->FlushWithReply(status);

	Unlock();

	return status;
}


status_t
BWindow::Perform(perform_code d, void *arg)
{
	return BLooper::Perform(d, arg);
}


status_t
BWindow::SetType(window_type type)
{
	window_look look;
	window_feel feel;
	_DecomposeType(type, &look, &feel);

	status_t status = SetLook(look);
	if (status == B_OK)
		status = SetFeel(feel);

	return status;
}


window_type
BWindow::Type() const
{
	return _ComposeType(fLook, fFeel);
}


status_t
BWindow::SetLook(window_look look)
{
	BAutolock locker(this);

	fLink->StartMessage(AS_SET_LOOK);
	fLink->Attach<int32>((int32)look);

	status_t status = B_ERROR;
	if (fLink->FlushWithReply(status) == B_OK && status == B_OK)
		fLook = look;

	// TODO: this could have changed the window size, and thus, we
	//	need to get it from the server (and call _AdoptResize()).

	return status;
}


window_look
BWindow::Look() const
{
	return fLook;
}


status_t
BWindow::SetFeel(window_feel feel)
{
	BAutolock locker(this);

	fLink->StartMessage(AS_SET_FEEL);
	fLink->Attach<int32>((int32)feel);

	status_t status = B_ERROR;
	if (fLink->FlushWithReply(status) == B_OK && status == B_OK)
		fFeel = feel;

	return status;
}


window_feel
BWindow::Feel() const
{
	return fFeel;
}


status_t
BWindow::SetFlags(uint32 flags)
{
	BAutolock locker(this);

	fLink->StartMessage(AS_SET_FLAGS);
	fLink->Attach<uint32>(flags);

	int32 status = B_ERROR;
	if (fLink->FlushWithReply(status) == B_OK && status == B_OK)
		fFlags = flags;

	return status;
}


uint32
BWindow::Flags() const
{
	return fFlags;
}


status_t
BWindow::SetWindowAlignment(window_alignment mode,
	int32 h, int32 hOffset, int32 width, int32 widthOffset,
	int32 v, int32 vOffset, int32 height, int32 heightOffset)
{
	if ((mode & (B_BYTE_ALIGNMENT | B_PIXEL_ALIGNMENT)) == 0
		|| (hOffset >= 0 && hOffset <= h)
		|| (vOffset >= 0 && vOffset <= v)
		|| (widthOffset >= 0 && widthOffset <= width)
		|| (heightOffset >= 0 && heightOffset <= height))
		return B_BAD_VALUE;

	// TODO: test if hOffset = 0 and set it to 1 if true.

	Lock();
	fLink->StartMessage(AS_SET_ALIGNMENT);
	fLink->Attach<int32>((int32)mode);
	fLink->Attach<int32>(h);
	fLink->Attach<int32>(hOffset);
	fLink->Attach<int32>(width);
	fLink->Attach<int32>(widthOffset);
	fLink->Attach<int32>(v);
	fLink->Attach<int32>(vOffset);
	fLink->Attach<int32>(height);
	fLink->Attach<int32>(heightOffset);

	status_t status = B_ERROR;
	fLink->FlushWithReply(status);

	Unlock();

	return status;
}


status_t
BWindow::GetWindowAlignment(window_alignment *mode,
	int32 *h, int32 *hOffset, int32 *width, int32 *widthOffset,
	int32 *v, int32 *vOffset, int32 *height, int32 *heightOffset) const
{
	const_cast<BWindow *>(this)->Lock();
	fLink->StartMessage(AS_GET_ALIGNMENT);

	status_t status = B_ERROR;
	if (fLink->FlushWithReply(status) == B_OK && status == B_OK) {
		fLink->Read<int32>((int32 *)mode);
		fLink->Read<int32>(h);
		fLink->Read<int32>(hOffset);
		fLink->Read<int32>(width);
		fLink->Read<int32>(widthOffset);
		fLink->Read<int32>(v);
		fLink->Read<int32>(hOffset);
		fLink->Read<int32>(height);
		fLink->Read<int32>(heightOffset);
	}

	const_cast<BWindow *>(this)->Unlock();
	return status;
}


uint32
BWindow::Workspaces() const
{
	uint32 workspaces = 0;

	const_cast<BWindow *>(this)->Lock();
	fLink->StartMessage(AS_GET_WORKSPACES);

	status_t status;
	if (fLink->FlushWithReply(status) == B_OK && status == B_OK)
		fLink->Read<uint32>(&workspaces);

	const_cast<BWindow *>(this)->Unlock();
	return workspaces;
}


void
BWindow::SetWorkspaces(uint32 workspaces)
{
	// TODO: don't forget about Tracker's background window.
	if (fFeel != B_NORMAL_WINDOW_FEEL)
		return;

	Lock();
	fLink->StartMessage(AS_SET_WORKSPACES);
	fLink->Attach<uint32>(workspaces);
	fLink->Flush();
	Unlock();
}


BView *
BWindow::LastMouseMovedView() const
{
	return fLastMouseMovedView;
}


void 
BWindow::MoveBy(float dx, float dy)
{
	if (dx == 0.0 && dy == 0.0)
		return;

	Lock();

	fLink->StartMessage(AS_WINDOW_MOVE);
	fLink->Attach<float>(dx);
	fLink->Attach<float>(dy);
	fLink->Flush();

	fFrame.OffsetBy(dx, dy);

	Unlock();
}


void
BWindow::MoveTo(BPoint point)
{
	Lock();

	point.x = roundf(point.x);
	point.y = roundf(point.y);

	if (fFrame.left != point.x || fFrame.top != point.y) {
		float xOffset = point.x - fFrame.left;
		float yOffset = point.y - fFrame.top;

		MoveBy(xOffset, yOffset);
	}

	Unlock();
}


void
BWindow::MoveTo(float x, float y)
{
	MoveTo(BPoint(x, y));
}


void
BWindow::ResizeBy(float dx, float dy)
{
	Lock();

	dx = roundf(dx);
	dy = roundf(dy);

	// stay in minimum & maximum frame limits
	if (fFrame.Width() + dx < fMinWidth)
		dx = fMinWidth - fFrame.Width();
	if (fFrame.Width() + dx > fMaxWidth)
		dx = fMaxWidth - fFrame.Width();
	if (fFrame.Height() + dy < fMinHeight)
		dy = fMinHeight - fFrame.Height();
	if (fFrame.Height() + dy > fMaxHeight)
		dy = fMaxHeight - fFrame.Height();

	if (dx != 0.0 || dy != 0.0) {
		fLink->StartMessage(AS_WINDOW_RESIZE);
		fLink->Attach<float>(dx);
		fLink->Attach<float>(dy);
		fLink->Flush();

		fFrame.SetRightBottom(fFrame.RightBottom() + BPoint(dx, dy));
		_AdoptResize();
	}
	Unlock();
}


void
BWindow::ResizeTo(float width, float height)
{
	Lock();
	ResizeBy(width - fFrame.Width(), height - fFrame.Height());
	Unlock();
}


void
BWindow::Show()
{
	bool isLocked = this->IsLocked();

	fShowLevel--;

	if (fShowLevel == 0) {
		STRACE(("BWindow(%s): sending AS_SHOW_WINDOW message...\n", Name()));
		if (Lock()) {
			fLink->StartMessage(AS_SHOW_WINDOW);
			fLink->Flush();
			Unlock();
		}
	}

	// if it's the fist time Show() is called... start the Looper thread.
	if (Thread() == B_ERROR) {
		// normally this won't happen, but I want to be sure!
		if (!isLocked) 
			Lock();
		Run();
	}
}


void
BWindow::Hide()
{
	if (fShowLevel == 0 && Lock()) {
		fLink->StartMessage(AS_HIDE_WINDOW);
		fLink->Flush();
		Unlock();
	}
	fShowLevel++;
}


bool
BWindow::IsHidden() const
{
	return fShowLevel > 0; 
}


bool
BWindow::QuitRequested()
{
	return BLooper::QuitRequested();
}


thread_id
BWindow::Run()
{
	return BLooper::Run();
}


status_t
BWindow::GetSupportedSuites(BMessage *data)
{
	if (data == NULL)
		return B_BAD_VALUE;

	status_t status = data->AddString("Suites", "suite/vnd.Be-window");
	if (status == B_OK) {
		BPropertyInfo propertyInfo(sWindowPropInfo);

		status = data->AddFlat("message", &propertyInfo);
		if (status == B_OK)
			status = BLooper::GetSupportedSuites(data);
	}

	return status;
}


BHandler *
BWindow::ResolveSpecifier(BMessage *msg, int32 index, BMessage *specifier,
	int32 what,	const char *property)
{
	if (msg->what == B_WINDOW_MOVE_BY
		|| msg->what == B_WINDOW_MOVE_TO)
		return this;

	BPropertyInfo propertyInfo(sWindowPropInfo);
	if (propertyInfo.FindMatch(msg, index, specifier, what, property) >= 0) {
		if (!strcmp(property, "View")) {
			// we will NOT pop the current specifier
			return fTopView;
		} else if (!strcmp(property, "MenuBar")) {
			if (fKeyMenuBar) {
				msg->PopSpecifier();
				return fKeyMenuBar;
			} else {
				BMessage replyMsg(B_MESSAGE_NOT_UNDERSTOOD);
				replyMsg.AddInt32("error", B_NAME_NOT_FOUND);
				replyMsg.AddString("message", "This window doesn't have a main MenuBar");
				msg->SendReply(&replyMsg);
				return NULL;
			}
		} else
			return this;
	}

	return BLooper::ResolveSpecifier(msg, index, specifier, what, property);
}


//	#pragma mark - Private Methods


void 
BWindow::_InitData(BRect frame, const char* title, window_look look,
	window_feel feel, uint32 flags,	uint32 workspace, int32 bitmapToken)
{
	STRACE(("BWindow::InitData()\n"));

	if (be_app == NULL) {
		debugger("You need a valid BApplication object before interacting with the app_server");
		return;
	}

	frame.left = roundf(frame.left);
	frame.top = roundf(frame.top);
	frame.right = roundf(frame.right);
	frame.bottom = roundf(frame.bottom);

	fFrame = frame;

	if (title == NULL)
		title = "";
	fTitle = strdup(title);
	SetName(title);

	fFeel = feel;
	fLook = look;
	fFlags = flags;

	fInTransaction = false;
	fActive = false;
	fShowLevel = 1;

	fTopView = NULL;
	fFocus = NULL;
	fLastMouseMovedView	= NULL;
	fKeyMenuBar = NULL;
	fDefaultButton = NULL;

	// Shortcut 'Q' is handled in _HandleKeyDown() directly, as its message
	// get sent to the application, and not one of our handlers
	if ((fFlags & B_NOT_CLOSABLE) == 0)
		AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	AddShortcut('X', B_COMMAND_KEY, new BMessage(B_CUT), NULL);
	AddShortcut('C', B_COMMAND_KEY, new BMessage(B_COPY), NULL);
	AddShortcut('V', B_COMMAND_KEY, new BMessage(B_PASTE), NULL);
	AddShortcut('A', B_COMMAND_KEY, new BMessage(B_SELECT_ALL), NULL);

	fPulseEnabled = false;
	fPulseRate = 0;
	fPulseRunner = NULL;

	// TODO: is this correct??? should the thread loop be started???
	//SetPulseRate( 500000 );

	// TODO:  see if you can use 'fViewsNeedPulse'

	fIsFilePanel = false;

	// TODO: see WHEN is this used!
	fMaskActivated = false;

	// TODO: see WHEN is this used!
	fWaitingForMenu = false;
	fMenuSem = -1;

	fMinimized = false;

	fMaxZoomHeight = 32768.0;
	fMaxZoomWidth = 32768.0;
	fMinHeight = 0.0;
	fMinWidth = 0.0;
	fMaxHeight = 32768.0;
	fMaxWidth = 32768.0;

	fLastViewToken = B_NULL_TOKEN;

	// TODO: other initializations!

	// Create the server-side window

	port_id receivePort = create_port(B_LOOPER_PORT_DEFAULT_CAPACITY, "w<app_server");
	if (receivePort < B_OK) {
		debugger("Could not create BWindow's receive port, used for interacting with the app_server!");
		delete this;
		return;
	}

	STRACE(("BWindow::InitData(): contacting app_server...\n"));

	// HERE we are in BApplication's thread, so for locking we use be_app variable
	// we'll lock the be_app to be sure we're the only one writing at BApplication's server port
	bool locked = false;
	if (!be_app->IsLocked()) {
		be_app->Lock();
		locked = true; 
	}

	// let app_server know that a window has been created.
	fLink = new BPrivate::PortLink(
		BApplication::Private::ServerLink()->SenderPort(), receivePort);

	if (bitmapToken < 0) {
		fLink->StartMessage(AS_CREATE_WINDOW);
	} else {
		fLink->StartMessage(AS_CREATE_OFFSCREEN_WINDOW);
		fLink->Attach<int32>(bitmapToken);
	}

	fLink->Attach<BRect>(fFrame);
	fLink->Attach<uint32>((uint32)fLook);
	fLink->Attach<uint32>((uint32)fFeel);
	fLink->Attach<uint32>(fFlags);
	fLink->Attach<uint32>(workspace);
	fLink->Attach<int32>(_get_object_token_(this));
	fLink->Attach<port_id>(receivePort);
	fLink->Attach<port_id>(fMsgPort);
	fLink->AttachString(title);

	port_id sendPort;
	int32 code;
	if (fLink->FlushWithReply(code) == B_OK
		&& code == B_OK
		&& fLink->Read<port_id>(&sendPort) == B_OK) {
		fLink->SetSenderPort(sendPort);

		// read the frame size and its limits that were really
		// enforced on the server side

		fLink->Read<BRect>(&fFrame);
		fLink->Read<float>(&fMinWidth);
		fLink->Read<float>(&fMaxWidth);
		fLink->Read<float>(&fMinHeight);
		fLink->Read<float>(&fMaxHeight);

		fMaxZoomWidth = fMaxWidth;
		fMaxZoomHeight = fMaxHeight;
	} else
		sendPort = -1;

	if (locked)
		be_app->Unlock();

	STRACE(("Server says that our send port is %ld\n", sendPort));
	STRACE(("Window locked?: %s\n", IsLocked() ? "True" : "False"));

	_CreateTopView();
}


//!	Reads all pending messages from the window port and put them into the queue.
void
BWindow::_DequeueAll()
{
	//	Get message count from port
	int32 count = port_count(fMsgPort);

	for (int32 i = 0; i < count; i++) {
		BMessage *message = MessageFromPort(0);
		if (message != NULL)
			fQueue->AddMessage(message);
	}
}


/*!	This here is an almost complete code duplication to BLooper::task_looper()
	but with some important differences:
	 a)	it uses the _DetermineTarget() method to tell what the later target of
		a message will be, if no explicit target is supplied.
	 b)	it calls _UnpackMessage() and _SanitizeMessage() to duplicate the message
	 	to all of its intended targets, and to add all fields the target would
	 	expect in such a message.

	This is important because the app_server sends all input events to the
	preferred handler, and expects them to be correctly distributed to their
	intended targets.
*/
void
BWindow::task_looper()
{
	STRACE(("info: BWindow::task_looper() started.\n"));

	// Check that looper is locked (should be)
	AssertLocked();
	Unlock();

	if (IsLocked())
		debugger("window must not be locked!");

	// loop: As long as we are not terminating.
	while (!fTerminating) {
		// TODO: timeout determination algo
		//	Read from message port (how do we determine what the timeout is?)
		BMessage* msg = MessageFromPort();

		// Did we get a message?
		if (msg) {
			// Add to queue
			fQueue->AddMessage(msg);
		} else
			continue;

		//	Get message count from port
		int32 msgCount = port_count(fMsgPort);
		for (int32 i = 0; i < msgCount; ++i) {
			// Read 'count' messages from port (so we will not block)
			// We use zero as our timeout since we know there is stuff there
			msg = MessageFromPort(0);
			// Add messages to queue
			if (msg)
				fQueue->AddMessage(msg);
		}

		// loop: As long as there are messages in the queue and the port is
		//		 empty... and we are not terminating, of course.
		bool dispatchNextMessage = true;
		while (!fTerminating && dispatchNextMessage) {
			//	Get next message from queue (assign to fLastMessage)
			fLastMessage = fQueue->NextMessage();

			// Lock the looper
			Lock();
			if (!fLastMessage) {
				// No more messages: Unlock the looper and terminate the
				// dispatch loop.
				dispatchNextMessage = false;
			} else {
				// Get the target handler
				BMessage::Private messagePrivate(fLastMessage);
				bool usePreferred = messagePrivate.UsePreferredTarget();
				BHandler *handler = NULL;
				bool dropMessage = false;

				if (usePreferred) {
					handler = PreferredHandler();
					if (handler == NULL)
						handler = this;
				} else {
					gDefaultTokens.GetToken(messagePrivate.GetTarget(),
						B_HANDLER_TOKEN, (void **)&handler);

					// if this handler doesn't belong to us, we drop the message
					if (handler != NULL && handler->Looper() != this) {
						dropMessage = true;
						handler = NULL;
					}
				}

				if ((handler == NULL && !dropMessage) || usePreferred)
					handler = _DetermineTarget(fLastMessage, handler);

				unpack_cookie cookie;
				while (_UnpackMessage(cookie, &fLastMessage, &handler, &usePreferred)) {
					// if there is no target handler, the message is dropped
					if (handler != NULL) {
						// Is this a scripting message?
						if (fLastMessage->HasSpecifiers()) {
							int32 index = 0;
							// Make sure the current specifier is kosher
							if (fLastMessage->GetCurrentSpecifier(&index) == B_OK)
								handler = resolve_specifier(handler, fLastMessage);
						}

						if (handler != NULL)
							handler = _TopLevelFilter(fLastMessage, handler);

						if (handler != NULL) {
							_SanitizeMessage(fLastMessage, handler, usePreferred);
							DispatchMessage(fLastMessage, handler);
						}
					}

					// Delete the current message
					delete fLastMessage;
					fLastMessage = NULL;
				}
			}

			if (fTerminating) {
				// we leave the looper locked when we quit
				return;
			}

			Unlock();

			// Are any messages on the port?
			if (port_count(fMsgPort) > 0) {
				// Do outer loop
				dispatchNextMessage = false;
			}
		}
	}
}


window_type
BWindow::_ComposeType(window_look look, window_feel feel) const
{
	switch (feel) {
		case B_NORMAL_WINDOW_FEEL:
			switch (look) {
				case B_TITLED_WINDOW_LOOK:
					return B_TITLED_WINDOW;

				case B_DOCUMENT_WINDOW_LOOK:
					return B_DOCUMENT_WINDOW;

				case B_BORDERED_WINDOW_LOOK:
					return B_BORDERED_WINDOW;
				
				default:
					return B_UNTYPED_WINDOW;
			}
			break;

		case B_MODAL_APP_WINDOW_FEEL:
			if (look == B_MODAL_WINDOW_LOOK)
				return B_MODAL_WINDOW;
			break;

		case B_FLOATING_APP_WINDOW_FEEL:
			if (look == B_FLOATING_WINDOW_LOOK)
				return B_FLOATING_WINDOW;
			break;

		default:
			return B_UNTYPED_WINDOW;
	}

	return B_UNTYPED_WINDOW;
}


void
BWindow::_DecomposeType(window_type type, window_look *_look,
	window_feel *_feel) const
{
	switch (type) {
		case B_DOCUMENT_WINDOW:
			*_look = B_DOCUMENT_WINDOW_LOOK;
			*_feel = B_NORMAL_WINDOW_FEEL;
			break;

		case B_MODAL_WINDOW:
			*_look = B_MODAL_WINDOW_LOOK;
			*_feel = B_MODAL_APP_WINDOW_FEEL;
			break;

		case B_FLOATING_WINDOW:
			*_look = B_FLOATING_WINDOW_LOOK;
			*_feel = B_FLOATING_APP_WINDOW_FEEL;
			break;

		case B_BORDERED_WINDOW:
			*_look = B_BORDERED_WINDOW_LOOK;
			*_feel = B_NORMAL_WINDOW_FEEL;
			break;

		case B_TITLED_WINDOW:
		case B_UNTYPED_WINDOW:
		default:
			*_look = B_TITLED_WINDOW_LOOK;
			*_feel = B_NORMAL_WINDOW_FEEL;
			break;
	}
}


void
BWindow::_CreateTopView()
{
	STRACE(("_CreateTopView(): enter\n"));

	BRect frame = fFrame.OffsetToCopy(B_ORIGIN);
	fTopView = new BView(frame, "fTopView",
		B_FOLLOW_ALL, B_WILL_DRAW);
	fTopView->fTopLevelView = true;

	//inhibit check_lock()
	fLastViewToken = _get_object_token_(fTopView);

	// set fTopView's owner, add it to window's eligible handler list
	// and also set its next handler to be this window.

	STRACE(("Calling setowner fTopView = %p this = %p.\n", 
		fTopView, this));

	fTopView->_SetOwner(this);

	// we can't use AddChild() because this is the top view
  	fTopView->_CreateSelf();

	STRACE(("BuildTopView ended\n"));
}


/*!
	Resizes the top view to match the window size. This will also
	adapt the size of all its child views as needed.
	This method has to be called whenever the frame of the window
	changes.
*/
void
BWindow::_AdoptResize()
{
	// Resize views according to their resize modes - this
	// saves us some server communication, as the server
	// does the same with our views on its side.

	int32 deltaWidth = (int32)(fFrame.Width() - fTopView->Bounds().Width());
	int32 deltaHeight = (int32)(fFrame.Height() - fTopView->Bounds().Height());
	if (deltaWidth == 0 && deltaHeight == 0)
		return;

	fTopView->_ResizeBy(deltaWidth, deltaHeight);
}


void
BWindow::_SetFocus(BView *focusView, bool notifyInputServer)
{
	if (fFocus == focusView)
		return;

	if (focusView)
		focusView->MakeFocus(true);

	// TODO: Notify the input server if we are passing focus
	// from a view which has the B_INPUT_METHOD_AWARE to a one
	// which does not, or vice-versa
	if (notifyInputServer) {
		// TODO: Send a message to input server using
		// control_input_server()
	}
}


/*!
	\brief Determines the target of a message received for the
		focus view.
*/
BHandler *
BWindow::_DetermineTarget(BMessage *message, BHandler *target)
{
	if (target == NULL)
		target = this;

	switch (message->what) {
		case B_KEY_DOWN:
		case B_KEY_UP:
		{
			// if we have a default button, it might want to hear
			// about pressing the <enter> key
			int32 rawChar;
			if (DefaultButton() != NULL
				&& message->FindInt32("raw_char", &rawChar) == B_OK
				&& rawChar == B_ENTER)
				return DefaultButton();

			// supposed to fall through
		}
		case B_UNMAPPED_KEY_DOWN:
		case B_UNMAPPED_KEY_UP:
		case B_MODIFIERS_CHANGED:
			// these messages should be dispatched by the focus view
			if (CurrentFocus() != NULL)
				return CurrentFocus();
			break;

		case B_MOUSE_DOWN:
		case B_MOUSE_UP:
		case B_MOUSE_MOVED:
		case B_MOUSE_WHEEL_CHANGED:
			// is there a token of the view that is currently under the mouse?
			int32 token;
			if (message->FindInt32("_view_token", &token) == B_OK) {
				BView* view = _FindView(token);
				if (view != NULL)
					return view;
			}

			// if there is no valid token in the message, we try our
			// luck with the last target, if available
			if (fLastMouseMovedView != NULL)
				return fLastMouseMovedView;
			break;

		case B_PULSE:
		case B_QUIT_REQUESTED:
			// TODO: test wether R5 will let BView dispatch these messages
			return this;

		default: 
			break;
	}

	return target;
}


/*!
	\brief Distributes the message to its intended targets. This is done for
		all messages that should go to the preferred handler.

	Returns \c true in case the message should still be dispatched
*/
bool
BWindow::_UnpackMessage(unpack_cookie& cookie, BMessage** _message, BHandler** _target,
	bool* _usePreferred)
{
	if (cookie.message == NULL)
		return false;

	if (cookie.index == 0 && !cookie.tokens_scanned) {
		if (!*_usePreferred) {
			// we only consider messages targeted at the preferred handler
			cookie.message = NULL;
			return true;
		}

		// initialize our cookie
		cookie.message = *_message;
		cookie.focus = *_target;

		if (cookie.focus != NULL)
			cookie.focus_token = _get_object_token_(*_target);

		if (fLastMouseMovedView != NULL && cookie.message->what == B_MOUSE_MOVED)
			cookie.last_view_token = _get_object_token_(fLastMouseMovedView);

		*_usePreferred = false;
	}

	_DequeueAll();

	// distribute the message to all targets specified in the
	// message directly (but not to the focus view)

	for (int32 token; !cookie.tokens_scanned
			&& cookie.message->FindInt32("_token", cookie.index, &token) == B_OK;
			cookie.index++) {
		// focus view is preferred and should get its message directly
		if (token == cookie.focus_token) {
			cookie.found_focus = true;
			continue;
		}
		if (token == cookie.last_view_token)
			continue;

		BView* target = _FindView(token);
		if (target == NULL)
			continue;

		*_message = new BMessage(*cookie.message);
		*_target = target;
		cookie.index++;
		return true;
	}

	cookie.tokens_scanned = true;

	// if there is a last mouse moved view, and the new focus is
	// different, the previous view wants to get its B_EXITED_VIEW
	// message
	if (cookie.last_view_token != B_NULL_TOKEN && fLastMouseMovedView != NULL
		&& fLastMouseMovedView != cookie.focus) {
		*_message = new BMessage(*cookie.message);
		*_target = fLastMouseMovedView;
		cookie.last_view_token = B_NULL_TOKEN;
		return true;
	}

	bool dispatchToFocus = true;

	// check if the focus token is still valid (could have been removed in the mean time)
	BHandler* handler;
	if (gDefaultTokens.GetToken(cookie.focus_token, B_HANDLER_TOKEN, (void**)&handler) != B_OK
		|| handler->Looper() != this)
		dispatchToFocus = false;
	
	if (dispatchToFocus && cookie.index > 0) {
		// should this message still be dispatched by the focus view?
		bool feedFocus;
		if (!cookie.found_focus
			&& (cookie.message->FindBool("_feed_focus", &feedFocus) != B_OK
				|| feedFocus == false))
			dispatchToFocus = false;
	}

	if (!dispatchToFocus) {
		delete cookie.message;
		cookie.message = NULL;
		return false;
	}

	*_message = cookie.message;
	*_target = cookie.focus;
	*_usePreferred = true;
	cookie.message = NULL;
	return true;
}


void
BWindow::_SanitizeMessage(BMessage* message, BHandler* target, bool usePreferred)
{
	if (target == NULL)
		return;

	switch (message->what) {
		case B_MOUSE_MOVED:
		case B_MOUSE_UP:
		case B_MOUSE_DOWN:
			BPoint where;
			if (message->FindPoint("screen_where", &where) != B_OK)
				break;

			// add local window coordinates
			message->AddPoint("where", ConvertFromScreen(where));

			BView* view = dynamic_cast<BView*>(target);
			if (view != NULL) {
				// add local view coordinates
				message->AddPoint("be:view_where", view->ConvertFromScreen(where));

				if (message->what == B_MOUSE_MOVED) {
					// is there a token of the view that is currently under the mouse?
					BView* viewUnderMouse = NULL;
					int32 token;
					if (message->FindInt32("_view_token", &token) == B_OK)
						viewUnderMouse = _FindView(token);

					// add transit information
					int32 transit;
					if (viewUnderMouse == view) {
						// the mouse is over the target view
						if (fLastMouseMovedView != view)
							transit = B_ENTERED_VIEW;
						else
							transit = B_INSIDE_VIEW;
					} else {
						// the mouse is not over the target view
						if (view == fLastMouseMovedView)
							transit = B_EXITED_VIEW;
						else
							transit = B_OUTSIDE_VIEW;
					}

					message->AddInt32("be:transit", transit);

					if (usePreferred || viewUnderMouse == NULL)
						fLastMouseMovedView = viewUnderMouse;
				}
			}
			break;
	}
}


bool
BWindow::_HandleKeyDown(char key, uint32 modifiers)
{
	// TODO: ask people if using 'raw_char' is OK ?

	// handle BMenuBar key
	if (key == B_ESCAPE && (modifiers & B_COMMAND_KEY) != 0
		&& fKeyMenuBar) {
		// TODO: ask Marc about 'fWaitingForMenu' member!

		// fWaitingForMenu = true;
		fKeyMenuBar->StartMenuBar(0, true, false, NULL);
		return true;
	}

	// Keyboard navigation through views
	// (B_OPTION_KEY makes BTextViews and friends navigable, even in editing mode)
	if (key == B_TAB && (modifiers & (B_COMMAND_KEY | B_OPTION_KEY)) != 0) {
		_KeyboardNavigation();
		return true;
	}

	// Handle shortcuts
	if ((modifiers & B_COMMAND_KEY) != 0) {
		// Command+q has been pressed, so, we will quit
		// the shortcut mechanism doesn't allow handlers outside the window
		// TODO: in BeOS, this shortcut can be removed!
		if (key == 'Q' || key == 'q') {
			be_app->PostMessage(B_QUIT_REQUESTED);
			return true;
		}

		Shortcut* shortcut = _FindShortcut(key, modifiers);
		if (shortcut != NULL) {
			// TODO: would be nice to move this functionality to
			//	a Shortcut::Invoke() method - but since BMenu::InvokeItem()
			//	(and BMenuItem::Invoke()) are private, I didn't want
			//	to mess with them (BMenuItem::Invoke() is public in
			//	Dano/Zeta, though, maybe we should just follow their
			//	example)
			if (shortcut->MenuItem() != NULL) {
				BMenu* menu = shortcut->MenuItem()->Menu();
				if (menu != NULL)
					menu->InvokeItem(shortcut->MenuItem(), true);
			} else {
				BHandler* target = shortcut->Target();
				if (target == NULL)
					target = CurrentFocus();

				BMessage* message = shortcut->Message();
				if (message->ReplaceInt64("when", system_time()) != B_OK)
					message->AddInt64("when", system_time());

				PostMessage(message, target);
			}

			return true;
		}
	}

	// TODO: convert keys to the encoding of the target view

	return false;
}


void
BWindow::_KeyboardNavigation()
{
	BMessage *message = CurrentMessage();
	if (message == NULL)
		return;

	const char *bytes;
	uint32 modifiers;
	if (message->FindString("bytes", &bytes) != B_OK
		|| bytes[0] != B_TAB)
		return;

	message->FindInt32("modifiers", (int32*)&modifiers);

	BView *nextFocus;
	int32 jumpGroups = modifiers & B_CONTROL_KEY ? B_NAVIGABLE_JUMP : B_NAVIGABLE;
	if (modifiers & B_SHIFT_KEY)
		nextFocus = _FindPreviousNavigable(fFocus, jumpGroups);
	else
		nextFocus = _FindNextNavigable(fFocus, jumpGroups);

	if (nextFocus && nextFocus != fFocus)
		_SetFocus(nextFocus, false);
}


BMessage *
BWindow::ConvertToMessage(void *raw, int32 code)
{
	return BLooper::ConvertToMessage(raw, code);
}


BWindow::Shortcut *
BWindow::_FindShortcut(uint32 key, uint32 modifiers)
{
	int32 count = fShortcuts.CountItems();

	key = Shortcut::PrepareKey(key);
	modifiers = Shortcut::PrepareModifiers(modifiers);

	for (int32 index = 0; index < count; index++) {
		Shortcut *shortcut = (Shortcut *)fShortcuts.ItemAt(index);

		if (shortcut->Matches(key, modifiers))
			return shortcut;
	}

	return NULL;
}


BView *
BWindow::_FindView(int32 token)
{
	BHandler* handler;
	if (gDefaultTokens.GetToken(token, B_HANDLER_TOKEN, (void**)&handler) != B_OK)
		return NULL;

	// the view must belong to us in order to be found by this method
	BView* view = dynamic_cast<BView*>(handler);
	if (view != NULL && view->Window() == this)
		return view;

	return NULL;
}


BView *
BWindow::_FindView(BView *view, BPoint point) const
{
	// TODO: this is totally broken (bounds vs. frame) - since
	//	BView::Bounds() potentially queries the app_server
	//	anyway, we could just let the app_server answer this
	//	query directly.
	if (view->Bounds().Contains(point) && !view->fFirstChild)
		return view;

	BView *child = view->fFirstChild;

	while (child != NULL) {
		if ((view = _FindView(child, point)) != NULL)
			return view;

		child = child->fNextSibling;
	}

	return NULL;
}


BView *
BWindow::_FindNextNavigable(BView *focus, uint32 flags)
{
	if (focus == NULL)
		focus = fTopView;

	BView *nextFocus = focus;

	// Search the tree for views that accept focus
	while (true) {
		if (nextFocus->fFirstChild)
			nextFocus = nextFocus->fFirstChild;
		else if (nextFocus->fNextSibling)
			nextFocus = nextFocus->fNextSibling;
		else {
			while (!nextFocus->fNextSibling && nextFocus->fParent)
				nextFocus = nextFocus->fParent;

			if (nextFocus == fTopView)
				nextFocus = nextFocus->fFirstChild;
			else
				nextFocus = nextFocus->fNextSibling;
		}

		// It means that the hole tree has been searched and there is no
		// view with B_NAVIGABLE_JUMP flag set!
		if (nextFocus == focus)
			return NULL;

		if (nextFocus->Flags() & flags)
			return nextFocus;
	}
}


BView *
BWindow::_FindPreviousNavigable(BView *focus, uint32 flags)
{
	BView *prevFocus = focus;

	// Search the tree for views that accept focus
	while (true) {
		BView *view;
		if ((view = _LastViewChild(prevFocus)) != NULL)
			prevFocus = view;
		else if (prevFocus->fPreviousSibling)
			prevFocus = prevFocus->fPreviousSibling;
		else {
			while (!prevFocus->fPreviousSibling && prevFocus->fParent)
				prevFocus = prevFocus->fParent;

			if (prevFocus == fTopView)
				prevFocus = _LastViewChild(prevFocus);
			else
				prevFocus = prevFocus->fPreviousSibling;
		}

		// It means that the hole tree has been searched and there is no
		// view with B_NAVIGABLE_JUMP flag set!
		if (prevFocus == focus)
			return NULL;

		if (prevFocus->Flags() & flags)
			return prevFocus;
	}
}


BView *
BWindow::_LastViewChild(BView *parent)
{
	BView *last = parent->fFirstChild;
	if (last == NULL)
		return NULL;

	while (last->fNextSibling)
		last = last->fNextSibling;

	return last;
}


void
BWindow::SetIsFilePanel(bool isFilePanel)
{
	fIsFilePanel = isFilePanel;
}


bool
BWindow::IsFilePanel() const
{
	return fIsFilePanel;
}


//------------------------------------------------------------------------------
// Virtual reserved Functions

void BWindow::_ReservedWindow1() {}
void BWindow::_ReservedWindow2() {}
void BWindow::_ReservedWindow3() {}
void BWindow::_ReservedWindow4() {}
void BWindow::_ReservedWindow5() {}
void BWindow::_ReservedWindow6() {}
void BWindow::_ReservedWindow7() {}
void BWindow::_ReservedWindow8() {}

void
BWindow::PrintToStream() const
{
	printf("BWindow '%s' data:\
		Title			= %s\
		Token			= %ld\
		InTransaction 	= %s\
		Active 			= %s\
		fShowLevel		= %d\
		Flags			= %lx\
		send_port		= %ld\
		receive_port	= %ld\
		fTopView name	= %s\
		focus view name	= %s\
		lastMouseMoved	= %s\
		fLink			= %p\
		KeyMenuBar name	= %s\
		DefaultButton	= %s\
		# of shortcuts	= %ld",
		Name(), fTitle,
		_get_object_token_(this),		
		fInTransaction == true ? "yes" : "no",
		fActive == true ? "yes" : "no",
		fShowLevel,
		fFlags,
		fLink->SenderPort(),
		fLink->ReceiverPort(),
		fTopView != NULL ? fTopView->Name() : "NULL",
		fFocus != NULL ? fFocus->Name() : "NULL",
		fLastMouseMovedView != NULL ? fLastMouseMovedView->Name() : "NULL",
		fLink,
		fKeyMenuBar != NULL ? fKeyMenuBar->Name() : "NULL",
		fDefaultButton != NULL ? fDefaultButton->Name() : "NULL",
		fShortcuts.CountItems());
/*
	for( int32 i=0; i<accelList.CountItems(); i++){
		_BCmdKey	*key = (_BCmdKey*)accelList.ItemAt(i);
		printf("\tShortCut %ld: char %s\n\t\t message: \n", i, (key->key > 127)?"ASCII":"UNICODE");
		key->message->PrintToStream();
	}
*/	
	printf("\
		topViewToken	= %ld\
		pluseEnabled	= %s\
		isFilePanel		= %s\
		MaskActivated	= %s\
		pulseRate		= %lld\
		waitingForMenu	= %s\
		minimized		= %s\
		Menu semaphore	= %ld\
		maxZoomHeight	= %f\
		maxZoomWidth	= %f\
		minWindHeight	= %f\
		minWindWidth	= %f\
		maxWindHeight	= %f\
		maxWindWidth	= %f\
		frame			= ( %f, %f, %f, %f )\
		look			= %d\
		feel			= %d\
		lastViewToken	= %ld\
		pulseRunner		= %s\n",
		fTopViewToken,
		fPulseEnabled==true?"Yes":"No",
		fIsFilePanel==true?"Yes":"No",
		fMaskActivated==true?"Yes":"No",
		fPulseRate,
		fWaitingForMenu==true?"Yes":"No",
		fMinimized==true?"Yes":"No",
		fMenuSem,
		fMaxZoomHeight,
		fMaxZoomWidth,
		fMinHeight,
		fMinWidth,
		fMaxHeight,
		fMaxWidth,
		fFrame.left, fFrame.top, fFrame.right, fFrame.bottom, 
		(int16)fLook,
		(int16)fFeel,
		fLastViewToken,
		fPulseRunner != NULL ? "In place" : "NULL");
}

/*
TODO list:
	*) test arguments for SetWindowAligment
	*) call hook functions: MenusBeginning, MenusEnded. Add menu activation code.
*/

