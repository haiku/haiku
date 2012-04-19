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

#include <Debug.h>

#include "BarView.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AppFileInfo.h>
#include <Bitmap.h>
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
#include "ResourceSet.h"
#include "StatusView.h"
#include "TeamMenuItem.h"


const int32 kDefaultRecentDocCount = 10;
const int32 kDefaultRecentFolderCount = 10;
const int32 kDefaultRecentAppCount = 10;

const int32 kMenuTrackMargin = 20;


TBarView::TBarView(BRect frame, bool vertical, bool left, bool top,
		uint32 state, float)
	: BView(frame, "BarView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
	fBarMenuBar(NULL),
	fExpando(NULL),
	fTrayLocation(1),
	fVertical(vertical),
	fTop(top),
	fLeft(left),
	fState(static_cast<int32>(state)),
	fRefsRcvdOnly(true),
	fDragMessage(NULL),
	fCachedTypesList(NULL),
	fMaxRecentDocs(kDefaultRecentDocCount),
	fMaxRecentApps(kDefaultRecentAppCount),
	fLastDragItem(NULL)
{
	fReplicantTray = new TReplicantTray(this, fVertical);
	fDragRegion = new TDragRegion(this, fReplicantTray);
	fDragRegion->AddChild(fReplicantTray);
	if (fTrayLocation != 0)
		AddChild(fDragRegion);
}


TBarView::~TBarView()
{
	delete fDragMessage;
	delete fCachedTypesList;

	RemoveExpandedItems();
}


void
TBarView::AttachedToWindow()
{
	BView::AttachedToWindow();

	SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));
	SetFont(be_plain_font);

	UpdateEventMask();
	UpdatePlacement();

	fTrackingHookData.fTrackingHook = MenuTrackingHook;
	fTrackingHookData.fTarget = BMessenger(this);
	fTrackingHookData.fDragMessage = new BMessage(B_REFS_RECEIVED);
}


void
TBarView::DetachedFromWindow()
{
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
		BRect frame(fExpando->Frame());
		StrokeLine(BPoint(frame.left, frame.top - 1),
			BPoint(frame.right, frame.top -1));
	}
}


void
TBarView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_LOCALE_CHANGED:
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
			AddItem(new BMessage(*message), B_DESKBAR_TRAY, &id);
			break;
		}

		default:
			BView::MessageReceived(message);
	}
}


void
TBarView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	if (Window() == NULL || EventMask() == 0)
		return;

	desk_settings* settings = ((TBarApp*)be_app)->Settings();
	bool alwaysOnTop = settings->alwaysOnTop;
	bool autoRaise = settings->autoRaise;
	bool autoHide = settings->autoHide;

	if (!autoRaise && !autoHide)
		return;

	if (DragRegion()->IsDragging())
		return;

	bool isTopMost = Window()->Feel() == B_FLOATING_ALL_WINDOW_FEEL;

	// Auto-Raise
	where = ConvertToScreen(where);
	BRect screenFrame = (BScreen(Window())).Frame();
	if ((where.x == screenFrame.left || where.x == screenFrame.right
			|| where.y == screenFrame.top || where.y == screenFrame.bottom)
		&& Window()->Frame().Contains(where)) {
		// cursor is on a screen edge within the window frame

		if (!alwaysOnTop && autoRaise && !isTopMost)
			RaiseDeskbar(true);

		if (autoHide && IsHidden())
			HideDeskbar(false);
	} else {
		// cursor is not on screen edge
		BRect preventHideArea = Window()->Frame().InsetByCopy(
			-kMaxPreventHidingDist, -kMaxPreventHidingDist);

		if (preventHideArea.Contains(where))
			return;

		// cursor to bar distance above threshold
		if (!alwaysOnTop && autoRaise && isTopMost)
			RaiseDeskbar(false);

		if (autoHide && !IsHidden())
			HideDeskbar(true);
	}
}


void
TBarView::MouseDown(BPoint where)
{
	where = ConvertToScreen(where);

	if (Window()->Frame().Contains(where)) {
		Window()->Activate();

		if ((modifiers() & (B_CONTROL_KEY | B_COMMAND_KEY | B_OPTION_KEY
					| B_SHIFT_KEY)) == (B_CONTROL_KEY | B_COMMAND_KEY)) {
			// The window key was pressed - enter dragging code
			DragRegion()->MouseDown(
				DragRegion()->DragRegion().LeftTop());
			return;
		}
	} else {
		// hide deskbar if required
		desk_settings* settings = ((TBarApp*)be_app)->Settings();
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
	// top or bottom, full
	if (!fVertical && fBarMenuBar) {
		fBarMenuBar->RemoveSelf();
		delete fBarMenuBar;
		fBarMenuBar = NULL;
	}

	// top or bottom expando mode has Be menu built in for tracking
	// only for vertical mini or expanded
	// mini mode will have team menu added as part of BarMenuBar
	if (fVertical && !fBarMenuBar) {
		// create the Be menu
		BRect mbarFrame(Bounds());
		mbarFrame.bottom = mbarFrame.top + kMenuBarHeight;
		fBarMenuBar = new TBarMenuBar(this, mbarFrame, "BarMenuBar");
		AddChild(fBarMenuBar);
	}

	// if there isn't a bemenu at this point,
	// DB should be in top/bottom mode, else error
	if (!fBarMenuBar)
		return;

	float width = sMinimumWindowWidth;
	BPoint loc(B_ORIGIN);
	BRect menuFrame(fBarMenuBar->Frame());
	if (fState == kFullState) {
		fBarMenuBar->RemoveTeamMenu();
		// TODO: Magic constants need explanation
		width = 8 + 16 + 8;
		loc = Bounds().LeftTop();
	} else if (fState == kExpandoState) {
		// shows apps below tray
		fBarMenuBar->RemoveTeamMenu();
		if (fVertical)
			width += 1;
		else
			width = floorf(width) / 2;
		loc = Bounds().LeftTop();
	} else {
		// mini mode, DeskbarMenu next to team menu
		fBarMenuBar->AddTeamMenu();
	}

	fBarMenuBar->SmartResize(width, menuFrame.Height());
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

		if (fVertical) {
			statusLoc.y = fBarMenuBar->Frame().bottom + 1;
			statusLoc.x = 0;
			if (fLeft && fVertical)
				fReplicantTray->MoveTo(5, 2);
			else
				fReplicantTray->MoveTo(2, 2);
		} else {
			BRect screenFrame = (BScreen(Window())).Frame();
			statusLoc.x = screenFrame.right - fDragRegion->Bounds().Width();
			statusLoc.y = -1;
		}

		fDragRegion->MoveTo(statusLoc);
	}
}


void
TBarView::PlaceApplicationBar()
{
	if (fExpando != NULL) {
		SaveExpandedItems();
		fExpando->RemoveSelf();
		delete fExpando;
		fExpando = NULL;
	}

	BRect screenFrame = (BScreen(Window())).Frame();
	if (fState == kMiniState) {
		SizeWindow(screenFrame);
		PositionWindow(screenFrame);
		Window()->UpdateIfNeeded();
		Invalidate();
		return;
	}

	BRect expandoFrame(0, 0, 0, 0);
	if (fVertical) {
		// top left/right
		if (fTrayLocation != 0)
			expandoFrame.top = fDragRegion->Frame().bottom + 1;
		else
			expandoFrame.top = fBarMenuBar->Frame().bottom + 1;

		expandoFrame.bottom = expandoFrame.top + 1;
		if (fState == kFullState)
			expandoFrame.right = fBarMenuBar->Frame().Width();
		else
			expandoFrame.right = sMinimumWindowWidth;
	} else {
		// top or bottom
		expandoFrame.top = 0;
		int32 iconSize = static_cast<TBarApp*>(be_app)->IconSize();
		expandoFrame.bottom = iconSize + 4;
		if (fTrayLocation != 0)
			expandoFrame.right = fDragRegion->Frame().left - 1;
		else
			expandoFrame.right = screenFrame.Width();
	}

	bool hideLabels = ((TBarApp*)be_app)->Settings()->hideLabels;

	fExpando = new TExpandoMenuBar(this, expandoFrame, "ExpandoMenuBar",
		fVertical, !hideLabels && fState != kFullState);
	AddChild(fExpando);

	if (fVertical)
		ExpandItems();

	SizeWindow(screenFrame);
	PositionWindow(screenFrame);
	Window()->UpdateIfNeeded();
	Invalidate();
}


void
TBarView::GetPreferredWindowSize(BRect screenFrame, float* width, float* height)
{
	float windowHeight = 0;
	float windowWidth = sMinimumWindowWidth;
	bool setToHiddenSize = ((TBarApp*)be_app)->Settings()->autoHide
		&& IsHidden() && !DragRegion()->IsDragging();
	int32 iconSize = static_cast<TBarApp*>(be_app)->IconSize();

	if (setToHiddenSize) {
		windowHeight = kHiddenDimension;

		if (fState == kExpandoState && !fVertical) {
			// top or bottom, full
			fExpando->CheckItemSizes(0);
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
				windowHeight = fExpando->Frame().bottom;
			} else {
				// top or bottom, full
				fExpando->CheckItemSizes(0);
				windowHeight = iconSize + 4;
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
	float windowWidth, windowHeight;
	GetPreferredWindowSize(screenFrame, &windowWidth, &windowHeight);
	Window()->ResizeTo(windowWidth, windowHeight);
	if (fExpando)
		fExpando->CheckForSizeOverrun();
}


void
TBarView::PositionWindow(BRect screenFrame)
{
	float windowWidth, windowHeight;
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
TBarView::SaveSettings()
{
	desk_settings* settings = ((TBarApp*)be_app)->Settings();

	settings->vertical = fVertical;
	settings->left = fLeft;
	settings->top = fTop;
	settings->state = (uint32)fState;
	settings->width = 0;

	fReplicantTray->SaveTimeSettings();
}


void
TBarView::UpdateEventMask()
{
	SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);

#if 0
	desk_settings* settings = ((TBarApp*)be_app)->Settings();
	if (settings->autoRaise || settings->autoHide)
		SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	else
		SetEventMask(0);
#endif
}


void
TBarView::UpdatePlacement()
{
	ChangeState(fState, fVertical, fLeft, fTop);
}


void
TBarView::ChangeState(int32 state, bool vertical, bool left, bool top)
{
	bool vertSwap = (fVertical != vertical);
	bool leftSwap = (fLeft != left);
	bool stateChanged = (fState != state);

	fState = state;
	fVertical = vertical;
	fLeft = left;
	fTop = top;

	// Send a message to the preferences window to let it know to enable
	// or disable preference items
	if (stateChanged || vertSwap)
		be_app->PostMessage(kStateChanged);

	PlaceDeskbarMenu();
	PlaceTray(vertSwap, leftSwap);
	PlaceApplicationBar();
}


void
TBarView::SaveExpandedItems()
{
	if (fExpando == NULL || fExpando->CountItems() <= 0)
		return;

	// Get a list of the signatures of expanded apps. Can't use
	// team_id because there can be more than one team per application
	for (int32 i = 0; i < fExpando->CountItems(); i++) {
		TTeamMenuItem* teamItem
			= dynamic_cast<TTeamMenuItem*>(fExpando->ItemAt(i));

		if (teamItem != NULL && teamItem->IsExpanded())
			AddExpandedItem(teamItem->Signature());
	}
}


void
TBarView::RemoveExpandedItems()
{
	while (!fExpandedItems.IsEmpty())
		delete static_cast<BString*>(fExpandedItems.RemoveItem((int32)0));
	fExpandedItems.MakeEmpty();
}


void
TBarView::ExpandItems()
{
	if (fExpando == NULL || !fVertical || fState != kExpandoState
		|| !static_cast<TBarApp*>(be_app)->Settings()->superExpando
		|| fExpandedItems.CountItems() <= 0)
		return;

	// Start at the 'bottom' of the list working up.
	// Prevents being thrown off by expanding items.
	for (int32 i = fExpando->CountItems() - 1; i >= 0; i--) {
		TTeamMenuItem* teamItem
			= dynamic_cast<TTeamMenuItem*>(fExpando->ItemAt(i));

		if (teamItem != NULL) {
			// Start at the 'bottom' of the fExpandedItems list working up
			// matching the order of the fExpando list in the outer loop.
			for (int32 j = fExpandedItems.CountItems() - 1; j >= 0; j--) {
				BString* itemSig =
					static_cast<BString*>(fExpandedItems.ItemAt(j));

				if (itemSig->Compare(teamItem->Signature()) == 0) {
					// Found it, expand the item and delete signature from
					// the list so that we don't consider it for later items.
					teamItem->ToggleExpandState(false);
					fExpandedItems.RemoveItem(j);
					delete itemSig;
					break;
				}
			}
		}
	}

	// Clean up the expanded items list
	RemoveExpandedItems();

	fExpando->SizeWindow();
}


void
TBarView::AddExpandedItem(const char* signature)
{
	bool shouldAdd = true;

	for (int32 i = 0; i < fExpandedItems.CountItems(); i++) {
		BString *itemSig = static_cast<BString*>(fExpandedItems.ItemAt(i));
		if (itemSig->Compare(signature) == 0) {
			// already in the list, don't add the signature
			shouldAdd = false;
			break;
		}
	}

	if (shouldAdd)
		fExpandedItems.AddItem(static_cast<void*>(new BString(signature)));
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

	if (fExpando && fExpando->Frame().Contains(loc)) {
		ConvertToScreen(&loc);
		BPoint expandoLocation = fExpando->ConvertFromScreen(loc);
		TTeamMenuItem* item = fExpando->TeamItemAtPoint(expandoLocation);

		if (fLastDragItem)
			init_tracking_hook(fLastDragItem, NULL, NULL);

		if (item) {
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

	if (fExpando) {
		if (fLastDragItem) {
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

	if (strcmp(signature, kTrackerSignature) == 0) {
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
	if (fDragMessage) {
		if (fRefsRcvdOnly) {
			// current message sent to apps is only B_REFS_RECEIVED
			fDragMessage->what = B_REFS_RECEIVED;
		}

		BRoster roster;
		if (signature && strlen(signature) > 0 && roster.IsRunning(signature)) {
			BMessenger mess(signature);
			// drag message is still owned by DB, copy is sent
			// can toss it after send
			err = mess.SendMessage(fDragMessage);
		} else if (ref) {
			FSLaunchItem((const entry_ref*)ref, (const BMessage*)fDragMessage,
				true, true);
		} else if (signature && strlen(signature) > 0) {
			roster.Launch(signature, fDragMessage);
		}
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
	return fReplicantTray->IconCount();
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

