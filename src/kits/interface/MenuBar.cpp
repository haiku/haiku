//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Menubar.cpp
//	Authors:		Marc Flerackers (mflerackers@androme.be)
//					Stefano Ceccherini (burton666@libero.it)
//	Description:	BMenuBar is a menu that's at the root of a menu hierarchy.
//------------------------------------------------------------------------------
// TODO: Finish this class

#include <Application.h>
#include <Autolock.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Window.h>

#include <AppMisc.h>
#include <TokenSpace.h>

struct menubar_data
{
	BMenuBar *menuBar;
	int32 menuIndex;
	
	bool sticky;
	bool showMenu;
	
	bool useRect;
	BRect rect;
};


BMenuBar::BMenuBar(BRect frame, const char *title, uint32 resizeMask,
				   menu_layout layout, bool resizeToFit)
	:	BMenu(frame, title, resizeMask,
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
	:	BMenu(data),
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
		
	// TODO: InitData() ??	
}


BMenuBar::~BMenuBar()
{
	if (fTrackingPID >= 0) {
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
	else
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
	// TODO: implement additional border styles
	if (IsEnabled()) {
		BRect bounds(Bounds());
			
		PushState();
	
		// Restore the background color in case a menuitem
		// was selected.
		SetHighColor(ui_color(B_MENU_BACKGROUND_COLOR));
		FillRect(bounds & updateRect);
			
		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_LIGHTEN_2_TINT));
		StrokeLine(BPoint(0.0f, bounds.bottom - 2.0f), BPoint(0.0f, 0.0f));
		StrokeLine(BPoint(bounds.right, 0.0f));

		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_1_TINT));
		StrokeLine(BPoint(1.0f, bounds.bottom - 1.0f),
			BPoint(bounds.right, bounds.bottom - 1.0f));

		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_2_TINT));
		StrokeLine(BPoint(0.0f, bounds.bottom), BPoint(bounds.right, bounds.bottom));
		StrokeLine(BPoint(bounds.right, 0.0f), BPoint(bounds.right, bounds.bottom));
		
		PopState();
				
		DrawItems(updateRect);
			
	} else {
		LayoutItems(0);
		Sync();
		Invalidate();
	}
}	


void 
BMenuBar::AttachedToWindow()
{
	Install(Window());
	Window()->SetKeyMenuBar(this);

	BMenu::AttachedToWindow();
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
BMenuBar::ResolveSpecifier(BMessage *msg, int32 index,
									 BMessage *specifier, int32 form,
									 const char *property)
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
BMenuBar::StartMenuBar(int32 menuIndex, bool sticky, bool showMenu,
							BRect *specialRect)
{
	BWindow *window = Window();
	if (!window) 
		debugger("MenuBar must be added to a window before it can be used.");
	
	BAutolock lock(window);	
	if (lock.IsLocked()) {
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
				
			send_data(fTrackingPID, 0, &data, sizeof(data));
			resume_thread(fTrackingPID);
		
		} else {
			_set_menu_sem_(window, B_NO_MORE_SEMS);
			delete_sem(fMenuSem);
		}
	}
}


long 
BMenuBar::TrackTask(void *arg)
{
	menubar_data data;
	thread_id id;
	
	receive_data(&id, &data, sizeof(data));
	
	BMenuBar *menuBar = data.menuBar;
	BWindow *window = menuBar->Window();
	
	menuBar->SetStickyMode(data.sticky);
	
	int32 action;
	menuBar->Track(&action, data.menuIndex, data.showMenu);
	
	// Sends a _MENUS_DONE_ message to the BWindow.
	// Weird: There is a _MENUS_DONE_ message but not a 
	// _MENUS_BEGINNING_ message, in fact the MenusBeginning()
	// hook function is called directly.
	window->PostMessage(_MENUS_DONE_);
	
	_set_menu_sem_(window, B_BAD_SEM_ID);
	delete_sem(menuBar->fMenuSem);
	menuBar->fMenuSem = B_BAD_SEM_ID;
		
	return 0;
}


BMenuItem *
BMenuBar::Track(int32 *action, int32 startIndex, bool showMenu)
{
	// TODO: This function is very incomplete and just partially working:
	// For example, it doesn't respect the "sticky mode" setting.
	// Cleanup
	BMenuItem *resultItem = NULL;
	BWindow *window = Window();
	if (window->LockWithTimeout(200000) == B_OK) {
		BPoint where;
		ulong buttons;
		do {
			snooze(40000);
			GetMouse(&where, &buttons);
			BMenuItem *menuItem = HitTestItems(where, B_ORIGIN); 
			if (menuItem) {
				SelectItem(menuItem);
				BMenu *menu = menuItem->Submenu();
				// TODO: Actually, this test shouldn't be needed, as
				// all BMenuBar's BMenuItems are BMenus.
				if (menu) {
					if (IsStickyPrefOn())
						menu->SetStickyMode(true);
					do {
						snooze(40000);
						GetMouse(&where, &buttons);
						
						// If we aren't over this BMenu anymore, exit the tracking loop.
						BMenuItem *testItem = HitTestItems(where, B_ORIGIN);
						if (testItem != NULL && testItem != menuItem)
							break;
					
						resultItem = menu->_track((int *)action, startIndex);
						
						// "action" is "5" when the BMenu is closed.
					} while (*action != 5);
				}
				SelectItem(NULL);
				Invalidate();	
			}			
		} while (buttons != 0);
		
		window->Unlock();
	}
	
	if (resultItem != NULL)
		resultItem->Invoke();
	
	return resultItem;
}


void 
BMenuBar::StealFocus()
{
	BWindow *window = Window();
	if (window && window->Lock()) {
		BView *focus = window->CurrentFocus();
		if (focus)
			fPrevFocusToken = _get_object_token_(focus);
		MakeFocus();
		window->Unlock();
	}
}


void 
BMenuBar::RestoreFocus()
{
	BWindow *window = Window();
	if (window && window->Lock()) {
		BHandler *handler = NULL;
		if (BPrivate::gDefaultTokens.GetToken(fPrevFocusToken, B_HANDLER_TOKEN,
				(void **)&handler, NULL) == B_OK) {
			BView *view = dynamic_cast<BView *>(handler);
			// TODO: Are there other things to do in case the BHandler
			// is not a BView ?
			if (view != NULL)
				view->MakeFocus();
		}
		fPrevFocusToken = NULL;
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
