#ifndef _COLOR_CONVERSION_H_
#define _COLOR_CONVERSION_H_


#include <GraphicsDefs.h>

class BPoint;


namespace BPrivate {

status_t ConvertBits(const void *srcBits, void *dstBits, int32 srcBitsLength,
	int32 dstBitsLength, int32 srcBytesPerRow, int32 dstBytesPerRow,
	color_space srcColorSpace, color_space dstColorSpace, int32 width,
	int32 height);
status_t ConvertBits(const void *srcBits, void *dstBits, int32 srcBitsLength,
	int32 dstBitsLength, int32 srcBytesPerRow, int32 dstBytesPerRow,
	color_space srcColorSpace, color_space dstColorSpace, BPoint srcOffset,
	BPoint dstOffset, int32 width, int32 height);


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
	inline uint8 IndexForRGBA32(uint32 rgba) const;
	inline uint8 IndexForGray(uint8 gray) const;

	inline const rgb_color &RGBColorForIndex(uint8 index) const;
	inline uint16 RGB15ColorForIndex(uint8 index) const;
	inline uint16 RGB16ColorForIndex(uint8 index) const;
	inline uint32 RGBA32ColorForIndex(uint8 index) const;
	inline void RGBA32ColorForIndex(uint8 index, uint8 &red, uint8 &green,
								   uint8 &blue, uint8 &alpha) const;
	inline uint8 GrayColorForIndex(uint8 index) const;

	static status_t InitializeDefault(bool useServer = false);

private:
	static void _InitializeDefaultAppServer();
	static void _InitializeDefaultNoAppServer();

private:
	const color_map	*fColorMap;
	color_map		*fOwnColorMap;
	status_t		fCStatus;
};

}	// namespace BPrivate

#endif
