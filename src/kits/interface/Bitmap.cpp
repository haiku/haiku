//------------------------------------------------------------------------------
//	Copyright (c) 2001-2004, OpenBeOS
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
//					DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	BBitmap objects represent off-screen windows that
//					contain bitmap data.
//------------------------------------------------------------------------------

#include <algorithm>
#include <limits.h>
#include <new>
#include <stdlib.h>

#include <Bitmap.h>
#include <InterfaceDefs.h>
#include <Locker.h>
#include <OS.h>

// Includes to be able to talk to the app_server
#include <Application.h>
#include <ServerProtocol.h>
#include <PortMessage.h>
#include <AppServerLink.h>

enum {
	NOT_IMPLEMENTED	= B_ERROR,
};

// TODO: system palette -- hard-coded for now, when the app server is ready
// we should use system_colors() or BScreen::ColorMap().
const rgb_color kSystemPalette[] = {
 {   0,   0,   0, 255 }, {   8,   8,   8, 255 }, {  16,  16,  16, 255 },
 {  24,  24,  24, 255 }, {  32,  32,  32, 255 }, {  40,  40,  40, 255 },
 {  48,  48,  48, 255 }, {  56,  56,  56, 255 }, {  64,  64,  64, 255 },
 {  72,  72,  72, 255 }, {  80,  80,  80, 255 }, {  88,  88,  88, 255 },
 {  96,  96,  96, 255 }, { 104, 104, 104, 255 }, { 112, 112, 112, 255 },
 { 120, 120, 120, 255 }, { 128, 128, 128, 255 }, { 136, 136, 136, 255 },
 { 144, 144, 144, 255 }, { 152, 152, 152, 255 }, { 160, 160, 160, 255 },
 { 168, 168, 168, 255 }, { 176, 176, 176, 255 }, { 184, 184, 184, 255 },
 { 192, 192, 192, 255 }, { 200, 200, 200, 255 }, { 208, 208, 208, 255 },
 { 216, 216, 216, 255 }, { 224, 224, 224, 255 }, { 232, 232, 232, 255 },
 { 240, 240, 240, 255 }, { 248, 248, 248, 255 }, {   0,   0, 255, 255 },
 {   0,   0, 229, 255 }, {   0,   0, 204, 255 }, {   0,   0, 179, 255 },
 {   0,   0, 154, 255 }, {   0,   0, 129, 255 }, {   0,   0, 105, 255 },
 {   0,   0,  80, 255 }, {   0,   0,  55, 255 }, {   0,   0,  30, 255 },
 { 255,   0,   0, 255 }, { 228,   0,   0, 255 }, { 203,   0,   0, 255 },
 { 178,   0,   0, 255 }, { 153,   0,   0, 255 }, { 128,   0,   0, 255 },
 { 105,   0,   0, 255 }, {  80,   0,   0, 255 }, {  55,   0,   0, 255 },
 {  30,   0,   0, 255 }, {   0, 255,   0, 255 }, {   0, 228,   0, 255 },
 {   0, 203,   0, 255 }, {   0, 178,   0, 255 }, {   0, 153,   0, 255 },
 {   0, 128,   0, 255 }, {   0, 105,   0, 255 }, {   0,  80,   0, 255 },
 {   0,  55,   0, 255 }, {   0,  30,   0, 255 }, {   0, 152,  51, 255 },
 { 255, 255, 255, 255 }, { 203, 255, 255, 255 }, { 203, 255, 203, 255 },
 { 203, 255, 152, 255 }, { 203, 255, 102, 255 }, { 203, 255,  51, 255 },
 { 203, 255,   0, 255 }, { 152, 255, 255, 255 }, { 152, 255, 203, 255 },
 { 152, 255, 152, 255 }, { 152, 255, 102, 255 }, { 152, 255,  51, 255 },
 { 152, 255,   0, 255 }, { 102, 255, 255, 255 }, { 102, 255, 203, 255 },
 { 102, 255, 152, 255 }, { 102, 255, 102, 255 }, { 102, 255,  51, 255 },
 { 102, 255,   0, 255 }, {  51, 255, 255, 255 }, {  51, 255, 203, 255 },
 {  51, 255, 152, 255 }, {  51, 255, 102, 255 }, {  51, 255,  51, 255 },
 {  51, 255,   0, 255 }, { 255, 152, 255, 255 }, { 255, 152, 203, 255 },
 { 255, 152, 152, 255 }, { 255, 152, 102, 255 }, { 255, 152,  51, 255 },
 { 255, 152,   0, 255 }, {   0, 102, 255, 255 }, {   0, 102, 203, 255 },
 { 203, 203, 255, 255 }, { 203, 203, 203, 255 }, { 203, 203, 152, 255 },
 { 203, 203, 102, 255 }, { 203, 203,  51, 255 }, { 203, 203,   0, 255 },
 { 152, 203, 255, 255 }, { 152, 203, 203, 255 }, { 152, 203, 152, 255 },
 { 152, 203, 102, 255 }, { 152, 203,  51, 255 }, { 152, 203,   0, 255 },
 { 102, 203, 255, 255 }, { 102, 203, 203, 255 }, { 102, 203, 152, 255 },
 { 102, 203, 102, 255 }, { 102, 203,  51, 255 }, { 102, 203,   0, 255 },
 {  51, 203, 255, 255 }, {  51, 203, 203, 255 }, {  51, 203, 152, 255 },
 {  51, 203, 102, 255 }, {  51, 203,  51, 255 }, {  51, 203,   0, 255 },
 { 255, 102, 255, 255 }, { 255, 102, 203, 255 }, { 255, 102, 152, 255 },
 { 255, 102, 102, 255 }, { 255, 102,  51, 255 }, { 255, 102,   0, 255 },
 {   0, 102, 152, 255 }, {   0, 102, 102, 255 }, { 203, 152, 255, 255 },
 { 203, 152, 203, 255 }, { 203, 152, 152, 255 }, { 203, 152, 102, 255 },
 { 203, 152,  51, 255 }, { 203, 152,   0, 255 }, { 152, 152, 255, 255 },
 { 152, 152, 203, 255 }, { 152, 152, 152, 255 }, { 152, 152, 102, 255 },
 { 152, 152,  51, 255 }, { 152, 152,   0, 255 }, { 102, 152, 255, 255 },
 { 102, 152, 203, 255 }, { 102, 152, 152, 255 }, { 102, 152, 102, 255 },
 { 102, 152,  51, 255 }, { 102, 152,   0, 255 }, {  51, 152, 255, 255 },
 {  51, 152, 203, 255 }, {  51, 152, 152, 255 }, {  51, 152, 102, 255 },
 {  51, 152,  51, 255 }, {  51, 152,   0, 255 }, { 230, 134,   0, 255 },
 { 255,  51, 203, 255 }, { 255,  51, 152, 255 }, { 255,  51, 102, 255 },
 { 255,  51,  51, 255 }, { 255,  51,   0, 255 }, {   0, 102,  51, 255 },
 {   0, 102,   0, 255 }, { 203, 102, 255, 255 }, { 203, 102, 203, 255 },
 { 203, 102, 152, 255 }, { 203, 102, 102, 255 }, { 203, 102,  51, 255 },
 { 203, 102,   0, 255 }, { 152, 102, 255, 255 }, { 152, 102, 203, 255 },
 { 152, 102, 152, 255 }, { 152, 102, 102, 255 }, { 152, 102,  51, 255 },
 { 152, 102,   0, 255 }, { 102, 102, 255, 255 }, { 102, 102, 203, 255 },
 { 102, 102, 152, 255 }, { 102, 102, 102, 255 }, { 102, 102,  51, 255 },
 { 102, 102,   0, 255 }, {  51, 102, 255, 255 }, {  51, 102, 203, 255 },
 {  51, 102, 152, 255 }, {  51, 102, 102, 255 }, {  51, 102,  51, 255 },
 {  51, 102,   0, 255 }, { 255,   0, 255, 255 }, { 255,   0, 203, 255 },
 { 255,   0, 152, 255 }, { 255,   0, 102, 255 }, { 255,   0,  51, 255 },
 { 255, 175,  19, 255 }, {   0,  51, 255, 255 }, {   0,  51, 203, 255 },
 { 203,  51, 255, 255 }, { 203,  51, 203, 255 }, { 203,  51, 152, 255 },
 { 203,  51, 102, 255 }, { 203,  51,  51, 255 }, { 203,  51,   0, 255 },
 { 152,  51, 255, 255 }, { 152,  51, 203, 255 }, { 152,  51, 152, 255 },
 { 152,  51, 102, 255 }, { 152,  51,  51, 255 }, { 152,  51,   0, 255 },
 { 102,  51, 255, 255 }, { 102,  51, 203, 255 }, { 102,  51, 152, 255 },
 { 102,  51, 102, 255 }, { 102,  51,  51, 255 }, { 102,  51,   0, 255 },
 {  51,  51, 255, 255 }, {  51,  51, 203, 255 }, {  51,  51, 152, 255 },
 {  51,  51, 102, 255 }, {  51,  51,  51, 255 }, {  51,  51,   0, 255 },
 { 255, 203, 102, 255 }, { 255, 203, 152, 255 }, { 255, 203, 203, 255 },
 { 255, 203, 255, 255 }, {   0,  51, 152, 255 }, {   0,  51, 102, 255 },
 {   0,  51,  51, 255 }, {   0,  51,   0, 255 }, { 203,   0, 255, 255 },
 { 203,   0, 203, 255 }, { 203,   0, 152, 255 }, { 203,   0, 102, 255 },
 { 203,   0,  51, 255 }, { 255, 227,  70, 255 }, { 152,   0, 255, 255 },
 { 152,   0, 203, 255 }, { 152,   0, 152, 255 }, { 152,   0, 102, 255 },
 { 152,   0,  51, 255 }, { 152,   0,   0, 255 }, { 102,   0, 255, 255 },
 { 102,   0, 203, 255 }, { 102,   0, 152, 255 }, { 102,   0, 102, 255 },
 { 102,   0,  51, 255 }, { 102,   0,   0, 255 }, {  51,   0, 255, 255 },
 {  51,   0, 203, 255 }, {  51,   0, 152, 255 }, {  51,   0, 102, 255 },
 {  51,   0,  51, 255 }, {  51,   0,   0, 255 }, { 255, 203,  51, 255 },
 { 255, 203,   0, 255 }, { 255, 255,   0, 255 }, { 255, 255,  51, 255 },
 { 255, 255, 102, 255 }, { 255, 255, 152, 255 }, { 255, 255, 203, 255 },
 { 255, 255, 255, 255 }
};

// is_supported
/*!	\brief Returns whether or not a color space is supported by BBitmap.
	\param colorSpace The color space in question.
	\return \c true, if \a colorSpace is supported, \c false otherwise.
*/
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
		case B_YCbCr422: case B_YCbCr411: case B_YCbCr444: case B_YCbCr420:
		case B_YUV422: case B_YUV411: case B_YUV444: case B_YUV420:
		case B_UVL24: case B_UVL32: case B_UVLA32:
		case B_LAB24: case B_LAB32: case B_LABA32:
		case B_HSI24: case B_HSI32: case B_HSIA32:
		case B_HSV24: case B_HSV32: case B_HSVA32:
		case B_HLS24: case B_HLS32: case B_HLSA32:
		case B_CMY24: case B_CMY32: case B_CMYA32: case B_CMYK32:
			result = true;
			break;
		// unsupported
		case B_NO_COLOR_SPACE:
		case B_YUV9: case B_YUV12:
			break;
	}
	return result;
}

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

// brightness_for
/*!	\brief Returns the brightness of an RGB 24 color.
	\param red Value of the red component.
	\param green Value of the green component.
	\param blue Value of the blue component.
	\return The brightness for the supplied RGB color as a value between 0
			and 255.
*/
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
/*!	\brief Returns the "distance" between two RGB colors.

	This functions defines an metric on the RGB color space. The distance
	between two colors is 0, if and only if the colors are equal.

	\param red1 Red component of the first color.
	\param green1 Green component of the first color.
	\param blue1 Blue component of the first color.
	\param red2 Red component of the second color.
	\param green2 Green component of the second color.
	\param blue2 Blue component of the second color.
	\return The distance between the given colors.
*/
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

// bit_mask, inverse_bit_mask
static inline int32 bit_mask(int32 bit)			{ return (1 << bit); }
static inline int32 inverse_bit_mask(int32 bit)	{ return ~bit_mask(bit); }


//////////////////////
// PaletteConverter //
//////////////////////

namespace BPrivate {

/*!	\brief Helper class for conversion between RGB and palette colors.
*/
class PaletteConverter {
public:
	PaletteConverter();
	PaletteConverter(const rgb_color *palette);
	PaletteConverter(const color_map *colorMap);
	~PaletteConverter();

	status_t SetTo(const rgb_color *palette);
	status_t SetTo(const color_map *colorMap);
	status_t InitCheck() const;

	inline uint8 IndexForRGB15(uint16 rgb) const;
	inline uint8 IndexForRGB15(uint8 red, uint8 green, uint8 blue) const;
	inline uint8 IndexForRGB16(uint16 rgb) const;
	inline uint8 IndexForRGB16(uint8 red, uint8 green, uint8 blue) const;
	inline uint8 IndexForRGB24(uint32 rgb) const;
	inline uint8 IndexForRGB24(uint8 red, uint8 green, uint8 blue) const;
	inline uint8 IndexForGray(uint8 gray) const;

	inline const rgb_color &RGBColorForIndex(uint8 index) const;
	inline uint16 RGB15ColorForIndex(uint8 index) const;
	inline uint16 RGB16ColorForIndex(uint8 index) const;
	inline uint32 RGB24ColorForIndex(uint8 index) const;
	inline void RGB24ColorForIndex(uint8 index, uint8 &red, uint8 &green,
								   uint8 &blue) const;
	inline uint8 GrayColorForIndex(uint8 index) const;

private:
	const color_map	*fColorMap;
	color_map		*fOwnColorMap;
	status_t		fCStatus;
};

}	// namespace BPrivate

using BPrivate::PaletteConverter;
using namespace std;

// constructor
/*!	\brief Creates an uninitialized PaletteConverter.
*/
PaletteConverter::PaletteConverter()
	: fColorMap(NULL),
	  fOwnColorMap(NULL),
	  fCStatus(B_NO_INIT)
{
}

// constructor
/*!	\brief Creates a PaletteConverter and initializes it to the supplied
		   palette.
	\param palette The palette being a 256 entry rgb_color array.
*/
PaletteConverter::PaletteConverter(const rgb_color *palette)
	: fColorMap(NULL),
	  fOwnColorMap(NULL),
	  fCStatus(B_NO_INIT)
{
	SetTo(palette);
}

// constructor
/*!	\brief Creates a PaletteConverter and initializes it to the supplied
		   color map.
	\param colorMap The completely initialized color map.
*/
PaletteConverter::PaletteConverter(const color_map *colorMap)
	: fColorMap(NULL),
	  fOwnColorMap(NULL),
	  fCStatus(B_NO_INIT)
{
	SetTo(colorMap);
}

// destructor
/*!	\brief Frees all resources associated with this object.
*/
PaletteConverter::~PaletteConverter()
{
	delete fOwnColorMap;
}

// SetTo
/*!	\brief Initializes the converter to the supplied palette.
	\param palette The palette being a 256 entry rgb_color array.
	\return \c B_OK, if everything went fine, an error code otherwise.
*/
status_t
PaletteConverter::SetTo(const rgb_color *palette)
{
	// cleanup
	SetTo((const color_map*)NULL);
	status_t error = (palette ? B_OK : B_BAD_VALUE);
	// alloc color map
	if (error == B_OK) {
		fOwnColorMap = new(nothrow) color_map;
		if (fOwnColorMap == NULL)
			error = B_NO_MEMORY;
	}
	// init color map
	if (error == B_OK) {
		fColorMap = fOwnColorMap;
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
	fCStatus = error;
	return error;
}

// SetTo
/*!	\brief Initializes the converter to the supplied color map.
	\param colorMap The completely initialized color map.
	\return \c B_OK, if everything went fine, an error code otherwise.
*/
status_t
PaletteConverter::SetTo(const color_map *colorMap)
{
	// cleanup
	if (fOwnColorMap) {
		delete fOwnColorMap;
		fOwnColorMap = NULL;
	}
	// set
	fColorMap = colorMap;
	fCStatus = (fColorMap ? B_OK : B_BAD_VALUE);
	return fCStatus;
}

// InitCheck
/*!	\brief Returns the result of the last initialization via constructor or
		   SetTo().
	\return \c B_OK, if the converter is properly initialized, an error code
			otherwise.
*/
status_t
PaletteConverter::InitCheck() const
{
	return fCStatus;
}

// IndexForRGB15
/*!	\brief Returns the palette color index closest to a given RGB 15 color.

	The object must be properly initialized.

	\param rgb The RGB 15 color value (R[14:10]G[9:5]B[4:0]).
	\return The palette color index for the supplied color.
*/
inline
uint8
PaletteConverter::IndexForRGB15(uint16 rgb) const
{
	return fColorMap->index_map[rgb];
}

// IndexForRGB15
/*!	\brief Returns the palette color index closest to a given RGB 15 color.

	The object must be properly initialized.

	\param red Red component of the color (R[4:0]).
	\param green Green component of the color (G[4:0]).
	\param blue Blue component of the color (B[4:0]).
	\return The palette color index for the supplied color.
*/
inline
uint8
PaletteConverter::IndexForRGB15(uint8 red, uint8 green, uint8 blue) const
{
	// the 5 least significant bits are used
	return fColorMap->index_map[(red << 10) | (green << 5) | blue];
}

// IndexForRGB16
/*!	\brief Returns the palette color index closest to a given RGB 16 color.

	The object must be properly initialized.

	\param rgb The RGB 16 color value (R[15:11]G[10:5]B[4:0]).
	\return The palette color index for the supplied color.
*/
inline
uint8
PaletteConverter::IndexForRGB16(uint16 rgb) const
{
	return fColorMap->index_map[(rgb >> 1) & 0x7fe0 | rgb & 0x1f];
}

// IndexForRGB16
/*!	\brief Returns the palette color index closest to a given RGB 16 color.

	The object must be properly initialized.

	\param red Red component of the color (R[4:0]).
	\param green Green component of the color (G[5:0]).
	\param blue Blue component of the color (B[4:0]).
	\return The palette color index for the supplied color.
*/
inline
uint8
PaletteConverter::IndexForRGB16(uint8 red, uint8 green, uint8 blue) const
{
	// the 5 (for red, blue) / 6 (for green) least significant bits are used
	return fColorMap->index_map[(red << 10) | ((green & 0x3e) << 4) | blue];
}

// IndexForRGB24
/*!	\brief Returns the palette color index closest to a given RGB 32 color.

	The object must be properly initialized.

	\param rgb The RGB 32 color value (R[31:24]G[23:16]B[15:8]).
	\return The palette color index for the supplied color.
*/
inline
uint8
PaletteConverter::IndexForRGB24(uint32 rgb) const
{
	return fColorMap->index_map[((rgb & 0xf8000000) >> 17)
								| ((rgb & 0xf80000) >> 14)
								| ((rgb & 0xf800) >> 11)];
}

// IndexForRGB24
/*!	\brief Returns the palette color index closest to a given RGB 24 color.

	The object must be properly initialized.

	\param red Red component of the color.
	\param green Green component of the color.
	\param blue Blue component of the color.
	\return The palette color index for the supplied color.
*/
inline
uint8
PaletteConverter::IndexForRGB24(uint8 red, uint8 green, uint8 blue) const
{
	return fColorMap->index_map[((red & 0xf8) << 7)
								| ((green & 0xf8) << 2)
								| (blue >> 3)];
}

// IndexForGray
/*!	\brief Returns the palette color index closest to a given Gray 8 color.

	The object must be properly initialized.

	\param gray The Gray 8 color value.
	\return The palette color index for the supplied color.
*/
inline
uint8
PaletteConverter::IndexForGray(uint8 gray) const
{
	return IndexForRGB24(gray, gray, gray);
}

// RGBColorForIndex
/*!	\brief Returns the RGB color for a given palette color index.

	The object must be properly initialized.

	\param index The palette color index.
	\return The color for the supplied palette color index.
*/
inline
const rgb_color &
PaletteConverter::RGBColorForIndex(uint8 index) const
{
	return fColorMap->color_list[index];
}

// RGB15ColorForIndex
/*!	\brief Returns the RGB 15 color for a given palette color index.

	The object must be properly initialized.

	\param index The palette color index.
	\return The color for the supplied palette color index
			(R[14:10]G[9:5]B[4:0]).
*/
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
/*!	\brief Returns the RGB 16 color for a given palette color index.

	The object must be properly initialized.

	\param index The palette color index.
	\return The color for the supplied palette color index
			(R[15:11]G[10:5]B[4:0]).
*/
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
/*!	\brief Returns the RGB 24 color for a given palette color index.

	The object must be properly initialized.

	\param index The palette color index.
	\return The color for the supplied palette color index
			(R[31:24]G[23:16]B[15:8]).
*/
inline
uint32
PaletteConverter::RGB24ColorForIndex(uint8 index) const
{
	const rgb_color &color = fColorMap->color_list[index];
	return (color.red << 24) | (color.green << 16) | (color.blue >> 8);
}

// RGB24ColorForIndex
/*!	\brief Returns the RGB 24 color for a given palette color index.

	The object must be properly initialized.

	\param index The palette color index.
	\param red Reference to the variable the red component shall be stored
		   into.
	\param green Reference to the variable the green component shall be stored
		   into.
	\param blue Reference to the variable the blue component shall be stored
		   into.
*/
inline
void
PaletteConverter::RGB24ColorForIndex(uint8 index, uint8 &red, uint8 &green,
									 uint8 &blue) const
{
	const rgb_color &color = fColorMap->color_list[index];
	red = color.red;
	green = color.green;
	blue = color.blue;
}

// GrayColorForIndex
/*!	\brief Returns the Gray 8 color for a given palette color index.

	The object must be properly initialized.

	\param index The palette color index.
	\return The color for the supplied palette color index.
*/
inline
uint8
PaletteConverter::GrayColorForIndex(uint8 index) const
{
	const rgb_color &color = fColorMap->color_list[index];
	return brightness_for(color.red, color.green, color.blue);
}

// TODO: Remove these and palette_converter() when BScreen is available.
static BLocker			gPaletteConverterLock;
static PaletteConverter	gPaletteConverter;

// palette_converter
/*!	\brief Returns a PaletteConverter using the system color palette.
	\return A PaletteConverter.
*/
static
const PaletteConverter*
palette_converter()
{
	if (gPaletteConverterLock.Lock()) {
		if (gPaletteConverter.InitCheck() != B_OK)
			gPaletteConverter.SetTo(kSystemPalette);
		gPaletteConverterLock.Unlock();
	}
	return &gPaletteConverter;
}


/////////////
// BBitmap //
/////////////

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
	  fToken(-1),
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
/*!	\brief Frees all resources associated with this object.
*/
BBitmap::~BBitmap()
{
	// TODO: don't free fBasePtr, as it's owned by the app_server
	// should probably decrement an associated reference count, though,
	// so the app_server knows this bitmap isn't used anymore
}

// unarchiving constructor
/*!	\brief Unarchives a bitmap from a BMessage.

	Implements BFlattenable.

	\param data The archive.
*/
BBitmap::BBitmap(BMessage *data)
{
}

// Instantiate
/*!	\brief Instantiates a BBitmap from an archive.

	Implements BFlattenable.

	\param data The archive.
	\return A bitmap reconstructed from the archive or \c NULL, if an error
			occured.
*/
BArchivable *
BBitmap::Instantiate(BMessage *data)
{
	return NULL;	// not implemented
}

// Archive
/*!	\brief Archives the BBitmap object.

	Implements BFlattenable.

	\param data The archive.
	\param deep \c true, if child object shall be archived as well, \c false
		   otherwise.
	\return \c B_OK, if everything went fine, an error code otherwise.
*/
status_t
BBitmap::Archive(BMessage *data, bool deep) const
{
	return NOT_IMPLEMENTED;
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
	return NOT_IMPLEMENTED;
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
	return fArea;
}

// Bits
/*!	\brief Returns the pointer to the bitmap data.
	\return The pointer to the bitmap data.
*/
void *
BBitmap::Bits() const
{
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

////////////////////////////////////////
// structures defining the pixel layout

struct rgb32_pixel {
	uint8 blue;
	uint8 green;
	uint8 red;
	uint8 alpha;
};

struct rgb32_big_pixel {
	uint8 red;
	uint8 green;
	uint8 blue;
	uint8 alpha;
};

struct rgb24_pixel {
	uint8 blue;
	uint8 green;
	uint8 red;
};

struct rgb24_big_pixel {
	uint8 red;
	uint8 green;
	uint8 blue;
};

struct rgb16_pixel {
	uint8 gb;	// G[2:0],B[4:0]
	uint8 rg;	// 16: R[4:0],G[5:3]
				// 15: -[0],R[4:0],G[4:3]
};

struct rgb16_big_pixel {
	uint8 rg;	// 16: R[4:0],G[5:3]
				// 15: -[0],R[4:0],G[4:3]
	uint8 gb;	// G[2:0],B[4:0]
};

////////////////////////////////////////////////////////
// types defining what is needed to store a color value

struct rgb_color_value {
	uint8 red;
	uint8 green;
	uint8 blue;
};

typedef uint8 gray_color_value;

////////////////////////////////////////////////////////////////////
// Reader classes being able to read pixels of certain color spaces

// BaseReader
template<typename _PixelType>
struct BaseReader {
	typedef _PixelType		pixel_t;

	BaseReader(const void *data) : pixels((const pixel_t*)data) {}

	inline void SetTo(const void *data) { pixels = (const pixel_t*)data; }

	inline void NextRow(int32 skip)
	{
		pixels = (const pixel_t*)((const char*)pixels + skip);
	}

	const pixel_t *pixels;
};

// RGB24Reader
template<typename _PixelType>
struct RGB24Reader : public BaseReader<_PixelType> {
	typedef rgb_color_value	preferred_color_value_t;
	typedef _PixelType		pixel_t;

	RGB24Reader(const void *data) : BaseReader<_PixelType>(data) {}

	inline void Read(rgb_color_value &color)
	{
		const pixel_t &pixel = *pixels;
		color.red = pixel.red;
		color.green = pixel.green;
		color.blue = pixel.blue;
		pixels++;
	}

	inline void Read(gray_color_value &gray)
	{
		rgb_color_value color;
		Read(color);
		gray = brightness_for(color.red, color.green, color.blue);
	}
};

// RGB16Reader
template<typename _PixelType>
struct RGB16Reader : public BaseReader<_PixelType> {
	typedef rgb_color_value	preferred_color_value_t;
	typedef _PixelType		pixel_t;

	RGB16Reader(const void *data) : BaseReader<_PixelType>(data) {}

	inline void Read(rgb_color_value &color)
	{
		// rg: R[4:0],G[5:3]
		// gb: G[2:0],B[4:0]
		const pixel_t &pixel = *pixels;
		color.red = pixel.rg & 0xf8;
		color.green = ((pixel.rg & 0x07) << 5) & ((pixel.gb & 0xe0) >> 3);
		color.blue = (pixel.gb & 0x1f) << 3;
		color.red |= color.red >> 5;
		color.green |= color.green >> 6;
		color.blue |= color.blue >> 5;
		pixels++;
	}

	inline void Read(gray_color_value &gray)
	{
		rgb_color_value color;
		Read(color);
		gray = brightness_for(color.red, color.green, color.blue);
	}
};

// RGB15Reader
template<typename _PixelType>
struct RGB15Reader : public BaseReader<_PixelType> {
	typedef rgb_color_value	preferred_color_value_t;
	typedef _PixelType		pixel_t;

	RGB15Reader(const void *data) : BaseReader<_PixelType>(data) {}

	inline void Read(rgb_color_value &color)
	{
		// rg: -[0],R[4:0],G[4:3]
		// gb: G[2:0],B[4:0]
		const pixel_t &pixel = *pixels;
		color.red = (pixel.rg & 0x7c) << 1;
		color.green = ((pixel.rg & 0x03) << 6) & ((pixel.gb & 0xe0) >> 2);
		color.blue = (pixel.gb & 0x1f) << 3;
		color.red |= color.red >> 5;
		color.green |= color.green >> 5;
		color.blue |= color.blue >> 5;
		pixels++;
	}

	inline void Read(gray_color_value &gray)
	{
		rgb_color_value color;
		Read(color);
		gray = brightness_for(color.red, color.green, color.blue);
	}
};

// CMAP8Reader
struct CMAP8Reader : public BaseReader<uint8> {
	typedef rgb_color_value	preferred_color_value_t;

	CMAP8Reader(const void *data, const PaletteConverter &converter)
		: BaseReader<uint8>(data), converter(converter) {}

	inline void Read(rgb_color_value &color)
	{
		converter.RGB24ColorForIndex(*pixels, color.red, color.green,
									 color.blue);
		pixels++;
	}

	inline void Read(gray_color_value &gray)
	{
		gray = converter.GrayColorForIndex(*pixels);
		pixels++;
	}

	const PaletteConverter &converter;
};

// Gray8Reader
struct Gray8Reader : public BaseReader<uint8> {
	typedef gray_color_value	preferred_color_value_t;

	Gray8Reader(const void *data) : BaseReader<uint8>(data) {}

	inline void Read(rgb_color_value &color)
	{
		color.red = color.green = color.blue = *pixels;
		pixels++;
	}

	inline void Read(gray_color_value &gray)
	{
		gray = *pixels;
		pixels++;
	}
};

// Gray1Reader
struct Gray1Reader : public BaseReader<uint8> {
	typedef gray_color_value	preferred_color_value_t;

	Gray1Reader(const void *data) : BaseReader<uint8>(data), bit(7) {}

	inline void SetTo(const void *data)
	{
		pixels = (const pixel_t*)data;
		bit = 7;
	}

	inline void NextRow(int32 skip)
	{
		if (bit == 7)
			pixels = (const pixel_t*)((const char*)pixels + skip);
		else {
			pixels = (const pixel_t*)((const char*)pixels + skip + 1);
			bit = 7;
		}
	}

	inline void Read(rgb_color_value &color)
	{
		if (*pixels & bit_mask(bit))
			color.red = color.green = color.blue = 255;
		else
			color.red = color.green = color.blue = 0;
		bit--;
		if (bit == -1) {
			pixels++;
			bit = 7;
		}
	}

	inline void Read(gray_color_value &gray)
	{
		if (*pixels & bit_mask(bit))
			gray = 255;
		else
			gray = 0;
		bit--;
		if (bit == -1) {
			pixels++;
			bit = 7;
		}
	}

	int32 bit;
};


////////////////////////////////////////////////////////////////////
// Writer classes being able to read pixels of certain color spaces

// BaseWriter
template<typename _PixelType>
struct BaseWriter {
	typedef _PixelType		pixel_t;

	BaseWriter(void *data) : pixels((pixel_t*)data) {}

	inline void SetTo(void *data) { pixels = (pixel_t*)data; }

	pixel_t *pixels;
};


// RGB32Writer
template<typename _PixelType>
struct RGB32Writer : public BaseWriter<_PixelType> {
	typedef rgb_color_value	preferred_color_value_t;
	typedef _PixelType		pixel_t;

	RGB32Writer(void *data) : BaseWriter<_PixelType>(data) {}

	inline void Write(const rgb_color_value &color)
	{
		pixel_t &pixel = *pixels;
		pixel.red = color.red;
		pixel.green = color.green;
		pixel.blue = color.blue;
		pixel.alpha = 255;
		pixels++;
	}

	inline void Write(const gray_color_value &gray)
	{
		pixel_t &pixel = *pixels;
		pixel.red = gray;
		pixel.green = gray;
		pixel.blue = gray;
		pixel.alpha = 255;
		pixels++;
	}
};

// RGB24Writer
template<typename _PixelType>
struct RGB24Writer : public BaseWriter<_PixelType> {
	typedef rgb_color_value	preferred_color_value_t;
	typedef _PixelType		pixel_t;

	RGB24Writer(void *data) : BaseWriter<_PixelType>(data) {}

	inline void Write(const rgb_color_value &color)
	{
		pixel_t &pixel = *pixels;
		pixel.red = color.red;
		pixel.green = color.green;
		pixel.blue = color.blue;
		pixels++;
	}

	inline void Write(const gray_color_value &gray)
	{
		pixel_t &pixel = *pixels;
		pixel.red = gray;
		pixel.green = gray;
		pixel.blue = gray;
		pixels++;
	}
};

// RGB16Writer
template<typename _PixelType>
struct RGB16Writer : public BaseWriter<_PixelType> {
	typedef rgb_color_value	preferred_color_value_t;
	typedef _PixelType		pixel_t;

	RGB16Writer(void *data) : BaseWriter<_PixelType>(data) {}

	inline void Write(const rgb_color_value &color)
	{
		// rg: R[4:0],G[5:3]
		// gb: G[2:0],B[4:0]
		pixel_t &pixel = *pixels;
		pixel.rg = (color.red & 0xf8) | (color.green >> 5);
		pixel.gb = ((color.green & 0x1c) << 3) | (color.blue >> 3);
		pixels++;
	}

	inline void Write(const gray_color_value &gray)
	{
		pixel_t &pixel = *pixels;
		pixel.rg = (gray & 0xf8) | (gray >> 5);
		pixel.gb = ((gray & 0x1c) << 3) | (gray >> 3);
		pixels++;
	}
};

// RGB15Writer
template<typename _PixelType>
struct RGB15Writer : public BaseWriter<_PixelType> {
	typedef rgb_color_value	preferred_color_value_t;
	typedef _PixelType		pixel_t;

	RGB15Writer(void *data) : BaseWriter<_PixelType>(data) {}

	inline void Write(const rgb_color_value &color)
	{
		// rg: -[0],R[4:0],G[4:3]
		// gb: G[2:0],B[4:0]
		pixel_t &pixel = *pixels;
		pixel.rg = ((color.red & 0xf8) >> 1) | (color.green >> 6);
		pixel.gb = ((color.green & 0x38) << 2) | (color.blue >> 3);
		pixels++;
	}

	inline void Write(const gray_color_value &gray)
	{
		pixel_t &pixel = *pixels;
		pixel.rg = ((gray & 0xf8) >> 1) | (gray >> 6);
		pixel.gb = ((gray & 0x38) << 2) | (gray >> 3);
		pixels++;
	}
};

// CMAP8Writer
struct CMAP8Writer : public BaseWriter<uint8> {
	typedef rgb_color_value	preferred_color_value_t;

	CMAP8Writer(void *data, const PaletteConverter &converter)
		: BaseWriter<uint8>(data), converter(converter) {}

	inline void Write(const rgb_color_value &color)
	{
		*pixels = converter.IndexForRGB24(color.red, color.green, color.blue);
		pixels++;
	}

	inline void Write(const gray_color_value &gray)
	{
		*pixels = converter.IndexForGray(gray);
		pixels++;
	}

	const PaletteConverter &converter;
};

// Gray8Writer
struct Gray8Writer : public BaseWriter<uint8> {
	typedef gray_color_value	preferred_color_value_t;

	Gray8Writer(void *data) : BaseWriter<uint8>(data) {}

	inline void Write(const rgb_color_value &color)
	{
		*pixels = brightness_for(color.red, color.green, color.blue);
		pixels++;
	}

	inline void Write(const gray_color_value &gray)
	{
		*pixels = gray;
		pixels++;
	}
};

// Gray1Writer
struct Gray1Writer : public BaseWriter<uint8> {
	typedef gray_color_value	preferred_color_value_t;

	Gray1Writer(void *data) : BaseWriter<uint8>(data), bit(7) {}

	inline void SetTo(void *data) { pixels = (pixel_t*)data; bit = 7; }

	inline void Write(const gray_color_value &gray)
	{
		*pixels = (*pixels & inverse_bit_mask(bit))
				  | (gray & 0x80) >> (7 - bit);
		bit--;
		if (bit == -1) {
			pixels++;
			bit = 7;
		}
	}

	inline void Write(const rgb_color_value &color)
	{
		Write(brightness_for(color.red, color.green, color.blue));
	}

	int32 bit;
};


// set_bits_worker
/*!	\brief Worker function that reads bitmap data from one buffer and writes
		   it (converted) to another one.
	\param Reader The pixel reader class.
	\param Writer The pixel writer class.
	\param color_value_t The color value type used to transport a pixel from
		   the reader to the writer.
	\param inData A pointer to the buffer to be read.
	\param inLength The length (in bytes) of the "in" buffer.
	\param inBPR The number of bytes per row in the "in" buffer.
	\param inRowSkip The number of bytes per row in the "in" buffer serving as
		   padding.
	\param outData A pointer to the buffer to be written to.
	\param outLength The length (in bytes) of the "out" buffer.
	\param outOffset The offset (in bytes) relative to \a outData from which
		   the function shall start writing.
	\param outBPR The number of bytes per row in the "out" buffer.
	\param rawOutBPR The number of bytes per row in the "out" buffer actually
		   containing bitmap data (i.e. not including the padding).
	\param _reader A reader object. The pointer to the data doesn't need to
		   be initialized.
	\param _writer A writer object. The pointer to the data doesn't need to
		   be initialized.
	\return \c B_OK, if everything went fine, an error code otherwise.
*/
template<typename Reader, typename Writer, typename color_value_t>
static
status_t
set_bits_worker(const void *inData, int32 inLength, int32 inBPR,
				int32 inRowSkip, void *outData, int32 outLength,
				int32 outOffset, int32 outBPR, int32 rawOutBPR,
				Reader _reader, Writer _writer)
{
	status_t error = B_OK;
	Reader reader(_reader);
	Writer writer(_writer);
	reader.SetTo(inData);
	writer.SetTo((char*)outData + outOffset);
	const char *inEnd = (const char*)inData + inLength
						- sizeof(Reader::pixel_t);
	const char *inLastRow = (const char*)inData + inLength
							- (inBPR - inRowSkip);
	const char *outEnd = (const char*)outData + outLength
						 - sizeof(Writer::pixel_t);
	char *outRow = (char*)outData + outOffset - outOffset % outBPR;
	const char *outRowEnd = outRow + rawOutBPR - sizeof(Writer::pixel_t);
	while ((const char*)reader.pixels <= inEnd
		   && (const char*)writer.pixels <= outEnd) {
		// process one row
		if ((const char*)reader.pixels <= inLastRow) {
			// at least a complete row left
			while ((const char*)writer.pixels <= outRowEnd) {
				color_value_t color;
				reader.Read(color);
				writer.Write(color);
			}
		} else {
			// no complete row left
			// but maybe the complete end of the first row
			while ((const char*)reader.pixels <= inEnd
				   && (const char*)writer.pixels <= outRowEnd) {
				color_value_t color;
				reader.Read(color);
				writer.Write(color);
			}
		}
		// must be here, not in the if-branch (end of first row)
		outRow += outBPR;
		outRowEnd += outBPR;
		reader.NextRow(inRowSkip);
		writer.SetTo(outRow);
	}
	return error;
}

// set_bits_worker_gray1
/*!	\brief Worker function that reads bitmap data from one buffer and writes
		   it (converted) to another one, which uses color space \c B_GRAY1.
	\param Reader The pixel reader class.
	\param Writer The pixel writer class.
	\param color_value_t The color value type used to transport a pixel from
		   the reader to the writer.
	\param inData A pointer to the buffer to be read.
	\param inLength The length (in bytes) of the "in" buffer.
	\param inBPR The number of bytes per row in the "in" buffer.
	\param inRowSkip The number of bytes per row in the "in" buffer serving as
		   padding.
	\param outData A pointer to the buffer to be written to.
	\param outLength The length (in bytes) of the "out" buffer.
	\param outOffset The offset (in bytes) relative to \a outData from which
		   the function shall start writing.
	\param outBPR The number of bytes per row in the "out" buffer.
	\param width The number of pixels per row in "in" and "out" data.
	\param _reader A reader object. The pointer to the data doesn't need to
		   be initialized.
	\param _writer A writer object. The pointer to the data doesn't need to
		   be initialized.
	\return \c B_OK, if everything went fine, an error code otherwise.
*/
template<typename Reader, typename Writer, typename color_value_t>
static
status_t
set_bits_worker_gray1(const void *inData, int32 inLength, int32 inBPR,
					  int32 inRowSkip, void *outData, int32 outLength,
					  int32 outOffset, int32 outBPR, int32 width,
					  Reader _reader, Writer _writer)
{
	status_t error = B_OK;
	Reader reader(_reader);
	Writer writer(_writer);
	reader.SetTo(inData);
	writer.SetTo((char*)outData + outOffset);
	const char *inEnd = (const char*)inData + inLength
						- sizeof(Reader::pixel_t);
	const char *inLastRow = (const char*)inData + inLength
							- (inBPR - inRowSkip);
	const char *outEnd = (const char*)outData + outLength - outBPR;
	char *outRow = (char*)outData + outOffset - outOffset % outBPR;
	int32 x = max(0L, width - ((char*)outData + outOffset - outRow) * 8) - 1;
	while ((const char*)reader.pixels <= inEnd
		   && (const char*)writer.pixels <= outEnd) {
		// process one row
		if ((const char*)reader.pixels <= inLastRow) {
			// at least a complete row left
			while (x >= 0) {
				color_value_t color;
				reader.Read(color);
				writer.Write(color);
				x--;
			}
		} else {
			// no complete row left
			// but maybe the complete end of the first row
			while ((const char*)reader.pixels <= inEnd && x >= 0) {
				color_value_t color;
				reader.Read(color);
				writer.Write(color);
				x--;
			}
		}
		// must be here, not in the if-branch (end of first row)
		x = width - 1;
		outRow += outBPR;
		reader.NextRow(inRowSkip);
		writer.SetTo(outRow);
	}
	return error;
}

// set_bits
/*!	\brief Helper function that reads bitmap data from one buffer and writes
		   it (converted) to another one.
	\param Reader The pixel reader class.
	\param inData A pointer to the buffer to be read.
	\param inLength The length (in bytes) of the "in" buffer.
	\param inBPR The number of bytes per row in the "in" buffer.
	\param inRowSkip The number of bytes per row in the "in" buffer serving as
		   padding.
	\param outData A pointer to the buffer to be written to.
	\param outLength The length (in bytes) of the "out" buffer.
	\param outOffset The offset (in bytes) relative to \a outData from which
		   the function shall start writing.
	\param outBPR The number of bytes per row in the "out" buffer.
	\param rawOutBPR The number of bytes per row in the "out" buffer actually
		   containing bitmap data (i.e. not including the padding).
	\param outColorSpace Color space of the target buffer.
	\param width The number of pixels per row in "in" and "out" data.
	\param reader A reader object. The pointer to the data doesn't need to
		   be initialized.
	\param paletteConverter Reference to a PaletteConverter to be used, if
		   a conversion from or to \c B_CMAP8 has to be done.
	\return \c B_OK, if everything went fine, an error code otherwise.
*/
template<typename Reader>
static
status_t
set_bits(const void *inData, int32 inLength, int32 inBPR, int32 inRowSkip,
		 void *outData, int32 outLength, int32 outOffset, int32 outBPR,
		 int32 rawOutBPR, color_space outColorSpace, int32 width,
		 Reader reader, const PaletteConverter &paletteConverter)
{
	status_t error = B_OK;
	switch (outColorSpace) {
		// supported
		case B_RGB32: case B_RGBA32:
		{
			typedef RGB32Writer<rgb32_pixel> Writer;
			typedef typename Reader::preferred_color_value_t color_value_t;
			error = set_bits_worker<Reader, Writer, color_value_t>(
				inData, inLength, inBPR, inRowSkip, outData, outLength,
				outOffset, outBPR, rawOutBPR, reader, Writer(outData));
			break;
		}
		case B_RGB32_BIG: case B_RGBA32_BIG:
		{
			typedef RGB32Writer<rgb32_big_pixel> Writer;
			typedef typename Reader::preferred_color_value_t color_value_t;
			error = set_bits_worker<Reader, Writer, color_value_t>(
				inData, inLength, inBPR, inRowSkip, outData, outLength,
				outOffset, outBPR, rawOutBPR, reader, Writer(outData));
			break;
		}
		case B_RGB24:
		{
			typedef RGB24Writer<rgb24_pixel> Writer;
			typedef typename Reader::preferred_color_value_t color_value_t;
			error = set_bits_worker<Reader, Writer, color_value_t>(
				inData, inLength, inBPR, inRowSkip, outData, outLength,
				outOffset, outBPR, rawOutBPR, reader, Writer(outData));
			break;
		}
		case B_RGB24_BIG:
		{
			typedef RGB24Writer<rgb24_big_pixel> Writer;
			typedef typename Reader::preferred_color_value_t color_value_t;
			error = set_bits_worker<Reader, Writer, color_value_t>(
				inData, inLength, inBPR, inRowSkip, outData, outLength,
				outOffset, outBPR, rawOutBPR, reader, Writer(outData));
			break;
		}
		case B_RGB16:
		{
			typedef RGB16Writer<rgb16_pixel> Writer;
			typedef typename Reader::preferred_color_value_t color_value_t;
			error = set_bits_worker<Reader, Writer, color_value_t>(
				inData, inLength, inBPR, inRowSkip, outData, outLength,
				outOffset, outBPR, rawOutBPR, reader, Writer(outData));
			break;
		}
		case B_RGB16_BIG:
		{
			typedef RGB16Writer<rgb16_big_pixel> Writer;
			typedef typename Reader::preferred_color_value_t color_value_t;
			error = set_bits_worker<Reader, Writer, color_value_t>(
				inData, inLength, inBPR, inRowSkip, outData, outLength,
				outOffset, outBPR, rawOutBPR, reader, Writer(outData));
			break;
		}
		case B_RGB15: case B_RGBA15:
		{
			typedef RGB15Writer<rgb16_pixel> Writer;
			typedef typename Reader::preferred_color_value_t color_value_t;
			error = set_bits_worker<Reader, Writer, color_value_t>(
				inData, inLength, inBPR, inRowSkip, outData, outLength,
				outOffset, outBPR, rawOutBPR, reader, Writer(outData));
			break;
		}
		case B_RGB15_BIG: case B_RGBA15_BIG:
		{
			typedef RGB15Writer<rgb16_big_pixel> Writer;
			typedef typename Reader::preferred_color_value_t color_value_t;
			error = set_bits_worker<Reader, Writer, color_value_t>(
				inData, inLength, inBPR, inRowSkip, outData, outLength,
				outOffset, outBPR, rawOutBPR, reader, Writer(outData));
			break;
		}
		case B_CMAP8:
		{
			typedef CMAP8Writer Writer;
			typedef typename Reader::preferred_color_value_t color_value_t;
			error = set_bits_worker<Reader, Writer, color_value_t>(
				inData, inLength, inBPR, inRowSkip, outData, outLength,
				outOffset, outBPR, rawOutBPR, reader,
				Writer(outData, paletteConverter));
			break;
		}
		case B_GRAY8:
		{
			typedef Gray8Writer Writer;
			typedef gray_color_value color_value_t;
			error = set_bits_worker<Reader, Writer, color_value_t>(
				inData, inLength, inBPR, inRowSkip, outData, outLength,
				outOffset, outBPR, rawOutBPR, reader, Writer(outData));
			break;
		}
		case B_GRAY1:
		{
			typedef Gray1Writer Writer;
			typedef gray_color_value color_value_t;
			error = set_bits_worker_gray1<Reader, Writer, color_value_t>(
				inData, inLength, inBPR, inRowSkip, outData, outLength,
				outOffset, outBPR, width, reader, Writer(outData));
			break;
		}
		// unsupported
		case B_NO_COLOR_SPACE:
		case B_YUV9: case B_YUV12:
		case B_UVL32: case B_UVLA32:
		case B_LAB32: case B_LABA32:
		case B_HSI32: case B_HSIA32:
		case B_HSV32: case B_HSVA32:
		case B_HLS32: case B_HLSA32:
		case B_CMY32: case B_CMYA32: case B_CMYK32:
		case B_UVL24: case B_LAB24: case B_HSI24:
		case B_HSV24: case B_HLS24: case B_CMY24:
		case B_YCbCr422: case B_YUV422:
		case B_YCbCr411: case B_YUV411:
		case B_YCbCr444: case B_YUV444:
		case B_YCbCr420: case B_YUV420:
		default:
			error = B_BAD_VALUE;
			break;
	}
	return error;
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

	\note As this methods is apparently a bit strange to use, OBOS introduces
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
	status_t error = (InitCheck() == B_OK ? B_OK : B_NO_INIT);
	// check params 
	if (error == B_OK && (data == NULL || offset > fSize || length < 0))
		error = B_BAD_VALUE;
	// get BPR
	int32 width = 0;
	int32 inRowSkip = 0;
	if (error == B_OK) {
		width = fBounds.IntegerWidth() + 1;
		if (bpr < 0)
			bpr = get_bytes_per_row(colorSpace, width);
		inRowSkip = bpr - get_raw_bytes_per_row(colorSpace, width);
		if (inRowSkip < 0)
			error = B_BAD_VALUE;
	}
	if (error != B_OK) {
		// catch error case
	} else if (colorSpace == fColorSpace && bpr == fBytesPerRow) {
		length = min(length, fSize - offset);
		memcpy((char*)fBasePtr + offset, data, length);
	} else {
		// TODO: Retrieve color map from BScreen, when available:
		// PaletteConverter paletteConverter(BScreen().ColorMap());
		const PaletteConverter &paletteConverter = *palette_converter();
		int32 rawOutBPR = get_raw_bytes_per_row(fColorSpace, width);
		switch (colorSpace) {
			// supported
			case B_RGB32: case B_RGBA32:
			{
				typedef RGB24Reader<rgb32_pixel> Reader;
				error = set_bits<Reader>(data, length, bpr, inRowSkip,
					fBasePtr, fSize, offset, fBytesPerRow, rawOutBPR,
					fColorSpace, width, Reader(data), paletteConverter);
				break;
			}
			case B_RGB32_BIG: case B_RGBA32_BIG:
			{
				typedef RGB24Reader<rgb32_big_pixel> Reader;
				error = set_bits<Reader>(data, length, bpr, inRowSkip,
					fBasePtr, fSize, offset, fBytesPerRow, rawOutBPR,
					fColorSpace, width, Reader(data), paletteConverter);
				break;
			}
			case B_RGB24:
			{
				typedef RGB24Reader<rgb24_pixel> Reader;
				error = set_bits<Reader>(data, length, bpr, inRowSkip,
					fBasePtr, fSize, offset, fBytesPerRow, rawOutBPR,
					fColorSpace, width, Reader(data), paletteConverter);
				break;
			}
			case B_RGB24_BIG:
			{
				typedef RGB24Reader<rgb24_big_pixel> Reader;
				error = set_bits<Reader>(data, length, bpr, inRowSkip,
					fBasePtr, fSize, offset, fBytesPerRow, rawOutBPR,
					fColorSpace, width, Reader(data), paletteConverter);
				break;
			}
			case B_RGB16:
			{
				typedef RGB16Reader<rgb16_pixel> Reader;
				error = set_bits<Reader>(data, length, bpr, inRowSkip,
					fBasePtr, fSize, offset, fBytesPerRow, rawOutBPR,
					fColorSpace, width, Reader(data), paletteConverter);
				break;
			}
			case B_RGB16_BIG:
			{
				typedef RGB16Reader<rgb16_big_pixel> Reader;
				error = set_bits<Reader>(data, length, bpr, inRowSkip,
					fBasePtr, fSize, offset, fBytesPerRow, rawOutBPR,
					fColorSpace, width, Reader(data), paletteConverter);
				break;
			}
			case B_RGB15: case B_RGBA15:
			{
				typedef RGB15Reader<rgb16_pixel> Reader;
				error = set_bits<Reader>(data, length, bpr, inRowSkip,
					fBasePtr, fSize, offset, fBytesPerRow, rawOutBPR,
					fColorSpace, width, Reader(data), paletteConverter);
				break;
			}
			case B_RGB15_BIG: case B_RGBA15_BIG:
			{
				typedef RGB15Reader<rgb16_big_pixel> Reader;
				error = set_bits<Reader>(data, length, bpr, inRowSkip,
					fBasePtr, fSize, offset, fBytesPerRow, rawOutBPR,
					fColorSpace, width, Reader(data), paletteConverter);
				break;
			}
			case B_CMAP8:
			{
				typedef CMAP8Reader Reader;
				error = set_bits<Reader>(data, length, bpr, inRowSkip,
					fBasePtr, fSize, offset, fBytesPerRow, rawOutBPR,
					fColorSpace, width, Reader(data, paletteConverter),
					paletteConverter);
				break;
			}
			case B_GRAY8:
			{
				typedef Gray8Reader Reader;
				error = set_bits<Reader>(data, length, bpr, inRowSkip,
					fBasePtr, fSize, offset, fBytesPerRow, rawOutBPR,
					fColorSpace, width, Reader(data), paletteConverter);
				break;
			}
			case B_GRAY1:
			{
				typedef Gray1Reader Reader;
				error = set_bits<Reader>(data, length, bpr, inRowSkip,
					fBasePtr, fSize, offset, fBytesPerRow, rawOutBPR,
					fColorSpace, width, Reader(data), paletteConverter);
				break;
			}
			// unsupported
			case B_NO_COLOR_SPACE:
			case B_YUV9: case B_YUV12:
			case B_UVL32: case B_UVLA32:
			case B_LAB32: case B_LABA32:
			case B_HSI32: case B_HSIA32:
			case B_HSV32: case B_HSVA32:
			case B_HLS32: case B_HLSA32:
			case B_CMY32: case B_CMYA32: case B_CMYK32:
			case B_UVL24: case B_LAB24: case B_HSI24:
			case B_HSV24: case B_HLS24: case B_CMY24:
			case B_YCbCr422: case B_YUV422:
			case B_YCbCr411: case B_YUV411:
			case B_YCbCr444: case B_YUV444:
			case B_YCbCr420: case B_YUV420:
			default:
				error = B_BAD_VALUE;
				break;
		}
	}
	return error;
}

// ImportBits
/*!	\briefly Assigns another bitmap's data to this bitmap.

	The supplied bitmap must have the exactly same dimensions as this bitmap.
	Its data are converted to the color space of this bitmap.

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
	status_t error = (InitCheck() == B_OK ? B_OK : B_NO_INIT);
	// check param
	if (error == B_OK && bitmap == NULL)
		error = B_BAD_VALUE;
	if (error == B_OK && bitmap->InitCheck() != B_OK)
		error = B_BAD_VALUE;
	if (error == B_OK && bitmap->Bounds() != fBounds)
		error = B_BAD_VALUE;
	// set bits
	if (error == B_OK) {
		error = ImportBits(bitmap->Bits(), bitmap->BitsLength(),
						   bitmap->BytesPerRow(), 0, bitmap->ColorSpace());
	}
	return error;
}

// GetOverlayRestrictions
/*!	\brief ???
*/
status_t
BBitmap::GetOverlayRestrictions(overlay_restrictions *restrictions) const
{
	return NOT_IMPLEMENTED;
}

// AddChild
/*!	\brief Adds a BView to the bitmap's view hierarchy.

	The bitmap must accept views and the supplied view must not be child of
	another parent.

	\param view The view to be added.
*/
void
BBitmap::AddChild(BView *view)
{
}

// RemoveChild
/*!	\brief Removes a BView from the bitmap's view hierarchy.
	\param view The view to be removed.
*/
bool
BBitmap::RemoveChild(BView *view)
{
	return false;	// not implemented
}

// CountChildren
/*!	\brief Returns the number of BViews currently belonging to the bitmap.
	\return The number of BViews currently belonging to the bitmap.
*/
int32
BBitmap::CountChildren() const
{
	return 0;	// not implemented
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
	return NULL;	// not implemented
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
	return NULL;	// not implemented
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
	return NULL;	// not implemented
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
	return false;	// not implemented
}

// Unlock
/*!	\brief Unlocks the off-screen window that belongs to the bitmap.

	The bitmap must accept views, if locking should work.
*/
void
BBitmap::Unlock()
{
}

// IsLocked
/*!	\brief Returns whether or not the bitmap's off-screen window is locked.

	The bitmap must accept views, if locking should work.

	\return \c true, if the caller owns a lock , \c false otherwise.
*/
bool
BBitmap::IsLocked() const
{
	return false;	// not implemented
}

// Perform
/*!	\brief ???
*/
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
	return -1;	// not implemented
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
	status_t error = B_OK;

	PortMessage pmsg;
	BPrivate::BAppServerLink *link=new BPrivate::BAppServerLink();

	// clean up
	if (fBasePtr) {
//		free(fBasePtr);
		fBasePtr = NULL;

		// AS_DELETE_BITMAP:
		// Attached Data: 
		//	1) int32 server token
		
		// Reply Code: SERVER_TRUE if successful, 
		//				SERVER_FALSE if the buffer was already deleted
		// Reply Data:
		//		none
//		status_t freestat;
		link->SetOpCode(AS_DELETE_BITMAP);
		link->Attach<int32>(fServerToken);
		error=link->FlushWithReply(&pmsg);
		if(pmsg.Code()==SERVER_FALSE)
			error=B_NO_MEMORY;
		fBasePtr=NULL;
		fArea=-1;
		fServerToken=-1;
	}
	// check params
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
		int32 size = bytesPerRow * (bounds.IntegerHeight() + 1);
//		fBasePtr = malloc(size);

		// Ask the server (via our owning application) to create a bitmap.

		// Attach Data: 
		// 1) BRect bounds
		// 2) color_space space
		// 3) int32 bitmap_flags
		// 4) int32 bytes_per_row
		// 5) int32 screen_id::id
		link->SetOpCode(AS_CREATE_BITMAP);
		link->Attach<BRect>(bounds);
		link->Attach(&colorSpace, sizeof(color_space));
		link->Attach<int32>((int32)flags);
		link->Attach<int32>(bytesPerRow);
		link->Attach<int32>(screenID.id);
		
		// Reply Code: SERVER_TRUE
		// Reply Data:
		//	1) int32 server token
		//	2) area_id id of the area in which the bitmap data resides
		//	3) int32 area pointer offset used to calculate fBasePtr
		
		// alternatively, if something went wrong
		// Reply Code: SERVER_FALSE
		// Reply Data:
		//		None
		error=link->FlushWithReply(&pmsg);
		
		// We shouldn't ever have to execute this block, but just in case...
		if(error!=B_OK)
			fBasePtr=NULL;

		if(pmsg.Code()==SERVER_TRUE)
		{
			// Get token
			area_id bmparea;
			int32 areaoffset;
			
			pmsg.Read<int32>(&fServerToken);
			pmsg.Read<area_id>(&bmparea);
			pmsg.Read<int32>(&areaoffset);
			
			// Get the area in which the data resides
			fArea=clone_area("shared bitmap area",(void**)&fBasePtr,B_ANY_ADDRESS,
				B_READ_AREA | B_WRITE_AREA,bmparea);
			
			// Jump to the location in the area
			fBasePtr=(int8*)fBasePtr + areaoffset;

			fSize = size;
			fColorSpace = colorSpace;
			fBounds = bounds;
			fBytesPerRow = bytesPerRow;
			fFlags = flags;
		}
		else
		{
			fServerToken = -1;
			fArea = -1;
			fFlags = flags;
			
			error = B_NO_MEMORY;
		}
		fWindow = NULL;
		fToken = -1;
		fOrigArea = -1;
	}
	delete link;
	fInitError = error;
}

// AssertPtr
/*!	\brief ???
*/
void
BBitmap::AssertPtr()
{
}

