/*
 * Copyright 2001-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Rene Gollent (rene@gollent.com)
 *		Alexandre Deckner (alex@zappotek.com)
 */

//!	BDragger represents a replicant "handle".


#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <Dragger.h>
#include <LocaleBackend.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Shelf.h>
#include <Window.h>

#include <AutoLocker.h>

#include <AppServerLink.h>
#include <DragTrackingFilter.h>
#include <binary_compatibility/Interface.h>
#include <ServerProtocol.h>
#include <ViewPrivate.h>

#include "ZombieReplicantView.h"

using BPrivate::gLocaleBackend;
using BPrivate::LocaleBackend;

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Dragger"

#undef B_TRANSLATE
#define B_TRANSLATE(str) \
	gLocaleBackend->GetString(B_TRANSLATE_MARK(str), "Dragger")

const uint32 kMsgDragStarted = 'Drgs';

const unsigned char
kHandBitmap[] = {
	255, 255,   0,   0,   0, 255, 255, 255,
	255, 255,   0, 131, 131,   0, 255, 255,
	  0,   0,   0,   0, 131, 131,   0,   0,
	  0, 131,   0,   0, 131, 131,   0,   0,
	  0, 131, 131, 131, 131, 131,   0,   0,
	255,   0, 131, 131, 131, 131,   0,   0,
	255, 255,   0,   0,   0,   0,   0,   0,
	255, 255, 255, 255, 255, 255,   0,   0
};


namespace {

struct DraggerManager {
	bool	visible;
	bool	visibleInitialized;
	BList	list;

	DraggerManager()
		:
		visible(false),
		visibleInitialized(false),
		fLock("BDragger static")
	{
	}

	bool Lock()
	{
		return fLock.Lock();
	}

	void Unlock()
	{
		fLock.Unlock();
	}

	static DraggerManager* Default()
	{
		if (sDefaultInstance == NULL)
			pthread_once(&sDefaultInitOnce, &_InitSingleton);

		return sDefaultInstance;
	}

private:
	static void _InitSingleton()
	{
		sDefaultInstance = new DraggerManager;
	}

private:
	BLocker					fLock;

	static pthread_once_t	sDefaultInitOnce;
	static DraggerManager*	sDefaultInstance;
};

pthread_once_t DraggerManager::sDefaultInitOnce = PTHREAD_ONCE_INIT;
DraggerManager* DraggerManager::sDefaultInstance = NULL;

}	// unnamed namespace


BDragger::BDragger(BRect bounds, BView *target, uint32 rmask, uint32 flags)
	: BView(bounds, "_dragger_", rmask, flags),
	fTarget(target),
	fRelation(TARGET_UNKNOWN),
	fShelf(NULL),
	fTransition(false),
	fIsZombie(false),
	fErrCount(0),
	fPopUpIsCustom(false),
	fPopUp(NULL)
{
	_InitData();
}


BDragger::BDragger(BMessage *data)
	: BView(data),
	fTarget(NULL),
	fRelation(TARGET_UNKNOWN),
	fShelf(NULL),
	fTransition(false),
	fIsZombie(false),
	fErrCount(0),
	fPopUpIsCustom(false),
	fPopUp(NULL)
{
	data->FindInt32("_rel", (int32 *)&fRelation);

	_InitData();

	BMessage popupMsg;
	if (data->FindMessage("_popup", &popupMsg) == B_OK) {
		BArchivable *archivable = instantiate_object(&popupMsg);

		if (archivable) {
			fPopUp = dynamic_cast<BPopUpMenu *>(archivable);
			fPopUpIsCustom = true;
		}
	}
}


BDragger::~BDragger()
{
	delete fPopUp;
	delete fBitmap;
}


BArchivable	*
BDragger::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, "BDragger"))
		return new BDragger(data);
	return NULL;
}


status_t
BDragger::Archive(BMessage *data, bool deep) const
{
	status_t ret = BView::Archive(data, deep);
	if (ret != B_OK)
		return ret;

	BMessage popupMsg;

	if (fPopUp && fPopUpIsCustom) {
		bool windowLocked = fPopUp->Window()->Lock();

		ret = fPopUp->Archive(&popupMsg, deep);

		if (windowLocked)
			fPopUp->Window()->Unlock();
				// TODO: Investigate, in some (rare) occasions the menu window
				//		 has already been unlocked

		if (ret == B_OK)
			ret = data->AddMessage("_popup", &popupMsg);
	}

	if (ret == B_OK)
		ret = data->AddInt32("_rel", fRelation);
	return ret;
}


void
BDragger::AttachedToWindow()
{
	if (fIsZombie) {
		SetLowColor(kZombieColor);
		SetViewColor(kZombieColor);
	} else {
		SetLowColor(B_TRANSPARENT_COLOR);
		SetViewColor(B_TRANSPARENT_COLOR);
	}

	_DetermineRelationship();
	_AddToList();

	AddFilter(new DragTrackingFilter(this, kMsgDragStarted));
}


void
BDragger::DetachedFromWindow()
{
	_RemoveFromList();
}


void
BDragger::Draw(BRect update)
{
	BRect bounds(Bounds());

	if (AreDraggersDrawn() && (!fShelf || fShelf->AllowsDragging())) {
		if (Parent() && (Parent()->Flags() & B_DRAW_ON_CHILDREN) == 0) {
			uint32 flags = Parent()->Flags();
			Parent()->SetFlags(flags | B_DRAW_ON_CHILDREN);
			Parent()->Draw(Frame() & ConvertToParent(update));
			Parent()->Flush();
			Parent()->SetFlags(flags);
		}

		BPoint where = bounds.RightBottom() - BPoint(fBitmap->Bounds().Width(),
			fBitmap->Bounds().Height());
		SetDrawingMode(B_OP_OVER);
		DrawBitmap(fBitmap, where);
		SetDrawingMode(B_OP_COPY);

		if (fIsZombie) {
			// TODO: should draw it differently ?
		}
	} else if (IsVisibilityChanging()) {
		if (Parent()) {
			if ((Parent()->Flags() & B_DRAW_ON_CHILDREN) == 0) {
				uint32 flags = Parent()->Flags();
				Parent()->SetFlags(flags | B_DRAW_ON_CHILDREN);
				Parent()->Invalidate(Frame() & ConvertToParent(update));
				Parent()->SetFlags(flags);
			}
		} else {
			SetHighColor(255, 255, 255);
			FillRect(bounds);
		}
	}
}


void
BDragger::MouseDown(BPoint where)
{
	if (!fTarget || !AreDraggersDrawn())
		return;

	uint32 buttons;
	Window()->CurrentMessage()->FindInt32("buttons", (int32 *)&buttons);

	if (fShelf != NULL && (buttons & B_SECONDARY_MOUSE_BUTTON))
		_ShowPopUp(fTarget, where);
}


void
BDragger::MouseUp(BPoint point)
{
	BView::MouseUp(point);
}


void
BDragger::MouseMoved(BPoint point, uint32 code, const BMessage *msg)
{
	BView::MouseMoved(point, code, msg);
}


void
BDragger::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case B_TRASH_TARGET:
			if (fShelf != NULL)
				Window()->PostMessage(kDeleteReplicant, fTarget, NULL);
			else {
				(new BAlert(B_TRANSLATE("Warning"),
					B_TRANSLATE("Can't delete this replicant from its original "
					"application. Life goes on."),
					B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_FROM_WIDEST,
					B_WARNING_ALERT))->Go(NULL);
			}
			break;

		case _SHOW_DRAG_HANDLES_:
			// This code is used whenever the "are draggers drawn" option is
			// changed.
			if (fRelation == TARGET_IS_CHILD) {
				fTransition = true;
				Draw(Bounds());
				Flush();
				fTransition = false;
			} else {
				if ((fShelf && (fShelf->AllowsDragging() && AreDraggersDrawn()))
					|| AreDraggersDrawn()) {
					Show();
				} else
					Hide();
			}
			break;

		case kMsgDragStarted:
			if (fTarget != NULL) {
				BMessage archive(B_ARCHIVED_OBJECT);

				if (fRelation == TARGET_IS_PARENT)
					fTarget->Archive(&archive);
				else if (fRelation == TARGET_IS_CHILD)
					Archive(&archive);
				else {
					if (fTarget->Archive(&archive)) {
						BMessage archivedSelf(B_ARCHIVED_OBJECT);

						if (Archive(&archivedSelf))
							archive.AddMessage("__widget", &archivedSelf);
					}
				}

				archive.AddInt32("be:actions", B_TRASH_TARGET);
				BPoint offset;
				drawing_mode mode;
				BBitmap *bitmap = DragBitmap(&offset, &mode);
				if (bitmap != NULL)
					DragMessage(&archive, bitmap, mode, offset, this);
				else {
					DragMessage(&archive,
						ConvertFromScreen(fTarget->ConvertToScreen(fTarget->Bounds())),
						this);
				}
			}
			break;

		default:
			BView::MessageReceived(msg);
			break;
	}
}


void
BDragger::FrameMoved(BPoint newPosition)
{
	BView::FrameMoved(newPosition);
}


void
BDragger::FrameResized(float newWidth, float newHeight)
{
	BView::FrameResized(newWidth, newHeight);
}


status_t
BDragger::ShowAllDraggers()
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_SET_SHOW_ALL_DRAGGERS);
	link.Attach<bool>(true);

	status_t status = link.Flush();
	if (status == B_OK) {
		DraggerManager* manager = DraggerManager::Default();
		AutoLocker<DraggerManager> locker(manager);
		manager->visible = true;
		manager->visibleInitialized = true;
	}

	return status;
}


status_t
BDragger::HideAllDraggers()
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_SET_SHOW_ALL_DRAGGERS);
	link.Attach<bool>(false);

	status_t status = link.Flush();
	if (status == B_OK) {
		DraggerManager* manager = DraggerManager::Default();
		AutoLocker<DraggerManager> locker(manager);
		manager->visible = false;
		manager->visibleInitialized = true;
	}

	return status;
}


bool
BDragger::AreDraggersDrawn()
{
	DraggerManager* manager = DraggerManager::Default();
	AutoLocker<DraggerManager> locker(manager);

	if (!manager->visibleInitialized) {
		BPrivate::AppServerLink link;
		link.StartMessage(AS_GET_SHOW_ALL_DRAGGERS);

		status_t status;
		if (link.FlushWithReply(status) == B_OK && status == B_OK) {
			link.Read<bool>(&manager->visible);
			manager->visibleInitialized = true;
		} else
			return false;
	}

	return manager->visible;
}


BHandler*
BDragger::ResolveSpecifier(BMessage* message, int32 index, BMessage* specifier,
	int32 form, const char* property)
{
	return BView::ResolveSpecifier(message, index, specifier, form, property);
}


status_t
BDragger::GetSupportedSuites(BMessage* data)
{
	return BView::GetSupportedSuites(data);
}


status_t
BDragger::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= BDragger::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= BDragger::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= BDragger::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= BDragger::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= BDragger::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			BDragger::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
}
		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BDragger::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			BDragger::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			BDragger::DoLayout();
			return B_OK;
		}
	}

	return BView::Perform(code, _data);
}


void
BDragger::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}


void
BDragger::GetPreferredSize(float* _width, float* _height)
{
	BView::GetPreferredSize(_width, _height);
}


void
BDragger::MakeFocus(bool state)
{
	BView::MakeFocus(state);
}


void
BDragger::AllAttached()
{
	BView::AllAttached();
}


void
BDragger::AllDetached()
{
	BView::AllDetached();
}


status_t
BDragger::SetPopUp(BPopUpMenu *menu)
{
	if (menu != NULL && menu != fPopUp) {
		delete fPopUp;
		fPopUp = menu;
		fPopUpIsCustom = true;
		return B_OK;
	}
	return B_ERROR;
}


BPopUpMenu *
BDragger::PopUp() const
{
	if (fPopUp == NULL && fTarget)
		const_cast<BDragger *>(this)->_BuildDefaultPopUp();

	return fPopUp;
}


bool
BDragger::InShelf() const
{
	return fShelf != NULL;
}


BView *
BDragger::Target() const
{
	return fTarget;
}


BBitmap *
BDragger::DragBitmap(BPoint *offset, drawing_mode *mode)
{
	return NULL;
}


bool
BDragger::IsVisibilityChanging() const
{
	return fTransition;
}


void BDragger::_ReservedDragger2() {}
void BDragger::_ReservedDragger3() {}
void BDragger::_ReservedDragger4() {}


BDragger &
BDragger::operator=(const BDragger &)
{
	return *this;
}


/*static*/ void
BDragger::_UpdateShowAllDraggers(bool visible)
{
	DraggerManager* manager = DraggerManager::Default();
	AutoLocker<DraggerManager> locker(manager);

	manager->visibleInitialized = true;
	manager->visible = visible;

	for (int32 i = manager->list.CountItems(); i-- > 0;) {
		BDragger* dragger = (BDragger*)manager->list.ItemAt(i);
		BMessenger target(dragger);
		target.SendMessage(_SHOW_DRAG_HANDLES_);
	}
}


void
BDragger::_InitData()
{
	fBitmap = new BBitmap(BRect(0.0f, 0.0f, 7.0f, 7.0f), B_CMAP8, false, false);
	fBitmap->SetBits(kHandBitmap, fBitmap->BitsLength(), 0, B_CMAP8);

	// we need to translate some strings, and in order to do so, we need
	// to use the LocaleBackend to reach liblocale.so
	if (gLocaleBackend == NULL)
		LocaleBackend::LoadBackend();
}


void
BDragger::_AddToList()
{
	DraggerManager* manager = DraggerManager::Default();
	AutoLocker<DraggerManager> locker(manager);
	manager->list.AddItem(this);

	bool allowsDragging = true;
	if (fShelf)
		allowsDragging = fShelf->AllowsDragging();

	if (!AreDraggersDrawn() || !allowsDragging) {
		// The dragger is not shown - but we can't hide us in case we're the
		// parent of the actual target view (because then you couldn't see
		// it anymore).
		if (fRelation != TARGET_IS_CHILD && !IsHidden())
			Hide();
	}
}


void
BDragger::_RemoveFromList()
{
	DraggerManager* manager = DraggerManager::Default();
	AutoLocker<DraggerManager> locker(manager);
	manager->list.RemoveItem(this);
}


status_t
BDragger::_DetermineRelationship()
{
	if (fTarget) {
		if (fTarget == Parent())
			fRelation = TARGET_IS_PARENT;
		else if (fTarget == ChildAt(0))
			fRelation = TARGET_IS_CHILD;
		else
			fRelation = TARGET_IS_SIBLING;
	} else {
		if (fRelation == TARGET_IS_PARENT)
			fTarget = Parent();
		else if (fRelation == TARGET_IS_CHILD)
			fTarget = ChildAt(0);
		else
			return B_ERROR;
	}

	if (fRelation == TARGET_IS_PARENT) {
		BRect bounds (Frame());
		BRect parentBounds (Parent()->Bounds());
		if (!parentBounds.Contains(bounds))
			MoveTo(parentBounds.right - bounds.Width(),
				parentBounds.bottom - bounds.Height());
	}

	return B_OK;
}


status_t
BDragger::_SetViewToDrag(BView *target)
{
	if (target->Window() != Window())
		return B_ERROR;

	fTarget = target;

	if (Window())
		_DetermineRelationship();

	return B_OK;
}


void
BDragger::_SetShelf(BShelf *shelf)
{
	fShelf = shelf;
}


void
BDragger::_SetZombied(bool state)
{
	fIsZombie = state;

	if (state) {
		SetLowColor(kZombieColor);
		SetViewColor(kZombieColor);
	}
}


void
BDragger::_BuildDefaultPopUp()
{
	fPopUp = new BPopUpMenu("Shelf", false, false, B_ITEMS_IN_COLUMN);

	// About
	BMessage *msg = new BMessage(B_ABOUT_REQUESTED);

	const char *name = fTarget->Name();
	if (name)
		msg->AddString("target", name);

	BString about(B_TRANSLATE("About %app"B_UTF8_ELLIPSIS));
	about.ReplaceFirst("%app", name);

	fPopUp->AddItem(new BMenuItem(about.String(), msg));
	fPopUp->AddSeparatorItem();
	fPopUp->AddItem(new BMenuItem(B_TRANSLATE("Remove replicant"),
		new BMessage(kDeleteReplicant)));
}


void
BDragger::_ShowPopUp(BView *target, BPoint where)
{
	BPoint point = ConvertToScreen(where);

	if (!fPopUp && fTarget)
		_BuildDefaultPopUp();

	fPopUp->SetTargetForItems(fTarget);

	float menuWidth, menuHeight;
	fPopUp->GetPreferredSize(&menuWidth, &menuHeight);
	BRect rect(0, 0, menuWidth, menuHeight);
	rect.InsetBy(-0.5, -0.5);
	rect.OffsetTo(point);

	fPopUp->Go(point, true, false, rect, true);
}


#if __GNUC__ < 3

extern "C" BBitmap*
_ReservedDragger1__8BDragger(BDragger* dragger, BPoint* offset,
	drawing_mode* mode)
{
	return dragger->BDragger::DragBitmap(offset, mode);
}

#endif
