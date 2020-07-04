/*
 * Copyright 2001-2015, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (burton666@libero.it)
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include <MenuBar.h>

#include <math.h>

#include <Application.h>
#include <Autolock.h>
#include <ControlLook.h>
#include <LayoutUtils.h>
#include <MenuItem.h>
#include <Window.h>

#include <AppMisc.h>
#include <binary_compatibility/Interface.h>
#include <MenuPrivate.h>
#include <TokenSpace.h>
#include <InterfaceDefs.h>

#include "BMCPrivate.h"


using BPrivate::gDefaultTokens;


struct menubar_data {
	BMenuBar*	menuBar;
	int32		menuIndex;

	bool		sticky;
	bool		showMenu;

	bool		useRect;
	BRect		rect;
};


BMenuBar::BMenuBar(BRect frame, const char* name, uint32 resizingMode,
		menu_layout layout, bool resizeToFit)
	:
	BMenu(frame, name, resizingMode, B_WILL_DRAW | B_FRAME_EVENTS
		| B_FULL_UPDATE_ON_RESIZE, layout, resizeToFit),
	fBorder(B_BORDER_FRAME),
	fTrackingPID(-1),
	fPrevFocusToken(-1),
	fMenuSem(-1),
	fLastBounds(NULL),
	fTracking(false)
{
	_InitData(layout);
}


BMenuBar::BMenuBar(const char* name, menu_layout layout, uint32 flags)
	:
	BMenu(BRect(), name, B_FOLLOW_NONE,
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


BMenuBar::BMenuBar(BMessage* archive)
	:
	BMenu(archive),
	fBorder(B_BORDER_FRAME),
	fTrackingPID(-1),
	fPrevFocusToken(-1),
	fMenuSem(-1),
	fLastBounds(NULL),
	fTracking(false)
{
	int32 border;

	if (archive->FindInt32("_border", &border) == B_OK)
		SetBorder((menu_bar_border)border);

	menu_layout layout = B_ITEMS_IN_COLUMN;
	archive->FindInt32("_layout", (int32*)&layout);

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


BArchivable*
BMenuBar::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "BMenuBar"))
		return new BMenuBar(data);

	return NULL;
}


status_t
BMenuBar::Archive(BMessage* data, bool deep) const
{
	status_t err = BMenu::Archive(data, deep);

	if (err < B_OK)
		return err;

	if (Border() != B_BORDER_FRAME)
		err = data->AddInt32("_border", Border());

	return err;
}


// #pragma mark -


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
BMenuBar::AllAttached()
{
	BMenu::AllAttached();
}


void
BMenuBar::AllDetached()
{
	BMenu::AllDetached();
}


void
BMenuBar::WindowActivated(bool state)
{
	BView::WindowActivated(state);
}


void
BMenuBar::MakeFocus(bool state)
{
	BMenu::MakeFocus(state);
}


// #pragma mark -


void
BMenuBar::ResizeToPreferred()
{
	BMenu::ResizeToPreferred();
}


void
BMenuBar::GetPreferredSize(float* width, float* height)
{
	BMenu::GetPreferredSize(width, height);
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


// #pragma mark -


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


void
BMenuBar::Draw(BRect updateRect)
{
	if (_RelayoutIfNeeded()) {
		Invalidate();
		return;
	}

	BRect rect(Bounds());
	rgb_color base = LowColor();
	uint32 flags = 0;

	be_control_look->DrawBorder(this, rect, updateRect, base,
		B_PLAIN_BORDER, flags, BControlLook::B_BOTTOM_BORDER);

	be_control_look->DrawMenuBarBackground(this, rect, updateRect, base,
		0, fBorders);

	DrawItems(updateRect);
}


// #pragma mark -


void
BMenuBar::MessageReceived(BMessage* message)
{
	BMenu::MessageReceived(message);
}


void
BMenuBar::MouseDown(BPoint where)
{
	if (fTracking)
		return;

	uint32 buttons;
	GetMouse(&where, &buttons);

	BWindow* window = Window();
	if (!window->IsActive() || !window->IsFront()) {
		if ((mouse_mode() == B_FOCUS_FOLLOWS_MOUSE)
			|| ((mouse_mode() == B_CLICK_TO_FOCUS_MOUSE)
				&& ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0))) {
			// right-click to bring-to-front and send-to-back
			// (might cause some regressions in FFM)
			window->Activate();
			window->UpdateIfNeeded();
		}
	}

	StartMenuBar(-1, false, false);
}


void
BMenuBar::MouseUp(BPoint where)
{
	BView::MouseUp(where);
}


// #pragma mark -


BHandler*
BMenuBar::ResolveSpecifier(BMessage* msg, int32 index, BMessage* specifier,
	int32 form, const char* property)
{
	return BMenu::ResolveSpecifier(msg, index, specifier, form, property);
}


status_t
BMenuBar::GetSupportedSuites(BMessage* data)
{
	return BMenu::GetSupportedSuites(data);
}


// #pragma mark -


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
BMenuBar::SetBorders(uint32 borders)
{
	fBorders = borders;
}


uint32
BMenuBar::Borders() const
{
	return fBorders;
}


// #pragma mark -


status_t
BMenuBar::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= BMenuBar::MinSize();
			return B_OK;

		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= BMenuBar::MaxSize();
			return B_OK;

		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= BMenuBar::PreferredSize();
			return B_OK;

		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= BMenuBar::LayoutAlignment();
			return B_OK;

		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= BMenuBar::HasHeightForWidth();
			return B_OK;

		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			BMenuBar::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
		}

		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BMenuBar::SetLayout(data->layout);
			return B_OK;
		}

		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			BMenuBar::LayoutInvalidated(data->descendants);
			return B_OK;
		}

		case PERFORM_CODE_DO_LAYOUT:
		{
			BMenuBar::DoLayout();
			return B_OK;
		}
	}

	return BMenu::Perform(code, _data);
}


// #pragma mark -


void BMenuBar::_ReservedMenuBar1() {}
void BMenuBar::_ReservedMenuBar2() {}
void BMenuBar::_ReservedMenuBar3() {}
void BMenuBar::_ReservedMenuBar4() {}


BMenuBar&
BMenuBar::operator=(const BMenuBar &)
{
	return *this;
}


// #pragma mark -


void
BMenuBar::StartMenuBar(int32 menuIndex, bool sticky, bool showMenu,
	BRect* specialRect)
{
	if (fTracking)
		return;

	BWindow* window = Window();
	if (window == NULL)
		debugger("MenuBar must be added to a window before it can be used.");

	BAutolock lock(window);
	if (!lock.IsLocked())
		return;

	fPrevFocusToken = -1;
	fTracking = true;

	// We are called from the window's thread,
	// so let's call MenusBeginning() directly
	window->MenusBeginning();

	fMenuSem = create_sem(0, "window close sem");
	_set_menu_sem_(window, fMenuSem);

	fTrackingPID = spawn_thread(_TrackTask, "menu_tracking",
		B_DISPLAY_PRIORITY, NULL);
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


/*static*/ int32
BMenuBar::_TrackTask(void* arg)
{
	menubar_data data;
	thread_id id;
	receive_data(&id, &data, sizeof(data));

	BMenuBar* menuBar = data.menuBar;
	if (data.useRect)
		menuBar->fExtraRect = &data.rect;
	menuBar->_SetStickyMode(data.sticky);

	int32 action;
	menuBar->_Track(&action, data.menuIndex, data.showMenu);

	menuBar->fTracking = false;
	menuBar->fExtraRect = NULL;

	// We aren't the BWindow thread, so don't call MenusEnded() directly
	BWindow* window = menuBar->Window();
	window->PostMessage(_MENUS_DONE_);

	_set_menu_sem_(window, B_BAD_SEM_ID);
	delete_sem(menuBar->fMenuSem);
	menuBar->fMenuSem = B_BAD_SEM_ID;

	return 0;
}


BMenuItem*
BMenuBar::_Track(int32* action, int32 startIndex, bool showMenu)
{
	// TODO: Cleanup, merge some "if" blocks if possible
	BMenuItem* item = NULL;
	fState = MENU_STATE_TRACKING;
	fChosenItem = NULL;
		// we will use this for keyboard selection

	BPoint where;
	uint32 buttons;
	if (LockLooper()) {
		if (startIndex != -1) {
			be_app->ObscureCursor();
			_SelectItem(ItemAt(startIndex), true, false);
		}
		GetMouse(&where, &buttons);
		UnlockLooper();
	}

	while (fState != MENU_STATE_CLOSED) {
		bigtime_t snoozeAmount = 40000;
		if (!LockLooper())
			break;

		item = dynamic_cast<_BMCMenuBar_*>(this) != NULL ? ItemAt(0)
			: _HitTestItems(where, B_ORIGIN);

		if (_OverSubmenu(fSelected, ConvertToScreen(where))
			|| fState == MENU_STATE_KEY_TO_SUBMENU) {
			// call _Track() from the selected sub-menu when the mouse cursor
			// is over its window
			BMenu* submenu = fSelected->Submenu();
			UnlockLooper();
			snoozeAmount = 30000;
			submenu->_SetStickyMode(_IsStickyMode());
			int localAction;
			fChosenItem = submenu->_Track(&localAction);

			// The mouse could have meen moved since the last time we
			// checked its position, or buttons might have been pressed.
			// Unfortunately our child menus don't tell
			// us the new position.
			// TODO: Maybe have a shared struct between all menus
			// where to store the current mouse position ?
			// (Or just use the BView mouse hooks)
			BPoint newWhere;
			if (LockLooper()) {
				GetMouse(&newWhere, &buttons);
				UnlockLooper();
			}

			// Needed to make BMenuField child menus "sticky"
			// (see ticket #953)
			if (localAction == MENU_STATE_CLOSED) {
				if (fExtraRect != NULL && fExtraRect->Contains(where)
					&& point_distance(newWhere, where) < 9) {
					// 9 = 3 pixels ^ 2 (since point_distance() returns the
					// square of the distance)
					_SetStickyMode(true);
					fExtraRect = NULL;
				} else
					fState = MENU_STATE_CLOSED;
			}
			if (!LockLooper())
				break;
		} else if (item != NULL) {
			if (item->Submenu() != NULL && item != fSelected) {
				if (item->Submenu()->Window() == NULL) {
					// open the menu if it's not opened yet
					_SelectItem(item);
				} else {
					// Menu was already opened, close it and bail
					_SelectItem(NULL);
					fState = MENU_STATE_CLOSED;
					fChosenItem = NULL;
				}
			} else {
				// No submenu, just select the item
				_SelectItem(item);
			}
		} else if (item == NULL && fSelected != NULL
			&& !_IsStickyMode() && Bounds().Contains(where)) {
			_SelectItem(NULL);
			fState = MENU_STATE_TRACKING;
		}

		UnlockLooper();

		if (fState != MENU_STATE_CLOSED) {
			BPoint newWhere = where;
			uint32 newButtons = buttons;

			do {
				// If user doesn't move the mouse or change buttons loop
				// here so that we don't interfere with keyboard menu
				// navigation
				snooze(snoozeAmount);
				if (!LockLooper())
					break;

				GetMouse(&newWhere, &newButtons);
				UnlockLooper();
			} while (newWhere == where && newButtons == buttons
				&& fState == MENU_STATE_TRACKING);

			if (newButtons != 0 && _IsStickyMode()) {
				if (item == NULL || (item->Submenu() != NULL
						&& item->Submenu()->Window() != NULL)) {
					// clicked outside the menu bar or on item with already
					// open sub menu
					fState = MENU_STATE_CLOSED;
				} else
					_SetStickyMode(false);
			} else if (newButtons == 0 && !_IsStickyMode()) {
				if ((fSelected != NULL && fSelected->Submenu() == NULL)
					|| item == NULL) {
					// clicked on an item without a submenu or clicked and
					// released the mouse button outside the menu bar
					fChosenItem = fSelected;
					fState = MENU_STATE_CLOSED;
				} else
					_SetStickyMode(true);
			}
			where = newWhere;
			buttons = newButtons;
		}
	}

	if (LockLooper()) {
		if (fSelected != NULL)
			_SelectItem(NULL);

		if (fChosenItem != NULL)
			fChosenItem->Invoke();

		_RestoreFocus();
		UnlockLooper();
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

	BWindow* window = Window();
	if (window != NULL && window->Lock()) {
		BView* focusView = window->CurrentFocus();
		if (focusView != NULL && focusView != this)
			fPrevFocusToken = _get_object_token_(focusView);
		MakeFocus();
		window->Unlock();
	}
}


void
BMenuBar::_RestoreFocus()
{
	BWindow* window = Window();
	if (window != NULL && window->Lock()) {
		BHandler* handler = NULL;
		if (fPrevFocusToken != -1
			&& gDefaultTokens.GetToken(fPrevFocusToken, B_HANDLER_TOKEN,
				(void**)&handler) == B_OK) {
			BView* view = dynamic_cast<BView*>(handler);
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
	const float fontSize = be_plain_font->Size();
	float lr = fontSize * 2.0f / 3.0f, tb = fontSize / 6.0f;
	SetItemMargins(lr, tb, lr, tb);

	fBorders = BControlLook::B_ALL_BORDERS;
	fLastBounds = new BRect(Bounds());
	_SetIgnoreHidden(true);
	SetLowUIColor(B_MENU_BACKGROUND_COLOR);
	SetViewColor(B_TRANSPARENT_COLOR);
}
