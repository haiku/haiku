/*
 * Copyright 2001-2006, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (burton666@libero.it)
 */

#include <Application.h>
#include <Autolock.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Window.h>

#include <AppMisc.h>
#include <MenuPrivate.h>
#include <TokenSpace.h>


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
	: BMenu(frame, title, resizeMask,
			B_WILL_DRAW | B_FRAME_EVENTS, layout, resizeToFit),
	fBorder(B_BORDER_FRAME),
	fTrackingPID(-1),
	fPrevFocusToken(-1),
	fMenuSem(-1),
	fLastBounds(NULL),
	fTracking(false)
{
	InitData(layout);
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
	
	InitData(layout);	
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
	if (RelayoutIfNeeded()) {
		Invalidate();
		return;
	}

	// TODO: implement additional border styles
	rgb_color color = HighColor();
	
	BRect bounds(Bounds());
	// Restore the background of the previously selected menuitem
	DrawBackground(bounds & updateRect);

	SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_LIGHTEN_2_TINT));
	StrokeLine(BPoint(0.0f, bounds.bottom - 2.0f), BPoint(0.0f, 0.0f));
	StrokeLine(BPoint(bounds.right, 0.0f));

	SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_1_TINT));
	StrokeLine(BPoint(1.0f, bounds.bottom - 1.0f),
		BPoint(bounds.right, bounds.bottom - 1.0f));

	SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_2_TINT));
	StrokeLine(BPoint(0.0f, bounds.bottom), BPoint(bounds.right, bounds.bottom));
	StrokeLine(BPoint(bounds.right, 0.0f), BPoint(bounds.right, bounds.bottom));

	SetHighColor(color);
		// revert to previous used color (cheap PushState()/PopState())

	DrawItems(updateRect);
}


void
BMenuBar::AttachedToWindow()
{
	Install(Window());
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
	BRect bounds(Bounds());
	BRect rect(fLastBounds->right - 2, fLastBounds->top, bounds.right, bounds.bottom);
	fLastBounds->Set(0, 0, newWidth, newHeight);
	
	Invalidate(rect);

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
	
	fTrackingPID = spawn_thread(TrackTask, "menu_tracking", B_NORMAL_PRIORITY, NULL);
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


long 
BMenuBar::TrackTask(void *arg)
{
	menubar_data data;
	thread_id id;
	
	receive_data(&id, &data, sizeof(data));
	
	BMenuBar *menuBar = data.menuBar;
	menuBar->SetStickyMode(data.sticky);
	
	int32 action;
	menuBar->Track(&action, data.menuIndex, data.showMenu);
	
	menuBar->fTracking = false;
	
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


BMenuItem *
BMenuBar::Track(int32 *action, int32 startIndex, bool showMenu)
{	
	// TODO: Cleanup, merge some "if" blocks if possible
	BMenuItem *resultItem = NULL;
	BWindow *window = Window();
	int localAction = MENU_STATE_TRACKING;
	while (true) {
		bigtime_t snoozeAmount = 40000;
		bool locked = window->Lock();//WithTimeout(200000)
		if (!locked)
			break;

		BPoint where;
		ulong buttons;
		GetMouse(&where, &buttons, false);
			// Get the current mouse state
		window->UpdateIfNeeded();
		BMenuItem *menuItem = HitTestItems(where, B_ORIGIN);
		if (menuItem != NULL) {
			// Select item if:
			// - no previous selection
			// - nonsticky mode and different selection,
			// - clicked in sticky mode
			if (fSelected == NULL
				|| (!IsStickyMode() && menuItem != fSelected)
				|| (buttons != 0 && IsStickyMode())) {
				if (menuItem->Submenu() != NULL) {
					if (menuItem->Submenu()->Window() == NULL) {
						// open the menu if it's not opened yet
						SelectItem(menuItem);
						if (IsStickyMode())
							SetStickyMode(false);
					} else {
						// Menu was already opened, close it and bail
						SelectItem(NULL);
						localAction = MENU_STATE_CLOSED;
						resultItem = NULL;
					}
				} else {
					// No submenu, just select the item
					SelectItem(menuItem);					
				}
			}
		}

		if (fSelected != NULL && OverSubmenu(fSelected, ConvertToScreen(where))) {
			// call _track() from the selected sub-menu when the mouse cursor
			// is over its window
			BMenu *menu = fSelected->Submenu();
			if (menu != NULL) {
				window->Unlock();
				locked = false;
				snoozeAmount = 30000;
				if (IsStickyMode())
					menu->SetStickyMode(true);
				resultItem = menu->_track(&localAction, system_time());
			}
		} else if (menuItem == NULL && !IsStickyMode() && fState != MENU_STATE_TRACKING_SUBMENU) {
			SelectItem(NULL);
			fState = MENU_STATE_TRACKING;
		}
		
		if (locked)
			window->Unlock();
		
		if (localAction == MENU_STATE_CLOSED || (buttons != 0 && IsStickyMode() && menuItem == NULL))
			break;
		else if (buttons == 0 && !IsStickyMode()) {
			// On an item without a submenu
			if (fSelected != NULL && fSelected->Submenu() == NULL) {
				resultItem = fSelected;
				break;
			} else
				SetStickyMode(true);
		}

		if (snoozeAmount > 0)
			snooze(snoozeAmount);		
	}

	if (window->Lock()) {
		if (fSelected != NULL)
			SelectItem(NULL);
		if (resultItem != NULL)
			resultItem->Invoke();
		RestoreFocus();
		window->Unlock();
	}

	if (IsStickyMode())
		SetStickyMode(false);

	DeleteMenuWindow();

	if (action != NULL)
		*action = static_cast<int32>(localAction);

	return resultItem;
}


void 
BMenuBar::StealFocus()
{
	BWindow *window = Window();
	if (window != NULL && window->Lock()) {
		BView *focus = window->CurrentFocus();
		if (focus != NULL)
			fPrevFocusToken = _get_object_token_(focus);
		MakeFocus();
		window->Unlock();
	}
}


void 
BMenuBar::RestoreFocus()
{
	BWindow *window = Window();
	if (window != NULL && window->Lock()) {
		BHandler *handler = NULL;
		if (BPrivate::gDefaultTokens.GetToken(fPrevFocusToken, B_HANDLER_TOKEN,
				(void **)&handler) == B_OK) {
			BView *view = dynamic_cast<BView *>(handler);
			if (view != NULL && view->Window() == window)
				view->MakeFocus();
		}
		fPrevFocusToken = -1;
		window->Unlock();
	}
}


void 
BMenuBar::InitData(menu_layout layout)
{
	fLastBounds = new BRect(Bounds());
	SetItemMargins(8, 2, 8, 2);
	SetIgnoreHidden(true);
}
