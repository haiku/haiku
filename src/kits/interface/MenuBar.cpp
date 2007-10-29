/*
 * Copyright 2001-2007, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (burton666@libero.it)
 */

#include <math.h>

#include <MenuBar.h>

#include <Application.h>
#include <Autolock.h>
#include <LayoutUtils.h>
#include <MenuItem.h>
#include <Window.h>

#include <AppMisc.h>
#include <MenuPrivate.h>
#include <TokenSpace.h>

using BPrivate::gDefaultTokens;


struct menubar_data {
	BMenuBar *menuBar;
	int32 menuIndex;
	
	bool sticky;
	bool showMenu;
	
	bool useRect;
	BRect rect;
};


BMenuBar::BMenuBar(BRect frame, const char *title, uint32 resizeMask,
		menu_layout layout, bool resizeToFit)
	: BMenu(frame, title, resizeMask, B_WILL_DRAW | B_FRAME_EVENTS, layout,
		resizeToFit),
	fBorder(B_BORDER_FRAME),
	fTrackingPID(-1),
	fPrevFocusToken(-1),
	fMenuSem(-1),
	fLastBounds(NULL),
	fTracking(false)
{
	_InitData(layout);
}


BMenuBar::BMenuBar(const char *title, menu_layout layout, uint32 flags)
	: BMenu(BRect(), title, B_FOLLOW_NONE,
		flags | B_WILL_DRAW | B_FRAME_EVENTS | B_SUPPORTS_LAYOUT,
		layout, false),
	fBorder(B_BORDER_FRAME),
	fTrackingPID(-1),
	fPrevFocusToken(-1),
	fMenuSem(-1),
	fLastBounds(NULL),
	fTracking(false)
{
	_InitData(layout);
}


BMenuBar::BMenuBar(BMessage *data)
	: BMenu(data),
	fBorder(B_BORDER_FRAME),
	fTrackingPID(-1),
	fPrevFocusToken(-1),
	fMenuSem(-1),
	fLastBounds(NULL),
	fTracking(false)
{
	int32 border;
	
	if (data->FindInt32("_border", &border) == B_OK)
		SetBorder((menu_bar_border)border);
	
	menu_layout layout = B_ITEMS_IN_COLUMN;
	data->FindInt32("_layout", (int32 *)&layout);
	
	_InitData(layout);	
}


BMenuBar::~BMenuBar()
{
	if (fTracking) {
		status_t dummy;
		wait_for_thread(fTrackingPID, &dummy);
	}
	
	delete fLastBounds;
}


BArchivable *
BMenuBar::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, "BMenuBar"))
		return new BMenuBar(data);
	
	return NULL;
}


status_t
BMenuBar::Archive(BMessage *data, bool deep) const
{
	status_t err = BMenu::Archive(data, deep);

	if (err < B_OK)
		return err;
	
	if (Border() != B_BORDER_FRAME)
		err = data->AddInt32("_border", Border());

	return err;
}


void 
BMenuBar::SetBorder(menu_bar_border border)
{
	fBorder = border;
}


menu_bar_border
BMenuBar::Border() const
{
	return fBorder;
}


void
BMenuBar::Draw(BRect updateRect)
{
	if (_RelayoutIfNeeded()) {
		Invalidate();
		return;
	}

	// TODO: implement additional border styles
	rgb_color color = HighColor();
	
	BRect bounds(Bounds());
	// Restore the background of the previously selected menuitem
	DrawBackground(bounds & updateRect);

	rgb_color noTint = LowColor();

	SetHighColor(tint_color(noTint, B_LIGHTEN_2_TINT));
	StrokeLine(BPoint(0.0f, bounds.bottom - 2.0f), BPoint(0.0f, 0.0f));
	StrokeLine(BPoint(bounds.right, 0.0f));

	SetHighColor(tint_color(noTint, B_DARKEN_1_TINT));
	StrokeLine(BPoint(1.0f, bounds.bottom - 1.0f),
		BPoint(bounds.right, bounds.bottom - 1.0f));

	SetHighColor(tint_color(noTint, B_DARKEN_2_TINT));
	StrokeLine(BPoint(0.0f, bounds.bottom), BPoint(bounds.right, bounds.bottom));
	StrokeLine(BPoint(bounds.right, 0.0f), BPoint(bounds.right, bounds.bottom));

	SetHighColor(color);
		// revert to previous used color (cheap PushState()/PopState())

	_DrawItems(updateRect);
}


void
BMenuBar::AttachedToWindow()
{
	_Install(Window());
	Window()->SetKeyMenuBar(this);

	BMenu::AttachedToWindow();

	*fLastBounds = Bounds();
}


void 
BMenuBar::DetachedFromWindow()
{
	BMenu::DetachedFromWindow();
}


void 
BMenuBar::MessageReceived(BMessage *msg)
{
	BMenu::MessageReceived(msg);
}


void 
BMenuBar::MouseDown(BPoint where)
{
	if (fTracking)
		return;

	BWindow *window = Window();
	if (!window->IsActive() || !window->IsFront()) {
		window->Activate();
		window->UpdateIfNeeded();
	}
	
	StartMenuBar(-1, false, false);
}


void 
BMenuBar::WindowActivated(bool state)
{
	BView::WindowActivated(state);
}


void 
BMenuBar::MouseUp(BPoint where)
{
	BView::MouseUp(where);
}


void 
BMenuBar::FrameMoved(BPoint newPosition)
{	
	BMenu::FrameMoved(newPosition);
}


void 
BMenuBar::FrameResized(float newWidth, float newHeight)
{
	// invalidate right border
	if (newWidth != fLastBounds->Width()) {
		BRect rect(min_c(fLastBounds->right, newWidth), 0,
			max_c(fLastBounds->right, newWidth), newHeight);
		Invalidate(rect);
	}

	// invalidate bottom border
	if (newHeight != fLastBounds->Height()) {
		BRect rect(0, min_c(fLastBounds->bottom, newHeight) - 1,
			newWidth, max_c(fLastBounds->bottom, newHeight));
		Invalidate(rect);
	}

	fLastBounds->Set(0, 0, newWidth, newHeight);

	BMenu::FrameResized(newWidth, newHeight);
}


void 
BMenuBar::Show()
{
	BView::Show();
}


void 
BMenuBar::Hide()
{
	BView::Hide();
}


BHandler *
BMenuBar::ResolveSpecifier(BMessage *msg, int32 index, BMessage *specifier, int32 form, const char *property)
{
	return BMenu::ResolveSpecifier(msg, index, specifier, form, property);
}


status_t 
BMenuBar::GetSupportedSuites(BMessage *data)
{
	return BMenu::GetSupportedSuites(data);
}


void 
BMenuBar::ResizeToPreferred()
{
	BMenu::ResizeToPreferred();
}


void 
BMenuBar::GetPreferredSize(float *width, float *height)
{
	BMenu::GetPreferredSize(width, height);
}


void 
BMenuBar::MakeFocus(bool state)
{
	BMenu::MakeFocus(state);
}


void 
BMenuBar::AllAttached()
{
	BMenu::AllAttached();
}


void 
BMenuBar::AllDetached()
{
	BMenu::AllDetached();
}


status_t 
BMenuBar::Perform(perform_code d, void *arg)
{
	return BMenu::Perform(d, arg);
}


BSize
BMenuBar::MinSize()
{
	return BMenu::MinSize();
}


BSize
BMenuBar::MaxSize()
{
	BSize size = BMenu::MaxSize();
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(),
		BSize(B_SIZE_UNLIMITED, size.height));
}


BSize
BMenuBar::PreferredSize()
{
	return BMenu::PreferredSize();
}


// #pragma mark -


void BMenuBar::_ReservedMenuBar1() {}
void BMenuBar::_ReservedMenuBar2() {}
void BMenuBar::_ReservedMenuBar3() {}
void BMenuBar::_ReservedMenuBar4() {}


BMenuBar &
BMenuBar::operator=(const BMenuBar &)
{
	return *this;
}


void 
BMenuBar::StartMenuBar(int32 menuIndex, bool sticky, bool showMenu, BRect *specialRect)
{
	if (fTracking)
		return;

	BWindow *window = Window();
	if (window == NULL) 
		debugger("MenuBar must be added to a window before it can be used.");
	
	BAutolock lock(window);	
	if (!lock.IsLocked())
		return;
	
	fPrevFocusToken = -1;
	fTracking = true;
		
	window->MenusBeginning();
	
	fMenuSem = create_sem(0, "window close sem");
	_set_menu_sem_(window, fMenuSem);
	
	fTrackingPID = spawn_thread(_TrackTask, "menu_tracking", B_DISPLAY_PRIORITY, NULL);
	if (fTrackingPID >= 0) {
		menubar_data data;	
		data.menuBar = this;
		data.menuIndex = menuIndex;
		data.sticky = sticky;
		data.showMenu = showMenu;
		data.useRect = specialRect != NULL;
		if (data.useRect)
			data.rect = *specialRect;
		
		resume_thread(fTrackingPID);	
		send_data(fTrackingPID, 0, &data, sizeof(data));
	
	} else {
		fTracking = false;
		_set_menu_sem_(window, B_NO_MORE_SEMS);
		delete_sem(fMenuSem);
	}
}


/* static */
int32
BMenuBar::_TrackTask(void *arg)
{
	menubar_data data;
	thread_id id;
	receive_data(&id, &data, sizeof(data));

	BMenuBar *menuBar = data.menuBar;
	if (data.useRect)
		menuBar->fExtraRect = &data.rect;	
	menuBar->_SetStickyMode(data.sticky);
	
	int32 action;
	menuBar->_Track(&action, data.menuIndex, data.showMenu);
	
	menuBar->fTracking = false;
	menuBar->fExtraRect = NULL;

	// Sends a _MENUS_DONE_ message to the BWindow.
	// Weird: There is a _MENUS_DONE_ message but not a 
	// _MENUS_BEGINNING_ message, in fact the MenusBeginning()
	// hook function is called directly.
	BWindow *window = menuBar->Window();
	window->PostMessage(_MENUS_DONE_);
	
	_set_menu_sem_(window, B_BAD_SEM_ID);
	delete_sem(menuBar->fMenuSem);
	menuBar->fMenuSem = B_BAD_SEM_ID;
	
	return 0;
}


// Note: since sqrt is slow, we don't use it and return the square of the distance
// TODO: Move this to some common place, could be used in BMenu too.
#define square(x) ((x) * (x))
static float
point_distance(const BPoint &pointA, const BPoint &pointB)
{
	return square(pointA.x - pointB.x) + square(pointA.y - pointB.y);
}
#undef square


BMenuItem *
BMenuBar::_Track(int32 *action, int32 startIndex, bool showMenu)
{	
	// TODO: Cleanup, merge some "if" blocks if possible
	fChosenItem = NULL;
	
	BWindow *window = Window();
	fState = MENU_STATE_TRACKING;

	if (startIndex != -1) {
		be_app->ObscureCursor();
		if (window->Lock()) {
			_SelectItem(ItemAt(startIndex), true, true);
			window->Unlock();
		}
	}

	while (true) {
		bigtime_t snoozeAmount = 40000;
		bool locked = (Window() != NULL && window->Lock());//WithTimeout(200000)
		if (!locked)
			break;

		BPoint where;
		uint32 buttons;
		GetMouse(&where, &buttons, true);

		BMenuItem *menuItem = _HitTestItems(where, B_ORIGIN);
		if (menuItem != NULL) {
			// Select item if:
			// - no previous selection
			// - nonsticky mode and different selection,
			// - clicked in sticky mode
			if (fSelected == NULL
				|| (!_IsStickyMode() && menuItem != fSelected)
				|| (buttons != 0 && _IsStickyMode())) {
				if (menuItem->Submenu() != NULL) {
					if (menuItem->Submenu()->Window() == NULL) {
						// open the menu if it's not opened yet
						_SelectItem(menuItem);
						if (_IsStickyMode())
							_SetStickyMode(false);
					} else {
						// Menu was already opened, close it and bail
						_SelectItem(NULL);
						fState = MENU_STATE_CLOSED;
						fChosenItem = NULL;
					}
				} else {
					// No submenu, just select the item
					_SelectItem(menuItem);					
				}
			}
		}

		if (_OverSubmenu(fSelected, ConvertToScreen(where))) {
			// call _Track() from the selected sub-menu when the mouse cursor
			// is over its window
			BMenu *menu = fSelected->Submenu();
			window->Unlock();
			locked = false;
			snoozeAmount = 30000;
			bool wasSticky = _IsStickyMode();
			if (wasSticky)
				menu->_SetStickyMode(true);
			int localAction;
			fChosenItem = menu->_Track(&localAction);
			if (menu->State(NULL) == MENU_STATE_TRACKING
				&& menu->_IsStickyMode())
				menu->_SetStickyMode(false);

			// check if the user started holding down a mouse button in a submenu
			if (wasSticky && !_IsStickyMode()) {
				buttons = 1;
					// buttons must have been pressed in the meantime
			}

			// This code is needed to make menus
			// that are children of BMenuFields "sticky" (see ticket #953)
			if (localAction == MENU_STATE_CLOSED) {
				// The mouse could have meen moved since the last time we
				// checked its position. Unfortunately our child menus don't tell
				// us the new position.
				// TODO: Maybe have a shared struct between all menus
				// where to store the current mouse position ? 				
				BPoint newWhere;
				uint32 newButtons;
				if (window->Lock()) {
					GetMouse(&newWhere, &newButtons);
					window->Unlock();
				}

				if (fExtraRect != NULL && fExtraRect->Contains(where)
					// 9 = 3 pixels ^ 2 (since point_distance() returns the square of the distance)					
					&& point_distance(newWhere, where) < 9) {
					_SetStickyMode(true);
					fExtraRect = NULL;				
				} else
					fState = MENU_STATE_CLOSED;
			}
		} else if (menuItem == NULL && fSelected != NULL
			&& !_IsStickyMode() && fState != MENU_STATE_TRACKING_SUBMENU) {
			_SelectItem(NULL);
			fState = MENU_STATE_TRACKING;
		}

		if (locked)
			window->Unlock();

		if (fState == MENU_STATE_CLOSED 
			|| (buttons != 0 && _IsStickyMode() && menuItem == NULL))
			break;
		else if (buttons == 0 && !_IsStickyMode()) {
			if ((fSelected != NULL && fSelected->Submenu() == NULL)
				|| menuItem == NULL) {
				fChosenItem = fSelected;
				break;
			} else
				_SetStickyMode(true);
		}

		if (snoozeAmount > 0)
			snooze(snoozeAmount);		
	}

	if (window->Lock()) {
		if (fSelected != NULL)
			_SelectItem(NULL);	

		if (fChosenItem != NULL)
			fChosenItem->Invoke();
		_RestoreFocus();
		window->Unlock();
	}

	if (_IsStickyMode())
		_SetStickyMode(false);

	_DeleteMenuWindow();

	if (action != NULL)
		*action = fState;

	return fChosenItem;
}


void
BMenuBar::_StealFocus()
{
	// We already stole the focus, don't do anything
	if (fPrevFocusToken != -1)
		return;

	BWindow *window = Window();
	if (window != NULL && window->Lock()) {
		BView *focus = window->CurrentFocus();
		if (focus != NULL && focus != this)
			fPrevFocusToken = _get_object_token_(focus);
		MakeFocus();
		window->Unlock();
	}
}


void 
BMenuBar::_RestoreFocus()
{
	BWindow *window = Window();
	if (window != NULL && window->Lock()) {
		BHandler *handler = NULL;
		if (fPrevFocusToken != -1 
			&& gDefaultTokens.GetToken(fPrevFocusToken, B_HANDLER_TOKEN, (void **)&handler) == B_OK) {
			BView *view = dynamic_cast<BView *>(handler);
			if (view != NULL && view->Window() == window)
				view->MakeFocus();		
		
		} else if (IsFocus())
			MakeFocus(false);
		
		fPrevFocusToken = -1;
		window->Unlock();
	}
}


void 
BMenuBar::_InitData(menu_layout layout)
{
	fLastBounds = new BRect(Bounds());
	SetItemMargins(8, 2, 8, 2);
	_SetIgnoreHidden(true);
}
