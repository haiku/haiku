/*
 * Copyright 2006-2013, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


#include <Icon.h>

#include <string.h>

#include <new>

#include <Bitmap.h>

#include <AutoDeleter.h>


namespace BPrivate {


BIcon::BIcon()
	:
	fEnabledBitmaps(8, true),
	fDisabledBitmaps(8, true)
{
}


BIcon::~BIcon()
{
}


status_t
BIcon::SetTo(const BBitmap* bitmap, uint32 flags)
{
	if (!bitmap->IsValid())
		return B_BAD_VALUE;

	DeleteBitmaps();

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

	return error;
}


bool
BIcon::SetBitmap(BBitmap* bitmap, uint32 which)
{
	BitmapList& list = (which & B_DISABLED_ICON_BITMAP) == 0
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


BBitmap*
BIcon::Bitmap(uint32 which) const
{
	const BitmapList& list = (which & B_DISABLED_ICON_BITMAP) == 0
		? fEnabledBitmaps : fDisabledBitmaps;
	return list.ItemAt(which & ~uint32(B_DISABLED_ICON_BITMAP));
}


BBitmap*
BIcon::CreateBitmap(const BRect& bounds, color_space colorSpace, uint32 which)
{
	BBitmap* bitmap = new(std::nothrow) BBitmap(bounds, colorSpace);
	if (bitmap == NULL || !bitmap->IsValid() || !SetBitmap(bitmap, which)) {
		delete bitmap;
		return NULL;
	}

	return bitmap;
}


status_t
BIcon::SetExternalBitmap(const BBitmap* bitmap, uint32 which, uint32 flags)
{
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

	if (!SetBitmap(ourBitmap, which)) {
		if (ourBitmap != bitmap)
			delete ourBitmap;
		return B_NO_MEMORY;
	}

	return B_OK;
}


BBitmap*
BIcon::CopyBitmap(const BBitmap& bitmapToClone, uint32 which)
{
	BBitmap* bitmap = new(std::nothrow) BBitmap(bitmapToClone);
	if (bitmap == NULL || !bitmap->IsValid() || !SetBitmap(bitmap, which)) {
		delete bitmap;
		return NULL;
	}

	return bitmap;
}


void
BIcon::DeleteBitmaps()
{
	fEnabledBitmaps.MakeEmpty(true);
	fDisabledBitmaps.MakeEmpty(true);
}


/*static*/ status_t
BIcon::UpdateIcon(const BBitmap* bitmap, uint32 flags, BIcon*& _icon)
{
	if (bitmap == NULL) {
		delete _icon;
		_icon = NULL;
		return B_OK;
	}

	BIcon* icon = new(std::nothrow) BIcon;
	if (icon == NULL)
		return B_NO_MEMORY;

	status_t error = icon->SetTo(bitmap, flags);
	if (error != B_OK) {
		delete icon;
		return error;
	}

	_icon = icon;
	return B_OK;
}


/*static*/ status_t
BIcon::SetIconBitmap(const BBitmap* bitmap, uint32 which, uint32 flags,
	BIcon*& _icon)
{
	bool newIcon = false;
	if (_icon == NULL) {
		if (bitmap == NULL)
			return B_OK;

		_icon = new(std::nothrow) BIcon;
		if (_icon == NULL)
			return B_NO_MEMORY;
		newIcon = true;
	}

	status_t error = _icon->SetExternalBitmap(bitmap, which, flags);
	if (error != B_OK) {
		if (newIcon) {
			delete _icon;
			_icon = NULL;
		}
		return error;
	}

	return B_OK;
}


/*static*/ BBitmap*
BIcon::_ConvertToRGB32(const BBitmap* bitmap, bool noAppServerLink)
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
BIcon::_TrimBitmap(const BBitmap* bitmap, bool keepAspect,
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
BIcon::_MakeBitmaps(const BBitmap* bitmap, uint32 flags)
{
	// make our own versions of the bitmap
	BRect b(bitmap->Bounds());

	color_space format = bitmap->ColorSpace();
	BBitmap* normalBitmap = CreateBitmap(b, format, B_INACTIVE_ICON_BITMAP);
	if (normalBitmap == NULL)
		return B_NO_MEMORY;

	BBitmap* disabledBitmap = NULL;
	if ((flags & B_CREATE_DISABLED_ICON_BITMAPS) != 0) {
		disabledBitmap = CreateBitmap(b, format,
			B_INACTIVE_ICON_BITMAP | B_DISABLED_ICON_BITMAP);
		if (disabledBitmap == NULL)
			return B_NO_MEMORY;
	}

	BBitmap* clickedBitmap = NULL;
	if ((flags & (B_CREATE_ACTIVE_ICON_BITMAP
			| B_CREATE_PARTIALLY_ACTIVE_ICON_BITMAP)) != 0) {
		clickedBitmap = CreateBitmap(b, format, B_ACTIVE_ICON_BITMAP);
		if (clickedBitmap == NULL)
			return B_NO_MEMORY;
	}

	BBitmap* disabledClickedBitmap = NULL;
	if (disabledBitmap != NULL && clickedBitmap != NULL) {
		disabledClickedBitmap = CreateBitmap(b, format,
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
		if (CopyBitmap(clickedBitmap, B_PARTIALLY_ACTIVATE_ICON_BITMAP) == NULL)
			return B_NO_MEMORY;
		if ((flags & B_CREATE_DISABLED_ICON_BITMAPS) != 0) {
			if (CopyBitmap(disabledClickedBitmap,
					B_PARTIALLY_ACTIVATE_ICON_BITMAP | B_DISABLED_ICON_BITMAP)
					== NULL) {
				return B_NO_MEMORY;
			}
		}
	}

	return B_OK;
}


}	// namespace BPrivate
