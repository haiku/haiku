//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	File Name:		Bitmap.cpp
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	BBitmap objects represent off-screen windows that
//					contain bitmap data.
//------------------------------------------------------------------------------

#include <limits.h>
#include <new>
#include <stdlib.h>

#include <Bitmap.h>
#include <InterfaceDefs.h>

enum {
	NOT_IMPLEMENTED	= B_ERROR,
};


// is_supported
static inline
bool
is_supported(color_space colorSpace)
{
	bool result = false;
	switch (colorSpace) {
		// supported
		case B_RGB32:		case B_RGBA32:		case B_RGB24:
		case B_RGB32_BIG:	case B_RGBA32_BIG:	case B_RGB24_BIG:
		case B_RGB16:		case B_RGB15:		case B_RGBA15:
		case B_RGB16_BIG:	case B_RGB15_BIG:	case B_RGBA15_BIG:
		case B_CMAP8:		case B_GRAY8:		case B_GRAY1:
			result = true;
			break;
		// unsupported
		case B_NO_COLOR_SPACE:
		case B_YCbCr422: case B_YCbCr411: case B_YCbCr444: case B_YCbCr420:
		case B_YUV422: case B_YUV411: case B_YUV444: case B_YUV420:
		case B_YUV9: case B_YUV12:
		case B_UVL24: case B_UVL32: case B_UVLA32:
		case B_LAB24: case B_LAB32: case B_LABA32:
		case B_HSI24: case B_HSI32: case B_HSIA32:
		case B_HSV24: case B_HSV32: case B_HSVA32:
		case B_HLS24: case B_HLS32: case B_HLSA32:
		case B_CMY24: case B_CMY32: case B_CMYA32: case B_CMYK32:
			break;
	}
	return result;
}


// get_bytes_per_row
static inline
int32
get_bytes_per_row(color_space colorSpace, int32 width)
{
	int32 bpr = 0;
	switch (colorSpace) {
		// supported
		case B_RGB32:		case B_RGBA32:		case B_RGB24:
		case B_RGB32_BIG:	case B_RGBA32_BIG:	case B_RGB24_BIG:
			bpr = 4 * width;
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
		// unsupported
		case B_NO_COLOR_SPACE:
		case B_YCbCr422: case B_YCbCr411: case B_YCbCr444: case B_YCbCr420:
		case B_YUV422: case B_YUV411: case B_YUV444: case B_YUV420:
		case B_YUV9: case B_YUV12:
		case B_UVL24: case B_UVL32: case B_UVLA32:
		case B_LAB24: case B_LAB32: case B_LABA32:
		case B_HSI24: case B_HSI32: case B_HSIA32:
		case B_HSV24: case B_HSV32: case B_HSVA32:
		case B_HLS24: case B_HLS32: case B_HLSA32:
		case B_CMY24: case B_CMY32: case B_CMYA32: case B_CMYK32:
			break;
	}
	return bpr;
}

// brightness_for
static inline
uint8
brightness_for(uint8 red, uint8 green, uint8 blue)
{
	// brightness = 0.301 * red + 0.586 * green + 0.113 * blue
	// we use for performance reasons:
	// brightness = (308 * red + 600 * green + 116 * blue) / 1024
	return uint8((308 * red + 600 * green + 116 * blue) / 1024);
}

// color_distance
static inline
unsigned
color_distance(uint8 red1, uint8 green1, uint8 blue1,
			   uint8 red2, uint8 green2, uint8 blue2)
{
	// euklidian distance (its square actually)
	int rd = (int)red1 - (int)red2;
	int gd = (int)green1 - (int)green2;
	int bd = (int)blue1 - (int)blue2;
//	return rd * rd + gd * gd + bd * bd;

	// distance according to psycho-visual tests
	int rmean = ((int)red1 + (int)red2) / 2;
	return (((512 + rmean) * rd * rd) >> 8)
		   + 4 * gd * gd
		   + (((767 - rmean) * bd * bd) >> 8);
}

//////////////////////
// PaletteConverter //
//////////////////////

namespace BPrivate {

// helper class for palette color <-> linear color conversion

class PaletteConverter {
public:
	PaletteConverter(const rgb_color *palette);
	PaletteConverter(const color_map *colorMap);
	~PaletteConverter();

	inline uint8 IndexForRGB15(uint16 rgb) const;
	inline uint8 IndexForRGB15(uint8 red, uint8 green, uint8 blue) const;
	inline uint8 IndexForRGB16(uint16 rgb) const;
	inline uint8 IndexForRGB16(uint8 red, uint8 green, uint8 blue) const;
	inline uint8 IndexForRGB24(uint32 rgb) const;
	inline uint8 IndexForRGB24(uint8 red, uint8 green, uint8 blue) const;
	inline uint8 IndexForGrey(uint8 grey) const;

	inline const rgb_color &RGBColorForIndex(uint8 index) const;
	inline uint16 RGB15ColorForIndex(uint8 index) const;
	inline uint16 RGB16ColorForIndex(uint8 index) const;
	inline uint32 RGB24ColorForIndex(uint8 index) const;
	inline uint8 GreyColorForIndex(uint8 index) const;

private:
	const color_map	*fColorMap;
	color_map		*fOwnColorMap;
};

}	// namespace BPrivate

using BPrivate::PaletteConverter;

// constructor
PaletteConverter::PaletteConverter(const rgb_color *palette)
	: fColorMap(NULL),
	  fOwnColorMap(NULL)
{
	fOwnColorMap = new(nothrow) color_map;
	fColorMap = fOwnColorMap;
	if (fOwnColorMap && palette) {
		// init color list
		memcpy(fOwnColorMap->color_list, palette, sizeof(rgb_color) * 256);
		// init index map
		for (int32 color = 0; color < 32768; color++) {
			// get components
			uint8 red = (color & 0x7c00) >> 7;
			uint8 green = (color & 0x3e0) >> 2;
			uint8 blue = (color & 0x1f) << 3;
			red |= red >> 5;
			green |= green >> 5;
			blue |= blue >> 5;
			// find closest color
			uint8 closestIndex = 0;
			unsigned closestDistance = UINT_MAX;
			for (int32 i = 0; i < 256; i++) {
				const rgb_color &c = fOwnColorMap->color_list[i];
				unsigned distance = color_distance(red, green, blue,
												   c.red, c.green, c.blue);
				if (distance < closestDistance) {
					closestIndex = i;
					closestDistance = distance;
				}
			}
			fOwnColorMap->index_map[color] = closestIndex;
		}
		// no need to init inversion map
	}
}

// constructor
PaletteConverter::PaletteConverter(const color_map *colorMap)
	: fColorMap(colorMap),
	  fOwnColorMap(NULL)
{
}

// destructor
PaletteConverter::~PaletteConverter()
{
	delete fOwnColorMap;
}

// IndexForRGB15
inline
uint8
PaletteConverter::IndexForRGB15(uint16 rgb) const
{
	return fColorMap->index_map[rgb];
}

// IndexForRGB15
inline
uint8
PaletteConverter::IndexForRGB15(uint8 red, uint8 green, uint8 blue) const
{
	// the 5 least significant bits are used
	return fColorMap->index_map[(red << 10) | (green << 5) | blue];
}

// IndexForRGB16
inline
uint8
PaletteConverter::IndexForRGB16(uint16 rgb) const
{
	return fColorMap->index_map[(rgb >> 1) & 0x7fe0 | rgb & 0x1f];
}

// IndexForRGB16
inline
uint8
PaletteConverter::IndexForRGB16(uint8 red, uint8 green, uint8 blue) const
{
	// the 5 (for red, blue) / 6 (for green) least significant bits are used
	return fColorMap->index_map[(red << 10) | ((green & 0x3e) << 4) | blue];
}

// IndexForRGB24
inline
uint8
PaletteConverter::IndexForRGB24(uint32 rgb) const
{
	return fColorMap->index_map[((rgb & 0xf8000000) >> 17)
								| ((rgb & 0xf80000) >> 14)
								| ((rgb & 0xf800) >> 11)];
}

// IndexForRGB24
inline
uint8
PaletteConverter::IndexForRGB24(uint8 red, uint8 green, uint8 blue) const
{
	return fColorMap->index_map[((red & 0xf8) << 7)
								| ((green & 0xf8) << 2)
								| (blue >> 3)];
}

// IndexForGrey
inline
uint8
PaletteConverter::IndexForGrey(uint8 grey) const
{
	return IndexForRGB24(grey, grey, grey);
}

// RGBColorForIndex
inline
const rgb_color &
PaletteConverter::RGBColorForIndex(uint8 index) const
{
	return fColorMap->color_list[index];
}

// RGB15ColorForIndex
inline
uint16
PaletteConverter::RGB15ColorForIndex(uint8 index) const
{
	const rgb_color &color = fColorMap->color_list[index];
	return ((color.red & 0xf8) << 7)
		   | ((color.green & 0xf8) << 2)
		   | (color.blue >> 3);
}

// RGB16ColorForIndex
inline
uint16
PaletteConverter::RGB16ColorForIndex(uint8 index) const
{
	const rgb_color &color = fColorMap->color_list[index];
	return ((color.red & 0xf8) << 8)
		   | ((color.green & 0xfc) << 3)
		   | (color.blue >> 3);
}

// RGB24ColorForIndex
inline
uint32
PaletteConverter::RGB24ColorForIndex(uint8 index) const
{
	const rgb_color &color = fColorMap->color_list[index];
	return (color.red << 24) | (color.green << 16) | (color.blue >> 8);
}

// GreyColorForIndex
inline
uint8
PaletteConverter::GreyColorForIndex(uint8 index) const
{
	const rgb_color &color = fColorMap->color_list[index];
	return brightness_for(color.red, color.green, color.blue);
}


/////////////
// BBitmap //
/////////////

// constructor
BBitmap::BBitmap(BRect bounds, uint32 flags, color_space colorSpace,
				 int32 bytesPerRow, screen_id screenID)
	: fBasePtr(NULL),
	  fSize(0),
	  fColorSpace(B_NO_COLOR_SPACE),
	  fBounds(0, 0, -1, -1),
	  fBytesPerRow(0),
	  fWindow(NULL),
	  fServerToken(-1),
	  fToken(-1),
	  fArea(-1),
	  fOrigArea(-1),
	  fFlags(0),
	  fInitError(B_NO_INIT)
{
	InitObject(bounds, colorSpace, flags, bytesPerRow, screenID);
}

// constructor
BBitmap::BBitmap(BRect bounds, color_space colorSpace, bool acceptsViews,
				 bool needsContiguous)
	: fBasePtr(NULL),
	  fSize(0),
	  fColorSpace(B_NO_COLOR_SPACE),
	  fBounds(0, 0, -1, -1),
	  fBytesPerRow(0),
	  fWindow(NULL),
	  fServerToken(-1),
	  fToken(-1),
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
BBitmap::BBitmap(const BBitmap *source, bool acceptsViews,
				 bool needsContiguous)
	: fBasePtr(NULL),
	  fSize(0),
	  fColorSpace(B_NO_COLOR_SPACE),
	  fBounds(0, 0, -1, -1),
	  fBytesPerRow(0),
	  fWindow(NULL),
	  fServerToken(-1),
	  fToken(-1),
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
BBitmap::~BBitmap()
{
	if (fBasePtr)
		free(fBasePtr);
}

// unarchiving constructor
BBitmap::BBitmap(BMessage *data)
{
}

// Instantiate
BArchivable *
BBitmap::Instantiate(BMessage *data)
{
	return NULL;	// not implemented
}

// Archive
status_t
BBitmap::Archive(BMessage *data, bool deep) const
{
	return NOT_IMPLEMENTED;
}

// InitCheck
status_t
BBitmap::InitCheck() const
{
	return fInitError;
}

// IsValid
bool
BBitmap::IsValid() const
{
	return (InitCheck() == B_OK);
}

// LockBits
status_t
BBitmap::LockBits(uint32 *state)
{
	return NOT_IMPLEMENTED;
}

// UnlockBits
void
BBitmap::UnlockBits()
{
}

// Area
area_id
BBitmap::Area() const
{
	return NOT_IMPLEMENTED;
}

// Bits
void *
BBitmap::Bits() const
{
	return fBasePtr;
}

// BitsLength
int32
BBitmap::BitsLength() const
{
	return fSize;
}

// BytesPerRow
int32
BBitmap::BytesPerRow() const
{
	return fBytesPerRow;
}

// ColorSpace
color_space
BBitmap::ColorSpace() const
{
	return fColorSpace;
}

// Bounds
BRect
BBitmap::Bounds() const
{
	return fBounds;
}

// SetBits
void
BBitmap::SetBits(const void *data, int32 length, int32 offset,
				 color_space colorSpace)
{
}

// GetOverlayRestrictions
status_t
BBitmap::GetOverlayRestrictions(overlay_restrictions *restrictions) const
{
	return NOT_IMPLEMENTED;
}

// AddChild
void
BBitmap::AddChild(BView *view)
{
}

// RemoveChild
bool
BBitmap::RemoveChild(BView *view)
{
	return false;	// not implemented
}

// CountChildren
int32
BBitmap::CountChildren() const
{
	return 0;	// not implemented
}

// ChildAt
BView *
BBitmap::ChildAt(int32 index) const
{
	return NULL;	// not implemented
}

// FindView
BView *
BBitmap::FindView(const char *viewName) const
{
	return NULL;	// not implemented
}

// FindView
BView *
BBitmap::FindView(BPoint point) const
{
	return NULL;	// not implemented
}

// Lock
bool
BBitmap::Lock()
{
	return false;	// not implemented
}

// Unlock
void
BBitmap::Unlock()
{
}

// IsLocked
bool
BBitmap::IsLocked() const
{
	return false;	// not implemented
}

// Perform
status_t
BBitmap::Perform(perform_code d, void *arg)
{
	return NOT_IMPLEMENTED;
}

// FBC
void BBitmap::_ReservedBitmap1() {}
void BBitmap::_ReservedBitmap2() {}
void BBitmap::_ReservedBitmap3() {}

// copy constructor
BBitmap::BBitmap(const BBitmap &)
{
}

// =
BBitmap &
BBitmap::operator=(const BBitmap &)
{
	return *this;
}

// get_shared_pointer
char *
BBitmap::get_shared_pointer() const
{
	return NULL;	// not implemented
}

// set_bits
void
BBitmap::set_bits(long offset, char *data, long length)
{
}

// set_bits_24
void
BBitmap::set_bits_24(long offset, char *data, long length)
{
}

// set_bits_24_local_gray
void
BBitmap::set_bits_24_local_gray(long offset, char *data, long length)
{
}

// set_bits_24_local_256
void
BBitmap::set_bits_24_local_256(long offset, uchar *data, long length)
{
}

// set_bits_24_24
void
BBitmap::set_bits_24_24(long offset, char *data, long length,
						bool bigEndianDest)
{
}

// set_bits_8_24
void
BBitmap::set_bits_8_24(long offset, char *data, long length,
					   bool bigEndianDest)
{
}

// set_bits_gray_24
void
BBitmap::set_bits_gray_24(long offset, char *data, long length,
						  bool bigEndianDest)
{
}

// get_server_token
int32
BBitmap::get_server_token() const
{
	return -1;	// not implemented
}

// InitObject
void
BBitmap::InitObject(BRect bounds, color_space colorSpace, uint32 flags,
					int32 bytesPerRow, screen_id screenID)
{
	// clean up
	if (fBasePtr) {
		free(fBasePtr);
		fBasePtr = NULL;
	}
	// check params
	status_t error = B_OK;
	if (!bounds.IsValid() || !is_supported(colorSpace))
		error = B_BAD_VALUE;
	if (error == B_OK) {
		int32 bpr = get_bytes_per_row(colorSpace, bounds.IntegerWidth() + 1);
		if (bytesPerRow < 0)
			bytesPerRow = bpr;
		else if (bytesPerRow < bpr)
			error = B_BAD_VALUE;
	}
	// allocate the bitmap buffer
	if (error == B_OK) {
		int32 size = bytesPerRow * (bounds.IntegerWidth() + 1);
		fBasePtr = malloc(size);
		if (fBasePtr) {
			fSize = size;
			fColorSpace = colorSpace;
			fBounds = bounds;
			fBytesPerRow = bytesPerRow;
			fWindow = NULL;
			fServerToken = -1;
			fToken = -1;
			fArea = -1;
			fOrigArea = -1;
			fFlags = flags;
		} else
			error = B_NO_MEMORY;
	}
	fInitError = error;
}

// AssertPtr
void
BBitmap::AssertPtr()
{
}

