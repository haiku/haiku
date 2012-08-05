/*
 * Copyright 2006-2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de.
 */


#include "IconButton.h"

#include <new>
#include <stdio.h>

#include <Application.h>
#include <Bitmap.h>
#include <Control.h>
#include <ControlLook.h>
#include <Entry.h>
#include <IconUtils.h>
#include <Looper.h>
#include <Message.h>
#include <Mime.h>
#include <Path.h>
#include <Region.h>
#include <Resources.h>
#include <Roster.h>
#include <TranslationUtils.h>
#include <Window.h>


namespace BPrivate {


enum {
	STATE_NONE			= 0x0000,
	STATE_PRESSED		= 0x0002,
	STATE_INSIDE		= 0x0008,
	STATE_FORCE_PRESSED	= 0x0010,
};



BIconButton::BIconButton(const char* name, const char* label,
	BMessage* message, BHandler* target)
	:
	BControl(name, label, message, B_WILL_DRAW),
	fButtonState(0),
	fNormalBitmap(NULL),
	fDisabledBitmap(NULL),
	fClickedBitmap(NULL),
	fDisabledClickedBitmap(NULL),
	fTargetCache(target)
{
	SetTarget(target);
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetViewColor(B_TRANSPARENT_32_BIT);
}


BIconButton::~BIconButton()
{
	_DeleteBitmaps();
}


void
BIconButton::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BView::MessageReceived(message);
			break;
	}
}


void
BIconButton::AttachedToWindow()
{
	rgb_color background = B_TRANSPARENT_COLOR;
	if (BView* parent = Parent()) {
		background = parent->ViewColor();
		if (background == B_TRANSPARENT_COLOR)
			background = parent->LowColor();
	}
	if (background == B_TRANSPARENT_COLOR)
		background = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetLowColor(background);

	SetTarget(fTargetCache);
	if (!Target())
		SetTarget(Window());
}


void
BIconButton::Draw(BRect updateRect)
{
	rgb_color background = LowColor();

	BRect r(Bounds());

	uint32 flags = 0;
	BBitmap* bitmap = fNormalBitmap;
	if (!IsEnabled()) {
		flags |= BControlLook::B_DISABLED;
		bitmap = fDisabledBitmap;
	}
	if (_HasFlags(STATE_PRESSED) || _HasFlags(STATE_FORCE_PRESSED))
		flags |= BControlLook::B_ACTIVATED;

	if (ShouldDrawBorder()) {
		DrawBorder(r, updateRect, background, flags);
		DrawBackground(r, updateRect, background, flags);
	} else {
		SetHighColor(background);
		FillRect(r);
	}

	if (bitmap && bitmap->IsValid()) {
		if (bitmap->ColorSpace() == B_RGBA32
			|| bitmap->ColorSpace() == B_RGBA32_BIG) {
			SetDrawingMode(B_OP_ALPHA);
			SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		}
		float x = r.left + floorf((r.Width()
			- bitmap->Bounds().Width()) / 2.0 + 0.5);
		float y = r.top + floorf((r.Height()
			- bitmap->Bounds().Height()) / 2.0 + 0.5);
		DrawBitmap(bitmap, BPoint(x, y));
	}
}


bool
BIconButton::ShouldDrawBorder() const
{
	return (IsEnabled() && (IsInside() || IsTracking()))
		|| _HasFlags(STATE_FORCE_PRESSED);
}


void
BIconButton::DrawBorder(BRect& frame, const BRect& updateRect,
	const rgb_color& backgroundColor, uint32 flags)
{
	be_control_look->DrawButtonFrame(this, frame, updateRect, backgroundColor,
		backgroundColor, flags);
}


void
BIconButton::DrawBackground(BRect& frame, const BRect& updateRect,
	const rgb_color& backgroundColor, uint32 flags)
{
	be_control_look->DrawButtonBackground(this, frame, updateRect,
		backgroundColor, flags);
}


void
BIconButton::MouseDown(BPoint where)
{
	if (!IsValid())
		return;

	if (IsEnabled()) {
		if (Bounds().Contains(where)) {
			SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
			_SetFlags(STATE_PRESSED, true);
			_SetTracking(true);
		} else {
			_SetFlags(STATE_PRESSED, false);
			_SetTracking(false);
		}
	}
}


void
BIconButton::MouseUp(BPoint where)
{
	if (!IsValid())
		return;

	if (IsEnabled() && _HasFlags(STATE_PRESSED)
		&& Bounds().Contains(where)) {
		Invoke();
	} else if (Bounds().Contains(where))
		SetInside(true);

	_SetFlags(STATE_PRESSED, false);
	_SetTracking(false);
}


void
BIconButton::MouseMoved(BPoint where, uint32 transit, const BMessage* message)
{
	if (!IsValid())
		return;

	uint32 buttons = 0;
	Window()->CurrentMessage()->FindInt32("buttons", (int32*)&buttons);
	// catch a mouse up event that we might have missed
	if (!buttons && _HasFlags(STATE_PRESSED)) {
		MouseUp(where);
		return;
	}
	if (buttons != 0 && !IsTracking())
		return;

	SetInside((transit == B_INSIDE_VIEW || transit == B_ENTERED_VIEW)
		&& IsEnabled());
	if (IsTracking())
		_SetFlags(STATE_PRESSED, Bounds().Contains(where));
}


void
BIconButton::GetPreferredSize(float* width, float* height)
{
	float minWidth = 0.0f;
	float minHeight = 0.0f;
	if (IsValid()) {
		minWidth += fNormalBitmap->Bounds().IntegerWidth() + 1.0f;
		minHeight += fNormalBitmap->Bounds().IntegerHeight() + 1.0f;
	}

	const float kMinSpace = 15.0f;
	if (minWidth < kMinSpace)
		minWidth = kMinSpace;
	if (minHeight < kMinSpace)
		minHeight = kMinSpace;

	float hPadding = max_c(6.0f, ceilf(minHeight / 4.0f));
	float vPadding = max_c(6.0f, ceilf(minWidth / 4.0f));

	if (Label() != NULL && Label()[0] != '\0') {
		font_height fh;
		GetFontHeight(&fh);
		minHeight += ceilf(fh.ascent + fh.descent) + vPadding;
		minWidth += StringWidth(Label()) + vPadding;
	}

	if (width)
		*width = minWidth + hPadding;
	if (height)
		*height = minHeight + vPadding;
}


BSize
BIconButton::MinSize()
{
	BSize size;
	GetPreferredSize(&size.width, &size.height);
	return size;
}


BSize
BIconButton::MaxSize()
{
	return MinSize();
}


status_t
BIconButton::Invoke(BMessage* message)
{
	if (message == NULL)
		message = Message();
	if (message != NULL) {
		BMessage clone(*message);
		clone.AddInt64("be:when", system_time());
		clone.AddPointer("be:source", (BView*)this);
		clone.AddInt32("be:value", Value());
		return BInvoker::Invoke(&clone);
	}
	return BInvoker::Invoke(message);
}


void
BIconButton::SetPressed(bool pressed)
{
	_SetFlags(STATE_FORCE_PRESSED, pressed);
}


bool
BIconButton::IsPressed() const
{
	return _HasFlags(STATE_FORCE_PRESSED);
}


status_t
BIconButton::SetIcon(int32 resourceID)
{
	app_info info;
	status_t status = be_app->GetAppInfo(&info);
	if (status != B_OK)
		return status;

	BResources resources(&info.ref);
	status = resources.InitCheck();
	if (status != B_OK)
		return status;

	size_t size;
	const void* data = resources.LoadResource(B_VECTOR_ICON_TYPE, resourceID,
		&size);
	if (data != NULL) {
		BBitmap bitmap(BRect(0, 0, 31, 31), B_BITMAP_NO_SERVER_LINK, B_RGBA32);
		status = bitmap.InitCheck();
		if (status != B_OK)
			return status;
		status = BIconUtils::GetVectorIcon(reinterpret_cast<const uint8*>(data),
			size, &bitmap);
		if (status != B_OK)
			return status;
		return SetIcon(&bitmap);
	}
//	const void* data = resources.LoadResource(B_BITMAP_TYPE, resourceID, &size);
	return B_ERROR;
}


status_t
BIconButton::SetIcon(const char* pathToBitmap)
{
	if (pathToBitmap == NULL)
		return B_BAD_VALUE;

	status_t status = B_BAD_VALUE;
	BBitmap* fileBitmap = NULL;
	// try to load bitmap from either relative or absolute path
	BEntry entry(pathToBitmap, true);
	if (!entry.Exists()) {
		app_info info;
		status = be_app->GetAppInfo(&info);
		if (status == B_OK) {
			BEntry app_entry(&info.ref, true);
			BPath path;
			app_entry.GetPath(&path);
			status = path.InitCheck();
			if (status == B_OK) {
				status = path.GetParent(&path);
				if (status == B_OK) {
					status = path.Append(pathToBitmap, true);
					if (status == B_OK)
						fileBitmap = BTranslationUtils::GetBitmap(path.Path());
					else {
						printf("BIconButton::SetIcon() - path.Append() failed: "
							"%s\n", strerror(status));
					}
				} else {
					printf("BIconButton::SetIcon() - path.GetParent() failed: "
						"%s\n", strerror(status));
				}
			} else {
				printf("BIconButton::SetIcon() - path.InitCheck() failed: "
					"%s\n", strerror(status));
			}
		} else {
			printf("BIconButton::SetIcon() - be_app->GetAppInfo() failed: "
				"%s\n", strerror(status));
		}
	} else
		fileBitmap = BTranslationUtils::GetBitmap(pathToBitmap);
	if (fileBitmap) {
		status = _MakeBitmaps(fileBitmap);
		delete fileBitmap;
	} else
		status = B_ERROR;
	return status;
}


status_t
BIconButton::SetIcon(const BBitmap* bitmap)
{
	if (bitmap && bitmap->ColorSpace() == B_CMAP8) {
		status_t status = bitmap->InitCheck();
		if (status >= B_OK) {
			if (BBitmap* rgb32Bitmap = _ConvertToRGB32(bitmap)) {
				status = _MakeBitmaps(rgb32Bitmap);
				delete rgb32Bitmap;
			} else
				status = B_NO_MEMORY;
		}
		return status;
	} else
		return _MakeBitmaps(bitmap);
}


status_t
BIconButton::SetIcon(const BMimeType* fileType, bool small)
{
	status_t status = fileType ? fileType->InitCheck() : B_BAD_VALUE;
	if (status >= B_OK) {
		BBitmap* mimeBitmap = new(std::nothrow) BBitmap(BRect(0.0, 0.0, 15.0,
			15.0), B_CMAP8);
		if (mimeBitmap && mimeBitmap->IsValid()) {
			status = fileType->GetIcon(mimeBitmap, small ? B_MINI_ICON
				: B_LARGE_ICON);
			if (status >= B_OK) {
				if (BBitmap* bitmap = _ConvertToRGB32(mimeBitmap)) {
					status = _MakeBitmaps(bitmap);
					delete bitmap;
				} else {
					printf("BIconButton::SetIcon() - B_RGB32 bitmap is not "
						"valid\n");
				}
			} else {
				printf("BIconButton::SetIcon() - fileType->GetIcon() failed: "
					"%s\n", strerror(status));
			}
		} else
			printf("BIconButton::SetIcon() - B_CMAP8 bitmap is not valid\n");
		delete mimeBitmap;
	} else {
		printf("BIconButton::SetIcon() - fileType is not valid: %s\n",
			strerror(status));
	}
	return status;
}


status_t
BIconButton::SetIcon(const unsigned char* bitsFromQuickRes,
	uint32 width, uint32 height, color_space format, bool convertToBW)
{
	status_t status = B_BAD_VALUE;
	if (bitsFromQuickRes && width > 0 && height > 0) {
		BBitmap* quickResBitmap = new(std::nothrow) BBitmap(BRect(0.0, 0.0,
			width - 1.0, height - 1.0), format);
		status = quickResBitmap ? quickResBitmap->InitCheck() : B_ERROR;
		if (status >= B_OK) {
			// It doesn't look right to copy BitsLength() bytes, but bitmaps
			// exported from QuickRes still contain their padding, so it is
			// all right.
			memcpy(quickResBitmap->Bits(), bitsFromQuickRes,
				quickResBitmap->BitsLength());
			if (format != B_RGB32 && format != B_RGBA32
				&& format != B_RGB32_BIG && format != B_RGBA32_BIG) {
				// colorspace needs conversion
				BBitmap* bitmap = new(std::nothrow) BBitmap(
					quickResBitmap->Bounds(), B_RGB32, true);
				if (bitmap && bitmap->IsValid()) {
					BView* helper = new BView(bitmap->Bounds(), "helper",
						B_FOLLOW_NONE, B_WILL_DRAW);
					if (bitmap->Lock()) {
						bitmap->AddChild(helper);
						helper->SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
						helper->FillRect(helper->Bounds());
						helper->SetDrawingMode(B_OP_OVER);
						helper->DrawBitmap(quickResBitmap, BPoint(0.0, 0.0));
						helper->Sync();
						bitmap->Unlock();
					}
					status = _MakeBitmaps(bitmap);
				} else {
					printf("BIconButton::SetIcon() - B_RGB32 bitmap is not "
						"valid\n");
				}
				delete bitmap;
			} else {
				// native colorspace (32 bits)
				if (convertToBW) {
					// convert to gray scale icon
					uint8* bits = (uint8*)quickResBitmap->Bits();
					uint32 bpr = quickResBitmap->BytesPerRow();
					for (uint32 y = 0; y < height; y++) {
						uint8* handle = bits;
						uint8 gray;
						for (uint32 x = 0; x < width; x++) {
							gray = uint8((116 * handle[0] + 600 * handle[1]
								+ 308 * handle[2]) / 1024);
							handle[0] = gray;
							handle[1] = gray;
							handle[2] = gray;
							handle += 4;
						}
						bits += bpr;
					}
				}
				status = _MakeBitmaps(quickResBitmap);
			}
		} else {
			printf("BIconButton::SetIcon() - error allocating bitmap: "
				"%s\n", strerror(status));
		}
		delete quickResBitmap;
	}
	return status;
}


void
BIconButton::ClearIcon()
{
	_DeleteBitmaps();
	_Update();
}


void
BIconButton::TrimIcon(bool keepAspect)
{
	if (fNormalBitmap == NULL)
		return;

	uint8* bits = (uint8*)fNormalBitmap->Bits();
	uint32 bpr = fNormalBitmap->BytesPerRow();
	uint32 width = fNormalBitmap->Bounds().IntegerWidth() + 1;
	uint32 height = fNormalBitmap->Bounds().IntegerHeight() + 1;
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
		return;
	if (keepAspect) {
		float minInset = trimmed.left;
		minInset = min_c(minInset, trimmed.top);
		minInset = min_c(minInset, fNormalBitmap->Bounds().right
			- trimmed.right);
		minInset = min_c(minInset, fNormalBitmap->Bounds().bottom
			- trimmed.bottom);
		trimmed = fNormalBitmap->Bounds().InsetByCopy(minInset, minInset);
	}
	trimmed = trimmed & fNormalBitmap->Bounds();
	BBitmap trimmedBitmap(trimmed.OffsetToCopy(B_ORIGIN),
		B_BITMAP_NO_SERVER_LINK, B_RGBA32);
	bits = (uint8*)fNormalBitmap->Bits();
	bits += 4 * (int32)trimmed.left + bpr * (int32)trimmed.top;
	uint8* dst = (uint8*)trimmedBitmap.Bits();
	uint32 trimmedWidth = trimmedBitmap.Bounds().IntegerWidth() + 1;
	uint32 trimmedHeight = trimmedBitmap.Bounds().IntegerHeight() + 1;
	uint32 trimmedBPR = trimmedBitmap.BytesPerRow();
	for (uint32 y = 0; y < trimmedHeight; y++) {
		memcpy(dst, bits, trimmedWidth * 4);
		dst += trimmedBPR;
		bits += bpr;
	}
	SetIcon(&trimmedBitmap);
}


bool
BIconButton::IsValid() const
{
	return (fNormalBitmap && fDisabledBitmap && fClickedBitmap
		&& fDisabledClickedBitmap
		&& fNormalBitmap->IsValid()
		&& fDisabledBitmap->IsValid()
		&& fClickedBitmap->IsValid()
		&& fDisabledClickedBitmap->IsValid());
}


BBitmap*
BIconButton::Bitmap() const
{
	BBitmap* bitmap = NULL;
	if (fNormalBitmap && fNormalBitmap->IsValid()) {
		bitmap = new(std::nothrow) BBitmap(fNormalBitmap);
		if (bitmap != NULL && bitmap->IsValid()) {
			// TODO: remove this functionality when we use real transparent
			// bitmaps
			uint8* bits = (uint8*)bitmap->Bits();
			uint32 bpr = bitmap->BytesPerRow();
			uint32 width = bitmap->Bounds().IntegerWidth() + 1;
			uint32 height = bitmap->Bounds().IntegerHeight() + 1;
			color_space format = bitmap->ColorSpace();
			if (format == B_CMAP8) {
				// replace gray with magic transparent index
			} else if (format == B_RGB32) {
				for (uint32 y = 0; y < height; y++) {
					uint8* bitsHandle = bits;
					for (uint32 x = 0; x < width; x++) {
						if (bitsHandle[0] == 216
							&& bitsHandle[1] == 216
							&& bitsHandle[2] == 216) {
							// make this pixel completely transparent
							bitsHandle[3] = 0;
						}
						bitsHandle += 4;
					}
					bits += bpr;
				}
			}
		} else {
			delete bitmap;
			bitmap = NULL;
		}
	}
	return bitmap;
}


void
BIconButton::SetValue(int32 value)
{
	BControl::SetValue(value);
	_SetFlags(STATE_PRESSED, value != 0);
}


void
BIconButton::SetEnabled(bool enabled)
{
	BControl::SetEnabled(enabled);
	if (!enabled) {
		SetInside(false);
		_SetTracking(false);
	}
}


// #pragma mark - protected


bool
BIconButton::IsInside() const
{
	return _HasFlags(STATE_INSIDE);
}


void
BIconButton::SetInside(bool inside)
{
	_SetFlags(STATE_INSIDE, inside);
}


// #pragma mark - private


BBitmap*
BIconButton::_ConvertToRGB32(const BBitmap* bitmap) const
{
	BBitmap* convertedBitmap = new(std::nothrow) BBitmap(bitmap->Bounds(),
		B_BITMAP_ACCEPTS_VIEWS, B_RGBA32);
	if (convertedBitmap && convertedBitmap->IsValid()) {
		memset(convertedBitmap->Bits(), 0, convertedBitmap->BitsLength());
		BView* helper = new BView(bitmap->Bounds(), "helper",
								  B_FOLLOW_NONE, B_WILL_DRAW);
		if (convertedBitmap->Lock()) {
			convertedBitmap->AddChild(helper);
			helper->SetDrawingMode(B_OP_OVER);
			helper->DrawBitmap(bitmap, BPoint(0.0, 0.0));
			helper->Sync();
			convertedBitmap->Unlock();
		}
	} else {
		delete convertedBitmap;
		convertedBitmap = NULL;
	}
	return convertedBitmap;
}


status_t
BIconButton::_MakeBitmaps(const BBitmap* bitmap)
{
	status_t status = bitmap ? bitmap->InitCheck() : B_BAD_VALUE;
	if (status == B_OK) {
		// make our own versions of the bitmap
		BRect b(bitmap->Bounds());
		_DeleteBitmaps();
		color_space format = bitmap->ColorSpace();
		fNormalBitmap = new(std::nothrow) BBitmap(b, format);
		fDisabledBitmap = new(std::nothrow) BBitmap(b, format);
		fClickedBitmap = new(std::nothrow) BBitmap(b, format);
		fDisabledClickedBitmap = new(std::nothrow) BBitmap(b, format);
		if (IsValid()) {
			// copy bitmaps from file bitmap
			uint8* nBits = (uint8*)fNormalBitmap->Bits();
			uint8* dBits = (uint8*)fDisabledBitmap->Bits();
			uint8* cBits = (uint8*)fClickedBitmap->Bits();
			uint8* dcBits = (uint8*)fDisabledClickedBitmap->Bits();
			uint8* fBits = (uint8*)bitmap->Bits();
			int32 nbpr = fNormalBitmap->BytesPerRow();
			int32 fbpr = bitmap->BytesPerRow();
			int32 pixels = b.IntegerWidth() + 1;
			int32 lines = b.IntegerHeight() + 1;
			// nontransparent version:
			if (format == B_RGB32 || format == B_RGB32_BIG) {
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
						cBits[nOffset + 0] = (uint8)((float)nBits[nOffset + 0]
							* 0.8);
						cBits[nOffset + 1] = (uint8)((float)nBits[nOffset + 1]
							* 0.8);
						cBits[nOffset + 2] = (uint8)((float)nBits[nOffset + 2]
							* 0.8);
						cBits[nOffset + 3] = 255;
						// disabled bits have less contrast (lame method...)
						uint8 grey = 216;
						float dist = (nBits[nOffset + 0] - grey) * 0.4;
						dBits[nOffset + 0] = (uint8)(grey + dist);
						dist = (nBits[nOffset + 1] - grey) * 0.4;
						dBits[nOffset + 1] = (uint8)(grey + dist);
						dist = (nBits[nOffset + 2] - grey) * 0.4;
						dBits[nOffset + 2] = (uint8)(grey + dist);
						dBits[nOffset + 3] = 255;
						// disabled bits have less contrast (lame method...)
						grey = 188;
						dist = (nBits[nOffset + 0] - grey) * 0.4;
						dcBits[nOffset + 0] = (uint8)(grey + dist);
						dist = (nBits[nOffset + 1] - grey) * 0.4;
						dcBits[nOffset + 1] = (uint8)(grey + dist);
						dist = (nBits[nOffset + 2] - grey) * 0.4;
						dcBits[nOffset + 2] = (uint8)(grey + dist);
						dcBits[nOffset + 3] = 255;
					}
					nBits += nbpr;
					dBits += nbpr;
					cBits += nbpr;
					dcBits += nbpr;
					fBits += fbpr;
				}
			// transparent version:
			} else if (format == B_RGBA32 || format == B_RGBA32_BIG) {
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
						cBits[nOffset + 0] = (uint8)(nBits[nOffset + 0] * 0.8);
						cBits[nOffset + 1] = (uint8)(nBits[nOffset + 1] * 0.8);
						cBits[nOffset + 2] = (uint8)(nBits[nOffset + 2] * 0.8);
						cBits[nOffset + 3] = fBits[fOffset + 3];
						// disabled bits have less opacity

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
						// disabled bits have less contrast (lame method...)
						dcBits[nOffset + 0] = (uint8)(dBits[nOffset + 0] * 0.8);
						dcBits[nOffset + 1] = (uint8)(dBits[nOffset + 1] * 0.8);
						dcBits[nOffset + 2] = (uint8)(dBits[nOffset + 2] * 0.8);
						dcBits[nOffset + 3] = (uint8)(fBits[fOffset + 3] * 0.3);
					}
					nBits += nbpr;
					dBits += nbpr;
					cBits += nbpr;
					dcBits += nbpr;
					fBits += fbpr;
				}
			// unsupported format
			} else {
				printf("BIconButton::_MakeBitmaps() - bitmap has unsupported "
					"colorspace\n");
				status = B_MISMATCHED_VALUES;
				_DeleteBitmaps();
			}
		} else {
			printf("BIconButton::_MakeBitmaps() - error allocating local "
				"bitmaps\n");
			status = B_NO_MEMORY;
			_DeleteBitmaps();
		}
	} else
		printf("BIconButton::_MakeBitmaps() - bitmap is not valid\n");
	return status;
}


void
BIconButton::_DeleteBitmaps()
{
	delete fNormalBitmap;
	fNormalBitmap = NULL;
	delete fDisabledBitmap;
	fDisabledBitmap = NULL;
	delete fClickedBitmap;
	fClickedBitmap = NULL;
	delete fDisabledClickedBitmap;
	fDisabledClickedBitmap = NULL;
}


void
BIconButton::_Update()
{
	if (LockLooper()) {
		Invalidate();
		UnlockLooper();
	}
}


void
BIconButton::_SetFlags(uint32 flags, bool set)
{
	if (_HasFlags(flags) != set) {
		if (set)
			fButtonState |= flags;
		else
			fButtonState &= ~flags;

		if ((flags & STATE_PRESSED) != 0)
			SetValueNoUpdate(set ? B_CONTROL_ON : B_CONTROL_OFF);
		_Update();
	}
}


bool
BIconButton::_HasFlags(uint32 flags) const
{
	return (fButtonState & flags) != 0;
}


//!	This one calls _Update() if needed; BControl::SetTracking() isn't virtual.
void
BIconButton::_SetTracking(bool tracking)
{
	if (IsTracking() == tracking)
		return;

	SetTracking(tracking);
	_Update();
}


}	// namespace BPrivate
