/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

#include <Bitmap.h>
#include <Message.h>

enum {
	COMPRESSION_NONE = 0,
	COMPRESSION_ZLIB = 2,
};

// compress_bitmap_zlib
bool
compress_bitmap_zlib(const BBitmap* bitmap, void** buffer, unsigned* size)
{
	bool result = false;
	if (bitmap) {
		Bytef* src = (Bytef*)bitmap->Bits();
		uLong srcLength = bitmap->BitsLength();
		*size = (unsigned)ceilf(srcLength * 1.01) + 12;
		*buffer = malloc(*size);
		if (*buffer) {
			int ret = compress2((Bytef*)*buffer,
								(uLongf*)size,
								src,
								srcLength,
								3);
			if (ret == Z_OK) {
// printf("zlib compressed %ld bytes bitmap into %d bytes (%f%%)\n",
//	srcLength, *size, ((float)*size / (float)srcLength) * 100.0);
				if ((unsigned)ceilf(srcLength * 1.01) + 12 != *size) {
					void* tmpBuffer = realloc(*buffer, *size);
					if (tmpBuffer) {
						*buffer = tmpBuffer;
						result = true;
					}
				}
			}
			if (ret != Z_OK || !result) {
				// error compressing
				free(*buffer);
				*buffer = NULL;
				*size = 0;
				fprintf(stderr, "zlib compression error: %d\n", ret);
			}
		} else
			*size = 0;
	}
	return result;
}

// decompress_bitmap_zlib
BBitmap*
decompress_bitmap_zlib(const void* buffer, unsigned int size,
					   BRect frame, color_space format)
{
	BBitmap* bitmap = new BBitmap(frame, 0, format);
	if (bitmap->IsValid()) {
		if (buffer) {
			Bytef* dst = (Bytef*)bitmap->Bits();
			uLongf dstLength = bitmap->BitsLength();

			int ret = uncompress(dst,
								 &dstLength,
								 (const Bytef*)buffer,
								 (uLong)size);
			if (ret != Z_OK || dstLength != (uint32)bitmap->BitsLength()) {
				// decompression error!
				fprintf(stderr, "decompress_bitmap_zlib() failed "
								"- corrupted input buffer or file!\n");
			}
		} else {
			memset(bitmap->Bits(), 0, bitmap->BitsLength());
		}
	} else {
		delete bitmap;
		bitmap = NULL;
	}
	return bitmap;
}



// archive_bitmap
status_t
archive_bitmap(const BBitmap* bitmap, BMessage* into, const char* fieldName)
{
	status_t ret = B_BAD_VALUE;
	if (bitmap && bitmap->IsValid() && into) {
		void* buffer;
		unsigned size;
		if (compress_bitmap_zlib(bitmap, &buffer, &size)) {
			ret = into->AddData(fieldName, B_RAW_TYPE, buffer, size);
			if (ret >= B_OK)
				ret = into->AddInt32("compression", COMPRESSION_ZLIB);
			if (ret >= B_OK)
				ret = into->AddRect("construction bounds", bitmap->Bounds());
			if (ret >= B_OK)
				ret = into->AddInt32("format", bitmap->ColorSpace());
			free(buffer);
		}
	}
	return ret;
}

// extract_bitmap
status_t
extract_bitmap(BBitmap** bitmap, const BMessage* from, const char* fieldName)
{
	status_t ret = B_BAD_VALUE;
	if (bitmap && from) {
		*bitmap = NULL;
		// no compression (flattened BBitmap archive)
		BMessage bitmapArchive;
		if ((ret = from->FindMessage(fieldName, &bitmapArchive)) >= B_OK) {
			*bitmap = new BBitmap(&bitmapArchive);
		}
		// compression
		if (!*bitmap) {
			const void* compressedData = NULL;
			ssize_t compressedSize = 0;
			compressedData = NULL;
			compressedSize = 0;
			BRect bounds;
			color_space format;
			uint32 compression;
			if (((ret = from->FindData(fieldName,
									  B_RAW_TYPE, &compressedData,
									  &compressedSize)) >= B_OK
				  // this is for backward compatibility
				  || (ret = from->FindData("current compressed data",
									  B_RAW_TYPE, &compressedData,
									  &compressedSize)) >= B_OK)
				&& (ret = from->FindRect("construction bounds",
				   						 &bounds)) >= B_OK) {
			
				// compression defaults to NONE for backward compatibility
				if (from->FindInt32("compression", (int32*)&compression) < B_OK)
					compression = COMPRESSION_NONE;
				// format defaults to B_RGBA32 for backward compatibility
				if (from->FindInt32("format", (int32*)&format) < B_OK)
					format = B_RGBA32;

				switch (compression) {
					case COMPRESSION_ZLIB:
						*bitmap = decompress_bitmap_zlib(compressedData,
														 compressedSize,
														 bounds, format);
						break;
				}
			}
		}
		if (*bitmap)
			ret = (*bitmap)->InitCheck();
		else if (ret >= B_OK)
			ret = B_NO_MEMORY;
		if (ret < B_OK) {
			delete *bitmap;
			*bitmap = NULL;
		}
	}
	return ret;
}
