/*
 * Copyright 2001-2013, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers, mflerackers@androme.be
 *		Stephan Aßmus, superstippi@gmx.de
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */

/*!	BControl is the base class for user-event handling objects. */


#include <stdlib.h>
#include <string.h>

#include <Bitmap.h>
#include <Control.h>
#include <ObjectList.h>
#include <PropertyInfo.h>
#include <Window.h>

#include <AutoDeleter.h>
#include <binary_compatibility/Interface.h>


static property_info sPropertyList[] = {
	{
		"Enabled",
		{ B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0,
		{ B_BOOL_TYPE }
	},
	{
		"Label",
		{ B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0,
		{ B_STRING_TYPE }
	},
	{
		"Value",
		{ B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0,
		{ B_INT32_TYPE }
	},
	{}
};


struct BControl::IconData {
public:
	IconData()
		:
		fEnabledBitmaps(8, true),
		fDisabledBitmaps(8, true)
	{
	}

	~IconData()
	{
		DeleteBitmaps();
	}

	BBitmap* CreateBitmap(const BRect& bounds, color_space colorSpace,
		uint32 which)
	{
		BBitmap* bitmap = new(std::nothrow) BBitmap(bounds, colorSpace);
		if (bitmap == NULL || !bitmap->IsValid() || !SetBitmap(bitmap, which)) {
			delete bitmap;
			return NULL;
		}

		return bitmap;
	}

	BBitmap* CopyBitmap(const BBitmap& bitmapToClone, uint32 which)
	{
		BBitmap* bitmap = new(std::nothrow) BBitmap(bitmapToClone);
		if (bitmap == NULL || !bitmap->IsValid() || !SetBitmap(bitmap, which)) {
			delete bitmap;
			return NULL;
		}

		return bitmap;
	}

	bool SetBitmap(BBitmap* bitmap, uint32 which)
	{
		IconList& list = (which & B_DISABLED_ICON_BITMAP) == 0
			? fEnabledBitmaps : fDisabledBitmaps;
		which &= ~uint32(B_DISABLED_ICON_BITMAP);

		int32 count = list.CountItems();
		if ((int32)which < count) {
			list.ReplaceItem(which, bitmap);
			return true;
		}

		while (list.CountItems() < (int32)which) {
			if (!list.AddItem((BBitmap*)NULL))
				return false;
		}

		return list.AddItem(bitmap);
	}

	BBitmap* Bitmap(uint32 which) const
	{
		const IconList& list = (which & B_DISABLED_ICON_BITMAP) == 0
			? fEnabledBitmaps : fDisabledBitmaps;
		return list.ItemAt(which & ~uint32(B_DISABLED_ICON_BITMAP));
	}

	void DeleteBitmaps()
	{
		fEnabledBitmaps.MakeEmpty(true);
		fDisabledBitmaps.MakeEmpty(true);
	}

private:
	typedef BObjectList<BBitmap> IconList;

private:
	IconList	fEnabledBitmaps;
	IconList	fDisabledBitmaps;
};


BControl::BControl(BRect frame, const char *name, const char *label,
	BMessage *message, uint32 resizingMode, uint32 flags)
	: BView(frame, name, resizingMode, flags)
{
	InitData(NULL);

	SetLabel(label);
	SetMessage(message);
}


BControl::BControl(const char *name, const char *label, BMessage *message,
	uint32 flags)
	: BView(name, flags)
{
	InitData(NULL);

	SetLabel(label);
	SetMessage(message);
}


BControl::~BControl()
{
	free(fLabel);
	delete fIconData;
	SetMessage(NULL);
}


BControl::BControl(BMessage *archive)
	: BView(archive)
{
	InitData(archive);

	BMessage message;
	if (archive->FindMessage("_msg", &message) == B_OK)
		SetMessage(new BMessage(message));

	const char *label;
	if (archive->FindString("_label", &label) == B_OK)
		SetLabel(label);

	int32 value;
	if (archive->FindInt32("_val", &value) == B_OK)
		SetValue(value);

	bool toggle;
	if (archive->FindBool("_disable", &toggle) == B_OK)
		SetEnabled(!toggle);

	if (archive->FindBool("be:wants_nav", &toggle) == B_OK)
		fWantsNav = toggle;
}


BArchivable *
BControl::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BControl"))
		return new BControl(archive);

	return NULL;
}


status_t
BControl::Archive(BMessage *archive, bool deep) const
{
	status_t status = BView::Archive(archive, deep);

	if (status == B_OK && Message())
		status = archive->AddMessage("_msg", Message());

	if (status == B_OK && fLabel)
		status = archive->AddString("_label", fLabel);

	if (status == B_OK && fValue != B_CONTROL_OFF)
		status = archive->AddInt32("_val", fValue);

	if (status == B_OK && !fEnabled)
		status = archive->AddBool("_disable", true);

	return status;
}


void
BControl::WindowActivated(bool active)
{
	BView::WindowActivated(active);

	if (IsFocus())
		Invalidate();
}


void
BControl::AttachedToWindow()
{
	rgb_color color;

	BView* parent = Parent();
	if (parent != NULL) {
		// inherit the color from parent
		color = parent->ViewColor();
		if (color == B_TRANSPARENT_COLOR)
			color = ui_color(B_PANEL_BACKGROUND_COLOR);
	} else
		color = ui_color(B_PANEL_BACKGROUND_COLOR);

	SetViewColor(color);
	SetLowColor(color);

	if (!Messenger().IsValid())
		SetTarget(Window());

	BView::AttachedToWindow();
}


void
BControl::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}


void
BControl::AllAttached()
{
	BView::AllAttached();
}


void
BControl::AllDetached()
{
	BView::AllDetached();
}


void
BControl::MessageReceived(BMessage *message)
{
	if (message->what == B_GET_PROPERTY || message->what == B_SET_PROPERTY) {
		BMessage reply(B_REPLY);
		bool handled = false;

		BMessage specifier;
		int32 index;
		int32 form;
		const char *property;
		if (message->GetCurrentSpecifier(&index, &specifier, &form, &property) == B_OK) {
			if (strcmp(property, "Label") == 0) {
				if (message->what == B_GET_PROPERTY) {
					reply.AddString("result", fLabel);
					handled = true;
				} else {
					// B_SET_PROPERTY
					const char *label;
					if (message->FindString("data", &label) == B_OK) {
						SetLabel(label);
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			} else if (strcmp(property, "Value") == 0) {
				if (message->what == B_GET_PROPERTY) {
					reply.AddInt32("result", fValue);
					handled = true;
				} else {
					// B_SET_PROPERTY
					int32 value;
					if (message->FindInt32("data", &value) == B_OK) {
						SetValue(value);
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			} else if (strcmp(property, "Enabled") == 0) {
				if (message->what == B_GET_PROPERTY) {
					reply.AddBool("result", fEnabled);
					handled = true;
				} else {
					// B_SET_PROPERTY
					bool enabled;
					if (message->FindBool("data", &enabled) == B_OK) {
						SetEnabled(enabled);
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			}
		}

		if (handled) {
			message->SendReply(&reply);
			return;
		}
	}

	BView::MessageReceived(message);
}


void
BControl::MakeFocus(bool focused)
{
	if (focused == IsFocus())
		return;

	BView::MakeFocus(focused);

 	if (Window()) {
		fFocusChanging = true;
		Invalidate(Bounds());
		Flush();
		fFocusChanging = false;
	}
}


void
BControl::KeyDown(const char *bytes, int32 numBytes)
{
	if (*bytes == B_ENTER || *bytes == B_SPACE) {
		if (!fEnabled)
			return;

		SetValue(Value() ? B_CONTROL_OFF : B_CONTROL_ON);
		Invoke();
	} else
		BView::KeyDown(bytes, numBytes);
}


void
BControl::MouseDown(BPoint point)
{
	BView::MouseDown(point);
}


void
BControl::MouseUp(BPoint point)
{
	BView::MouseUp(point);
}


void
BControl::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	BView::MouseMoved(point, transit, message);
}


void
BControl::SetLabel(const char *label)
{
	if (label != NULL && !label[0])
		label = NULL;

	// Has the label been changed?
	if ((fLabel && label && !strcmp(fLabel, label))
		|| ((fLabel == NULL || !fLabel[0]) && label == NULL))
		return;

	free(fLabel);
	fLabel = label ? strdup(label) : NULL;

	InvalidateLayout();
	Invalidate();
}


const char *
BControl::Label() const
{
	return fLabel;
}


void
BControl::SetValue(int32 value)
{
	if (value == fValue)
		return;

	fValue = value;
	Invalidate();
}


void
BControl::SetValueNoUpdate(int32 value)
{
	fValue = value;
}


int32
BControl::Value() const
{
	return fValue;
}


void
BControl::SetEnabled(bool enabled)
{
	if (fEnabled == enabled)
		return;

	fEnabled = enabled;

	if (fEnabled && fWantsNav)
		SetFlags(Flags() | B_NAVIGABLE);
	else if (!fEnabled && (Flags() & B_NAVIGABLE)) {
		fWantsNav = true;
		SetFlags(Flags() & ~B_NAVIGABLE);
	} else
		fWantsNav = false;

	if (Window()) {
		Invalidate(Bounds());
		Flush();
	}
}


bool
BControl::IsEnabled() const
{
	return fEnabled;
}


void
BControl::GetPreferredSize(float *_width, float *_height)
{
	BView::GetPreferredSize(_width, _height);
}


void
BControl::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}


status_t
BControl::Invoke(BMessage *message)
{
	bool notify = false;
	uint32 kind = InvokeKind(&notify);

	if (!message && !notify)
		message = Message();

	BMessage clone(kind);

	if (!message) {
		if (!IsWatched())
			return B_BAD_VALUE;
	} else
		clone = *message;

	clone.AddInt64("when", (int64)system_time());
	clone.AddPointer("source", this);
	clone.AddInt32("be:value", fValue);
	clone.AddMessenger("be:sender", BMessenger(this));

	// ToDo: is this correct? If message == NULL (even if IsWatched()), we always return B_BAD_VALUE
	status_t err;
	if (message)
		err = BInvoker::Invoke(&clone);
	else
		err = B_BAD_VALUE;

	// TODO: asynchronous messaging
	SendNotices(kind, &clone);

	return err;
}


BHandler *
BControl::ResolveSpecifier(BMessage *message, int32 index,
	BMessage *specifier, int32 what, const char *property)
{
	BPropertyInfo propInfo(sPropertyList);

	if (propInfo.FindMatch(message, 0, specifier, what, property) >= B_OK)
		return this;

	return BView::ResolveSpecifier(message, index, specifier, what,
		property);
}


status_t
BControl::GetSupportedSuites(BMessage *message)
{
	message->AddString("suites", "suite/vnd.Be-control");

	BPropertyInfo propInfo(sPropertyList);
	message->AddFlat("messages", &propInfo);

	return BView::GetSupportedSuites(message);
}


status_t
BControl::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= BControl::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= BControl::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= BControl::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= BControl::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= BControl::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			BControl::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
}
		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BControl::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			BControl::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			BControl::DoLayout();
			return B_OK;
		}
		case PERFORM_CODE_SET_ICON:
		{
			perform_data_set_icon* data = (perform_data_set_icon*)_data;
			return BControl::SetIcon(data->icon, data->flags);
		}
	}

	return BView::Perform(code, _data);
}


status_t
BControl::SetIcon(const BBitmap* bitmap, uint32 flags)
{
	if (bitmap == NULL) {
		delete fIconData;
		fIconData = NULL;
		return B_OK;
	}

	if (!bitmap->IsValid())
		return B_BAD_VALUE;

	// check the color space
	bool hasAlpha = false;
	bool canUseForMakeBitmaps = false;

	switch (bitmap->ColorSpace()) {
		case B_RGBA32:
		case B_RGBA32_BIG:
			hasAlpha = true;
			// fall through
		case B_RGB32:
		case B_RGB32_BIG:
			canUseForMakeBitmaps = true;
			break;

		case B_UVLA32:
		case B_LABA32:
		case B_HSIA32:
		case B_HSVA32:
		case B_HLSA32:
		case B_CMYA32:
		case B_RGBA15:
		case B_RGBA15_BIG:
		case B_CMAP8:
			hasAlpha = true;
			break;

		default:
			break;
	}

	BBitmap* trimmedBitmap = NULL;

	// trim the bitmap, if requested and the bitmap actually has alpha
	status_t error;
	if ((flags & (B_TRIM_ICON_BITMAP | B_TRIM_ICON_BITMAP_KEEP_ASPECT)) != 0
		&& hasAlpha) {
		if (bitmap->ColorSpace() == B_RGBA32) {
			error = _TrimBitmap(bitmap,
				(flags & B_TRIM_ICON_BITMAP_KEEP_ASPECT) != 0, trimmedBitmap);
		} else {
			BBitmap* rgb32Bitmap = _ConvertToRGB32(bitmap, true);
			if (rgb32Bitmap != NULL) {
				error = _TrimBitmap(rgb32Bitmap,
					(flags & B_TRIM_ICON_BITMAP_KEEP_ASPECT) != 0,
					trimmedBitmap);
				delete rgb32Bitmap;
			} else
				error = B_NO_MEMORY;
		}

		if (error != B_OK)
			return error;

		bitmap = trimmedBitmap;
		canUseForMakeBitmaps = true;
	}

	// create the bitmaps
	if (canUseForMakeBitmaps) {
		error = _MakeBitmaps(bitmap, flags);
	} else {
		if (BBitmap* rgb32Bitmap = _ConvertToRGB32(bitmap, true)) {
			error = _MakeBitmaps(rgb32Bitmap, flags);
			delete rgb32Bitmap;
		} else
			error = B_NO_MEMORY;
	}

	delete trimmedBitmap;

	InvalidateLayout();
	Invalidate();

	return error;
}


status_t
BControl::SetIconBitmap(const BBitmap* bitmap, uint32 which, uint32 flags)
{
	if (fIconData == NULL) {
		fIconData = new(std::nothrow) IconData;
		if (fIconData == NULL)
			return B_NO_MEMORY;
	}

	BBitmap* ourBitmap = NULL;
	if (bitmap != NULL) {
		if (!bitmap->IsValid())
			return B_BAD_VALUE;

		if ((flags & B_KEEP_ICON_BITMAP) != 0) {
			ourBitmap = const_cast<BBitmap*>(bitmap);
		} else {
			ourBitmap = _ConvertToRGB32(bitmap);
			if (ourBitmap == NULL)
				return B_NO_MEMORY;
		}
	}

	if (!fIconData->SetBitmap(ourBitmap, which)) {
		if (ourBitmap != bitmap)
			delete ourBitmap;
		return B_NO_MEMORY;
	}

	InvalidateLayout();
	Invalidate();

	return B_OK;
}
	

const BBitmap*
BControl::IconBitmap(uint32 which) const
{
	return fIconData != NULL ? fIconData->Bitmap(which) : NULL;
}


bool
BControl::IsFocusChanging() const
{
	return fFocusChanging;
}


bool
BControl::IsTracking() const
{
	return fTracking;
}


void
BControl::SetTracking(bool state)
{
	fTracking = state;
}


extern "C" status_t
B_IF_GCC_2(_ReservedControl1__8BControl, _ZN8BControl17_ReservedControl1Ev)(
	BControl* control, const BBitmap* icon, uint32 flags)
{
	// SetIcon()
	perform_data_set_icon data;
	data.icon = icon;
	data.flags = flags;
	return control->Perform(PERFORM_CODE_SET_ICON, &data);
}


/*static*/ BBitmap*
BControl::_ConvertToRGB32(const BBitmap* bitmap, bool noAppServerLink)
{
	BBitmap* rgb32Bitmap = new(std::nothrow) BBitmap(bitmap->Bounds(),
		noAppServerLink ? B_BITMAP_NO_SERVER_LINK : 0, B_RGBA32);
	if (rgb32Bitmap == NULL)
		return NULL;

	if (!rgb32Bitmap->IsValid() || rgb32Bitmap->ImportBits(bitmap)!= B_OK) {
		delete rgb32Bitmap;
		return NULL;
	}

	return rgb32Bitmap;
}


/*static*/ status_t
BControl::_TrimBitmap(const BBitmap* bitmap, bool keepAspect,
	BBitmap*& _trimmedBitmap)
{
	if (bitmap == NULL || !bitmap->IsValid()
		|| bitmap->ColorSpace() != B_RGBA32) {
		return B_BAD_VALUE;
	}

	uint8* bits = (uint8*)bitmap->Bits();
	uint32 bpr = bitmap->BytesPerRow();
	uint32 width = bitmap->Bounds().IntegerWidth() + 1;
	uint32 height = bitmap->Bounds().IntegerHeight() + 1;
	BRect trimmed(INT32_MAX, INT32_MAX, INT32_MIN, INT32_MIN);

	for (uint32 y = 0; y < height; y++) {
		uint8* b = bits + 3;
		bool rowHasAlpha = false;
		for (uint32 x = 0; x < width; x++) {
			if (*b) {
				rowHasAlpha = true;
				if (x < trimmed.left)
					trimmed.left = x;
				if (x > trimmed.right)
					trimmed.right = x;
			}
			b += 4;
		}
		if (rowHasAlpha) {
			if (y < trimmed.top)
				trimmed.top = y;
			if (y > trimmed.bottom)
				trimmed.bottom = y;
		}
		bits += bpr;
	}

	if (!trimmed.IsValid())
		return B_BAD_VALUE;

	if (keepAspect) {
		float minInset = trimmed.left;
		minInset = min_c(minInset, trimmed.top);
		minInset = min_c(minInset, bitmap->Bounds().right - trimmed.right);
		minInset = min_c(minInset, bitmap->Bounds().bottom - trimmed.bottom);
		trimmed = bitmap->Bounds().InsetByCopy(minInset, minInset);
	}
	trimmed = trimmed & bitmap->Bounds();

	BBitmap* trimmedBitmap = new(std::nothrow) BBitmap(
		trimmed.OffsetToCopy(B_ORIGIN), B_BITMAP_NO_SERVER_LINK, B_RGBA32);
	if (trimmedBitmap == NULL)
		return B_NO_MEMORY;

	bits = (uint8*)bitmap->Bits();
	bits += 4 * (int32)trimmed.left + bpr * (int32)trimmed.top;
	uint8* dst = (uint8*)trimmedBitmap->Bits();
	uint32 trimmedWidth = trimmedBitmap->Bounds().IntegerWidth() + 1;
	uint32 trimmedHeight = trimmedBitmap->Bounds().IntegerHeight() + 1;
	uint32 trimmedBPR = trimmedBitmap->BytesPerRow();
	for (uint32 y = 0; y < trimmedHeight; y++) {
		memcpy(dst, bits, trimmedWidth * 4);
		dst += trimmedBPR;
		bits += bpr;
	}

	_trimmedBitmap = trimmedBitmap;
	return B_OK;
}


status_t
BControl::_MakeBitmaps(const BBitmap* bitmap, uint32 flags)
{
	// make our own versions of the bitmap
	BRect b(bitmap->Bounds());

	IconData* iconData = new(std::nothrow) IconData;
	if (iconData == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<IconData> iconDataDeleter(iconData);

	color_space format = bitmap->ColorSpace();
	BBitmap* normalBitmap = iconData->CreateBitmap(b, format,
		B_INACTIVE_ICON_BITMAP);
	if (normalBitmap == NULL)
		return B_NO_MEMORY;

	BBitmap* disabledBitmap = NULL;
	if ((flags & B_CREATE_DISABLED_ICON_BITMAPS) != 0) {
		disabledBitmap = iconData->CreateBitmap(b, format,
			B_INACTIVE_ICON_BITMAP | B_DISABLED_ICON_BITMAP);
		if (disabledBitmap == NULL)
			return B_NO_MEMORY;
	}

	BBitmap* clickedBitmap = NULL;
	if ((flags & (B_CREATE_ACTIVE_ICON_BITMAP
			| B_CREATE_PARTIALLY_ACTIVE_ICON_BITMAP)) != 0) {
		clickedBitmap = iconData->CreateBitmap(b, format, B_ACTIVE_ICON_BITMAP);
		if (clickedBitmap == NULL)
			return B_NO_MEMORY;
	}

	BBitmap* disabledClickedBitmap = NULL;
	if (disabledBitmap != NULL && clickedBitmap != NULL) {
		disabledClickedBitmap = iconData->CreateBitmap(b, format,
			B_ACTIVE_ICON_BITMAP | B_DISABLED_ICON_BITMAP);
		if (disabledClickedBitmap == NULL)
			return B_NO_MEMORY;
	}

	// copy bitmaps from file bitmap
	uint8* nBits = normalBitmap != NULL ? (uint8*)normalBitmap->Bits() : NULL;
	uint8* dBits = disabledBitmap != NULL
		? (uint8*)disabledBitmap->Bits() : NULL;
	uint8* cBits = clickedBitmap != NULL ? (uint8*)clickedBitmap->Bits() : NULL;
	uint8* dcBits = disabledClickedBitmap != NULL
		? (uint8*)disabledClickedBitmap->Bits() : NULL;
	uint8* fBits = (uint8*)bitmap->Bits();
	int32 nbpr = normalBitmap->BytesPerRow();
	int32 fbpr = bitmap->BytesPerRow();
	int32 pixels = b.IntegerWidth() + 1;
	int32 lines = b.IntegerHeight() + 1;
	if (format == B_RGB32 || format == B_RGB32_BIG) {
		// nontransparent version

		// iterate over color components
		for (int32 y = 0; y < lines; y++) {
			for (int32 x = 0; x < pixels; x++) {
				int32 nOffset = 4 * x;
				int32 fOffset = 4 * x;
				nBits[nOffset + 0] = fBits[fOffset + 0];
				nBits[nOffset + 1] = fBits[fOffset + 1];
				nBits[nOffset + 2] = fBits[fOffset + 2];
				nBits[nOffset + 3] = 255;

				// clicked bits are darker (lame method...)
				if (cBits != NULL) {
					cBits[nOffset + 0] = uint8((float)nBits[nOffset + 0] * 0.8);
					cBits[nOffset + 1] = uint8((float)nBits[nOffset + 1] * 0.8);
					cBits[nOffset + 2] = uint8((float)nBits[nOffset + 2] * 0.8);
					cBits[nOffset + 3] = 255;
				}

				// disabled bits have less contrast (lame method...)
				if (dBits != NULL) {
					uint8 grey = 216;
					float dist = (nBits[nOffset + 0] - grey) * 0.4;
					dBits[nOffset + 0] = (uint8)(grey + dist);
					dist = (nBits[nOffset + 1] - grey) * 0.4;
					dBits[nOffset + 1] = (uint8)(grey + dist);
					dist = (nBits[nOffset + 2] - grey) * 0.4;
					dBits[nOffset + 2] = (uint8)(grey + dist);
					dBits[nOffset + 3] = 255;
				}

				// disabled bits have less contrast (lame method...)
				if (dcBits != NULL) {
					uint8 grey = 188;
					float dist = (nBits[nOffset + 0] - grey) * 0.4;
					dcBits[nOffset + 0] = (uint8)(grey + dist);
					dist = (nBits[nOffset + 1] - grey) * 0.4;
					dcBits[nOffset + 1] = (uint8)(grey + dist);
					dist = (nBits[nOffset + 2] - grey) * 0.4;
					dcBits[nOffset + 2] = (uint8)(grey + dist);
					dcBits[nOffset + 3] = 255;
				}
			}
			fBits += fbpr;
			nBits += nbpr;
			if (cBits != NULL)
				cBits += nbpr;
			if (dBits != NULL)
				dBits += nbpr;
			if (dcBits != NULL)
				dcBits += nbpr;
		}
	} else if (format == B_RGBA32 || format == B_RGBA32_BIG) {
		// transparent version

		// iterate over color components
		for (int32 y = 0; y < lines; y++) {
			for (int32 x = 0; x < pixels; x++) {
				int32 nOffset = 4 * x;
				int32 fOffset = 4 * x;
				nBits[nOffset + 0] = fBits[fOffset + 0];
				nBits[nOffset + 1] = fBits[fOffset + 1];
				nBits[nOffset + 2] = fBits[fOffset + 2];
				nBits[nOffset + 3] = fBits[fOffset + 3];

				// clicked bits are darker (lame method...)
				if (cBits != NULL) {
					cBits[nOffset + 0] = (uint8)(nBits[nOffset + 0] * 0.8);
					cBits[nOffset + 1] = (uint8)(nBits[nOffset + 1] * 0.8);
					cBits[nOffset + 2] = (uint8)(nBits[nOffset + 2] * 0.8);
					cBits[nOffset + 3] = fBits[fOffset + 3];
				}

				// disabled bits have less opacity
				if (dBits != NULL) {
					uint8 grey = ((uint16)nBits[nOffset + 0] * 10
					    + nBits[nOffset + 1] * 60
						+ nBits[nOffset + 2] * 30) / 100;
					float dist = (nBits[nOffset + 0] - grey) * 0.3;
					dBits[nOffset + 0] = (uint8)(grey + dist);
					dist = (nBits[nOffset + 1] - grey) * 0.3;
					dBits[nOffset + 1] = (uint8)(grey + dist);
					dist = (nBits[nOffset + 2] - grey) * 0.3;
					dBits[nOffset + 2] = (uint8)(grey + dist);
					dBits[nOffset + 3] = (uint8)(fBits[fOffset + 3] * 0.3);
				}

				// disabled bits have less contrast (lame method...)
				if (dcBits != NULL) {
					dcBits[nOffset + 0] = (uint8)(dBits[nOffset + 0] * 0.8);
					dcBits[nOffset + 1] = (uint8)(dBits[nOffset + 1] * 0.8);
					dcBits[nOffset + 2] = (uint8)(dBits[nOffset + 2] * 0.8);
					dcBits[nOffset + 3] = (uint8)(fBits[fOffset + 3] * 0.3);
				}
			}
			fBits += fbpr;
			nBits += nbpr;
			if (cBits != NULL)
				cBits += nbpr;
			if (dBits != NULL)
				dBits += nbpr;
			if (dcBits != NULL)
				dcBits += nbpr;
		}
	} else {
		// unsupported format
		return B_BAD_VALUE;
	}

	// make the partially-on bitmaps a copy of the on bitmaps
	if ((flags & B_CREATE_PARTIALLY_ACTIVE_ICON_BITMAP) != 0) {
		if (iconData->CopyBitmap(clickedBitmap,
				B_PARTIALLY_ACTIVATE_ICON_BITMAP) == NULL) {
			return B_NO_MEMORY;
		}
		if ((flags & B_CREATE_DISABLED_ICON_BITMAPS) != 0) {
			if (iconData->CopyBitmap(disabledClickedBitmap,
					B_PARTIALLY_ACTIVATE_ICON_BITMAP | B_DISABLED_ICON_BITMAP)
					== NULL) {
				return B_NO_MEMORY;
			}
		}
	}

	delete fIconData;
	fIconData = iconDataDeleter.Detach();
	return B_OK;
}


void BControl::_ReservedControl2() {}
void BControl::_ReservedControl3() {}
void BControl::_ReservedControl4() {}


BControl &
BControl::operator=(const BControl &)
{
	return *this;
}


void
BControl::InitData(BMessage *data)
{
	fLabel = NULL;
	SetLabel(B_EMPTY_STRING);
	fValue = B_CONTROL_OFF;
	fEnabled = true;
	fFocusChanging = false;
	fTracking = false;
	fWantsNav = Flags() & B_NAVIGABLE;
	fIconData = NULL;

	if (data && data->HasString("_fname"))
		SetFont(be_plain_font, B_FONT_FAMILY_AND_STYLE);
}

