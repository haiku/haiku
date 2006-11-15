/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "IconView.h"
#include "MimeTypeListView.h"

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
#	include <IconUtils.h>
#endif

#include <Application.h>
#include <AppFileInfo.h>
#include <Bitmap.h>
#include <MenuItem.h>
#include <Mime.h>
#include <NodeMonitor.h>
#include <PopUpMenu.h>
#include <Resources.h>
#include <Roster.h>

#include <new>
#include <stdlib.h>
#include <string.h>

using namespace std;


#ifdef HAIKU_TARGET_PLATFORM_HAIKU
status_t
icon_for_type(BMimeType& type, uint8** _data, size_t* _size,
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
	}
	if (_source)
		*_source = source;

	return source != kNoIcon ? B_OK : B_ERROR;
}
#endif


status_t
icon_for_type(BMimeType& type, BBitmap& bitmap, icon_size size,
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
Icon::SetTo(BAppFileInfo& info, const char* type)
{
	Unset();

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	uint8* data;
	size_t size;
	if (info.GetIconForType(type, &data, &size) == B_OK) {
		// we have the vector icon, no need to get the rest
		AdoptData(data, size);
		return;
	}
#endif

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
Icon::SetTo(entry_ref& ref, const char* type)
{
	Unset();

	BFile file(&ref, B_READ_ONLY);
	BAppFileInfo info(&file);
	if (file.InitCheck() == B_OK
		&& info.InitCheck() == B_OK)
		SetTo(info, type);
}


status_t
Icon::CopyTo(BAppFileInfo& info, const char* type, bool force)
{
	status_t status = B_OK;

	if (fLarge != NULL || force)
		status = info.SetIconForType(type, fLarge, B_LARGE_ICON);
	if (fMini != NULL || force)
		status = info.SetIconForType(type, fMini, B_MINI_ICON);
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	if (fData != NULL || force)
		status = info.SetIconForType(type, fData, fSize);
#endif
	return status;
}


status_t
Icon::CopyTo(entry_ref& ref, const char* type, bool force)
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
Icon::CopyTo(BMessage& message)
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
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	if (status == B_OK && fData != NULL)
		status = message.AddData("icon", B_VECTOR_ICON_TYPE, fData, fSize);
#endif

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
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
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
#endif
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

	*_data = data;
	*_size = fSize;
	return B_OK;
}


status_t
Icon::GetIcon(BBitmap* bitmap) const
{
	if (bitmap == NULL)
		return B_BAD_VALUE;

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	if (fData != NULL && BIconUtils::GetVectorIcon(fData, fSize, bitmap) == B_OK)
		return B_OK;
#endif

	int32 width = bitmap->Bounds().IntegerWidth() + 1;

	if (width == B_LARGE_ICON && fLarge != NULL) {
		bitmap->SetBits(fLarge->Bits(), fLarge->BitsLength(), 0, fLarge->ColorSpace());
		return B_OK;
	}
	if (width == B_MINI_ICON && fMini != NULL) {
		bitmap->SetBits(fMini->Bits(), fMini->BitsLength(), 0, fMini->ColorSpace());
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
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	int32 kSpace = B_RGB32;
#else
	int32 kSpace = B_CMAP8;
#endif
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


IconView::IconView(BRect rect, const char* name, uint32 resizeMode, uint32 flags)
	: BView(rect, name, resizeMode, B_WILL_DRAW | flags),
	fIconSize(B_LARGE_ICON),
	fIcon(NULL),
	fHeapIcon(NULL),
	fIconData(NULL),
	fHasRef(false),
	fIsLive(false),
	fTracking(false),
	fDragging(false),
	fDropTarget(false),
	fEnabled(true)
{
}


IconView::~IconView()
{
	delete fIcon;
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
	if (fIsLive)
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
	if (message->WasDropped() && _AcceptsDrag(message)) {
		// set icon from message
		BBitmap* mini = NULL;
		BBitmap* large = NULL;
		const uint8* data = NULL;
		ssize_t size = 0;

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		message->FindData("icon", B_VECTOR_ICON_TYPE, (const void**)&data, &size);
#endif

		BMessage archive;
		if (message->FindMessage("icon/large", &archive) == B_OK)
			large = (BBitmap*)BBitmap::Instantiate(&archive);
		if (message->FindMessage("icon/mini", &archive) == B_OK)
			mini = (BBitmap*)BBitmap::Instantiate(&archive);

		if (large != NULL || mini != NULL || (data != NULL && size > 0))
			_SetIcon(large, mini, data, size);

		entry_ref ref;
		if (message->FindRef("refs", &ref) == B_OK)
			_SetIcon(&ref);

		return;
	}

	switch (message->what) {
		case kMsgIconInvoked:
			_AddOrEditIcon();
			break;
		case kMsgRemoveIcon:
			_RemoveIcon();
			break;

		case B_NODE_MONITOR:
		{
			int32 opcode;
			if (message->FindInt32("opcode", &opcode) != B_OK
				|| opcode != B_ATTR_CHANGED)
				break;

			const char* name;
			if (message->FindString("attr", &name) != B_OK)
				break;

			if (!strcmp(name, "BEOS:L:STD_ICON"))
				Update();
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


bool
IconView::_AcceptsDrag(const BMessage* message)
{
	if (!fEnabled)
		return false;

	type_code type;
	int32 count;
	if (message->GetInfo("refs", &type, &count) == B_OK && count == 1 && type == B_REF_TYPE) {
		// if we're bound to an entry, check that no one drops this to us
		entry_ref ref;
		if (fHasRef && message->FindRef("refs", &ref) == B_OK && fRef == ref)
			return false;

		return true;
	}

	if (message->GetInfo("icon/large", &type) == B_OK && type == B_MESSAGE_TYPE
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		|| message->GetInfo("icon", &type) == B_OK && type == B_VECTOR_ICON_TYPE
#endif
		|| message->GetInfo("icon/mini", &type) == B_OK && type == B_MESSAGE_TYPE)
		return true;

	return false;
}


BRect
IconView::_BitmapRect() const
{
	return BRect(0, 0, 31, 31);
}


void
IconView::Draw(BRect updateRect)
{
	SetDrawingMode(B_OP_ALPHA);

	if (fHeapIcon != NULL)
		DrawBitmap(fHeapIcon, _BitmapRect().LeftTop());
	else if (fIcon != NULL)
		DrawBitmap(fIcon, _BitmapRect().LeftTop());
	else if (!fDropTarget) {
		// draw frame so that the user knows here is something he
		// might be able to click on
		SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
		StrokeRect(Bounds());
	}

	if (IsFocus()) {
		// mark this view as a having focus
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		StrokeRect(_BitmapRect());
	}
	if (fDropTarget) {
		// mark this view as a drop target
		SetHighColor(0, 0, 0);
		SetPenSize(2);
		BRect rect = _BitmapRect();
// TODO: this is an incompatibility between R5 and Haiku and should be fixed!
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		rect.left++;
		rect.top++;
#else
		rect.right--;
		rect.bottom--;
#endif
		StrokeRect(rect);
		SetPenSize(1);
	}
}


void
IconView::GetPreferredSize(float* _width, float* _height)
{
	if (_width)
		*_width = ceilf(fIconSize);

	if (_height)
		*_height = ceilf(fIconSize);
}


void
IconView::MouseDown(BPoint where)
{
	if (!fEnabled)
		return;

	int32 buttons = B_PRIMARY_MOUSE_BUTTON;
	int32 clicks = 1;
	if (Looper() != NULL && Looper()->CurrentMessage() != NULL) {
		if (Looper()->CurrentMessage()->FindInt32("buttons", &buttons) != B_OK)
			buttons = B_PRIMARY_MOUSE_BUTTON;
		if (Looper()->CurrentMessage()->FindInt32("clicks", &clicks) != B_OK)
			clicks = 1;
	}

	if ((buttons & B_PRIMARY_MOUSE_BUTTON) != 0 && _BitmapRect().Contains(where)) {
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

		if (fIcon != NULL)
			menu->AddItem(new BMenuItem("Edit Icon" B_UTF8_ELLIPSIS, new BMessage(kMsgEditIcon)));
		else
			menu->AddItem(new BMenuItem("Add Icon" B_UTF8_ELLIPSIS, new BMessage(kMsgAddIcon)));

		menu->AddItem(new BMenuItem("Remove Icon", new BMessage(kMsgRemoveIcon)));
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
		if (fHasRef) {
			icon = new ::Icon;
			icon->SetTo(fRef, fFileType.Length() > 0 ? fFileType.String() : NULL);
		}

		icon->CopyTo(message);

		BBitmap *dragBitmap = new BBitmap(fIcon);
		DragMessage(&message, dragBitmap, B_OP_ALPHA, fDragPoint, this);
		fDragging = true;
		SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	}

	if (dragMessage != NULL && !fDragging && _AcceptsDrag(dragMessage)) {
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

	BView::KeyDown(bytes, numBytes);
}


void
IconView::MakeFocus(bool focus)
{
	if (focus != IsFocus())
		Invalidate();

	BView::MakeFocus(focus);
}


void
IconView::SetTo(entry_ref& ref, const char* fileType)
{
	if (fHasRef && fIsLive)
		_StopWatching();

	fHasRef = true;
	fRef = ref;

	if (fileType != NULL)
		fFileType = fileType;
	else
		fFileType = "";

	fIsLive = true;
	_StartWatching();
	Update();
}


void
IconView::SetTo(::Icon* icon)
{
	if (fIconData == icon)
		return;

	fIconData = icon;
	fHasRef = false;
	Update();
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
		const void* data = NULL;
		// TODO: get vector heap icon!
		if (resources != NULL)
			data = resources->LoadResource('ICON', "icon heap", NULL);
		if (data != NULL) {
			fHeapIcon = Icon::AllocateBitmap(B_LARGE_ICON, B_CMAP8);
			memcpy(fHeapIcon->Bits(), data, fHeapIcon->BitsLength());
		}
	} else {
		delete fHeapIcon;
		fHeapIcon = NULL;
	}
}


void
IconView::SetTarget(const BMessenger& target)
{
	fTarget = target;
}


void
IconView::SetEnabled(bool enabled)
{
	if (fEnabled == enabled)
		return;

	fEnabled = enabled;
	Invalidate();
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
		if (fIconData != NULL)
			fIconData->Unset();

		BFile file(&fRef, B_READ_ONLY);
		if (file.InitCheck() != B_OK)
			return;

		const char* fileType = fFileType.Length() > 0 ? fFileType.String() : NULL;

		BAppFileInfo info;
		if (info.SetTo(&file) != B_OK)
			return;

		icon = Icon::AllocateBitmap(fIconSize);
		if (icon != NULL && info.GetIconForType(fileType, icon, (icon_size)fIconSize) != B_OK) {
			delete icon;
			return;
		}

		if (fIconData != NULL)
			fIconData->SetTo(info, fileType);
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
IconView::Invoke()
{
	fTarget.SendMessage(kMsgIconInvoked);
}


Icon*
IconView::Icon()
{
	return fIconData;
}


void
IconView::_AddOrEditIcon()
{
	BMessage message;
	if (fIsLive) {
		// in live mode, Icon-O-Matic can change the icon directly, and
		// we'll pick it up via node monitoring
		message.what = B_REFS_RECEIVED;
		message.AddRef("refs", &fRef);
		if (fFileType.Length() > 0)
			message.AddString("file type", fFileType.String());
	} else {
		// in static mode, Icon-O-Matic need to return the buffer it
		// changed once its done
		message.what = 0;
		// TODO: this interface is yet to be determined
		message.AddMessenger("reply to", BMessenger(this));
	}

	be_roster->Launch("application/x-vnd.Haiku-icon_o_matic", &message);
}


void
IconView::_SetIcon(BBitmap* large, BBitmap* mini, const uint8* data, size_t size, bool force)
{
	if (fIsLive) {
		BFile file(&fRef, B_READ_WRITE);

		BAppFileInfo info(&file);
		if (info.InitCheck() == B_OK) {
			const char* type = fFileType.Length() > 0 ? fFileType.String() : NULL;

			if (large != NULL || force)
				info.SetIconForType(type, large, B_LARGE_ICON);
			if (mini != NULL || force)
				info.SetIconForType(type, mini, B_MINI_ICON);
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
			if (data != NULL || force)
				info.SetIconForType(type, data, size);
#endif
		}
		// the icon shown will be updated using node monitoring
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
}


void
IconView::_SetIcon(entry_ref* ref)
{
	// retrieve icons from file
	BFile file(ref, B_READ_ONLY);
	BAppFileInfo info(&file);
	if (file.InitCheck() != B_OK || info.InitCheck() != B_OK)
		return;

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	// try vector/PNG icon first
	uint8* data = NULL;
	size_t size = 0;
	if (info.GetIcon(&data, &size) == B_OK) {
		_SetIcon(NULL, NULL, data, size);
		free(data);
		return;
	}
#endif

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
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		if (icon_for_type(mimeType, &data, &size) != B_OK) {
			// only try large/mini icons when there is no vector icon
#endif
			if (large != NULL && icon_for_type(mimeType, *large, B_LARGE_ICON) == B_OK)
				hasLarge = true;
			if (mini != NULL && icon_for_type(mimeType, *mini, B_MINI_ICON) == B_OK)
				hasMini = true;
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		}
#endif
	}

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	if (data != NULL) {
		_SetIcon(NULL, NULL, data, size);
		free(data);
	} else
#endif
	if (hasLarge || hasMini)
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

	BNode node(&fRef);
	node_ref nodeRef;
	if (node.InitCheck() == B_OK
		&& node.GetNodeRef(&nodeRef) == B_OK)
		watch_node(&nodeRef, B_WATCH_ATTR, this);
}


void
IconView::_StopWatching()
{
	stop_watching(this);
}

