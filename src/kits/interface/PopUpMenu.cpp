//------------------------------------------------------------------------------
//	Copyright (c) 2001-2004, Haiku
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
//	File Name:		PopUpMenu.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BPopUpMenu represents a menu that pops up when you
//                  activate it.
//------------------------------------------------------------------------------

#include <Application.h>
#include <Looper.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Window.h>


struct popup_menu_data 
{
	BPopUpMenu *object;
	BWindow *window;
	BMenuItem *selected;
	
	BPoint where;
	BRect rect;
	
	bool async;
	bool auto_invoke;
	bool start_opened;
	bool use_rect;
	
	sem_id lock;
};


BPopUpMenu::BPopUpMenu(const char *title, bool radioMode, bool autoRename,
						menu_layout layout)
	:
	BMenu(title, layout),
	fUseWhere(false),
	fAutoDestruct(false),
	fTrackThread(-1)
{
	if (radioMode)
		SetRadioMode(true);

	if (autoRename)
		SetLabelFromMarked(true);
}


BPopUpMenu::BPopUpMenu(BMessage *archive)
	:
	BMenu(archive),
	fUseWhere(false),
	fAutoDestruct(false),
	fTrackThread(-1)
{
}


BPopUpMenu::~BPopUpMenu()
{
	if (fTrackThread >= 0) {
		status_t status;
		while (wait_for_thread(fTrackThread, &status) == B_INTERRUPTED)
			;
		
	}
}


status_t
BPopUpMenu::Archive(BMessage *data, bool deep) const
{
	return BMenu::Archive(data, deep);
}


BArchivable *
BPopUpMenu::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, "BPopUpMenu"))
		return new BPopUpMenu(data);

	return NULL;
}


BMenuItem *
BPopUpMenu::Go(BPoint where, bool delivers_message, bool open_anyway, bool async)
{
	return _go(where, delivers_message, open_anyway, NULL, async);
}


BMenuItem *
BPopUpMenu::Go(BPoint where, bool deliversMessage, bool openAnyway,
	BRect clickToOpen, bool async)
{
	return _go(where, deliversMessage, openAnyway, &clickToOpen, async);
}


void
BPopUpMenu::MessageReceived(BMessage *msg)
{
	BMenu::MessageReceived(msg);
}


void
BPopUpMenu::MouseDown(BPoint point)
{
	BView::MouseDown(point);
}


void
BPopUpMenu::MouseUp(BPoint point)
{
	BView::MouseUp(point);
}


void
BPopUpMenu::MouseMoved(BPoint point, uint32 code, const BMessage *msg)
{
	BView::MouseMoved(point, code, msg);
}


void
BPopUpMenu::AttachedToWindow()
{
	BMenu::AttachedToWindow();
}


void
BPopUpMenu::DetachedFromWindow()
{
	BMenu::DetachedFromWindow();
}


void
BPopUpMenu::FrameMoved(BPoint newPosition)
{
	BMenu::FrameMoved(newPosition);
}


void
BPopUpMenu::FrameResized(float newWidth, float newHeight)
{
	BMenu::FrameResized(newWidth, newHeight);
}


BHandler *
BPopUpMenu::ResolveSpecifier(BMessage *msg, int32 index, BMessage *specifier,
	int32 form, const char *property)
{
	return BMenu::ResolveSpecifier(msg, index, specifier, form, property);
}


status_t
BPopUpMenu::GetSupportedSuites(BMessage *data)
{
	return BMenu::GetSupportedSuites(data);
}


status_t
BPopUpMenu::Perform(perform_code d, void *arg)
{
	return BMenu::Perform(d, arg);
}


void
BPopUpMenu::ResizeToPreferred()
{
	BMenu::ResizeToPreferred();
}


void
BPopUpMenu::GetPreferredSize(float *_width, float *_height)
{
	BMenu::GetPreferredSize(_width, _height);
}


void
BPopUpMenu::MakeFocus(bool state)
{
	BMenu::MakeFocus(state);
}


void
BPopUpMenu::AllAttached()
{
	BMenu::AllAttached();
}


void
BPopUpMenu::AllDetached()
{
	BMenu::AllDetached();
}


void
BPopUpMenu::SetAsyncAutoDestruct(bool state)
{
	fAutoDestruct = state;
}


bool
BPopUpMenu::AsyncAutoDestruct() const
{
	return fAutoDestruct;
}


BPoint
BPopUpMenu::ScreenLocation()
{
	if (fUseWhere)
		return fWhere;

	BMenuItem *item = Superitem();
	BMenu *menu = Supermenu();
	BMenuItem *selectedItem = FindItem(item->Label());
	BRect rect = item->Frame();
	BPoint point = rect.LeftTop();

	menu->ConvertToScreen(&point);

	if (selectedItem)
		point.y -= selectedItem->Frame().top;

	return point;
}


//------------------------------------------------------------------------------
//	#pragma mark -
//	private methods


void BPopUpMenu::_ReservedPopUpMenu1() {}
void BPopUpMenu::_ReservedPopUpMenu2() {}
void BPopUpMenu::_ReservedPopUpMenu3() {}


BPopUpMenu &
BPopUpMenu::operator=(const BPopUpMenu &)
{
	return *this;
}


BMenuItem *
BPopUpMenu::_go(BPoint where, bool autoInvoke, bool startOpened,
		BRect *_specialRect, bool async)
{
	BMenuItem *selected = NULL;
	
	// Can't use Window(), as the BPopUpMenu isn't attached
	BLooper *looper = BLooper::LooperForThread(find_thread(NULL));
	BWindow *window = dynamic_cast<BWindow *>(looper);
	
	popup_menu_data *data = new popup_menu_data;
	sem_id sem = create_sem(0, "window close lock");
	
	// Asynchronous menu: we set the BWindow semaphore
	// and let BWindow do the job for us (??? this is what
	// it's probably happening, _set_menu_sem_() is undocumented)
	if (async) {
		data->window = window;
		_set_menu_sem_(window, sem);
	} 
	
	data->object = this;
	data->auto_invoke = autoInvoke;
	data->use_rect = _specialRect != NULL;
	if (_specialRect != NULL)
		data->rect = *_specialRect;
	data->async = async;
	data->where = where;
	data->start_opened = startOpened;
	data->window = NULL;
	data->selected = selected;
	data->lock = sem;
	
	// Spawn the tracking thread
	thread_id thread = spawn_thread(entry, "popup", B_NORMAL_PRIORITY, data); 
	
	if (thread >= 0)
		resume_thread(thread);
	else {
		// Something went wrong. Cleanup and return NULL
		delete_sem(sem);
		if (async)
			_set_menu_sem_(window, B_NO_MORE_SEMS);
		delete data;
		return NULL;
	}
	// Synchronous menu: we block on the sem till
	// the other thread deletes it.
	if (!async) {
		if (window) {
			while (acquire_sem_etc(sem, 1, B_TIMEOUT, 50000) != B_BAD_SEM_ID)
				window->UpdateIfNeeded();	
		}
		
		status_t unused;
		while (wait_for_thread(thread, &unused) == B_INTERRUPTED)
			;
		
		selected = data->selected;
		
		delete data;
	}
		
	return selected;
}


int32
BPopUpMenu::entry(void *arg)
{
	popup_menu_data *data = static_cast<popup_menu_data *>(arg);
	BPopUpMenu *menu = data->object;
	
	BPoint where = data->where;
	BRect *rect = NULL;
	bool auto_invoke = data->auto_invoke;
	bool start_opened = data->start_opened;
		
	if (data->use_rect)
		rect = &data->rect;
	
	BMenuItem *selected = menu->start_track(where, auto_invoke,
								start_opened, rect);
	
	// Put the selected item in the shared struct.
	data->selected = selected;
	
	delete_sem(data->lock);
	
	// Reset the window menu semaphore	
	if (data->window)
		_set_menu_sem_(data->window, B_NO_MORE_SEMS);
	
	// Commit suicide if needed	
	if (menu->fAutoDestruct)
		delete menu;	
			
	if (data->async)
		delete data;
		
	return 0;
}


BMenuItem *
BPopUpMenu::start_track(BPoint where, bool autoInvoke,
		bool startOpened, BRect *_specialRect)
{
	BMenuItem *result = NULL;
	
	fUseWhere = true;
	fWhere = where;
	
	// Show the menu's window
	Show();
	
	// Wait some time then track the menu
	snooze(50000);
	result = Track(startOpened, _specialRect);
	if (result != NULL && autoInvoke)
		result->Invoke();
	
	fUseWhere = false;
	
	Hide();
	
	return result;
}

