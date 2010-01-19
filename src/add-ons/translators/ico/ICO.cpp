/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

// ToDo: This definitely needs to be worked over for endian issues

#include "ICO.h"
#include "ICOTranslator.h"

#include <ByteOrder.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//#define TRACE_ICO
#ifdef TRACE_ICO
#	define TRACE(x) printf x
#else
#	define TRACE(x) ;
#endif


using namespace ICO;


static const rgba32_color kMagicTransparentColor = *(rgba32_color *)&B_TRANSPARENT_MAGIC_RGBA32;


class TempAllocator {
	public:
		TempAllocator() : fMemory(NULL) {}
		~TempAllocator() { free(fMemory); }

		void *Allocate(size_t size) { return fMemory = malloc(size); }

	private:
		void	*fMemory;
};


bool 
ico_header::IsValid() const
{
	return reserved == 0
		&& (type == kTypeIcon || type == kTypeCursor)
		&& entry_count < 32;
}


void
ico_header::SwapToHost()
{
	swap_data(B_UINT16_TYPE, this, sizeof(ico_header), B_SWAP_LENDIAN_TO_HOST);
}


void
ico_header::SwapFromHost()
{
	swap_data(B_UINT16_TYPE, this, sizeof(ico_header), B_SWAP_HOST_TO_LENDIAN);
}


//	#pragma mark -


void
ico_dir_entry::SwapToHost()
{
	swap_data(B_UINT16_TYPE, &planes, sizeof(uint16) * 2, B_SWAP_LENDIAN_TO_HOST);
	swap_data(B_UINT32_TYPE, &size, sizeof(uint32) * 2, B_SWAP_LENDIAN_TO_HOST);
}


void
ico_dir_entry::SwapFromHost()
{
	swap_data(B_UINT16_TYPE, &planes, sizeof(uint16) * 2, B_SWAP_HOST_TO_LENDIAN);
	swap_data(B_UINT32_TYPE, &size, sizeof(uint32) * 2, B_SWAP_HOST_TO_LENDIAN);
}


//	#pragma mark -


bool
ico_bitmap_header::IsValid() const
{
	return size == sizeof(ico_bitmap_header) && compression == 0
		&& (bits_per_pixel == 1 || bits_per_pixel == 4 || bits_per_pixel == 8
			|| bits_per_pixel == 16 || bits_per_pixel == 24 || bits_per_pixel == 32);
}


void
ico_bitmap_header::SwapToHost()
{
	swap_data(B_UINT32_TYPE, &size, sizeof(uint32) * 3, B_SWAP_LENDIAN_TO_HOST);
	swap_data(B_UINT16_TYPE, &planes, sizeof(uint16) * 2, B_SWAP_LENDIAN_TO_HOST);
	swap_data(B_UINT32_TYPE, &compression, sizeof(uint32) * 6, B_SWAP_LENDIAN_TO_HOST);
}


void
ico_bitmap_header::SwapFromHost()
{
	swap_data(B_UINT32_TYPE, &size, sizeof(uint32) * 3, B_SWAP_HOST_TO_LENDIAN);
	swap_data(B_UINT16_TYPE, &planes, sizeof(uint16) * 2, B_SWAP_HOST_TO_LENDIAN);
	swap_data(B_UINT32_TYPE, &compression, sizeof(uint32) * 6, B_SWAP_HOST_TO_LENDIAN);
}


//	#pragma mark -


static inline uint8 *
get_data_row(uint8 *data, int32 dataSize, int32 rowBytes, int32 row)
{
	return data + dataSize - (row + 1) * rowBytes;
}


static inline int32
get_bytes_per_row(int32 width, int32 bitsPerPixel)
{
	return (((bitsPerPixel * width + 7) / 8) + 3) & ~3;
}


static inline void
set_1_bit_per_pixel(uint8 *line, int32 x, int32 value)
{
	int32 mask = 1 << (7 - (x & 7));
	if (value)
		line[x / 8] |= mask;
	else
		line[x / 8] &= ~mask;
}


static inline void
set_4_bits_per_pixel(uint8 *line, int32 x, int32 value)
{
	int32 shift = x & 1 ? 0 : 4;
	int32 mask = ~0L & (0xf0 >> shift);

	line[x / 2] &= mask;
	line[x / 2] |= value << shift;
}


static inline int32
get_1_bit_per_pixel(uint8 *line, int32 x)
{
	return (line[x / 8] >> (7 - (x & 7))) & 1;
}


static inline int32
get_4_bits_per_pixel(uint8 *line, int32 x)
{
	return (line[x / 2] >> (4 * ((x + 1) & 1))) & 0xf;
}


static uint8
get_alpha_value(color_space space, uint32 value)
{
	// ToDo: support more color spaces
	if (space == B_RGBA32)
		return value >> 24;

	return 0;
}


static uint16
rgba32_color_to_16_bit_color(rgba32_color &color)
{
	return ((color.blue >> 3) << 11) | ((color.green >> 2) << 5) | (color.red >> 3);
}


static int32
find_rgba32_color(rgba32_color *palette, int32 numColors, rgba32_color &color)
{
	// ToDo: sorting and binary search?
	for (int32 i = 0; i < numColors; i++) {
		if (palette[i] == color)
			return i;
	}

	return -1;
}


static inline rgba32_color
get_rgba32_color_from_bits(TranslatorBitmap &bitsHeader, uint8 *data, int32 x, int32 y)
{
	data += bitsHeader.rowBytes * y;

	switch (bitsHeader.colors) {
		case B_RGBA32:
			return *(rgba32_color *)(data + 4 * x);
		case B_RGB32:
		default:
			// stupid applications like ArtPaint use the alpha channel in B_RGB32 images...
			rgba32_color color = *(rgba32_color *)(data + 4 * x);
			if (color.alpha >= 128)
				color.alpha = 255;
			else
				color.alpha = 0;
			return color;
		// ToDo: support some more color spaces...
	}
}


static int32
fill_palette(TranslatorBitmap &bitsHeader, uint8 *data, rgba32_color *palette)
{
	int32 numColors = 0;

	for (int32 y = 0; y < bitsHeader.bounds.IntegerHeight() + 1; y++) {
		for (int32 x = 0; x < bitsHeader.bounds.IntegerWidth() + 1; x++) {
			rgba32_color color = get_rgba32_color_from_bits(bitsHeader, data, x, y);

			int32 index = find_rgba32_color(palette, numColors, color);
			if (index == -1) {
				// add this color if there is space left
				if (numColors == 256)
					return -1;

				color.alpha = 0;
					// the alpha channel is actually unused
				palette[numColors++] = color;
			}
		}
	}

	return numColors;
}


/**	This function is used to determine, if a true alpha channel has to
 *	be used in order to preserve all information.
 */

static bool
has_true_alpha_channel(color_space space, uint8 *data,
	int32 width, int32 height, int32 bytesPerRow)
{
	for (int32 y = 0; y < height; y++) {
		for (int32 x = 0; x < width; x++) {
			uint8 value = get_alpha_value(space, ((uint32 *)data)[x]);
			if (value != 0 && value != 255)
				return true;
		}

		data += bytesPerRow;
	}

	return false;
}


static status_t
convert_data_to_bits(ico_dir_entry &entry, ico_bitmap_header &header,
	const rgba32_color *palette, BPositionIO &source,
	BPositionIO &target)
{
	uint16 bitsPerPixel = header.bits_per_pixel;

	// round row bytes to next 4 byte boundary
	int32 xorRowBytes = get_bytes_per_row(entry.width, header.bits_per_pixel);
	int32 andRowBytes = 0;
	if (bitsPerPixel != 32)
		andRowBytes = get_bytes_per_row(entry.width, 1);
	int32 outRowBytes = entry.width * 4;

	// allocate buffers
	TempAllocator xorAllocator, andAllocator, rowAllocator;

	int32 xorDataSize = xorRowBytes * entry.height;
	uint8 *xorData = (uint8 *)xorAllocator.Allocate(xorDataSize);
	if (xorData == NULL)
		return B_NO_MEMORY;

	int32 andDataSize = andRowBytes * entry.height;
	uint8 *andData = NULL;
	if (bitsPerPixel != 32) {
		andData = (uint8 *)andAllocator.Allocate(andDataSize);
		if (andData == NULL)
			return B_NO_MEMORY;
	}

	rgba32_color *outRowData = (rgba32_color *)rowAllocator.Allocate(outRowBytes);
	if (outRowData == NULL)
		return B_NO_MEMORY;

	ssize_t bytesRead = source.Read(xorData, xorDataSize);
	if (bytesRead != xorDataSize)
		return B_BAD_DATA;

	if (bitsPerPixel != 32) {
		bytesRead = source.Read(andData, andDataSize);
		if (bytesRead != andDataSize) {
			// reading the alpha channel failed, so we're ignoring it
			// (but we're still able to show the image data)
			andData = NULL;
		}
	}

	for (uint32 row = 0; row < entry.height; row++) {
		for (uint32 x = 0; x < entry.width; x++) {
			uint8 *line = get_data_row(xorData, xorDataSize, xorRowBytes, row);

			if (palette != NULL) {
				uint8 index;

				switch (bitsPerPixel) {
					case 1:
						index = get_1_bit_per_pixel(line, x);
						break;
					case 4:
						index = get_4_bits_per_pixel(line, x);
						break;
					case 8:
					default:
						index = line[x];
						break;
				}

				outRowData[x] = palette[index];
			} else {
				switch (bitsPerPixel) {
					case 16:
					{
						uint16 color = ((uint16 *)line)[x];
						outRowData[x].blue = (color >> 11) << 3;
						outRowData[x].green = ((color >> 5) & 0x3f) << 3;
						outRowData[x].red = (color & 0x1f) << 3;
						break;
					}

					case 24:
						outRowData[x].blue = line[x * 3 + 0];
						outRowData[x].green = line[x * 3 + 1];
						outRowData[x].red = line[x * 3 + 2];
						break;

					case 32:
						outRowData[x] = ((rgba32_color *)line)[x];
						break;
				}
			}

			if (bitsPerPixel != 32) {
				// set alpha channel
				if (andData != NULL
					&& get_1_bit_per_pixel(get_data_row(andData, andDataSize, andRowBytes, row), x))
					outRowData[x] = kMagicTransparentColor;
				else
					outRowData[x].alpha = 255;
			} else if (outRowData[x].alpha == 0)
				outRowData[x] = kMagicTransparentColor;
		}

		ssize_t bytesWritten = target.Write(outRowData, outRowBytes);
		if (bytesWritten < B_OK)
			return bytesWritten;
		if (bytesWritten != outRowBytes)
			return B_IO_ERROR;
	}

	return B_OK;
}


static status_t
convert_bits_to_data(TranslatorBitmap &bitsHeader, uint8 *bitsData, ico_dir_entry &entry,
	ico_bitmap_header &header, rgba32_color *palette, BPositionIO &target)
{
	int32 bitsPerPixel = header.bits_per_pixel;

	// round row bytes to next 4 byte boundary
	int32 xorRowBytes = get_bytes_per_row(entry.width, bitsPerPixel);
	int32 andRowBytes = get_bytes_per_row(entry.width, 1);

	TempAllocator xorAllocator, andAllocator;

	uint8 *xorRowData = (uint8 *)xorAllocator.Allocate(xorRowBytes);
	if (xorRowData == NULL)
		return B_NO_MEMORY;

	uint8 *andRowData = (uint8 *)andAllocator.Allocate(andRowBytes);
	if (andRowData == NULL)
		return B_NO_MEMORY;

	int32 numColors = 1 << bitsPerPixel;

	// write XOR data (the actual image data)
	// (ICO data is upside down, so we're starting at the last line)

	for (uint32 row = entry.height; row-- > 0;) {
		for (uint32 x = 0; x < entry.width; x++) {
			rgba32_color color = get_rgba32_color_from_bits(bitsHeader, bitsData, x, row);

			if (palette != NULL) {
				uint8 index = find_rgba32_color(palette, numColors, color);

				switch (bitsPerPixel) {
					case 1:
						set_1_bit_per_pixel(xorRowData, x, index);
						break;
					case 4:
						set_4_bits_per_pixel(xorRowData, x, index);
						break;
					case 8:
					default:
						xorRowData[x] = index;
						break;
				}
			} else {
				switch (bitsPerPixel) {
					default:
					case 16:
					{
						uint16 *data = (uint16 *)xorRowData;
						data[x] = rgba32_color_to_16_bit_color(color); 
						break;
					}

					case 24:
					{
						xorRowData[x * 3 + 0] = color.blue;
						xorRowData[x * 3 + 1] = color.green;
						xorRowData[x * 3 + 2] = color.red;
						break;
					}

					case 32:
					{
						rgba32_color *data = (rgba32_color *)xorRowData;
						data[x] = color;
						break;
					}
				}
			}
		}

		ssize_t bytesWritten = target.Write(xorRowData, xorRowBytes);
		if (bytesWritten < B_OK)
			return bytesWritten;
		if (bytesWritten != xorRowBytes)
			return B_IO_ERROR;
	}

	if (bitsPerPixel == 32) {
		// the alpha channel has already been written with the image data
		return B_OK;
	}

	// write AND data (the transparency bit)

	for (uint32 row = entry.height; row-- > 0;) {
		for (uint32 x = 0; x < entry.width; x++) {
			rgba32_color color = get_rgba32_color_from_bits(bitsHeader, bitsData, x, row);
			bool transparent = *(uint32 *)&color == B_TRANSPARENT_MAGIC_RGBA32 || color.alpha == 0;

			set_1_bit_per_pixel(andRowData, x, transparent ? 1 : 0);
		}

		ssize_t bytesWritten = target.Write(andRowData, andRowBytes);
		if (bytesWritten < B_OK)
			return bytesWritten;
		if (bytesWritten != andRowBytes)
			return B_IO_ERROR;
	}

	return B_OK;
}


//	#pragma mark -


bool
ICO::is_valid_size(int32 size)
{
	return size == 16 || size == 32 || size == 48;
}


status_t
ICO::identify(BMessage *settings, BPositionIO &stream, uint8 &type, int32 &bitsPerPixel)
{
	// read in the header

	ico_header header;
	if (stream.Read(&header, sizeof(ico_header)) != (ssize_t)sizeof(ico_header))
		return B_BAD_VALUE;

	header.SwapToHost();

	// check header

	if (!header.IsValid())
		return B_BAD_VALUE;

	int32 iconIndex = 0;
	type = header.type;

	if (settings) {
		// Add page count to ioExtension
		settings->RemoveName(kDocumentCount);
		settings->AddInt32(kDocumentCount, header.entry_count);

		// Check if a document index has been specified	
		if (settings->FindInt32(kDocumentIndex, &iconIndex) == B_OK)
			iconIndex--;
		else
			iconIndex = 0;

		if (iconIndex < 0 || iconIndex >= header.entry_count)
			return B_NO_TRANSLATOR;
	}

	TRACE(("iconIndex = %ld, count = %ld\n", iconIndex, header.entry_count));

	// read in directory entries

	for (uint32 i = 0; i < header.entry_count; i++) {
		ico_dir_entry entry;
		if (stream.Read(&entry, sizeof(ico_dir_entry)) != (ssize_t)sizeof(ico_dir_entry))
			return B_BAD_VALUE;

		entry.SwapToHost();
		TRACE(("width: %d, height: %d, planes: %d, color_count: %d, bits_per_pixel: %d, size: %ld, offset: %ld\n",
			entry.width, entry.height, entry.planes, entry.color_count, entry.bits_per_pixel,
			entry.size, entry.offset));

		ico_bitmap_header bitmapHeader;
		if (stream.ReadAt(entry.offset, &bitmapHeader, sizeof(ico_bitmap_header)) != (ssize_t)sizeof(ico_bitmap_header))
			return B_BAD_VALUE;

		bitmapHeader.SwapToHost();
		TRACE(("size: %ld, width: %ld, height: %ld, bits_per_pixel: %d, x/y per meter: %ld:%ld, compression: %ld, image_size: %ld, colors used: %ld, important colors: %ld\n",
			bitmapHeader.size, bitmapHeader.width, bitmapHeader.height, bitmapHeader.bits_per_pixel,
			bitmapHeader.x_pixels_per_meter, bitmapHeader.y_pixels_per_meter,
			bitmapHeader.compression, bitmapHeader.image_size, bitmapHeader.colors_used,
			bitmapHeader.important_colors));

		if (!bitmapHeader.IsValid())
			return B_BAD_VALUE;

		if ((uint32)iconIndex == i)
			bitsPerPixel = bitmapHeader.bits_per_pixel;
	}

	return B_OK;
}


/**	Converts an ICO image of any type into a B_RGBA32 B_TRANSLATOR_BITMAP.
 */

status_t
ICO::convert_ico_to_bits(BMessage *settings, BPositionIO &source, BPositionIO &target)
{
	ico_header header;
	if (source.Read(&header, sizeof(ico_header)) != (ssize_t)sizeof(ico_header))
		return B_BAD_VALUE;

	header.SwapToHost();

	// check header

	if (!header.IsValid())
		return B_BAD_VALUE;

	int32 iconIndex = 0;

	if (settings) {
		// Check if a document index has been specified	
		if (settings->FindInt32(kDocumentIndex, &iconIndex) == B_OK)
			iconIndex--;
		else
			iconIndex = 0;

		if (iconIndex < 0 || iconIndex >= header.entry_count)
			return B_BAD_VALUE;
	}

	// read in selected entry

	ico_dir_entry entry;
	if (source.ReadAt(sizeof(ico_header) + sizeof(ico_dir_entry) * iconIndex,
			&entry, sizeof(ico_dir_entry)) != (ssize_t)sizeof(ico_dir_entry))
		return B_BAD_VALUE;

	entry.SwapToHost();
	source.Seek(entry.offset, SEEK_SET);

	ico_bitmap_header bitmapHeader;
	if (source.Read(&bitmapHeader, sizeof(ico_bitmap_header)) != (ssize_t)sizeof(ico_bitmap_header))
		return B_BAD_VALUE;

	bitmapHeader.SwapToHost();

	if (!bitmapHeader.IsValid())
		return B_BAD_VALUE;

	if (bitmapHeader.compression != 0)
		return EOPNOTSUPP;

	int32 numColors = 0;
	if (bitmapHeader.bits_per_pixel <= 8)
		numColors = 1L << bitmapHeader.bits_per_pixel;

	// This is a work-around for a broken ICO file writer that publishes
	// a wrong image height in the ico_dir_entry structure
	if (entry.size != 0 && 2 * entry.width == entry.height && numColors != 0
		&& sizeof(rgba32_color) * numColors + entry.width * entry.height > entry.size)
		entry.height = entry.width;

	TranslatorBitmap bitsHeader;
	bitsHeader.magic = B_TRANSLATOR_BITMAP;
	bitsHeader.bounds.left = 0;
	bitsHeader.bounds.top = 0;
	bitsHeader.bounds.right = entry.width - 1;
	bitsHeader.bounds.bottom = entry.height - 1;
	bitsHeader.bounds.Set(0, 0, entry.width - 1, entry.height - 1);
	bitsHeader.rowBytes = entry.width * 4;
	bitsHeader.colors = B_RGBA32;
	bitsHeader.dataSize = bitsHeader.rowBytes * entry.height;

	// read in palette

	rgba32_color palette[256];
	if (numColors > 0) {
		if (source.Read(palette, numColors * 4) != numColors * 4)
			return B_BAD_VALUE;

		// clear alpha channel (it's not used in ICO color information)
		for (int32 i = 0; i < numColors; i++)
			palette[i].alpha = 0;
	}

	// write out Be's Bitmap header
	swap_data(B_UINT32_TYPE, &bitsHeader, sizeof(TranslatorBitmap), B_SWAP_HOST_TO_BENDIAN);
	target.Write(&bitsHeader, sizeof(TranslatorBitmap));

	return convert_data_to_bits(entry, bitmapHeader, numColors > 0 ? palette : NULL, source, target);
}


status_t
ICO::convert_bits_to_ico(BMessage *settings, BPositionIO &source,
	TranslatorBitmap &bitsHeader, BPositionIO &target)
{
	int32 width = bitsHeader.bounds.IntegerWidth() + 1;
	int32 height = bitsHeader.bounds.IntegerHeight() + 1;
	if (!is_valid_size(width) || !is_valid_size(height))
		return B_BAD_VALUE;

	int32 bitsPerPixel;
	switch (bitsHeader.colors) {
		case B_RGBA32:
			bitsPerPixel = 32;
			break;
		case B_RGB32:
			bitsPerPixel = 24;
			break;
		case B_RGB16:
			bitsPerPixel = 16;
			break;
		case B_CMAP8:
		case B_GRAY8:
			bitsPerPixel = 8;
			break;
		case B_GRAY1:
			bitsPerPixel = 1;
			break;
		default:
			fprintf(stderr, "unsupported color space.\n");
			return B_BAD_VALUE;
	}

	TempAllocator dataAllocator;
	uint8 *bitsData = (uint8 *)dataAllocator.Allocate(bitsHeader.rowBytes * height);
	if (bitsData == NULL)
		return B_NO_MEMORY;
		
	ssize_t bytesRead = source.Read(bitsData, bitsHeader.rowBytes * height);
	if (bytesRead < B_OK)
		return bytesRead;

	rgba32_color palette[256];
	if (bitsPerPixel > 8) {
		// it's a non-palette mode - but does it have to be?
		if (bitsHeader.colors != B_RGBA32
			|| !has_true_alpha_channel(bitsHeader.colors, bitsData,
					width, height, bitsHeader.rowBytes)) {
			memset(palette, 0, sizeof(palette));

			// count colors
			int32 colors = fill_palette(bitsHeader, bitsData, palette);
			if (colors != -1) {
				// we fit into a palette mode
				if (colors > 16)
					bitsPerPixel = 8;
				else if (colors > 2)
					bitsPerPixel = 4;
				else
					bitsPerPixel = 1;
			}
		}
	}
	int32 numColors = 1 << bitsPerPixel;

	ico_header header;
	header.type = B_HOST_TO_LENDIAN_INT16(1);
	header.entry_count = B_HOST_TO_LENDIAN_INT16(1);
	header.reserved = 0;
	
	ssize_t bytesWritten = target.Write(&header, sizeof(ico_header));
	if (bytesWritten < B_OK)
		return bytesWritten;

	ico_dir_entry entry;
	entry.width = width;
	entry.height = height;
	entry.planes = 1;
	entry.bits_per_pixel = bitsPerPixel;
	entry.color_count = 0;
	if (bitsPerPixel <= 8)
		entry.color_count = numColors;

	// When bits_per_pixel == 32, the data already contains the alpha channel

	int32 xorRowBytes = get_bytes_per_row(width, bitsPerPixel);
	int32 andRowBytes = 0;
	if (bitsPerPixel != 32)
		andRowBytes = get_bytes_per_row(width, 1);

	entry.size = sizeof(ico_bitmap_header) + width * (xorRowBytes + andRowBytes);
	if (bitsPerPixel <= 8)
		entry.size += numColors * sizeof(rgba32_color);
	entry.offset = sizeof(ico_header) + sizeof(ico_dir_entry);
	entry.reserved = 0;

	ico_bitmap_header bitmapHeader;
	memset(&bitmapHeader, 0, sizeof(ico_bitmap_header));
	bitmapHeader.size = sizeof(ico_bitmap_header);
	bitmapHeader.width = width;
	bitmapHeader.height = height + (bitsPerPixel == 32 ? 0 : height);
	bitmapHeader.bits_per_pixel = bitsPerPixel;
	bitmapHeader.planes = 1;
	bitmapHeader.image_size = 0;
	if (bitsPerPixel <= 8)
		bitmapHeader.colors_used = numColors;

	entry.SwapFromHost();
	bitmapHeader.SwapFromHost();

	bytesWritten = target.Write(&entry, sizeof(ico_dir_entry));
	if (bytesWritten < B_OK)
		return bytesWritten;

	bytesWritten = target.Write(&bitmapHeader, sizeof(ico_bitmap_header));
	if (bytesWritten < B_OK)
		return bytesWritten;

	// we'll need them in convert_bits_to_data()
	entry.SwapToHost();
	bitmapHeader.SwapToHost();

	if (bitsPerPixel <= 8) {
		bytesWritten = target.Write(palette, numColors * sizeof(rgba32_color));
		if (bytesWritten < B_OK)
			return bytesWritten;
	}

	return convert_bits_to_data(bitsHeader, bitsData, entry, bitmapHeader,
		bitsPerPixel <= 8 ? palette : NULL, target);
}

