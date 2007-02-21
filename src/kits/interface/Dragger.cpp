/*
 * Copyright 2001-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 */

//!	BDragger represents a replicant "handle".


#include <AppServerLink.h>
#include <ServerProtocol.h>
#include <ViewPrivate.h>

#include <Alert.h>
#include <Autolock.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Dragger.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Shelf.h>
#include <Window.h>

#include <stdio.h>
#include <stdlib.h>


bool BDragger::sVisible;
bool BDragger::sVisibleInitialized;
BLocker BDragger::sLock("BDragger static");
BList BDragger::sList;

const static rgb_color kZombieColor = {220, 220, 220, 255};

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


BDragger::BDragger(BRect bounds, BView *target, uint32 rmask, uint32 flags)
	: BView(bounds, "_dragger_", rmask, flags),
	fTarget(target),
	fRelation(TARGET_UNKNOWN),
	fShelf(NULL),
	fTransition(false),
	fIsZombie(false),
	fErrCount(0),
	fPopUp(NULL)
{
	fBitmap = new BBitmap(BRect(0.0f, 0.0f, 7.0f, 7.0f), B_CMAP8, false, false);
	fBitmap->SetBits(kHandBitmap, fBitmap->BitsLength(), 0, B_CMAP8);
}


BDragger::BDragger(BMessage *data)
	: BView(data),
	fTarget(NULL),
	fRelation(TARGET_UNKNOWN),
	fShelf(NULL),
	fTransition(false),
	fIsZombie(false),
	fErrCount(0),
	fPopUp(NULL)
{
	data->FindInt32("_rel", (int32 *)&fRelation);

	fBitmap = new BBitmap(BRect(0.0f, 0.0f, 7.0f, 7.0f), B_CMAP8, false, false);
	fBitmap->SetBits(kHandBitmap, fBitmap->BitsLength(), 0, B_CMAP8);

	BMessage popupMsg;
	if (data->FindMessage("_popup", &popupMsg) == B_OK) {
		BArchivable *archivable = instantiate_object(&popupMsg);

		if (archivable)
			fPopUp = dynamic_cast<BPopUpMenu *>(archivable);
	}
}


BDragger::~BDragger()
{
	SetPopUp(NULL);
	
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
	
	if (fPopUp) {
		ret = fPopUp->Archive(&popupMsg, deep);
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
		BPoint where = bounds.RightBottom() - BPoint(fBitmap->Bounds().Width(), fBitmap->Bounds().Height());
		SetDrawingMode(B_OP_OVER);
		DrawBitmap(fBitmap, where);
		SetDrawingMode(B_OP_COPY);

		if (fIsZombie) {
			// TODO: should draw it differently ?
		}
	} else if (IsVisibilityChanging()) {
		if (Parent())
			Parent()->Invalidate(Frame());
		
		else {
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

	if (buttons & B_SECONDARY_MOUSE_BUTTON) {
		if (!fShelf || !fTarget) {
			beep();
			return;
		}

		_ShowPopUp(fTarget, where);
	} else {
		bigtime_t time = system_time();
		bigtime_t clickSpeed = 0;

		get_click_speed(&clickSpeed);

		bool drag = false;

		while (true) {
			BPoint mousePoint;
			GetMouse(&mousePoint, &buttons);

			if (!buttons || system_time() > time + clickSpeed)
				break;

			if (mousePoint != where) {
				drag = true;
				break;
			}

			snooze(40000);
		}

		if (drag) {
			BMessage archive(B_ARCHIVED_OBJECT);

			if (fRelation == TARGET_IS_PARENT)
				fTarget->Archive(&archive);
			else if (fRelation == TARGET_IS_CHILD)
				Archive(&archive);
			else {
				if (fTarget->Archive(&archive)) {
					BMessage widget(B_ARCHIVED_OBJECT);

					if (Archive(&widget))
						archive.AddMessage("__widget", &widget);
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
		} else
			_ShowPopUp(fTarget, where);
	}
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
	if (msg->what == B_TRASH_TARGET) {
		if (fShelf != NULL)
			Window()->PostMessage(kDeleteReplicant, fTarget, NULL);
		else {
			(new BAlert("??",
				"Can't delete this replicant from its original application. Life goes on.",
				"OK", NULL, NULL, B_WIDTH_FROM_WIDEST, B_WARNING_ALERT))->Go(NULL);
		}
	} else if (msg->what == _SHOW_DRAG_HANDLES_) {
		// this code is used whenever the "are draggers drawn" option is changed
		if (fRelation == TARGET_IS_CHILD) {
			fTransition = true;
			Invalidate();
			Flush();
			fTransition = false;
		} else {
			if ((fShelf && (fShelf->AllowsDragging() && AreDraggersDrawn()))
				|| AreDraggersDrawn())
				Show();
			else
				Hide();
		}
	} else
		BView::MessageReceived(msg);
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
		sVisible = true;
		sVisibleInitialized = true;
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
		sVisible = false;
		sVisibleInitialized = true;
	}

	return status;
}


bool
BDragger::AreDraggersDrawn()
{
	BAutolock _(sLock);

	if (!sVisibleInitialized) {
		BPrivate::AppServerLink link;
		link.StartMessage(AS_GET_SHOW_ALL_DRAGGERS);

		status_t status;
		if (link.FlushWithReply(status) == B_OK && status == B_OK) {
			link.Read<bool>(&sVisible);
			sVisibleInitialized = true;
		} else
			return false;
	}

	return sVisible;
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
BDragger::Perform(perform_code d, void* arg)
{
	return BView::Perform(d, arg);
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
	if (fPopUp != menu)
		delete fPopUp;

	fPopUp = menu;
	return B_OK;
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
	BAutolock _(sLock);

	sVisibleInitialized = true;
	sVisible = visible;

	for (int32 i = sList.CountItems(); i-- > 0;) {
		BDragger* dragger = (BDragger*)sList.ItemAt(i);
		BMessenger target(dragger);
		target.SendMessage(_SHOW_DRAG_HANDLES_);
	}	
}


void
BDragger::_AddToList()
{
	BAutolock _(sLock);
	sList.AddItem(this);

	bool allowsDragging = true;
	if (fShelf)
		allowsDragging = fShelf->AllowsDragging();

	if (!AreDraggersDrawn() || !allowsDragging) {
		// The dragger is not shown - but we can't hide us in case we're the
		// parent of the actual target view (because then you couldn't see
		// it anymore).
		if (fRelation != TARGET_IS_CHILD)
			Hide();
	}
}


void
BDragger::_RemoveFromList()
{
	BAutolock _(sLock);
	sList.RemoveItem(this);
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

	char about[B_OS_NAME_LENGTH];
	snprintf(about, B_OS_NAME_LENGTH, "About %s", name);
	
	fPopUp->AddItem(new BMenuItem(about, msg));
	fPopUp->AddSeparatorItem();
	fPopUp->AddItem(new BMenuItem("Delete", new BMessage(kDeleteReplicant)));
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
