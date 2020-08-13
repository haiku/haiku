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
const float kMinTeamItemHeight = 20.0f;
const float kVPad = 2.0f;
const float kIconPadding = 8.0f;
const float kScrollerDimension = 12.0f;

const uint32 kUpdateOrientation = 'UpOr';


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
	fBarWindow(NULL),
	fInlineScrollView(NULL),
	fBarMenuBar(NULL),
	fExpandoMenuBar(NULL),
	fTrayLocation(1),
	fIsRaised(false),
	fMouseDownOutside(false),
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
	fMouseFilter(NULL),
	fTabHeight(kMenuBarHeight)
{
	// get window tab height
	BWindow* tmpWindow = new(std::nothrow) BWindow(BRect(), NULL,
		B_TITLED_WINDOW, 0);
	if (tmpWindow != NULL) {
		BMessage settings;
		if (tmpWindow->GetDecoratorSettings(&settings) == B_OK) {
			BRect tabRect;
			if (settings.FindRect("tab frame", &tabRect) == B_OK)
				fTabHeight = tabRect.Height();
		}
		delete tmpWindow;
	}

	// determine the initial Be menu size
	// (will be updated later)
	BRect menuFrame(frame);
	if (fVertical)
		menuFrame.bottom = menuFrame.top + fTabHeight - 1;
	else
		menuFrame.bottom = menuFrame.top + TeamMenuItemHeight();

	// create and add the Be menu
	fBarMenuBar = new TBarMenuBar(menuFrame, "BarMenuBar", this);
	AddChild(fBarMenuBar);

	// create the status tray
	fReplicantTray = new TReplicantTray(this);

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

	// create and add the expando menu bar
	fExpandoMenuBar = new TExpandoMenuBar(
		fVertical ? B_ITEMS_IN_COLUMN : B_ITEMS_IN_ROW, this);
	fInlineScrollView = new TInlineScrollView(fExpandoMenuBar,
		fVertical ? B_VERTICAL : B_HORIZONTAL);
	AddChild(fInlineScrollView);

	// hide the expando menu bar in mini-mode
	if (state == kMiniState)
		fInlineScrollView->Hide();

	// if auto-hide is on and we're not already hidden, hide ourself
	if (fBarApp->Settings()->autoHide && !IsHidden())
		Hide();
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

	fBarWindow = dynamic_cast<TBarWindow*>(Window());

	SetViewUIColor(B_MENU_BACKGROUND_COLOR);
	SetFont(be_plain_font);

	fMouseFilter = new BarViewMessageFilter(this);
	Window()->AddCommonFilter(fMouseFilter);

	fTrackingHookData.fTrackingHook = MenuTrackingHook;
	fTrackingHookData.fTarget = BMessenger(this);
	fTrackingHookData.fDragMessage = new BMessage(B_REFS_RECEIVED);

	if (!fVertical)
		UpdatePlacement(); // update MenuBarHeight
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
TBarView::MouseDown(BPoint where)
{
	// exit if menu or calendar is showing
	if (fBarWindow == NULL || fBarWindow->IsShowingMenu()
		|| fReplicantTray->fTime->IsShowingCalendar()) {
		return BView::MouseDown(where);
	}

	// where is relative to status tray while mouse is over it so pull
	// the screen point out of the message instead
	BMessage* currentMessage = Window()->CurrentMessage();
	if (currentMessage == NULL)
		return BView::MouseDown(where);

	desk_settings* settings = fBarApp->Settings();
	bool alwaysOnTop = settings->alwaysOnTop;
	bool autoRaise = settings->autoRaise;
	bool autoHide = settings->autoHide;

	BPoint whereScreen = currentMessage->GetPoint("screen_where",
		ConvertToScreen(where));
	fMouseDownOutside = !Window()->Frame().Contains(whereScreen);

	if (fMouseDownOutside) {
		// lower Deskbar
		if (!alwaysOnTop && autoRaise && fIsRaised)
			RaiseDeskbar(false);

		// hide Deskbar
		if (autoHide && !IsHidden())
			HideDeskbar(true);
	} else {
		// Activate Deskbar on click only if not in auto-raise mode and not
		// in always-on-top mode. In auto-raise mode click activates through
		// foreground windows, which we don't want. We don't ever want to
		// activate Deskbar in always-on-top mode because Deskbar is
		// already on top and we don't want to change the active window.
		if (!autoRaise && !alwaysOnTop)
			Window()->Activate(true);

		if ((modifiers() & (B_CONTROL_KEY | B_COMMAND_KEY | B_OPTION_KEY
				| B_SHIFT_KEY)) == (B_CONTROL_KEY | B_COMMAND_KEY)) {
			// The window key was pressed - enter dragging code
			fDragRegion->MouseDown(fDragRegion->DragRegion().LeftTop());
			return BView::MouseDown(where);
		}
	}

	BView::MouseDown(where);
}


void
TBarView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	if (fDragRegion->IsDragging())
		return fDragRegion->MouseMoved(where, transit, dragMessage);
	else if (fResizeControl->IsResizing())
		return BView::MouseMoved(where, transit, dragMessage);

	desk_settings* settings = fBarApp->Settings();
	bool alwaysOnTop = settings->alwaysOnTop;
	bool autoRaise = settings->autoRaise;
	bool autoHide = settings->autoHide;

	// exit if both auto-raise and auto-hide are off
	if (!autoRaise && !autoHide) {
		// turn off mouse tracking
		SetEventMask(0);

		return BView::MouseMoved(where, transit, dragMessage);
	} else if (EventMask() != B_POINTER_EVENTS) {
		// track mouse outside view
		SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	}

	// exit if menu or calendar is showing
	if (fBarWindow == NULL || fBarWindow->IsShowingMenu()
		|| fReplicantTray->fTime->IsShowingCalendar()) {
		return BView::MouseMoved(where, transit, dragMessage);
	}

	// where is relative to status tray while mouse is over it so pull
	// the screen point out of the message instead
	BMessage* currentMessage = Window()->CurrentMessage();
	if (currentMessage == NULL)
		return BView::MouseMoved(where, transit, dragMessage);

	BPoint whereScreen = currentMessage->GetPoint("screen_where",
		ConvertToScreen(where));
	BRect screenFrame = (BScreen(Window())).Frame();
	bool onScreenEdge = whereScreen.x == screenFrame.left
		|| whereScreen.x == screenFrame.right
		|| whereScreen.y == screenFrame.top
		|| whereScreen.y == screenFrame.bottom;

	// Auto-Raise and Auto-Hide
	if (!Window()->Frame().Contains(whereScreen)) {
		// lower Deskbar
		if (!alwaysOnTop && autoRaise && fIsRaised && !fMouseDownOutside)
			RaiseDeskbar(false);

		// check if cursor to bar distance is below threshold
		BRect preventHideArea = Window()->Frame().InsetByCopy(
			-kMaxPreventHidingDist, -kMaxPreventHidingDist);
		if (!preventHideArea.Contains(whereScreen)
			&& autoHide && !IsHidden()) {
			// hide Deskbar
			HideDeskbar(true);
		}
	} else if (onScreenEdge) {
		// cursor is on a screen edge within the window frame

		// raise Deskbar
		if (!alwaysOnTop && autoRaise && !fIsRaised && !fMouseDownOutside)
			RaiseDeskbar(true);

		// show Deskbar
		if (autoHide && IsHidden())
			HideDeskbar(false);
	}

	BView::MouseMoved(where, transit, dragMessage);
}


void
TBarView::MouseUp(BPoint where)
{
	fMouseDownOutside = false;

	BView::MouseUp(where);
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
		height = fTabHeight;
	} else {
		// horizontal
		if (fState == kMiniState) {
			width = gMinimumWindowWidth;
			height = std::max(fTabHeight,
				kGutter + fReplicantTray->MaxReplicantHeight() + kGutter);
		} else {
			width = gMinimumWindowWidth / 2 + kIconPadding;
			height = std::max(TeamMenuItemHeight(),
				kGutter + fReplicantTray->MaxReplicantHeight() + kGutter);
		}
	}
	menuFrame.bottom = menuFrame.top + height;

	if (fBarMenuBar == NULL) {
		// create the Be menu
		fBarMenuBar = new TBarMenuBar(menuFrame, "BarMenuBar", this);
		AddChild(fBarMenuBar);
	} else
		fBarMenuBar->SmartResize(-1, -1);

	if (fState == kMiniState) {
		// vertical or horizontal mini
		fBarMenuBar->RemoveSeperatorItem();
		fBarMenuBar->AddTeamMenu();
	} else if (fVertical) {
		fBarMenuBar->RemoveSeperatorItem();
		fBarMenuBar->RemoveTeamMenu();
	} else {
		fBarMenuBar->RemoveTeamMenu();
		fBarMenuBar->AddSeparatorItem();
	}

	fBarMenuBar->SmartResize(width, height);
	fBarMenuBar->MoveTo(B_ORIGIN);
}


void
TBarView::PlaceTray(bool vertSwap, bool leftSwap)
{
	BPoint statusLoc;
	if (fTrayLocation == 0) {
		// no replicant tray mode, not used
		if (!fReplicantTray->IsHidden())
			fReplicantTray->Hide();
		return;
	} else if (fReplicantTray->IsHidden())
		fReplicantTray->Show();

	fReplicantTray->RealignReplicants();
	fDragRegion->ResizeToPreferred();
		// also resizes replicant tray

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
		// horizontal
		if (fState == kMiniState) {
			// horizontal mini
			statusLoc.x = fLeft ? fBarMenuBar->Frame().right + 1 : 0;
			statusLoc.y = 0;

			// move past dragger and top border
			// and make room for the top and bottom borders
			fReplicantTray->MoveTo(fLeft ? kDragWidth : 0, kGutter);
			fReplicantTray->ResizeBy(0, -4);
		} else {
			// move tray right and down to not cover border, resize by same
			fReplicantTray->MoveTo(2, 0);
			fReplicantTray->ResizeBy(-2, 0);
			BRect screenFrame = (BScreen(Window())).Frame();
			statusLoc.x = screenFrame.right - fDragRegion->Bounds().Width();
			statusLoc.y = 0;
		}
	}

	fDragRegion->MoveTo(statusLoc);

	// make room for top and bottom border
	fResizeControl->ResizeTo(kDragWidth, fDragRegion->Bounds().Height() - 2);

	if (fVertical) {
		// move resize control into place based on width setting
		fResizeControl->MoveTo(
			fLeft ? fBarApp->Settings()->width - kDragWidth : 0, 1);
		if (fResizeControl->IsHidden())
			fResizeControl->Show();
	} else {
		// hide resize control
		if (!fResizeControl->IsHidden())
			fResizeControl->Hide();
	}

	fDragRegion->Invalidate();
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
		if (!fVertical) {
			// move the menu bar into place after the window has been resized
			// based on replicant tray
			fBarMenuBar->MoveTo(fLeft ? 0 : fDragRegion->Bounds().right + 1,
				0);
		}
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
		expandoFrame.right = fBarApp->Settings()->width;
		expandoFrame.bottom = fState == kFullState ? screenFrame.bottom
			: Frame().bottom;
	} else {
		// top or bottom
		expandoFrame.top = 0;
		expandoFrame.bottom = TeamMenuItemHeight();
		expandoFrame.left = gMinimumWindowWidth / 2 + kIconPadding;
		expandoFrame.right = screenFrame.Width();
		if (fTrayLocation != 0 && fDragRegion != NULL)
			expandoFrame.right -= fDragRegion->Frame().Width() + 1;
	}

	fInlineScrollView->DetachScrollers();
	fInlineScrollView->MoveTo(expandoFrame.LeftTop());
	fInlineScrollView->ResizeTo(expandoFrame.Width(), fVertical
		? screenFrame.bottom - expandoFrame.top : expandoFrame.bottom);
	fExpandoMenuBar->ResizeTo(expandoFrame.Width(), expandoFrame.Height());
	fExpandoMenuBar->MoveTo(0, 0);
	fExpandoMenuBar->BuildItems();
	fExpandoMenuBar->SizeWindow(0);
}


void
TBarView::GetPreferredWindowSize(BRect screenFrame, float* width,
	float* height)
{
	float windowHeight = 0;
	float windowWidth = 0;
	bool setToHiddenSize = fBarApp->Settings()->autoHide && IsHidden()
		&& !fDragRegion->IsDragging();

	if (setToHiddenSize) {
		windowHeight = kHiddenDimension;

		if (fState == kExpandoState && !fVertical) {
			// top or bottom, full
			windowWidth = screenFrame.Width();
		} else
			windowWidth = kHiddenDimension;
	} else if (fVertical) {
		if (fState == kFullState) {
			// full state has minimum screen window height
			windowHeight = std::max(screenFrame.bottom, windowHeight);
		} else {
			// mini or expando
			if (fTrayLocation != 0)
				windowHeight = fDragRegion->Frame().bottom;
			else
				windowHeight = fBarMenuBar->Frame().bottom;

			if (fState == kExpandoState && fExpandoMenuBar != NULL) {
				// top left or right
				windowHeight += fExpandoMenuBar->Bounds().Height();
					// use Height() here, not bottom so view can be scrolled
			}
		}

		windowWidth = fBarApp->Settings()->width;
	} else {
		// horizontal
		if (fState == kMiniState) {
			// four corners horizontal
			windowHeight = fBarMenuBar->Frame().Height();
			windowWidth = fDragRegion->Frame().Width()
				+ fBarMenuBar->Frame().Width() + 1;
		} else {
			// horizontal top or bottom
			windowHeight = std::max(TeamMenuItemHeight(),
				kGutter + fReplicantTray->MaxReplicantHeight() + kGutter);
			windowWidth = screenFrame.Width();
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
	ResizeTo(windowWidth, windowHeight);
}


void
TBarView::PositionWindow(BRect screenFrame)
{
	float windowWidth;
	float windowHeight;
	GetPreferredWindowSize(screenFrame, &windowWidth, &windowHeight);

	BPoint moveLoc(0, 0);
	// right, expanded, mini, or full
	if (!fLeft && (fVertical || fState == kMiniState))
		moveLoc.x = screenFrame.right - windowWidth;

	// bottom, full
	if (!fTop)
		moveLoc.y = screenFrame.bottom - windowHeight;

	Window()->MoveTo(moveLoc);
}


void
TBarView::CheckForScrolling()
{
	if (fInlineScrollView == NULL && fExpandoMenuBar == NULL)
		return;

	if (fExpandoMenuBar->CheckForSizeOverrun())
		fInlineScrollView->AttachScrollers();
	else
		fInlineScrollView->DetachScrollers();
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

		TBarWindow* barWindow = dynamic_cast<TBarWindow*>(Window());
		if (barWindow != NULL)
			barWindow->SetSizeLimits();

		if (vertSwap && fExpandoMenuBar != NULL) {
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

	PlaceDeskbarMenu();
	PlaceTray(vertSwap, leftSwap);
	PlaceApplicationBar();
}


void
TBarView::RaiseDeskbar(bool raise)
{
	fIsRaised = raise;

	// raise or lower Deskbar without changing the active window
	if (raise) {
		Window()->SetFeel(B_FLOATING_ALL_WINDOW_FEEL);
		Window()->SetFeel(B_NORMAL_WINDOW_FEEL);
	} else
		Window()->SendBehind(Window());
}


void
TBarView::HideDeskbar(bool hide)
{
	BRect screenFrame = (BScreen(Window())).Frame();

	if (hide) {
		Hide();
		if (fBarWindow != NULL)
			fBarWindow->SetSizeLimits();

		PositionWindow(screenFrame);
		SizeWindow(screenFrame);
	} else {
		Show();
		if (fBarWindow != NULL)
			fBarWindow->SetSizeLimits();

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

	TBarView* barView = dynamic_cast<TBarView*>(data->fTarget.Target(NULL));
	if (!barView || !menu->LockLooper())
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
		if (barView->LockLooper()) {
			TExpandoMenuBar* expandoMenuBar = barView->ExpandoMenuBar();
			TBarWindow* barWindow
				= dynamic_cast<TBarWindow*>(barView->Window());
			TDeskbarMenu* deskbarMenu = barWindow->DeskbarMenu();

			if (deskbarMenu && deskbarMenu->LockLooper()) {
				deskbarMenu->ConvertFromScreen(&location);
				if (deskbarMenu->Frame().Contains(location))
					endMenu = false;

				deskbarMenu->UnlockLooper();
			}

			if (endMenu && expandoMenuBar) {
				expandoMenuBar->ConvertFromScreen(&location);
				BMenuItem* item = expandoMenuBar->TeamItemAtPoint(location);
				if (item)
					endMenu = false;
			}
			barView->UnlockLooper();
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


float
TBarView::TeamMenuItemHeight() const
{
	const int32 iconSize = fBarApp->IconSize();
	float iconSizePadded = kVPad + iconSize + kVPad;

	font_height fontHeight;
	if (fExpandoMenuBar != NULL)
		fExpandoMenuBar->GetFontHeight(&fontHeight);
	else
		GetFontHeight(&fontHeight);

	float labelHeight = fontHeight.ascent + fontHeight.descent;
	labelHeight = labelHeight < kMinTeamItemHeight ? kMinTeamItemHeight
		: ceilf(labelHeight * 1.1f);

	bool hideLabels = static_cast<TBarApp*>(be_app)->Settings()->hideLabels;
	if (hideLabels && iconSize > B_MINI_ICON) {
		// height is determined based solely on icon size
		return iconSizePadded;
	} else if (!fVertical || (fVertical && iconSize <= B_LARGE_ICON)) {
		// horizontal or vertical with label on same row as icon:
		// height based on icon size or font size, whichever is bigger
		return std::max(iconSizePadded, labelHeight);
	} else if (fVertical && iconSize > B_LARGE_ICON) {
		// vertical with label below icon: height based on icon and label
		return ceilf(iconSizePadded + labelHeight);
	} else {
		// height is determined based solely on label height
		return labelHeight;
	}
}
