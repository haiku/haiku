/*
 * Copyright 2001-2006, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (burton666@libero.it)
 */


#include <Application.h>
#include <Looper.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Window.h>

#include <new>

#include <binary_compatibility/Interface.h>


struct popup_menu_data {
	BPopUpMenu *object;
	BWindow *window;
	BMenuItem *selected;

	BPoint where;
	BRect rect;

	bool async;
	bool autoInvoke;
	bool startOpened;
	bool useRect;

	sem_id lock;
};


BPopUpMenu::BPopUpMenu(const char *title, bool radioMode, bool autoRename,
		menu_layout layout)
	: BMenu(title, layout),
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
	: BMenu(archive),
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
BPopUpMenu::Go(BPoint where, bool deliversMessage, bool openAnyway, bool async)
{
	return _Go(where, deliversMessage, openAnyway, NULL, async);
}


BMenuItem *
BPopUpMenu::Go(BPoint where, bool deliversMessage, bool openAnyway,
	BRect clickToOpen, bool async)
{
	return _Go(where, deliversMessage, openAnyway, &clickToOpen, async);
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
BPopUpMenu::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= BPopUpMenu::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= BPopUpMenu::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= BPopUpMenu::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= BPopUpMenu::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= BPopUpMenu::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			BPopUpMenu::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
}
		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BPopUpMenu::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			BPopUpMenu::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			BPopUpMenu::DoLayout();
			return B_OK;
		}
	}

	return BMenu::Perform(code, _data);
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

	BMenuItem *superItem = Superitem();
	BMenu *superMenu = Supermenu();
	BMenuItem *selectedItem = FindItem(superItem->Label());
	BPoint point = superItem->Frame().LeftTop();

	superMenu->ConvertToScreen(&point);

	if (selectedItem != NULL)
		point.y -= selectedItem->Frame().top;

	return point;
}


//	#pragma mark - private methods


void BPopUpMenu::_ReservedPopUpMenu1() {}
void BPopUpMenu::_ReservedPopUpMenu2() {}
void BPopUpMenu::_ReservedPopUpMenu3() {}


BPopUpMenu &
BPopUpMenu::operator=(const BPopUpMenu &)
{
	return *this;
}


BMenuItem *
BPopUpMenu::_Go(BPoint where, bool autoInvoke, bool startOpened,
		BRect *_specialRect, bool async)
{

	if (fTrackThread >= B_OK) {
		// we already have an active menu, wait for it to go away before
		// spawning another
		status_t unused;
		while (wait_for_thread(fTrackThread, &unused) == B_INTERRUPTED)
			;
	}
	popup_menu_data *data = new (std::nothrow) popup_menu_data;
	if (!data)
		return NULL;

	sem_id sem = create_sem(0, "window close lock");
	if (sem < B_OK) {
		delete data;
		return NULL;
	}

	// Get a pointer to the window from which Go() was called
	BWindow *window = dynamic_cast<BWindow *>(BLooper::LooperForThread(find_thread(NULL)));
	data->window = window;

	// Asynchronous menu: we set the BWindow menu's semaphore
	// and let BWindow block when needed
	if (async && window != NULL) {
		_set_menu_sem_(window, sem);
	}

	data->object = this;
	data->autoInvoke = autoInvoke;
	data->useRect = _specialRect != NULL;
	if (_specialRect != NULL)
		data->rect = *_specialRect;
	data->async = async;
	data->where = where;
	data->startOpened = startOpened;
	data->selected = NULL;
	data->lock = sem;

	// Spawn the tracking thread
	fTrackThread = spawn_thread(_thread_entry, "popup", B_DISPLAY_PRIORITY, data);
	if (fTrackThread < B_OK) {
		// Something went wrong. Cleanup and return NULL
		delete_sem(sem);
		if (async && window != NULL)
			_set_menu_sem_(window, B_BAD_SEM_ID);
		delete data;
		return NULL;
	}

	resume_thread(fTrackThread);

	if (!async)
		return _WaitMenu(data);

	return 0;
}


/* static */
int32
BPopUpMenu::_thread_entry(void *arg)
{
	popup_menu_data *data = static_cast<popup_menu_data *>(arg);
	BPopUpMenu *menu = data->object;
	BRect *rect = NULL;

	if (data->useRect)
		rect = &data->rect;

	data->selected = menu->_StartTrack(data->where, data->autoInvoke, data->startOpened, rect);

	// Reset the window menu semaphore
	if (data->async && data->window)
		_set_menu_sem_(data->window, B_BAD_SEM_ID);

	delete_sem(data->lock);

	// Commit suicide if needed
	if (data->async && menu->fAutoDestruct) {
		menu->fTrackThread = -1;
		delete menu;
	}

	if (data->async)
		delete data;

	return 0;
}


BMenuItem *
BPopUpMenu::_StartTrack(BPoint where, bool autoInvoke, bool startOpened, BRect *_specialRect)
{
	fWhere = where;

	// I know, this doesn't look senseful, but don't be fooled,
	// fUseWhere is used in ScreenLocation(), which is a virtual
	// called by BMenu::Track()
	fUseWhere = true;

	// Determine when mouse-down-up will be taken as a 'press', rather than a 'click'
	bigtime_t clickMaxTime = 0;
	get_click_speed(&clickMaxTime);
	clickMaxTime += system_time();
	
	// Show the menu's window
	Show();
	snooze(50000);
	BMenuItem *result = Track(startOpened, _specialRect);

	// If it was a click, keep the menu open and tracking
	if (system_time() <= clickMaxTime)	
		result = Track(true, _specialRect);
	if (result != NULL && autoInvoke)
		result->Invoke();

	fUseWhere = false;

	Hide();
	be_app->ShowCursor();

	return result;
}


BMenuItem *
BPopUpMenu::_WaitMenu(void *_data)
{
	popup_menu_data *data = (popup_menu_data *)_data;
	BWindow *window = data->window;
	sem_id sem = data->lock;
	if (window != NULL) {
		while (acquire_sem_etc(sem, 1, B_TIMEOUT, 50000) != B_BAD_SEM_ID)
			window->UpdateIfNeeded();
	}

 	status_t unused;
	while (wait_for_thread(fTrackThread, &unused) == B_INTERRUPTED)
		;

	fTrackThread = -1;

	BMenuItem *selected = data->selected;
		// data->selected is filled by the tracking thread

	delete data;

	return selected;
}
