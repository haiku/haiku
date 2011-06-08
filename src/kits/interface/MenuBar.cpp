/*
 * Copyright 2001-2009, Haiku Inc. All rights reserved.
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


BMenuBar::BMenuBar(BRect frame, const char* title, uint32 resizeMask,
		menu_layout layout, bool resizeToFit)
	:
	BMenu(frame, title, resizeMask, B_WILL_DRAW | B_FRAME_EVENTS, layout,
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


BMenuBar::BMenuBar(const char* title, menu_layout layout, uint32 flags)
	:
	BMenu(BRect(), title, B_FOLLOW_NONE,
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


BMenuBar::BMenuBar(BMessage* data)
	:
	BMenu(data),
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
	data->FindInt32("_layout", (int32*)&layout);

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

	if (be_control_look != NULL) {
		BRect rect(Bounds());
		rgb_color base = LowColor();
		uint32 flags = 0;

		be_control_look->DrawBorder(this, rect, updateRect, base,
			B_PLAIN_BORDER, flags, BControlLook::B_BOTTOM_BORDER);

		be_control_look->DrawMenuBarBackground(this, rect, updateRect, base);

		_DrawItems(updateRect);
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
	StrokeLine(BPoint(0.0f, bounds.bottom),
		BPoint(bounds.right, bounds.bottom));
	StrokeLine(BPoint(bounds.right, 0.0f), BPoint(bounds.right, bounds.bottom));

	SetHighColor(color);
		// revert to previous used color (cheap PushState()/PopState())

	_DrawItems(updateRect);
}


// #pragma mark -


void
BMenuBar::MessageReceived(BMessage* msg)
{
	BMenu::MessageReceived(msg);
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
		case PERFORM_CODE_INVALIDATE_LAYOUT:
		{
			perform_data_invalidate_layout* data
				= (perform_data_invalidate_layout*)_data;
			BMenuBar::InvalidateLayout(data->descendants);
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


BMenuBar &
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

	fTrackingPID = spawn_thread(_TrackTask, "menu_tracking", B_DISPLAY_PRIORITY,
		NULL);
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
	fChosenItem = NULL;

	BWindow* window = Window();
	fState = MENU_STATE_TRACKING;

	BPoint where;
	uint32 buttons;
	if (window->Lock()) {
		if (startIndex != -1) {
			be_app->ObscureCursor();
			_SelectItem(ItemAt(startIndex), true, false);
		}
		GetMouse(&where, &buttons);
		window->Unlock();
	}

	while (fState != MENU_STATE_CLOSED) {
		bigtime_t snoozeAmount = 40000;
		if (Window() == NULL || !window->Lock())
			break;

		BMenuItem* menuItem = NULL;
		if (dynamic_cast<_BMCMenuBar_*>(this))
			menuItem = ItemAt(0);
		else
			menuItem = _HitTestItems(where, B_ORIGIN);
		if (_OverSubmenu(fSelected, ConvertToScreen(where))
			|| fState == MENU_STATE_KEY_TO_SUBMENU) {
			// call _Track() from the selected sub-menu when the mouse cursor
			// is over its window
			BMenu* menu = fSelected->Submenu();
			window->Unlock();
			snoozeAmount = 30000;
			bool wasSticky = _IsStickyMode();
			menu->_SetStickyMode(wasSticky);
			int localAction;
			fChosenItem = menu->_Track(&localAction);

			// The mouse could have meen moved since the last time we
			// checked its position, or buttons might have been pressed.
			// Unfortunately our child menus don't tell
			// us the new position.
			// TODO: Maybe have a shared struct between all menus
			// where to store the current mouse position ?
			// (Or just use the BView mouse hooks)
			BPoint newWhere;
			if (window->Lock()) {
				GetMouse(&newWhere, &buttons);
				window->Unlock();
			}

			// This code is needed to make menus
			// that are children of BMenuFields "sticky" (see ticket #953)
			if (localAction == MENU_STATE_CLOSED) {
				if (fExtraRect != NULL && fExtraRect->Contains(where)
					// 9 = 3 pixels ^ 2 (since point_distance() returns the
					// square of the distance)
					&& point_distance(newWhere, where) < 9) {
					_SetStickyMode(true);
					fExtraRect = NULL;
				} else
					fState = MENU_STATE_CLOSED;
			}
			if (!window->Lock())
				break;
		} else if (menuItem != NULL) {
			if (menuItem->Submenu() != NULL && menuItem != fSelected) {
				if (menuItem->Submenu()->Window() == NULL) {
					// open the menu if it's not opened yet
					_SelectItem(menuItem);
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
		} else if (menuItem == NULL && fSelected != NULL
			&& !_IsStickyMode() && Bounds().Contains(where)) {
			_SelectItem(NULL);
			fState = MENU_STATE_TRACKING;
		}

		window->Unlock();

		if (fState != MENU_STATE_CLOSED) {
			// If user doesn't move the mouse, loop here,
			// so we don't interfere with keyboard menu navigation
			BPoint newLocation = where;
			uint32 newButtons = buttons;
			do {
				snooze(snoozeAmount);
				if (!LockLooper())
					break;
				GetMouse(&newLocation, &newButtons, true);
				UnlockLooper();
			} while (newLocation == where && newButtons == buttons
				&& fState == MENU_STATE_TRACKING);

			where = newLocation;
			buttons = newButtons;

			if (buttons != 0 && _IsStickyMode()) {
				if (menuItem == NULL
					|| (menuItem->Submenu() && menuItem->Submenu()->Window())) {
					// clicked outside menu bar or on item with already
					// open sub menu
					fState = MENU_STATE_CLOSED;
				} else
					_SetStickyMode(false);
			} else if (buttons == 0 && !_IsStickyMode()) {
				if ((fSelected != NULL && fSelected->Submenu() == NULL)
					|| menuItem == NULL) {
					fChosenItem = fSelected;
					fState = MENU_STATE_CLOSED;
				} else
					_SetStickyMode(true);
			}
		}
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

	BWindow* window = Window();
	if (window != NULL && window->Lock()) {
		BView* focus = window->CurrentFocus();
		if (focus != NULL && focus != this)
			fPrevFocusToken = _get_object_token_(focus);
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
	fLastBounds = new BRect(Bounds());
	SetItemMargins(8, 2, 8, 2);
	_SetIgnoreHidden(true);
}
