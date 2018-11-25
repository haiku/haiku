/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered
trademarks of Be Incorporated in the United States and other countries. Other
brand product names are registered trademarks or trademarks of their respective
holders.
All rights reserved.
*/


#include "BarView.h"

#include <AppFileInfo.h>
#include <Bitmap.h>
#include <Debug.h>
#include <Directory.h>
#include <LocaleRoster.h>
#include <NodeInfo.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>

#include "icons.h"
#include "BarApp.h"
#include "BarMenuBar.h"
#include "BarWindow.h"
#include "DeskbarMenu.h"
#include "DeskbarUtils.h"
#include "ExpandoMenuBar.h"
#include "FSUtils.h"
#include "InlineScrollView.h"
#include "ResourceSet.h"
#include "StatusView.h"
#include "TeamMenuItem.h"


const int32 kDefaultRecentDocCount = 10;
const int32 kDefaultRecentAppCount = 10;

const int32 kMenuTrackMargin = 20;

const uint32 kUpdateOrientation = 'UpOr';
const float kSepItemWidth = 5.0f;


class BarViewMessageFilter : public BMessageFilter
{
	public:
		BarViewMessageFilter(TBarView* barView);
		virtual ~BarViewMessageFilter();

		virtual filter_result Filter(BMessage* message, BHandler** target);

	private:
		TBarView* fBarView;
};


BarViewMessageFilter::BarViewMessageFilter(TBarView* barView)
	:
	BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
	fBarView(barView)
{
}


BarViewMessageFilter::~BarViewMessageFilter()
{
}


filter_result
BarViewMessageFilter::Filter(BMessage* message, BHandler** target)
{
	if (message->what == B_MOUSE_DOWN || message->what == B_MOUSE_MOVED) {
		BPoint where = message->FindPoint("be:view_where");
		uint32 transit = message->FindInt32("be:transit");
		BMessage* dragMessage = NULL;
		if (message->HasMessage("be:drag_message")) {
			dragMessage = new BMessage();
			message->FindMessage("be:drag_message", dragMessage);
		}

		switch (message->what) {
			case B_MOUSE_DOWN:
				fBarView->MouseDown(where);
				break;

			case B_MOUSE_MOVED:
				fBarView->MouseMoved(where, transit, dragMessage);
				break;
		}

		delete dragMessage;
	}

	return B_DISPATCH_MESSAGE;
}


//	#pragma mark - TBarView


TBarView::TBarView(BRect frame, bool vertical, bool left, bool top,
	int32 state, float)
	:
	BView(frame, "BarView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
	fBarApp(static_cast<TBarApp*>(be_app)),
	fInlineScrollView(NULL),
	fBarMenuBar(NULL),
	fExpandoMenuBar(NULL),
	fTrayLocation(1),
	fVertical(vertical),
	fTop(top),
	fLeft(left),
	fState(state),
	fRefsRcvdOnly(true),
	fDragMessage(NULL),
	fCachedTypesList(NULL),
	fMaxRecentDocs(kDefaultRecentDocCount),
	fMaxRecentApps(kDefaultRecentAppCount),
	fLastDragItem(NULL),
	fMouseFilter(NULL)
{
	// determine the initial Be menu size
	BRect menuFrame(frame);
	if (fVertical)
		menuFrame.bottom = menuFrame.top + kMenuBarHeight;
	else
		menuFrame.bottom = menuFrame.top + fBarApp->IconSize() + 4;

	// create and add the Be menu
	fBarMenuBar = new TBarMenuBar(menuFrame, "BarMenuBar", this);
	AddChild(fBarMenuBar);

	// create the status tray
	fReplicantTray = new TReplicantTray(this, fVertical);

	// create the resize control
	fResizeControl = new TResizeControl(this);

	// create the drag region and add the resize control
	// and replicant tray to it
	fDragRegion = new TDragRegion(this, fReplicantTray);
	fDragRegion->AddChild(fResizeControl);
	fDragRegion->AddChild(fReplicantTray);

	// Add the drag region
	if (fTrayLocation != 0)
		AddChild(fDragRegion);

	// create and add the application menubar
	fExpandoMenuBar = new TExpandoMenuBar(this, fVertical);
	fInlineScrollView = new TInlineScrollView(fExpandoMenuBar,
		fVertical ? B_VERTICAL : B_HORIZONTAL);
	AddChild(fInlineScrollView);

	// If mini mode, hide the application menubar
	if (state == kMiniState)
		fInlineScrollView->Hide();
}


TBarView::~TBarView()
{
	delete fDragMessage;
	delete fCachedTypesList;
	delete fBarMenuBar;
}


void
TBarView::AttachedToWindow()
{
	BView::AttachedToWindow();

	SetViewUIColor(B_MENU_BACKGROUND_COLOR);
	SetFont(be_plain_font);

	fMouseFilter = new BarViewMessageFilter(this);
	Window()->AddCommonFilter(fMouseFilter);

	fTrackingHookData.fTrackingHook = MenuTrackingHook;
	fTrackingHookData.fTarget = BMessenger(this);
	fTrackingHookData.fDragMessage = new BMessage(B_REFS_RECEIVED);
}


void
TBarView::DetachedFromWindow()
{
	Window()->RemoveCommonFilter(fMouseFilter);
	delete fMouseFilter;
	fMouseFilter = NULL;
	delete fTrackingHookData.fDragMessage;
	fTrackingHookData.fDragMessage = NULL;
}


void
TBarView::Draw(BRect)
{
	BRect bounds(Bounds());

	rgb_color hilite = tint_color(ViewColor(), B_DARKEN_1_TINT);

	SetHighColor(hilite);
	if (AcrossTop())
		StrokeLine(bounds.LeftBottom(), bounds.RightBottom());
	else if (AcrossBottom())
		StrokeLine(bounds.LeftTop(), bounds.RightTop());

	if (fVertical && fState == kExpandoState) {
		SetHighColor(hilite);
		BRect frame(fExpandoMenuBar->Frame());
		StrokeLine(BPoint(frame.left, frame.top - 1),
			BPoint(frame.right, frame.top -1));
	}
}


void
TBarView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_LOCALE_CHANGED:
		case kRealignReplicants:
		case kShowHideTime:
		case kShowSeconds:
		case kShowDayOfWeek:
		case kShowTimeZone:
		case kGetClockSettings:
			fReplicantTray->MessageReceived(message);
			break;

		case B_REFS_RECEIVED:
			// received when an item is selected during DnD
			// message is targeted here from Be menu
			HandleDeskbarMenu(message);
			break;

		case B_ARCHIVED_OBJECT:
		{
			// this message has been retargeted to here
			// instead of directly to the replicant tray
			// so that I can follow the common pathway
			// for adding icons to the tray
			int32 id;
			if (AddItem(message, B_DESKBAR_TRAY, &id) == B_OK)
				Looper()->DetachCurrentMessage();
			break;
		}

		case kUpdateOrientation:
		{
			_ChangeState(message);
			break;
		}

		default:
			BView::MessageReceived(message);
	}
}


void
TBarView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	if (fDragRegion->IsDragging()) {
		fDragRegion->MouseMoved(where, transit, dragMessage);
		return;
	}

	if (transit == B_ENTERED_VIEW && EventMask() == 0)
		SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);

	BPoint whereScreen = ConvertToScreen(where);

	desk_settings* settings = fBarApp->Settings();
	bool alwaysOnTop = settings->alwaysOnTop;
	bool autoRaise = settings->autoRaise;
	bool autoHide = settings->autoHide;

	if (!autoRaise && !autoHide) {
		if (transit == B_EXITED_VIEW || transit == B_OUTSIDE_VIEW)
			SetEventMask(0);
		return;
	}

	bool isTopMost = Window()->Feel() == B_FLOATING_ALL_WINDOW_FEEL;

	// Auto-Raise
	BRect screenFrame = (BScreen(Window())).Frame();
	if ((whereScreen.x == screenFrame.left
			|| whereScreen.x == screenFrame.right
			|| whereScreen.y == screenFrame.top
			|| whereScreen.y == screenFrame.bottom)
		&& Window()->Frame().Contains(whereScreen)) {
		// cursor is on a screen edge within the window frame

		if (!alwaysOnTop && autoRaise && !isTopMost)
			RaiseDeskbar(true);

		if (autoHide && IsHidden())
			HideDeskbar(false);
	} else {
		TBarWindow* window = (TBarWindow*)Window();
		if (window->IsShowingMenu())
			return;

		// cursor is not on screen edge
		BRect preventHideArea = Window()->Frame().InsetByCopy(
			-kMaxPreventHidingDist, -kMaxPreventHidingDist);

		if (preventHideArea.Contains(whereScreen))
			return;

		// cursor to bar distance above threshold
		if (!alwaysOnTop && autoRaise && isTopMost) {
			RaiseDeskbar(false);
			SetEventMask(0);
		}

		if (autoHide && !IsHidden())
			HideDeskbar(true);
	}
}


void
TBarView::MouseDown(BPoint where)
{
	BPoint whereScreen = ConvertToScreen(where);

	if (Window()->Frame().Contains(whereScreen)) {
		Window()->Activate();

		if ((modifiers() & (B_CONTROL_KEY | B_COMMAND_KEY | B_OPTION_KEY
				| B_SHIFT_KEY)) == (B_CONTROL_KEY | B_COMMAND_KEY)) {
			// The window key was pressed - enter dragging code
			fDragRegion->MouseDown(fDragRegion->DragRegion().LeftTop());
			return;
		}
	} else {
		// hide deskbar if required
		desk_settings* settings = fBarApp->Settings();
		bool alwaysOnTop = settings->alwaysOnTop;
		bool autoRaise = settings->autoRaise;
		bool autoHide = settings->autoHide;
		bool isTopMost = Window()->Feel() == B_FLOATING_ALL_WINDOW_FEEL;

		if (!alwaysOnTop && autoRaise && isTopMost)
			RaiseDeskbar(false);

		if (autoHide && !IsHidden())
			HideDeskbar(true);
	}
}


void
TBarView::PlaceDeskbarMenu()
{
	float width = 0;
	float height = 0;

	// Calculate the size of the deskbar menu
	BRect menuFrame(Bounds());
	if (fVertical) {
		width = static_cast<TBarApp*>(be_app)->Settings()->width;
		height = 4 + fReplicantTray->MaxReplicantHeight();
		menuFrame.bottom = menuFrame.top + height;
	} else {
		width = gMinimumWindowWidth;
		height = fBarApp->IconSize() + 4;
		menuFrame.bottom = menuFrame.top + height;
	}

	if (fBarMenuBar == NULL) {
		// create the Be menu
		fBarMenuBar = new TBarMenuBar(menuFrame, "BarMenuBar", this);
		AddChild(fBarMenuBar);
	} else
		fBarMenuBar->SmartResize(-1, -1);

	BPoint loc(B_ORIGIN);
	if (fState == kFullState) {
		fBarMenuBar->RemoveTeamMenu();
		fBarMenuBar->RemoveSeperatorItem();
		loc = Bounds().LeftTop();
	} else if (fState == kExpandoState) {
		fBarMenuBar->RemoveTeamMenu();
		if (fVertical) {
			// shows apps below tray
			fBarMenuBar->RemoveSeperatorItem();
			width += 1;
		} else {
			// shows apps to the right of bemenu
			fBarMenuBar->AddSeparatorItem();
			width = floorf(width) / 2 + kSepItemWidth;
		}
		loc = Bounds().LeftTop();
	} else {
		// mini mode, DeskbarMenu next to team menu
		fBarMenuBar->RemoveSeperatorItem();
		fBarMenuBar->AddTeamMenu();
	}

	fBarMenuBar->SmartResize(width, height);
	fBarMenuBar->MoveTo(loc);
}


void
TBarView::PlaceTray(bool vertSwap, bool leftSwap)
{
	BPoint statusLoc;
	if (fState == kFullState) {
		fDragRegion->ResizeTo(fBarMenuBar->Frame().Width(), kMenuBarHeight);
		statusLoc.y = fBarMenuBar->Frame().bottom + 1;
		statusLoc.x = 0;
		fDragRegion->MoveTo(statusLoc);
		fDragRegion->Invalidate();

		if (!fReplicantTray->IsHidden())
			fReplicantTray->Hide();

		return;
	}

	if (fReplicantTray->IsHidden())
		fReplicantTray->Show();

	if (fTrayLocation != 0) {
		fReplicantTray->SetMultiRow(fVertical);
		fReplicantTray->RealignReplicants();
		fDragRegion->ResizeToPreferred();
			// also resizes replicant tray

		fResizeControl->ResizeTo(kDragWidth, fDragRegion->Bounds().Height()
			- 2); // make room for top and bottom border

		if (fVertical) {
			if (fResizeControl->IsHidden())
				fResizeControl->Show();

			if (fLeft) {
				// move replicant tray past dragger width on left
				// also down 1px so it won't cover the border
				fReplicantTray->MoveTo(kDragWidth + kGutter, kGutter);

				// shrink width by same amount
				fReplicantTray->ResizeBy(-(kDragWidth + kGutter), 0);
			} else {
				// move replicant tray down 1px so it won't cover the border
				fReplicantTray->MoveTo(0, kGutter);
			}

			statusLoc.x = 0;
			statusLoc.y = fBarMenuBar->Frame().bottom + 1;
		} else {
			if (!fResizeControl->IsHidden())
				fResizeControl->Hide();

			// move right and down to not cover border then resize to fit
			fReplicantTray->MoveTo(kGutter, kGutter);
			fReplicantTray->ResizeBy(-kGutter, -kGutter);
			BRect screenFrame = (BScreen(Window())).Frame();
			statusLoc.x = screenFrame.right - fDragRegion->Bounds().Width();
			statusLoc.y = -1;
		}

		fDragRegion->MoveTo(statusLoc);
		fDragRegion->Invalidate();

		if (fVertical && fLeft)
			fResizeControl->MoveTo(fDragRegion->Bounds().right - kDragWidth, 1);
		else
			fResizeControl->MoveTo(0, 1);

		fResizeControl->Invalidate();
	}
}


void
TBarView::PlaceApplicationBar()
{
	BRect screenFrame = (BScreen(Window())).Frame();
	if (fState == kMiniState) {
		if (!fInlineScrollView->IsHidden())
			fInlineScrollView->Hide();
		SizeWindow(screenFrame);
		PositionWindow(screenFrame);
		Window()->UpdateIfNeeded();
		Invalidate();
		return;
	}

	if (fInlineScrollView->IsHidden())
		fInlineScrollView->Show();

	BRect expandoFrame(0, 0, 0, 0);
	if (fVertical) {
		// left or right
		expandoFrame.left = fDragRegion->Frame().left;
		expandoFrame.top = fTrayLocation != 0 ? fDragRegion->Frame().bottom + 1
			: fBarMenuBar->Frame().bottom + 1;
		expandoFrame.right = fBarMenuBar->Frame().right;
		expandoFrame.bottom = fState == kFullState ? screenFrame.bottom
			: Frame().bottom;
	} else {
		// top or bottom
		expandoFrame.top = 0;
		expandoFrame.bottom = fBarApp->IconSize() + 4;

		if (fBarMenuBar != NULL)
			expandoFrame.left = fBarMenuBar->Frame().Width() + 1;

		if (fTrayLocation != 0 && fDragRegion != NULL) {
			expandoFrame.right = screenFrame.Width()
				- fDragRegion->Frame().Width() - 1;
		} else
			expandoFrame.right = screenFrame.Width();
	}

	fInlineScrollView->DetachScrollers();
	fInlineScrollView->MoveTo(expandoFrame.LeftTop());
	fInlineScrollView->ResizeTo(expandoFrame.Width(), fVertical
		? screenFrame.bottom - expandoFrame.top
		: expandoFrame.Height());
	fExpandoMenuBar->MoveTo(0, 0);
	fExpandoMenuBar->ResizeTo(expandoFrame.Width(), expandoFrame.Height());

	if (!fVertical) {
		// Set the max item width based on icon size
		fExpandoMenuBar->SetMaxItemWidth();
	}

	if (fState == kExpandoState)
		fExpandoMenuBar->BuildItems();

	SizeWindow(screenFrame);
	PositionWindow(screenFrame);
	fExpandoMenuBar->DoLayout();
		// force menu to resize
	CheckForScrolling();
	Window()->UpdateIfNeeded();
	Invalidate();
}


void
TBarView::GetPreferredWindowSize(BRect screenFrame, float* width, float* height)
{
	float windowHeight = 0;
	float windowWidth = fBarApp->Settings()->width;
	bool setToHiddenSize = fBarApp->Settings()->autoHide && IsHidden()
		&& !fDragRegion->IsDragging();

	if (setToHiddenSize) {
		windowHeight = kHiddenDimension;

		if (fState == kExpandoState && !fVertical) {
			// top or bottom, full
			fExpandoMenuBar->CheckItemSizes(0);
			windowWidth = screenFrame.Width();
		} else
			windowWidth = kHiddenDimension;
	} else {
		if (fState == kFullState) {
			windowHeight = screenFrame.bottom;
			windowWidth = fBarMenuBar->Frame().Width();
		} else if (fState == kExpandoState) {
			if (fVertical) {
				// top left or right
				if (fTrayLocation != 0)
					windowHeight = fDragRegion->Frame().bottom + 1;
				else
					windowHeight = fBarMenuBar->Frame().bottom + 1;

				windowHeight += fExpandoMenuBar->Bounds().Height();
			} else {
				// top or bottom, full
				fExpandoMenuBar->CheckItemSizes(0);
				windowHeight = fBarApp->IconSize() + 4;
				windowWidth = screenFrame.Width();
			}
		} else {
			// four corners
			if (fTrayLocation != 0)
				windowHeight = fDragRegion->Frame().bottom;
			else
				windowHeight = fBarMenuBar->Frame().bottom;
		}
	}

	*width = windowWidth;
	*height = windowHeight;
}


void
TBarView::SizeWindow(BRect screenFrame)
{
	float windowWidth;
	float windowHeight;
	GetPreferredWindowSize(screenFrame, &windowWidth, &windowHeight);
	Window()->ResizeTo(windowWidth, windowHeight);
}


void
TBarView::PositionWindow(BRect screenFrame)
{
	float windowWidth;
	float windowHeight;
	GetPreferredWindowSize(screenFrame, &windowWidth, &windowHeight);

	BPoint moveLoc(0, 0);
	// right, expanded
	if (!fLeft && fVertical) {
		if (fState == kFullState)
			moveLoc.x = screenFrame.right - fBarMenuBar->Frame().Width();
		else
			moveLoc.x = screenFrame.right - windowWidth;
	}

	// bottom, full or corners
	if (!fTop)
		moveLoc.y = screenFrame.bottom - windowHeight;

	Window()->MoveTo(moveLoc);
}


void
TBarView::CheckForScrolling()
{
	if (fInlineScrollView != NULL && fExpandoMenuBar != NULL) {
		if (fExpandoMenuBar->CheckForSizeOverrun())
			fInlineScrollView->AttachScrollers();
		else
			fInlineScrollView->DetachScrollers();
	}
}


void
TBarView::SaveSettings()
{
	desk_settings* settings = fBarApp->Settings();

	settings->vertical = fVertical;
	settings->left = fLeft;
	settings->top = fTop;
	settings->state = fState;

	fReplicantTray->SaveTimeSettings();
}


void
TBarView::UpdatePlacement()
{
	ChangeState(fState, fVertical, fLeft, fTop);
}


void
TBarView::ChangeState(int32 state, bool vertical, bool left, bool top,
	bool async)
{
	BMessage message(kUpdateOrientation);
	message.AddInt32("state", state);
	message.AddBool("vertical", vertical);
	message.AddBool("left", left);
	message.AddBool("top", top);

	if (async)
		BMessenger(this).SendMessage(&message);
	else
		_ChangeState(&message);
}


void
TBarView::_ChangeState(BMessage* message)
{
	int32 state = message->FindInt32("state");
	bool vertical = message->FindBool("vertical");
	bool left = message->FindBool("left");
	bool top = message->FindBool("top");

	bool vertSwap = (fVertical != vertical);
	bool leftSwap = (fLeft != left);
	bool stateChanged = (fState != state);

	fState = state;
	fVertical = vertical;
	fLeft = left;
	fTop = top;

	if (stateChanged || vertSwap) {
		be_app->PostMessage(kStateChanged);
			// Send a message to the preferences window to let it know to
			// enable or disable preference items.

		if (vertSwap) {
			fReplicantTray->fTime->SetOrientation(fVertical);
			if (fExpandoMenuBar != NULL) {
				if (fVertical) {
					fInlineScrollView->SetOrientation(B_VERTICAL);
					fExpandoMenuBar->SetMenuLayout(B_ITEMS_IN_COLUMN);
					fExpandoMenuBar->StartMonitoringWindows();
				} else {
					fInlineScrollView->SetOrientation(B_HORIZONTAL);
					fExpandoMenuBar->SetMenuLayout(B_ITEMS_IN_ROW);
					fExpandoMenuBar->StopMonitoringWindows();
				}
			}
		}
	}

	PlaceDeskbarMenu();
	PlaceTray(vertSwap, leftSwap);
	PlaceApplicationBar();
}


void
TBarView::RaiseDeskbar(bool raise)
{
	if (raise)
		Window()->SetFeel(B_FLOATING_ALL_WINDOW_FEEL);
	else
		Window()->SetFeel(B_NORMAL_WINDOW_FEEL);
}


void
TBarView::HideDeskbar(bool hide)
{
	BRect screenFrame = (BScreen(Window())).Frame();

	if (hide) {
		Hide();
		PositionWindow(screenFrame);
		SizeWindow(screenFrame);
	} else {
		Show();
		SizeWindow(screenFrame);
		PositionWindow(screenFrame);
	}
}


//	#pragma mark - Drag and Drop


void
TBarView::CacheDragData(const BMessage* incoming)
{
	if (!incoming)
		return;

	if (Dragging() && SpringLoadedFolderCompareMessages(incoming, fDragMessage))
		return;

	// disposes then fills cached drag message and
	// mimetypes list
	SpringLoadedFolderCacheDragData(incoming, &fDragMessage, &fCachedTypesList);
}


static void
init_tracking_hook(BMenuItem* item,
	bool (*hookFunction)(BMenu*, void*), void* state)
{
	if (!item)
		return;

	BMenu* windowMenu = item->Submenu();
	if (windowMenu) {
		// have a menu, set the tracking hook
		windowMenu->SetTrackingHook(hookFunction, state);
	}
}


status_t
TBarView::DragStart()
{
	if (!Dragging())
		return B_OK;

	BPoint loc;
	uint32 buttons;
	GetMouse(&loc, &buttons);

	if (fExpandoMenuBar != NULL && fExpandoMenuBar->Frame().Contains(loc)) {
		ConvertToScreen(&loc);
		BPoint expandoLocation = fExpandoMenuBar->ConvertFromScreen(loc);
		TTeamMenuItem* item = fExpandoMenuBar->TeamItemAtPoint(expandoLocation);

		if (fLastDragItem)
			init_tracking_hook(fLastDragItem, NULL, NULL);

		if (item != NULL) {
			if (item == fLastDragItem)
				return B_OK;

			fLastDragItem = item;
		}
	}

	return B_OK;
}


bool
TBarView::MenuTrackingHook(BMenu* menu, void* castToThis)
{
	// return true if the menu should go away
	TrackingHookData* data = static_cast<TrackingHookData*>(castToThis);
	if (!data)
		return false;

	TBarView* barview = dynamic_cast<TBarView*>(data->fTarget.Target(NULL));
	if (!barview || !menu->LockLooper())
		return false;

	uint32 buttons;
	BPoint location;
	menu->GetMouse(&location, &buttons);

	bool endMenu = true;
	BRect frame(menu->Bounds());
	frame.InsetBy(-kMenuTrackMargin, -kMenuTrackMargin);

	if (frame.Contains(location)) {
		// if current loc is still in the menu
		// keep tracking
		endMenu = false;
	} else {
		// see if the mouse is in the team/deskbar menu item
		menu->ConvertToScreen(&location);
		if (barview->LockLooper()) {
			TExpandoMenuBar* expando = barview->ExpandoMenuBar();
			TDeskbarMenu* bemenu
				= (dynamic_cast<TBarWindow*>(barview->Window()))->DeskbarMenu();

			if (bemenu && bemenu->LockLooper()) {
				bemenu->ConvertFromScreen(&location);
				if (bemenu->Frame().Contains(location))
					endMenu = false;

				bemenu->UnlockLooper();
			}

			if (endMenu && expando) {
				expando->ConvertFromScreen(&location);
				BMenuItem* item = expando->TeamItemAtPoint(location);
				if (item)
					endMenu = false;
			}
			barview->UnlockLooper();
		}
	}

	menu->UnlockLooper();
	return endMenu;
}


// used by WindowMenu and TeamMenu to
// set the tracking hook for dragging
TrackingHookData*
TBarView::GetTrackingHookData()
{
	// all tracking hook data is
	// preset in AttachedToWindow
	// data should never change
	return &fTrackingHookData;
}


void
TBarView::DragStop(bool full)
{
	if (!Dragging())
		return;

	if (fExpandoMenuBar != NULL) {
		if (fLastDragItem != NULL) {
			init_tracking_hook(fLastDragItem, NULL, NULL);
			fLastDragItem = NULL;
		}
	}

	if (full) {
		delete fDragMessage;
		fDragMessage = NULL;

		delete fCachedTypesList;
		fCachedTypesList = NULL;
	}
}


bool
TBarView::AppCanHandleTypes(const char* signature)
{
	// used for filtering apps/teams in the ExpandoMenuBar and TeamMenu

	if (modifiers() & B_CONTROL_KEY) {
		// control key forces acceptance, just like drag&drop on icons
		return true;
	}

	if (!signature || strlen(signature) == 0
		|| !fCachedTypesList || fCachedTypesList->CountItems() == 0)
		return false;

	if (strcasecmp(signature, kTrackerSignature) == 0) {
		// tracker should support all types
		// and should pass them on to the appropriate application
		return true;
	}

	entry_ref hintref;
	BMimeType appmime(signature);
	if (appmime.GetAppHint(&hintref) != B_OK)
		return false;

	// an app was found, now see if it supports any of
	// the refs in the message
	BFile file(&hintref, O_RDONLY);
	BAppFileInfo fileinfo(&file);

	// scan the cached mimetype list and see if this app
	// supports anything in the list
	// only one item needs to match in the list of refs

	int32 count = fCachedTypesList->CountItems();
	for (int32 i = 0 ; i < count ; i++) {
		if (fileinfo.IsSupportedType(fCachedTypesList->ItemAt(i)->String()))
			return true;
	}

	return false;
}


void
TBarView::SetDragOverride(bool on)
{
	fRefsRcvdOnly = on;
}


bool
TBarView::DragOverride()
{
	return fRefsRcvdOnly;
}


status_t
TBarView::SendDragMessage(const char* signature, entry_ref* ref)
{
	status_t err = B_ERROR;
	if (fDragMessage != NULL) {
		if (fRefsRcvdOnly) {
			// current message sent to apps is only B_REFS_RECEIVED
			fDragMessage->what = B_REFS_RECEIVED;
		}

		BRoster roster;
		if (signature != NULL && *signature != '\0'
			&& roster.IsRunning(signature)) {
			BMessenger messenger(signature);
			// drag message is still owned by DB, copy is sent
			// can toss it after send
			err = messenger.SendMessage(fDragMessage);
		} else if (ref != NULL) {
			FSLaunchItem((const entry_ref*)ref, (const BMessage*)fDragMessage,
				true, true);
		} else if (signature != NULL && *signature != '\0')
			roster.Launch(signature, fDragMessage);
	}

	return err;
}


bool
TBarView::InvokeItem(const char* signature)
{
	// sent from TeamMenuItem
	if (Dragging() && AppCanHandleTypes(signature)) {
		SendDragMessage(signature);
		// invoking okay to toss memory
		DragStop(true);
		return true;
	}

	return false;
}


void
TBarView::HandleDeskbarMenu(BMessage* messagewithdestination)
{
	if (!Dragging())
		return;

	// in mini-mode
	if (fVertical && fState != kExpandoState) {
		// if drop is in the team menu, bail
		if (fBarMenuBar->CountItems() >= 2) {
			uint32 buttons;
			BPoint location;
			GetMouse(&location, &buttons);
			if (fBarMenuBar->ItemAt(1)->Frame().Contains(location))
				return;
		}
	}

	if (messagewithdestination) {
		entry_ref ref;
		if (messagewithdestination->FindRef("refs", &ref) == B_OK) {
			BEntry entry(&ref, true);
			if (entry.IsDirectory()) {
				// if the ref received (should only be 1) is a directory
				// then add the drag refs to the directory
				AddRefsToDeskbarMenu(DragMessage(), &ref);
			} else
				SendDragMessage(NULL, &ref);
		}
	} else {
		// adds drag refs to top level in deskbar menu
		AddRefsToDeskbarMenu(DragMessage(), NULL);
	}

	// clean up drag message and types list
	DragStop(true);
}


//	#pragma mark - Add-ons


// shelf is ignored for now,
// it exists in anticipation of having other 'shelves' for
// storing items

status_t
TBarView::ItemInfo(int32 id, const char** name, DeskbarShelf* shelf)
{
	*shelf = B_DESKBAR_TRAY;
	return fReplicantTray->ItemInfo(id, name);
}


status_t
TBarView::ItemInfo(const char* name, int32* id, DeskbarShelf* shelf)
{
	*shelf = B_DESKBAR_TRAY;
	return fReplicantTray->ItemInfo(name, id);
}


bool
TBarView::ItemExists(int32 id, DeskbarShelf)
{
	return fReplicantTray->IconExists(id);
}


bool
TBarView::ItemExists(const char* name, DeskbarShelf)
{
	return fReplicantTray->IconExists(name);
}


int32
TBarView::CountItems(DeskbarShelf)
{
	return fReplicantTray->ReplicantCount();
}


BSize
TBarView::MaxItemSize(DeskbarShelf shelf)
{
	return BSize(fReplicantTray->MaxReplicantWidth(),
		fReplicantTray->MaxReplicantHeight());
}


status_t
TBarView::AddItem(BMessage* item, DeskbarShelf, int32* id)
{
	return fReplicantTray->AddIcon(item, id);
}


status_t
TBarView::AddItem(BEntry* entry, DeskbarShelf, int32* id)
{
	return fReplicantTray->LoadAddOn(entry, id);
}


void
TBarView::RemoveItem(int32 id)
{
	fReplicantTray->RemoveIcon(id);
}


void
TBarView::RemoveItem(const char* name, DeskbarShelf)
{
	fReplicantTray->RemoveIcon(name);
}


BRect
TBarView::OffsetIconFrame(BRect rect) const
{
	BRect frame(Frame());

	frame.left += fDragRegion->Frame().left + fReplicantTray->Frame().left
		+ rect.left;
	frame.top += fDragRegion->Frame().top + fReplicantTray->Frame().top
		+ rect.top;

	frame.right = frame.left + rect.Width();
	frame.bottom = frame.top + rect.Height();

	return frame;
}


BRect
TBarView::IconFrame(int32 id) const
{
	return OffsetIconFrame(fReplicantTray->IconFrame(id));
}


BRect
TBarView::IconFrame(const char* name) const
{
	return OffsetIconFrame(fReplicantTray->IconFrame(name));
}
