/*
 * Copyright 2001-2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold (bonefish@users.sf.net)
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

/** BBitmap objects represent off-screen windows that
 *	contain bitmap data.
 */

#include <algorithm>
#include <limits.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>

#include <Application.h>
#include <Bitmap.h>
#include <GraphicsDefs.h>
#include <Locker.h>
#include <View.h>
#include <Window.h>

#include "ColorConversion.h"

// Includes to be able to talk to the app_server
#include <ServerProtocol.h>
#include <AppServerLink.h>


// get_raw_bytes_per_row
/*!	\brief Returns the number of bytes per row needed to store the actual
		   bitmap data (not including any padding) given a color space and a
		   row width.
	\param colorSpace The color space.
	\param width The width.
	\return The number of bytes per row needed to store data for a row, or
			0, if the color space is not supported.
*/
static inline
int32
get_raw_bytes_per_row(color_space colorSpace, int32 width)
{
	int32 bpr = 0;
	switch (colorSpace) {
		// supported
		case B_RGB32: case B_RGBA32:
		case B_RGB32_BIG: case B_RGBA32_BIG:
		case B_UVL32: case B_UVLA32:
		case B_LAB32: case B_LABA32:
		case B_HSI32: case B_HSIA32:
		case B_HSV32: case B_HSVA32:
		case B_HLS32: case B_HLSA32:
		case B_CMY32: case B_CMYA32: case B_CMYK32:
			bpr = 4 * width;
			break;
		case B_RGB24: case B_RGB24_BIG:
		case B_UVL24: case B_LAB24: case B_HSI24:
		case B_HSV24: case B_HLS24: case B_CMY24:
			bpr = 3 * width;
			break;
		case B_RGB16:		case B_RGB15:		case B_RGBA15:
		case B_RGB16_BIG:	case B_RGB15_BIG:	case B_RGBA15_BIG:
			bpr = 2 * width;
			break;
		case B_CMAP8: case B_GRAY8:
			bpr = width;
			break;
		case B_GRAY1:
			bpr = (width + 7) / 8;
			break;
		case B_YCbCr422: case B_YUV422:
			bpr = (width + 3) / 4 * 8;
			break;
		case B_YCbCr411: case B_YUV411:
			bpr = (width + 3) / 4 * 6;
			break;
		case B_YCbCr444: case B_YUV444:
			bpr = (width + 3) / 4 * 12;
			break;
		case B_YCbCr420: case B_YUV420:
			bpr = (width + 3) / 4 * 6;
			break;
		// unsupported
		case B_NO_COLOR_SPACE:
		case B_YUV9: case B_YUV12:
			break;
	}
	return bpr;
}

// get_bytes_per_row
/*!	\brief Returns the number of bytes per row needed to store the bitmap
		   data (including any padding) given a color space and a row width.
	\param colorSpace The color space.
	\param width The width.
	\return The number of bytes per row needed to store data for a row, or
			0, if the color space is not supported.
*/
static inline
int32
get_bytes_per_row(color_space colorSpace, int32 width)
{
	int32 bpr = get_raw_bytes_per_row(colorSpace, width);
	// align to int32
	bpr = (bpr + 3) & 0x7ffffffc;
	return bpr;
}


// constructor
/*!	\brief Creates and initializes a BBitmap.
	\param bounds The bitmap dimensions.
	\param flags Creation flags.
	\param colorSpace The bitmap's color space.
	\param bytesPerRow The number of bytes per row the bitmap should use.
		   \c B_ANY_BYTES_PER_ROW to let the constructor choose an appropriate
		   value.
	\param screenID ???
*/
BBitmap::BBitmap(BRect bounds, uint32 flags, color_space colorSpace,
				 int32 bytesPerRow, screen_id screenID)
	: fBasePtr(NULL),
	  fSize(0),
	  fColorSpace(B_NO_COLOR_SPACE),
	  fBounds(0, 0, -1, -1),
	  fBytesPerRow(0),
	  fWindow(NULL),
	  fServerToken(-1),
	  fAreaOffset(-1),
	  fArea(-1),
	  fOrigArea(-1),
	  fFlags(0),
	  fInitError(B_NO_INIT)
{
	InitObject(bounds, colorSpace, flags, bytesPerRow, screenID);
}

// constructor
/*!	\brief Creates and initializes a BBitmap.
	\param bounds The bitmap dimensions.
	\param colorSpace The bitmap's color space.
	\param acceptsViews \c true, if the bitmap shall accept BViews, i.e. if
		   it shall be possible to attach BView to the bitmap and draw into
		   it.
	\param needsContiguous If \c true a physically contiguous chunk of memory
		   will be allocated.
*/
BBitmap::BBitmap(BRect bounds, color_space colorSpace, bool acceptsViews,
				 bool needsContiguous)
	: fBasePtr(NULL),
	  fSize(0),
	  fColorSpace(B_NO_COLOR_SPACE),
	  fBounds(0, 0, -1, -1),
	  fBytesPerRow(0),
	  fWindow(NULL),
	  fServerToken(-1),
	  fAreaOffset(-1),
	  fArea(-1),
	  fOrigArea(-1),
	  fFlags(0),
	  fInitError(B_NO_INIT)
{
	int32 flags = (acceptsViews ? B_BITMAP_ACCEPTS_VIEWS : 0)
				| (needsContiguous ? B_BITMAP_IS_CONTIGUOUS : 0);
	InitObject(bounds, colorSpace, flags, B_ANY_BYTES_PER_ROW,
			   B_MAIN_SCREEN_ID);

}

// constructor
/*!	\brief Creates a BBitmap as a clone of another bitmap.
	\param source The source bitmap.
	\param acceptsViews \c true, if the bitmap shall accept BViews, i.e. if
		   it shall be possible to attach BView to the bitmap and draw into
		   it.
	\param needsContiguous If \c true a physically contiguous chunk of memory
		   will be allocated.
*/
BBitmap::BBitmap(const BBitmap *source, bool acceptsViews,
				 bool needsContiguous)
	: fBasePtr(NULL),
	  fSize(0),
	  fColorSpace(B_NO_COLOR_SPACE),
	  fBounds(0, 0, -1, -1),
	  fBytesPerRow(0),
	  fWindow(NULL),
	  fServerToken(-1),
	  fAreaOffset(-1),
	  fArea(-1),
	  fOrigArea(-1),
	  fFlags(0),
	  fInitError(B_NO_INIT)
{
	if (source && source->IsValid()) {
		int32 flags = (acceptsViews ? B_BITMAP_ACCEPTS_VIEWS : 0)
					| (needsContiguous ? B_BITMAP_IS_CONTIGUOUS : 0);
		InitObject(source->Bounds(), source->ColorSpace(), flags,
				   source->BytesPerRow(), B_MAIN_SCREEN_ID);
		if (InitCheck() == B_OK)
			memcpy(Bits(), source->Bits(), BytesPerRow());
	}
}

// destructor
/*!	\brief Frees all resources associated with this object.
*/
BBitmap::~BBitmap()
{
	delete fWindow;
	CleanUp();
}

// unarchiving constructor
/*!	\brief Unarchives a bitmap from a BMessage.
	\param data The archive.
*/
BBitmap::BBitmap(BMessage *data)
	: BArchivable(data),
	  fBasePtr(NULL),
	  fSize(0),
	  fColorSpace(B_NO_COLOR_SPACE),
	  fBounds(0, 0, -1, -1),
	  fBytesPerRow(0),
	  fWindow(NULL),
	  fServerToken(-1),
	  fAreaOffset(-1),
	  fArea(-1),
	  fOrigArea(-1),
	  fFlags(0),
	  fInitError(B_NO_INIT)
{
	BRect bounds;
	data->FindRect("_frame", &bounds);

	color_space cspace;
	data->FindInt32("_cspace", (int32 *)&cspace);

	int32 flags = 0;
	data->FindInt32("_bmflags", &flags);

	int32 rowBytes = 0;
	data->FindInt32("_rowbytes", &rowBytes);

	InitObject(bounds, cspace, flags, rowBytes, B_MAIN_SCREEN_ID);

	if (data->HasData("_data", B_RAW_TYPE) && InitCheck() == B_OK) {
		AssertPtr();
		ssize_t size = 0;
		const void *buffer;
		if (data->FindData("_data", B_RAW_TYPE, &buffer, &size) == B_OK)
			memcpy(fBasePtr, buffer, size);
	}

	if (fFlags & B_BITMAP_ACCEPTS_VIEWS) {
		BMessage message;
		int32 i = 0;

		while (data->FindMessage("_view", i++, &message) == B_OK) {
			if (BView *view = dynamic_cast<BView *>(instantiate_object(&message)))
				AddChild(view);
		}
	}
}

// Instantiate
/*!	\brief Instantiates a BBitmap from an archive.
	\param data The archive.
	\return A bitmap reconstructed from the archive or \c NULL, if an error
			occured.
*/
BArchivable *
BBitmap::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, "BBitmap"))
		return new BBitmap(data);
	
	return NULL;
}

// Archive
/*!	\brief Archives the BBitmap object.
	\param data The archive.
	\param deep \c true, if child object shall be archived as well, \c false
		   otherwise.
	\return \c B_OK, if everything went fine, an error code otherwise.
*/
status_t
BBitmap::Archive(BMessage *data, bool deep) const
{
	BArchivable::Archive(data, deep);
	
	data->AddRect("_frame", fBounds);
	data->AddInt32("_cspace", (int32)fColorSpace);
	data->AddInt32("_bmflags", fFlags);
	data->AddInt32("_rowbytes", fBytesPerRow);
	
	if (deep) {
		if (fFlags & B_BITMAP_ACCEPTS_VIEWS) {
			BMessage views;
			for (int32 i = 0; i < CountChildren(); i++) {
				if (ChildAt(i)->Archive(&views, deep))
					data->AddMessage("_views", &views);
			}
		}
		// Note: R5 does not archive the data if B_BITMAP_IS_CONTIGNUOUS is
		// true and it does save all formats as B_RAW_TYPE and it does save
		// the data even if B_BITMAP_ACCEPTS_VIEWS is set (as opposed to
		// the BeBook)
		const_cast<BBitmap *>(this)->AssertPtr();
		data->AddData("_data", B_RAW_TYPE, fBasePtr, fSize);
	}
	
	return B_OK;
}

// InitCheck
/*!	\brief Returns the result from the construction.
	\return \c B_OK, if the object is properly initialized, an error code
			otherwise.
*/
status_t
BBitmap::InitCheck() const
{
	return fInitError;
}

// IsValid
/*!	\brief Returns whether or not the BBitmap object is valid.
	\return \c true, if the object is properly initialized, \c false otherwise.
*/
bool
BBitmap::IsValid() const
{
	return (InitCheck() == B_OK);
}

// LockBits
/*! \brief ???
*/
status_t
BBitmap::LockBits(uint32 *state)
{
	return B_ERROR;
}

// UnlockBits
/*! \brief ???
*/
void
BBitmap::UnlockBits()
{
}

// Area
/*! \brief Returns the ID of the area the bitmap data reside in.
	\return The ID of the area the bitmap data reside in.
*/
area_id
BBitmap::Area() const
{
	const_cast<BBitmap *>(this)->AssertPtr();
	return fArea;
}

// Bits
/*!	\brief Returns the pointer to the bitmap data.
	\return The pointer to the bitmap data.
*/
void *
BBitmap::Bits() const
{
	const_cast<BBitmap *>(this)->AssertPtr();
	return fBasePtr;
}

// BitsLength
/*!	\brief Returns the size of the bitmap data.
	\return The size of the bitmap data.
*/
int32
BBitmap::BitsLength() const
{
	return fSize;
}

// BytesPerRow
/*!	\brief Returns the number of bytes used to store a row of bitmap data.
	\return The number of bytes used to store a row of bitmap data.
*/
int32
BBitmap::BytesPerRow() const
{
	return fBytesPerRow;
}

// ColorSpace
/*!	\brief Returns the bitmap's color space.
	\return The bitmap's color space.
*/
color_space
BBitmap::ColorSpace() const
{
	return fColorSpace;
}

// Bounds
/*!	\brief Returns the bitmap's dimensions.
	\return The bitmap's dimensions.
*/
BRect
BBitmap::Bounds() const
{
	return fBounds;
}


// SetBits
/*!	\brief Assigns data to the bitmap.

	Data are directly written into the bitmap's data buffer, being converted
	beforehand, if necessary. Some conversions work rather unintuitively:
	- \c B_RGB32: The source buffer is supposed to contain \c B_RGB24_BIG
	  data without padding at the end of the rows.
	- \c B_RGB32: The source buffer is supposed to contain \c B_CMAP8
	  data without padding at the end of the rows.
	- other color spaces: The source buffer is supposed to contain data
	  according to the specified color space being rowwise padded to int32.

	The currently supported source/target color spaces are
	\c B_RGB{32,24,16,15}[_BIG], \c B_CMAP8 and \c B_GRAY{8,1}.

	\note As this methods is apparently a bit strange to use, Haiku introduces
		  ImportBits() methods, which are recommended to be used instead.

	\param data The data to be copied.
	\param length The length in bytes of the data to be copied.
	\param offset The offset (in bytes) relative to beginning of the bitmap
		   data specifying the position at which the source data shall be
		   written.
	\param colorSpace Color space of the source data.
*/
void
BBitmap::SetBits(const void *data, int32 length, int32 offset,
				 color_space colorSpace)
{
	status_t error = (InitCheck() == B_OK ? B_OK : B_NO_INIT);
	// check params
	if (error == B_OK && (data == NULL || offset > fSize || length < 0))
		error = B_BAD_VALUE;
	int32 width = 0;
	if (error == B_OK)
		width = fBounds.IntegerWidth() + 1;
	int32 inBPR = -1;
	// tweaks to mimic R5 behavior
	if (error == B_OK) {
		// B_RGB32 means actually unpadded B_RGB24_BIG
		if (colorSpace == B_RGB32) {
			colorSpace = B_RGB24_BIG;
			inBPR = width * 3;
		// If in color space is B_CMAP8, but the bitmap's is another one,
		// ignore source data row padding.
		} else if (colorSpace == B_CMAP8 && fColorSpace != B_CMAP8)
			inBPR = width;
	}
	// call the sane method, which does the actual work
	if (error == B_OK)
		error = ImportBits(data, length, inBPR, offset, colorSpace);
}

// ImportBits
/*!	\brief Assigns data to the bitmap.

	Data are directly written into the bitmap's data buffer, being converted
	beforehand, if necessary. Unlike for SetBits(), the meaning of
	\a colorSpace is exactly the expected one here, i.e. the source buffer
	is supposed to contain data of that color space. \a bpr specifies how
	many bytes the source contains per row. \c B_ANY_BYTES_PER_ROW can be
	supplied, if standard padding to int32 is used.

	The currently supported source/target color spaces are
	\c B_RGB{32,24,16,15}[_BIG], \c B_CMAP8 and \c B_GRAY{8,1}.

	\param data The data to be copied.
	\param length The length in bytes of the data to be copied.
	\param bpr The number of bytes per row in the source data.
	\param offset The offset (in bytes) relative to beginning of the bitmap
		   data specifying the position at which the source data shall be
		   written.
	\param colorSpace Color space of the source data.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a data, invalid \a bpr or \a offset, or
	  unsupported \a colorSpace.
*/
status_t
BBitmap::ImportBits(const void *data, int32 length, int32 bpr, int32 offset,
	color_space colorSpace)
{
	AssertPtr();

	if (InitCheck() != B_OK)
		return B_NO_INIT;

	if (!data || offset > fSize || length < 0)
		return B_BAD_VALUE;

	int32 width = fBounds.IntegerWidth() + 1;
	if (bpr < 0)
		bpr = get_bytes_per_row(colorSpace, width);

	return BPrivate::ConvertBits(data, (uint8*)fBasePtr + offset, length,
		fSize - offset, bpr, fBytesPerRow, colorSpace, fColorSpace, width,
		fBounds.IntegerHeight() + 1);
}


/*!	\brief Assigns data to the bitmap.

	Allows for a BPoint offset in the source and in the bitmap. The region
	of the source at \a from extending \a width and \a height is assigned
	(and converted if necessary) to the bitmap at \a to.

	The currently supported source/target color spaces are
	\c B_RGB{32,24,16,15}[_BIG], \c B_CMAP8 and \c B_GRAY{8,1}.

	\param data The data to be copied.
	\param length The length in bytes of the data to be copied.
	\param bpr The number of bytes per row in the source data.
	\param colorSpace Color space of the source data.
	\param from The offset in the source where reading should begin.
	\param to The offset in the bitmap where the source should be written.
	\param width The width (in pixels) to be imported.
	\param height The height (in pixels) to be imported.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a data, invalid \a bpr, unsupported
	  \a colorSpace or invalid width/height.
*/
status_t
BBitmap::ImportBits(const void *data, int32 length, int32 bpr,
	color_space colorSpace, BPoint from, BPoint to, int32 width, int32 height)
{
	AssertPtr();

	if (InitCheck() != B_OK)
		return B_NO_INIT;

	if (!data || length < 0 || bpr < 0 || width < 0 || height < 0)
		return B_BAD_VALUE;

	if (bpr < 0)
		bpr = get_bytes_per_row(colorSpace, fBounds.IntegerWidth() + 1);

	return BPrivate::ConvertBits(data, fBasePtr, length, fSize, bpr,
		fBytesPerRow, colorSpace, fColorSpace, from, to, width, height);
}


/*!	\briefly Assigns another bitmap's data to this bitmap.

	The supplied bitmap must have the exactly same dimensions as this bitmap.
	Its data is converted to the color space of this bitmap.

	The currently supported source/target color spaces are
	\c B_RGB{32,24,16,15}[_BIG], \c B_CMAP8 and \c B_GRAY{8,1}.

	\param bitmap The source bitmap.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a bitmap, or \a bitmap has other dimensions,
	  or the conversion from or to one of the color spaces is not supported.
*/
status_t
BBitmap::ImportBits(const BBitmap *bitmap)
{
	if (InitCheck() != B_OK)
		return B_NO_INIT;

	if (!bitmap || bitmap->InitCheck() != B_OK || bitmap->Bounds() != fBounds)
		return B_BAD_VALUE;

	return ImportBits(bitmap->Bits(), bitmap->BitsLength(),
		bitmap->BytesPerRow(), 0, bitmap->ColorSpace());
}


/*!	\brief Assigns data to the bitmap.

	Allows for a BPoint offset in the source and in the bitmap. The region
	of the source at \a from extending \a width and \a height is assigned
	(and converted if necessary) to the bitmap at \a to. The source bitmap is
	clipped to the bitmap and they don't need to have the same dimensions.

	The currently supported source/target color spaces are
	\c B_RGB{32,24,16,15}[_BIG], \c B_CMAP8 and \c B_GRAY{8,1}.

	\param bitmap The source bitmap.
	\param from The offset in the source where reading should begin.
	\param to The offset in the bitmap where the source should be written.
	\param width The width (in pixels) to be imported.
	\param height The height (in pixels) to be imported.
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a bitmap, the conversion from or to one of
	  the color spaces is not supported, or invalid width/height.
*/
status_t
BBitmap::ImportBits(const BBitmap *bitmap, BPoint from, BPoint to, int32 width,
	int32 height)
{
	if (InitCheck() != B_OK)
		return B_NO_INIT;

	if (!bitmap || bitmap->InitCheck() != B_OK)
		return B_BAD_VALUE;

	return ImportBits(bitmap->Bits(), bitmap->BitsLength(),
		bitmap->BytesPerRow(), bitmap->ColorSpace(), from, to, width, height);
}


/*!	\brief Returns the overlay_restrictions structure for this bitmap
*/
status_t
BBitmap::GetOverlayRestrictions(overlay_restrictions *restrictions) const
{
	if ((fFlags & B_BITMAP_WILL_OVERLAY) == 0)
		return B_BAD_TYPE;

	BPrivate::AppServerLink link;

	link.StartMessage(AS_GET_BITMAP_OVERLAY_RESTRICTIONS);
	link.Attach<int32>(fServerToken);

	status_t status;
	if (link.FlushWithReply(status) < B_OK)
		return B_ERROR;

	return status;
}


/*!	\brief Adds a BView to the bitmap's view hierarchy.

	The bitmap must accept views and the supplied view must not be child of
	another parent.

	\param view The view to be added.
*/
void
BBitmap::AddChild(BView *view)
{
	if (fWindow != NULL)
		fWindow->AddChild(view);
}

// RemoveChild
/*!	\brief Removes a BView from the bitmap's view hierarchy.
	\param view The view to be removed.
*/
bool
BBitmap::RemoveChild(BView *view)
{
	return fWindow != NULL ? fWindow->RemoveChild(view) : false;
}

// CountChildren
/*!	\brief Returns the number of BViews currently belonging to the bitmap.
	\return The number of BViews currently belonging to the bitmap.
*/
int32
BBitmap::CountChildren() const
{
	return fWindow != NULL ? fWindow->CountChildren() : 0;
}

// ChildAt
/*!	\brief Returns the BView at a certain index in the bitmap's list of views.
	\param index The index of the BView to be returned.
	\return The BView at index \a index or \c NULL, if the index is out of
			range.
*/
BView*
BBitmap::ChildAt(int32 index) const
{
	return fWindow != NULL ? fWindow->ChildAt(index) : NULL;
}

// FindView
/*!	\brief Returns a bitmap's BView with a certain name.
	\param name The name of the BView to be returned.
	\return The BView with the name \a name or \c NULL, if the bitmap doesn't
	know a view with that name.
*/
BView*
BBitmap::FindView(const char *viewName) const
{
	return fWindow != NULL ? fWindow->FindView(viewName) : NULL;
}

// FindView
/*!	\brief Returns a bitmap's BView at a certain location.
	\param point The location.
	\return The BView with located at \a point or \c NULL, if the bitmap
	doesn't know a view at this location.
*/
BView *
BBitmap::FindView(BPoint point) const
{
	return fWindow != NULL ? fWindow->FindView(point) : NULL;
}

// Lock
/*!	\brief Locks the off-screen window that belongs to the bitmap.

	The bitmap must accept views, if locking should work.

	\return \c true, if the lock was acquired successfully, \c false
			otherwise.
*/
bool
BBitmap::Lock()
{
	return fWindow != NULL ? fWindow->Lock() : false;
}

// Unlock
/*!	\brief Unlocks the off-screen window that belongs to the bitmap.

	The bitmap must accept views, if locking should work.
*/
void
BBitmap::Unlock()
{
	if (fWindow != NULL)
		fWindow->Unlock();
}

// IsLocked
/*!	\brief Returns whether or not the bitmap's off-screen window is locked.

	The bitmap must accept views, if locking should work.

	\return \c true, if the caller owns a lock , \c false otherwise.
*/
bool
BBitmap::IsLocked() const
{
	return fWindow != NULL ? fWindow->IsLocked() : false;
}

// Perform
/*!	\brief ???
*/
status_t
BBitmap::Perform(perform_code d, void *arg)
{
	return BArchivable::Perform(d, arg);
}

// FBC
void BBitmap::_ReservedBitmap1() {}
void BBitmap::_ReservedBitmap2() {}
void BBitmap::_ReservedBitmap3() {}

// copy constructor
/*!	\brief Privatized copy constructor to prevent usage.
*/
BBitmap::BBitmap(const BBitmap &)
{
}

// =
/*!	\brief Privatized assignment operator to prevent usage.
*/
BBitmap &
BBitmap::operator=(const BBitmap &)
{
	return *this;
}

// get_shared_pointer
/*!	\brief ???
*/
char *
BBitmap::get_shared_pointer() const
{
	return NULL;	// not implemented
}

// get_server_token
/*!	\brief ???
*/
int32
BBitmap::get_server_token() const
{
	return fServerToken;
}

// InitObject
/*!	\brief Initializes the bitmap.
	\param bounds The bitmap dimensions.
	\param colorSpace The bitmap's color space.
	\param flags Creation flags.
	\param bytesPerRow The number of bytes per row the bitmap should use.
		   \c B_ANY_BYTES_PER_ROW to let the constructor choose an appropriate
		   value.
	\param screenID ???
*/
void
BBitmap::InitObject(BRect bounds, color_space colorSpace, uint32 flags,
	int32 bytesPerRow, screen_id screenID)
{
//printf("BBitmap::InitObject(bounds: BRect(%.1f, %.1f, %.1f, %.1f), format: %ld, flags: %ld, bpr: %ld\n",
//	   bounds.left, bounds.top, bounds.right, bounds.bottom, colorSpace, flags, bytesPerRow);

	// TODO: Should we handle rounding of the "bounds" here? How does R5 behave?

	status_t error = B_OK;

#ifdef RUN_WITHOUT_APP_SERVER
	flags |= B_BITMAP_NO_SERVER_LINK;
#endif	// RUN_WITHOUT_APP_SERVER

	CleanUp();

	// check params
	if (!bounds.IsValid() || !bitmaps_support_space(colorSpace, NULL)) {
		error = B_BAD_VALUE;
	} else {
		// bounds is in floats and might be valid but much larger than what we can handle
		// the size could not be expressed in int32
		double realSize = bounds.Width() * bounds.Height();
		if (realSize > (double)(INT_MAX / 4)) {
			fprintf(stderr, "bitmap bounds is much too large: BRect(%.1f, %.1f, %.1f, %.1f)\n",
					bounds.left, bounds.top, bounds.right, bounds.bottom);
			error = B_BAD_VALUE;
		}
	}
	if (error == B_OK) {
		int32 bpr = get_bytes_per_row(colorSpace, bounds.IntegerWidth() + 1);
		if (bytesPerRow < 0)
			bytesPerRow = bpr;
		else if (bytesPerRow < bpr)
// NOTE: How does R5 behave?
			error = B_BAD_VALUE;
	}
	// allocate the bitmap buffer
	if (error == B_OK) {
		// NOTE: Maybe the code would look more robust if the
		// "size" was not calculated here when we ask the server
		// to allocate the bitmap. -Stephan
		int32 size = bytesPerRow * (bounds.IntegerHeight() + 1);

		if (flags & B_BITMAP_NO_SERVER_LINK) {
			fBasePtr = malloc(size);
			if (fBasePtr) {
				fSize = size;
				fColorSpace = colorSpace;
				fBounds = bounds;
				fBytesPerRow = bytesPerRow;
				fFlags = flags;
			} else
				error = B_NO_MEMORY;
		} else {
			// Ask the server (via our owning application) to create a bitmap.
			BPrivate::AppServerLink link;

			// Attach Data: 
			// 1) BRect bounds
			// 2) color_space space
			// 3) int32 bitmap_flags
			// 4) int32 bytes_per_row
			// 5) int32 screen_id::id
			link.StartMessage(AS_CREATE_BITMAP);
			link.Attach<BRect>(bounds);
			link.Attach<color_space>(colorSpace);
			link.Attach<int32>((int32)flags);
			link.Attach<int32>(bytesPerRow);
			link.Attach<int32>(screenID.id);

			// Reply Data:
			//	1) int32 server token
			//	2) area_id id of the area in which the bitmap data resides
			//	3) int32 area pointer offset used to calculate fBasePtr

			error = B_ERROR;
			if (link.FlushWithReply(error) == B_OK && error == B_OK) {
				// server side success
				// Get token
				link.Read<int32>(&fServerToken);
				link.Read<area_id>(&fOrigArea);
				link.Read<int32>(&fAreaOffset);
				
				if (fOrigArea >= B_OK) {
					fSize = size;
					fColorSpace = colorSpace;
					fBounds = bounds;
					fBytesPerRow = bytesPerRow;
					fFlags = flags;
				} else
					error = fOrigArea;
			}

			if (error < B_OK) {
				fBasePtr = NULL;
				fServerToken = -1;
				fArea = -1;
				fOrigArea = -1;
				fAreaOffset = -1;
				// NOTE: why not "0" in case of error?
				fFlags = flags;
			}
		}
		fWindow = NULL;
	}

	fInitError = error;
	// TODO: on success, handle clearing to white if the flags say so. Needs to be
	// dependent on color space.

	if (fInitError == B_OK) {
		if (flags & B_BITMAP_ACCEPTS_VIEWS) {
			fWindow = new BWindow(Bounds(), fServerToken);
			// A BWindow starts life locked and is unlocked
			// in Show(), but this window is never shown and
			// it's message loop is never started.
			fWindow->Unlock();
		}
	}
}


/*!
	\brief Cleans up any memory allocated by the bitmap and
		informs the server to do so as well (if needed).
*/
void
BBitmap::CleanUp()
{
	if (fBasePtr == NULL)
		return;

	if (fFlags & B_BITMAP_NO_SERVER_LINK) {
		free(fBasePtr);
	} else {
		BPrivate::AppServerLink link;
		// AS_DELETE_BITMAP:
		// Attached Data: 
		//	1) int32 server token
		link.StartMessage(AS_DELETE_BITMAP);
		link.Attach<int32>(fServerToken);
		link.Flush();
		
		if (fArea >= 0)
			delete_area(fArea);
		fArea = -1;
		fServerToken = -1;
		fAreaOffset = -1;
	}
	fBasePtr = NULL;
}


void
BBitmap::AssertPtr()
{
	if (fBasePtr == NULL && fAreaOffset != -1 && InitCheck() == B_OK) {
		// Offset was saved into "fArea" as we can't add
		// any member variable due to Binary compatibility
		// Get the area in which the data resides
		fArea = clone_area("shared bitmap area", (void **)&fBasePtr, B_ANY_ADDRESS,
								B_READ_AREA | B_WRITE_AREA, fOrigArea);
		
		if (fArea >= B_OK) {
			// Jump to the location in the area
			fBasePtr = (int8 *)fBasePtr + fAreaOffset;
		} else
			fBasePtr = NULL;
	}
}

