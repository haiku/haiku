/*
 * Copyright 2006-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "IconView.h"
#include "MimeTypeListView.h"

#include <Application.h>
#include <AppFileInfo.h>
#include <Attributes.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <IconEditorProtocol.h>
#include <IconUtils.h>
#include <Locale.h>
#include <MenuItem.h>
#include <Mime.h>
#include <NodeMonitor.h>
#include <PopUpMenu.h>
#include <Resources.h>
#include <Roster.h>
#include <Size.h>

#include <new>
#include <stdlib.h>
#include <string.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon View"


using namespace std;


status_t
icon_for_type(const BMimeType& type, uint8** _data, size_t* _size,
	icon_source* _source)
{
	if (_data == NULL || _size == NULL)
		return B_BAD_VALUE;

	icon_source source = kNoIcon;
	uint8* data;
	size_t size;

	if (type.GetIcon(&data, &size) == B_OK)
		source = kOwnIcon;

	if (source == kNoIcon) {
		// check for icon from preferred app

		char preferred[B_MIME_TYPE_LENGTH];
		if (type.GetPreferredApp(preferred) == B_OK) {
			BMimeType preferredApp(preferred);

			if (preferredApp.GetIconForType(type.Type(), &data, &size) == B_OK)
				source = kApplicationIcon;
		}
	}

	if (source == kNoIcon) {
		// check super type for an icon

		BMimeType superType;
		if (type.GetSupertype(&superType) == B_OK) {
			if (superType.GetIcon(&data, &size) == B_OK)
				source = kSupertypeIcon;
			else {
				// check the super type's preferred app
				char preferred[B_MIME_TYPE_LENGTH];
				if (superType.GetPreferredApp(preferred) == B_OK) {
					BMimeType preferredApp(preferred);

					if (preferredApp.GetIconForType(superType.Type(),
							&data, &size) == B_OK)
						source = kSupertypeIcon;
				}
			}
		}
	}

	if (source != kNoIcon) {
		*_data = data;
		*_size = size;
	} // NOTE: else there is no data, so nothing is leaked.
	if (_source)
		*_source = source;

	return source != kNoIcon ? B_OK : B_ERROR;
}


status_t
icon_for_type(const BMimeType& type, BBitmap& bitmap, icon_size size,
	icon_source* _source)
{
	icon_source source = kNoIcon;

	if (type.GetIcon(&bitmap, size) == B_OK)
		source = kOwnIcon;

	if (source == kNoIcon) {
		// check for icon from preferred app

		char preferred[B_MIME_TYPE_LENGTH];
		if (type.GetPreferredApp(preferred) == B_OK) {
			BMimeType preferredApp(preferred);

			if (preferredApp.GetIconForType(type.Type(), &bitmap, size) == B_OK)
				source = kApplicationIcon;
		}
	}

	if (source == kNoIcon) {
		// check super type for an icon

		BMimeType superType;
		if (type.GetSupertype(&superType) == B_OK) {
			if (superType.GetIcon(&bitmap, size) == B_OK)
				source = kSupertypeIcon;
			else {
				// check the super type's preferred app
				char preferred[B_MIME_TYPE_LENGTH];
				if (superType.GetPreferredApp(preferred) == B_OK) {
					BMimeType preferredApp(preferred);

					if (preferredApp.GetIconForType(superType.Type(),
							&bitmap, size) == B_OK)
						source = kSupertypeIcon;
				}
			}
		}
	}

	if (_source)
		*_source = source;

	return source != kNoIcon ? B_OK : B_ERROR;
}


//	#pragma mark -


Icon::Icon()
	:
	fLarge(NULL),
	fMini(NULL),
	fData(NULL),
	fSize(0)
{
}


Icon::Icon(const Icon& source)
	:
	fLarge(NULL),
	fMini(NULL),
	fData(NULL),
	fSize(0)
{
	*this = source;
}


Icon::~Icon()
{
	delete fLarge;
	delete fMini;
	free(fData);
}


void
Icon::SetTo(const BAppFileInfo& info, const char* type)
{
	Unset();

	uint8* data;
	size_t size;
	if (info.GetIconForType(type, &data, &size) == B_OK) {
		// we have the vector icon, no need to get the rest
		AdoptData(data, size);
		return;
	}

	BBitmap* icon = AllocateBitmap(B_LARGE_ICON, B_CMAP8);
	if (icon && info.GetIconForType(type, icon, B_LARGE_ICON) == B_OK)
		AdoptLarge(icon);
	else
		delete icon;

	icon = AllocateBitmap(B_MINI_ICON, B_CMAP8);
	if (icon && info.GetIconForType(type, icon, B_MINI_ICON) == B_OK)
		AdoptMini(icon);
	else
		delete icon;
}


void
Icon::SetTo(const entry_ref& ref, const char* type)
{
	Unset();

	BFile file(&ref, B_READ_ONLY);
	BAppFileInfo info(&file);
	if (file.InitCheck() == B_OK
		&& info.InitCheck() == B_OK)
		SetTo(info, type);
}


void
Icon::SetTo(const BMimeType& type, icon_source* _source)
{
	Unset();

	uint8* data;
	size_t size;
	if (icon_for_type(type, &data, &size, _source) == B_OK) {
		// we have the vector icon, no need to get the rest
		AdoptData(data, size);
		return;
	}

	BBitmap* icon = AllocateBitmap(B_LARGE_ICON, B_CMAP8);
	if (icon && icon_for_type(type, *icon, B_LARGE_ICON, _source) == B_OK)
		AdoptLarge(icon);
	else
		delete icon;

	icon = AllocateBitmap(B_MINI_ICON, B_CMAP8);
	if (icon && icon_for_type(type, *icon, B_MINI_ICON) == B_OK)
		AdoptMini(icon);
	else
		delete icon;
}


status_t
Icon::CopyTo(BAppFileInfo& info, const char* type, bool force) const
{
	status_t status = B_OK;

	if (fLarge != NULL || force)
		status = info.SetIconForType(type, fLarge, B_LARGE_ICON);
	if (fMini != NULL || force)
		status = info.SetIconForType(type, fMini, B_MINI_ICON);
	if (fData != NULL || force)
		status = info.SetIconForType(type, fData, fSize);
	return status;
}


status_t
Icon::CopyTo(const entry_ref& ref, const char* type, bool force) const
{
	BFile file;
	status_t status = file.SetTo(&ref, B_READ_ONLY);
	if (status < B_OK)
		return status;

	BAppFileInfo info(&file);
	status = info.InitCheck();
	if (status < B_OK)
		return status;

	return CopyTo(info, type, force);
}


status_t
Icon::CopyTo(BMimeType& type, bool force) const
{
	status_t status = B_OK;

	if (fLarge != NULL || force)
		status = type.SetIcon(fLarge, B_LARGE_ICON);
	if (fMini != NULL || force)
		status = type.SetIcon(fMini, B_MINI_ICON);
	if (fData != NULL || force)
		status = type.SetIcon(fData, fSize);
	return status;
}


status_t
Icon::CopyTo(BMessage& message) const
{
	status_t status = B_OK;

	if (status == B_OK && fLarge != NULL) {
		BMessage archive;
		status = fLarge->Archive(&archive);
		if (status == B_OK)
			status = message.AddMessage("icon/large", &archive);
	}
	if (status == B_OK && fMini != NULL) {
		BMessage archive;
		status = fMini->Archive(&archive);
		if (status == B_OK)
			status = message.AddMessage("icon/mini", &archive);
	}
	if (status == B_OK && fData != NULL)
		status = message.AddData("icon", B_VECTOR_ICON_TYPE, fData, fSize);

	return B_OK;
}


void
Icon::SetLarge(const BBitmap* large)
{
	if (large != NULL) {
		if (fLarge == NULL)
			fLarge = new BBitmap(BRect(0, 0, 31, 31), B_CMAP8);

		memcpy(fLarge->Bits(), large->Bits(), min_c(large->BitsLength(),
			fLarge->BitsLength()));
	} else {
		delete fLarge;
		fLarge = NULL;
	}
}


void
Icon::SetMini(const BBitmap* mini)
{
	if (mini != NULL) {
		if (fMini == NULL)
			fMini = new BBitmap(BRect(0, 0, 15, 15), B_CMAP8);

		memcpy(fMini->Bits(), mini->Bits(), min_c(mini->BitsLength(),
			fMini->BitsLength()));
	} else {
		delete fMini;
		fMini = NULL;
	}
}


void
Icon::SetData(const uint8* data, size_t size)
{
	free(fData);
	fData = NULL;

	if (data != NULL) {
		fData = (uint8*)malloc(size);
		if (fData != NULL) {
			fSize = size;
			//fType = B_VECTOR_ICON_TYPE;
			memcpy(fData, data, size);
		}
	}
}


void
Icon::Unset()
{
	delete fLarge;
	delete fMini;
	free(fData);

	fLarge = fMini = NULL;
	fData = NULL;
}


bool
Icon::HasData() const
{
	return fData != NULL || fLarge != NULL || fMini != NULL;
}


status_t
Icon::GetData(icon_size which, BBitmap** _bitmap) const
{
	BBitmap* source;
	switch (which) {
		case B_LARGE_ICON:
			source = fLarge;
			break;
		case B_MINI_ICON:
			source = fMini;
			break;
		default:
			return B_BAD_VALUE;
	}

	if (source == NULL)
		return B_ENTRY_NOT_FOUND;

	BBitmap* bitmap = new (nothrow) BBitmap(source);
	if (bitmap == NULL || bitmap->InitCheck() != B_OK) {
		delete bitmap;
		return B_NO_MEMORY;
	}

	*_bitmap = bitmap;
	return B_OK;
}


status_t
Icon::GetData(uint8** _data, size_t* _size) const
{
	if (fData == NULL)
		return B_ENTRY_NOT_FOUND;

	uint8* data = (uint8*)malloc(fSize);
	if (data == NULL)
		return B_NO_MEMORY;

	memcpy(data, fData, fSize);
	*_data = data;
	*_size = fSize;
	return B_OK;
}


status_t
Icon::GetIcon(BBitmap* bitmap) const
{
	if (bitmap == NULL)
		return B_BAD_VALUE;

	if (fData != NULL
		&& BIconUtils::GetVectorIcon(fData, fSize, bitmap) == B_OK)
		return B_OK;

	int32 width = bitmap->Bounds().IntegerWidth() + 1;

	if (width == B_LARGE_ICON && fLarge != NULL) {
		bitmap->SetBits(fLarge->Bits(), fLarge->BitsLength(), 0,
			fLarge->ColorSpace());
		return B_OK;
	}
	if (width == B_MINI_ICON && fMini != NULL) {
		bitmap->SetBits(fMini->Bits(), fMini->BitsLength(), 0,
			fMini->ColorSpace());
		return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


Icon&
Icon::operator=(const Icon& source)
{
	Unset();

	SetData(source.fData, source.fSize);
	SetLarge(source.fLarge);
	SetMini(source.fMini);

	return *this;
}


void
Icon::AdoptLarge(BBitmap *large)
{
	delete fLarge;
	fLarge = large;
}


void
Icon::AdoptMini(BBitmap *mini)
{
	delete fMini;
	fMini = mini;
}


void
Icon::AdoptData(uint8* data, size_t size)
{
	free(fData);
	fData = data;
	fSize = size;
}


/*static*/ BBitmap*
Icon::AllocateBitmap(int32 size, int32 space)
{
	int32 kSpace = B_RGBA32;
	if (space == -1)
		space = kSpace;

	BBitmap* bitmap = new (nothrow) BBitmap(BRect(0, 0, size - 1, size - 1),
		(color_space)space);
	if (bitmap == NULL || bitmap->InitCheck() != B_OK) {
		delete bitmap;
		return NULL;
	}

	return bitmap;
}


//	#pragma mark -


IconView::IconView(const char* name, uint32 flags)
	: BControl(name, NULL, NULL, B_WILL_DRAW | flags),
	fModificationMessage(NULL),
	fIconSize(B_LARGE_ICON),
	fIcon(NULL),
	fHeapIcon(NULL),
	fHasRef(false),
	fHasType(false),
	fIconData(NULL),
	fTracking(false),
	fDragging(false),
	fDropTarget(false),
	fShowEmptyFrame(true)
{
}


IconView::~IconView()
{
	delete fIcon;
	delete fModificationMessage;
}


void
IconView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fTarget = this;

	// SetTo() was already called before we were a valid messenger
	if (fHasRef || fHasType)
		_StartWatching();
}


void
IconView::DetachedFromWindow()
{
	_StopWatching();
}


void
IconView::MessageReceived(BMessage* message)
{
	if (message->WasDropped() && message->ReturnAddress() != BMessenger(this)
		&& AcceptsDrag(message)) {
		// set icon from message
		BBitmap* mini = NULL;
		BBitmap* large = NULL;
		const uint8* data = NULL;
		ssize_t size = 0;

		message->FindData("icon", B_VECTOR_ICON_TYPE, (const void**)&data,
			&size);

		BMessage archive;
		if (message->FindMessage("icon/large", &archive) == B_OK)
			large = (BBitmap*)BBitmap::Instantiate(&archive);
		if (message->FindMessage("icon/mini", &archive) == B_OK)
			mini = (BBitmap*)BBitmap::Instantiate(&archive);

		if (large != NULL || mini != NULL || (data != NULL && size > 0))
			_SetIcon(large, mini, data, size);
		else {
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK)
				_SetIcon(&ref);
		}

		delete large;
		delete mini;

		return;
	}

	switch (message->what) {
		case kMsgIconInvoked:
		case kMsgEditIcon:
		case kMsgAddIcon:
			_AddOrEditIcon();
			break;
		case kMsgRemoveIcon:
			_RemoveIcon();
			break;

		case B_NODE_MONITOR:
		{
			if (!fHasRef)
				break;

			int32 opcode;
			if (message->FindInt32("opcode", &opcode) != B_OK
				|| opcode != B_ATTR_CHANGED)
				break;

			const char* name;
			if (message->FindString("attr", &name) != B_OK)
				break;

			if (!strcmp(name, kAttrMiniIcon)
				|| !strcmp(name, kAttrLargeIcon)
				|| !strcmp(name, kAttrIcon))
				Update();
			break;
		}

		case B_META_MIME_CHANGED:
		{
			if (!fHasType)
				break;

			const char* type;
			int32 which;
			if (message->FindString("be:type", &type) != B_OK
				|| message->FindInt32("be:which", &which) != B_OK)
				break;

			if (!strcasecmp(type, fType.Type())) {
				switch (which) {
					case B_MIME_TYPE_DELETED:
						Unset();
						break;

					case B_ICON_CHANGED:
						Update();
						break;

					default:
						break;
				}
			} else if (fSource != kOwnIcon
				&& message->FindString("be:extra_type", &type) == B_OK
				&& !strcasecmp(type, fType.Type())) {
				// this change could still affect our current icon

				if (which == B_MIME_TYPE_DELETED
					|| which == B_PREFERRED_APP_CHANGED
					|| which == B_SUPPORTED_TYPES_CHANGED
					|| which == B_ICON_FOR_TYPE_CHANGED)
					Update();
			}
			break;
		}

		case B_ICON_DATA_EDITED:
		{
			const uint8* data;
			ssize_t size;
			if (message->FindData("icon data", B_VECTOR_ICON_TYPE,
					(const void**)&data, &size) < B_OK)
				break;

			_SetIcon(NULL, NULL, data, size);
			break;
		}

		default:
			BControl::MessageReceived(message);
			break;
	}
}


bool
IconView::AcceptsDrag(const BMessage* message)
{
	if (!IsEnabled())
		return false;

	type_code type;
	int32 count;
	if (message->GetInfo("refs", &type, &count) == B_OK && count == 1
		&& type == B_REF_TYPE) {
		// if we're bound to an entry, check that no one drops this to us
		entry_ref ref;
		if (fHasRef && message->FindRef("refs", &ref) == B_OK && fRef == ref)
			return false;

		return true;
	}

	if ((message->GetInfo("icon/large", &type) == B_OK
			&& type == B_MESSAGE_TYPE)
		|| (message->GetInfo("icon", &type) == B_OK
			&& type == B_VECTOR_ICON_TYPE)
		|| (message->GetInfo("icon/mini", &type) == B_OK
			&& type == B_MESSAGE_TYPE))
		return true;

	return false;
}


BRect
IconView::BitmapRect() const
{
	return BRect(0, 0, fIconSize - 1, fIconSize - 1);
}


void
IconView::Draw(BRect updateRect)
{
	SetDrawingMode(B_OP_ALPHA);

	if (fHeapIcon != NULL)
		DrawBitmap(fHeapIcon, BitmapRect().LeftTop());
	else if (fIcon != NULL)
		DrawBitmap(fIcon, BitmapRect().LeftTop());
	else if (!fDropTarget && fShowEmptyFrame) {
		// draw frame so that the user knows here is something he
		// might be able to click on
		SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
		StrokeRect(BitmapRect());
	}

	if (IsFocus()) {
		// mark this view as a having focus
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		StrokeRect(BitmapRect());
	}
	if (fDropTarget) {
		// mark this view as a drop target
		SetHighColor(0, 0, 0);
		SetPenSize(2);
		BRect rect = BitmapRect();
// TODO: this is an incompatibility between R5 and Haiku and should be fixed!
// (Necessary adjustment differs.)
		rect.left++;
		rect.top++;

		StrokeRect(rect);
		SetPenSize(1);
	}
}


void
IconView::GetPreferredSize(float* _width, float* _height)
{
	if (_width)
		*_width = fIconSize;

	if (_height)
		*_height = fIconSize;
}


BSize
IconView::MinSize()
{
	float width, height;
	GetPreferredSize(&width, &height);
	return BSize(width, height);
}


BSize
IconView::PreferredSize()
{
	return MinSize();
}


BSize
IconView::MaxSize()
{
	return MinSize();
}


void
IconView::MouseDown(BPoint where)
{
	if (!IsEnabled())
		return;

	int32 buttons = B_PRIMARY_MOUSE_BUTTON;
	int32 clicks = 1;
	if (Looper() != NULL && Looper()->CurrentMessage() != NULL) {
		if (Looper()->CurrentMessage()->FindInt32("buttons", &buttons) != B_OK)
			buttons = B_PRIMARY_MOUSE_BUTTON;
		if (Looper()->CurrentMessage()->FindInt32("clicks", &clicks) != B_OK)
			clicks = 1;
	}

	if ((buttons & B_PRIMARY_MOUSE_BUTTON) != 0
		&& BitmapRect().Contains(where)) {
		if (clicks == 2) {
			// double click - open Icon-O-Matic
			Invoke();
		} else if (fIcon != NULL) {
			// start tracking - this icon might be dragged around
			fDragPoint = where;
			fTracking = true;
		}
	}

	if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
		// show context menu

		ConvertToScreen(&where);

		BPopUpMenu* menu = new BPopUpMenu("context");
		menu->SetFont(be_plain_font);

		bool hasIcon = fHasType ? fSource == kOwnIcon : fIcon != NULL;
		if (hasIcon) {
			menu->AddItem(new BMenuItem(
				B_TRANSLATE("Edit icon" B_UTF8_ELLIPSIS),
				new BMessage(kMsgEditIcon)));
		} else {
			menu->AddItem(new BMenuItem(
				B_TRANSLATE("Add icon" B_UTF8_ELLIPSIS),
				new BMessage(kMsgAddIcon)));
		}

		BMenuItem* item = new BMenuItem(
			B_TRANSLATE("Remove icon"), new BMessage(kMsgRemoveIcon));
		if (!hasIcon)
			item->SetEnabled(false);

		menu->AddItem(item);
		menu->SetTargetForItems(fTarget);

		menu->Go(where, true, false, true);
	}
}


void
IconView::MouseUp(BPoint where)
{
	fTracking = false;
	fDragging = false;

	if (fDropTarget) {
		fDropTarget = false;
		Invalidate();
	}
}


void
IconView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	if (fTracking && !fDragging && fIcon != NULL
		&& (abs((int32)(where.x - fDragPoint.x)) > 3
			|| abs((int32)(where.y - fDragPoint.y)) > 3)) {
		// Start drag
		BMessage message(B_SIMPLE_DATA);

		::Icon* icon = fIconData;
		if (fHasRef || fHasType) {
			icon = new ::Icon;
			if (fHasRef)
				icon->SetTo(fRef, fType.Type());
			else if (fHasType)
				icon->SetTo(fType);
		}

		icon->CopyTo(message);

		if (icon != fIconData)
			delete icon;

		BBitmap *dragBitmap = new BBitmap(fIcon->Bounds(), B_RGBA32, true);
		dragBitmap->Lock();
		BView *view
			= new BView(dragBitmap->Bounds(), B_EMPTY_STRING, B_FOLLOW_NONE, 0);
		dragBitmap->AddChild(view);

		view->SetHighColor(B_TRANSPARENT_COLOR);
		view->FillRect(dragBitmap->Bounds());
		view->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetHighColor(0, 0, 0, 160);
		view->DrawBitmap(fIcon);

		view->Sync();
		dragBitmap->Unlock();

		DragMessage(&message, dragBitmap, B_OP_ALPHA,
			fDragPoint - BitmapRect().LeftTop(), this);
		fDragging = true;
		SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	}

	if (dragMessage != NULL && !fDragging && AcceptsDrag(dragMessage)) {
		bool dropTarget = transit == B_ENTERED_VIEW || transit == B_INSIDE_VIEW;
		if (dropTarget != fDropTarget) {
			fDropTarget = dropTarget;
			Invalidate();
		}
	} else if (fDropTarget) {
		fDropTarget = false;
		Invalidate();
	}
}


void
IconView::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes == 1) {
		switch (bytes[0]) {
			case B_DELETE:
			case B_BACKSPACE:
				_RemoveIcon();
				return;
			case B_ENTER:
			case B_SPACE:
				Invoke();
				return;
		}
	}

	BControl::KeyDown(bytes, numBytes);
}


void
IconView::MakeFocus(bool focus)
{
	if (focus != IsFocus())
		Invalidate();

	BControl::MakeFocus(focus);
}


void
IconView::SetTo(const entry_ref& ref, const char* fileType)
{
	Unset();

	fHasRef = true;
	fRef = ref;
	if (fileType != NULL)
		fType.SetTo(fileType);
	else
		fType.Unset();

	_StartWatching();
	Update();
}


void
IconView::SetTo(const BMimeType& type)
{
	Unset();

	if (type.Type() == NULL)
		return;

	fHasType = true;
	fType.SetTo(type.Type());

	_StartWatching();
	Update();
}


void
IconView::SetTo(::Icon* icon)
{
	if (fIconData == icon)
		return;

	Unset();

	fIconData = icon;

	Update();
}


void
IconView::Unset()
{
	if (fHasRef || fHasType)
		_StopWatching();

	fHasRef = false;
	fHasType = false;

	fType.Unset();
	fIconData = NULL;
}


void
IconView::Update()
{
	delete fIcon;
	fIcon = NULL;

	Invalidate();
		// this will actually trigger a redraw *after* we updated the icon below

	BBitmap* icon = NULL;

	if (fHasRef) {
		BFile file(&fRef, B_READ_ONLY);
		if (file.InitCheck() != B_OK)
			return;

		BAppFileInfo info;
		if (info.SetTo(&file) != B_OK)
			return;

		icon = Icon::AllocateBitmap(fIconSize);
		if (icon != NULL && info.GetIconForType(fType.Type(), icon,
				(icon_size)fIconSize) != B_OK) {
			delete icon;
			return;
		}
	} else if (fHasType) {
		icon = Icon::AllocateBitmap(fIconSize);
		if (icon != NULL && icon_for_type(fType, *icon, (icon_size)fIconSize,
				&fSource) != B_OK) {
			delete icon;
			return;
		}
	} else if (fIconData) {
		icon = Icon::AllocateBitmap(fIconSize);
		if (fIconData->GetIcon(icon) != B_OK) {
			delete icon;
			icon = NULL;
		}
	}

	fIcon = icon;
}


void
IconView::SetIconSize(int32 size)
{
	if (size < B_MINI_ICON)
		size = B_MINI_ICON;
	if (size > 256)
		size = 256;
	if (size == fIconSize)
		return;

	fIconSize = size;
	Update();
}


void
IconView::ShowIconHeap(bool show)
{
	if (show == (fHeapIcon != NULL))
		return;

	if (show) {
		BResources* resources = be_app->AppResources();
		if (resources != NULL) {
			const void* data = NULL;
			size_t size;
			data = resources->LoadResource('VICN', "icon heap", &size);
			if (data != NULL) {
				// got vector icon data
				fHeapIcon = Icon::AllocateBitmap(B_LARGE_ICON, B_RGBA32);
				if (BIconUtils::GetVectorIcon((const uint8*)data,
						size, fHeapIcon) != B_OK) {
					// bad data
					delete fHeapIcon;
					fHeapIcon = NULL;
					data = NULL;
				}
			}
			if (data == NULL) {
				// no vector icon or failed to get bitmap
				// try bitmap icon
				data = resources->LoadResource(B_LARGE_ICON_TYPE, "icon heap",
					NULL);
				if (data != NULL) {
					fHeapIcon = Icon::AllocateBitmap(B_LARGE_ICON, B_CMAP8);
					if (fHeapIcon != NULL) {
						memcpy(fHeapIcon->Bits(), data,
							fHeapIcon->BitsLength());
					}
				}
			}
		}
	} else {
		delete fHeapIcon;
		fHeapIcon = NULL;
	}
}


void
IconView::ShowEmptyFrame(bool show)
{
	if (show == fShowEmptyFrame)
		return;

	fShowEmptyFrame = show;
	if (fIcon == NULL)
		Invalidate();
}


status_t
IconView::SetTarget(const BMessenger& target)
{
	fTarget = target;
	return B_OK;
}


void
IconView::SetModificationMessage(BMessage* message)
{
	delete fModificationMessage;
	fModificationMessage = message;
}


status_t
IconView::Invoke(BMessage* message)
{
	if (message == NULL)
		fTarget.SendMessage(kMsgIconInvoked);
	else
		fTarget.SendMessage(message);
	return B_OK;
}


Icon*
IconView::Icon()
{
	return fIconData;
}


status_t
IconView::GetRef(entry_ref& ref) const
{
	if (!fHasRef)
		return B_BAD_TYPE;

	ref = fRef;
	return B_OK;
}


status_t
IconView::GetMimeType(BMimeType& type) const
{
	if (!fHasType)
		return B_BAD_TYPE;

	type.SetTo(fType.Type());
	return B_OK;
}


void
IconView::_AddOrEditIcon()
{
	BMessage message;
	if (fHasRef && fType.Type() == NULL) {
		// in ref mode, Icon-O-Matic can change the icon directly, and
		// we'll pick it up via node monitoring
		message.what = B_REFS_RECEIVED;
		message.AddRef("refs", &fRef);
	} else {
		// in static or MIME type mode, Icon-O-Matic needs to return the
		// buffer it changed once its done
		message.what = B_EDIT_ICON_DATA;
		message.AddMessenger("reply to", BMessenger(this));

		::Icon* icon = fIconData;
		if (icon == NULL) {
			icon = new ::Icon();
			if (fHasRef)
				icon->SetTo(fRef, fType.Type());
			else
				icon->SetTo(fType);
		}

		if (icon->HasData()) {
			uint8* data;
			size_t size;
			if (icon->GetData(&data, &size) == B_OK) {
				message.AddData("icon data", B_VECTOR_ICON_TYPE, data, size);
				free(data);
			}

			// TODO: somehow figure out how names of objects in the icon
			// can be preserved. Maybe in a second (optional) attribute
			// where ever a vector icon attribute is present?
		}

		if (icon != fIconData)
			delete icon;
	}

	be_roster->Launch("application/x-vnd.haiku-icon_o_matic", &message);
}


void
IconView::_SetIcon(BBitmap* large, BBitmap* mini, const uint8* data,
	size_t size, bool force)
{
	if (fHasRef) {
		BFile file(&fRef, B_READ_WRITE);

		BAppFileInfo info(&file);
		info.SetInfoLocation(B_USE_ATTRIBUTES);
		if (info.InitCheck() == B_OK) {
			if (large != NULL || force)
				info.SetIconForType(fType.Type(), large, B_LARGE_ICON);
			if (mini != NULL || force)
				info.SetIconForType(fType.Type(), mini, B_MINI_ICON);
			if (data != NULL || force)
				info.SetIconForType(fType.Type(), data, size);
		}
		// the icon shown will be updated using node monitoring
	} else if (fHasType) {
		if (large != NULL || force)
			fType.SetIcon(large, B_LARGE_ICON);
		if (mini != NULL || force)
			fType.SetIcon(mini, B_MINI_ICON);
		if (data != NULL || force)
			fType.SetIcon(data, size);
		// the icon shown will be updated automatically - we're watching
		// any changes to the MIME database
	} else if (fIconData != NULL) {
		if (large != NULL || force)
			fIconData->SetLarge(large);
		if (mini != NULL || force)
			fIconData->SetMini(mini);
		if (data != NULL || force)
			fIconData->SetData(data, size);

		// replace visible icon
		if (fIcon == NULL && fIconData->HasData())
			fIcon = Icon::AllocateBitmap(fIconSize);

		if (fIconData->GetIcon(fIcon) != B_OK) {
			delete fIcon;
			fIcon = NULL;
		}
		Invalidate();
	}

	if (fModificationMessage)
		Invoke(fModificationMessage);
}


void
IconView::_SetIcon(entry_ref* ref)
{
	// retrieve icons from file
	BFile file(ref, B_READ_ONLY);
	BAppFileInfo info(&file);
	if (file.InitCheck() != B_OK || info.InitCheck() != B_OK)
		return;

	// try vector/PNG icon first
	uint8* data = NULL;
	size_t size = 0;
	if (info.GetIcon(&data, &size) == B_OK) {
		_SetIcon(NULL, NULL, data, size);
		free(data);
		return;
	}

	// try large/mini icons
	bool hasMini = false;
	bool hasLarge = false;

	BBitmap* large = new BBitmap(BRect(0, 0, 31, 31), B_CMAP8);
	if (large->InitCheck() != B_OK) {
		delete large;
		large = NULL;
	}
	BBitmap* mini = new BBitmap(BRect(0, 0, 15, 15), B_CMAP8);
	if (mini->InitCheck() != B_OK) {
		delete mini;
		mini = NULL;
	}

	if (large != NULL && info.GetIcon(large, B_LARGE_ICON) == B_OK)
		hasLarge = true;
	if (mini != NULL && info.GetIcon(mini, B_MINI_ICON) == B_OK)
		hasMini = true;

	if (!hasMini && !hasLarge) {
		// TODO: don't forget device icons!

		// try MIME type icon
		char type[B_MIME_TYPE_LENGTH];
		if (info.GetType(type) != B_OK)
			return;

		BMimeType mimeType(type);
		if (icon_for_type(mimeType, &data, &size) != B_OK) {
			// only try large/mini icons when there is no vector icon
			if (large != NULL
				&& icon_for_type(mimeType, *large, B_LARGE_ICON) == B_OK)
				hasLarge = true;
			if (mini != NULL
				&& icon_for_type(mimeType, *mini, B_MINI_ICON) == B_OK)
				hasMini = true;
		}
	}

	if (data != NULL) {
		_SetIcon(NULL, NULL, data, size);
		free(data);
	} else if (hasLarge || hasMini)
		_SetIcon(large, mini, NULL, 0);

	delete large;
	delete mini;
}


void
IconView::_RemoveIcon()
{
	_SetIcon(NULL, NULL, NULL, 0, true);
}


void
IconView::_StartWatching()
{
	if (Looper() == NULL) {
		// we are not a valid messenger yet
		return;
	}

	if (fHasRef) {
		BNode node(&fRef);
		node_ref nodeRef;
		if (node.InitCheck() == B_OK
			&& node.GetNodeRef(&nodeRef) == B_OK)
			watch_node(&nodeRef, B_WATCH_ATTR, this);
	} else if (fHasType)
		BMimeType::StartWatching(this);
}


void
IconView::_StopWatching()
{
	if (fHasRef)
		stop_watching(this);
	else if (fHasType)
		BMimeType::StopWatching(this);
}


#if __GNUC__ == 2

status_t
IconView::SetTarget(BMessenger target)
{
	return BControl::SetTarget(target);
}


status_t
IconView::SetTarget(const BHandler* handler, const BLooper* looper = NULL)
{
	return BControl::SetTarget(handler,
		looper);
}

#endif

