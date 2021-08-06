/*
 * Copyright 2006-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Axel Dörfler, axeld@pinc-software.de
 *		John Scipione, jscipione@gmail.com
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de
 */


#include "IconUtils.h"

#include <new>
#include <fs_attr.h>
#include <stdio.h>
#include <string.h>

#include <Bitmap.h>
#include <Node.h>
#include <NodeInfo.h>
#include <String.h>
#include <TypeConstants.h>

#include "AutoDeleter.h"
#include "Icon.h"
#include "IconRenderer.h"
#include "FlatIconImporter.h"
#include "MessageImporter.h"


#define B_MINI_ICON_TYPE	'MICN'
#define B_LARGE_ICON_TYPE	'ICON'


_USING_ICON_NAMESPACE;


//	#pragma mark - Scaling functions


static void
scale_bilinear(uint8* bits, int32 srcWidth, int32 srcHeight, int32 dstWidth,
	int32 dstHeight, uint32 bpr)
{
	// first pass: scale bottom to top

	uint8* dst = bits + (dstHeight - 1) * bpr;
		// offset to bottom left pixel in target size
	for (int32 x = 0; x < srcWidth; x++) {
		uint8* d = dst;
		for (int32 y = dstHeight - 1; y >= 0; y--) {
			int32 lineF = (y << 8) * (srcHeight - 1) / (dstHeight - 1);
			int32 lineI = lineF >> 8;
			uint8 weight = (uint8)(lineF & 0xff);
			uint8* s1 = bits + lineI * bpr + 4 * x;
			if (weight == 0) {
				d[0] = s1[0];
				d[1] = s1[1];
				d[2] = s1[2];
				d[3] = s1[3];
			} else {
				uint8* s2 = s1 + bpr;

				d[0] = (((s2[0] - s1[0]) * weight) + (s1[0] << 8)) >> 8;
				d[1] = (((s2[1] - s1[1]) * weight) + (s1[1] << 8)) >> 8;
				d[2] = (((s2[2] - s1[2]) * weight) + (s1[2] << 8)) >> 8;
				d[3] = (((s2[3] - s1[3]) * weight) + (s1[3] << 8)) >> 8;
			}

			d -= bpr;
		}
		dst += 4;
	}

	// second pass: scale right to left

	dst = bits + (dstWidth - 1) * 4;
		// offset to top left pixel in target size
	for (int32 y = 0; y < dstWidth; y++) {
		uint8* d = dst;
		for (int32 x = dstWidth - 1; x >= 0; x--) {
			int32 columnF = (x << 8) * (srcWidth - 1) / (dstWidth - 1);
			int32 columnI = columnF >> 8;
			uint8 weight = (uint8)(columnF & 0xff);
			uint8* s1 = bits + y * bpr + 4 * columnI;
			if (weight == 0) {
				d[0] = s1[0];
				d[1] = s1[1];
				d[2] = s1[2];
				d[3] = s1[3];
			} else {
				uint8* s2 = s1 + 4;

				d[0] = (((s2[0] - s1[0]) * weight) + (s1[0] << 8)) >> 8;
				d[1] = (((s2[1] - s1[1]) * weight) + (s1[1] << 8)) >> 8;
				d[2] = (((s2[2] - s1[2]) * weight) + (s1[2] << 8)) >> 8;
				d[3] = (((s2[3] - s1[3]) * weight) + (s1[3] << 8)) >> 8;
			}

			d -= 4;
		}
		dst += bpr;
	}
}


static void
scale_down(const uint8* srcBits, uint8* dstBits, int32 srcWidth, int32 srcHeight,
	int32 dstWidth, int32 dstHeight)
{
	int32 l;
	int32 c;
	float t;
	float u;
	float tmp;
	float d1, d2, d3, d4;
		// coefficients
	rgb_color p1, p2, p3, p4;
		// nearby pixels
	rgb_color out;
		// color components

	for (int32 i = 0; i < dstHeight; i++) {
		for (int32 j = 0; j < dstWidth; j++) {
			tmp = (float)(i) / (float)(dstHeight - 1) * (srcHeight - 1);
			l = (int32)floorf(tmp);
			if (l < 0)
				l = 0;
			else if (l >= srcHeight - 1)
				l = srcHeight - 2;
			u = tmp - l;

			tmp = (float)(j) / (float)(dstWidth - 1) * (srcWidth - 1);
			c = (int32)floorf(tmp);
			if (c < 0)
				c = 0;
			else if (c >= srcWidth - 1)
				c = srcWidth - 2;
			t = tmp - c;

			// coefficients
			d1 = (1 - t) * (1 - u);
			d2 = t * (1 - u);
			d3 = t * u;
			d4 = (1 - t) * u;

			// nearby pixels
			p1 = *((rgb_color*)srcBits + (l * srcWidth) + c);
			p2 = *((rgb_color*)srcBits + (l * srcWidth) + c + 1);
			p3 = *((rgb_color*)srcBits + ((l + 1) * srcWidth) + c + 1);
			p4 = *((rgb_color*)srcBits + ((l + 1) * srcWidth) + c);

			// color components
			out.blue = (uint8)(p1.blue * d1 + p2.blue * d2 + p3.blue * d3
				+ p4.blue * d4);
			out.green = (uint8)(p1.green * d1 + p2.green * d2 + p3.green * d3
				+ p4.green * d4);
			out.red = (uint8)(p1.red * d1 + p2.red * d2 + p3.red * d3
				+ p4.red * d4);
			out.alpha = (uint8)(p1.alpha * d1 + p2.alpha * d2 + p3.alpha * d3
				+ p4.alpha * d4);

			// destination RGBA pixel
			*((rgb_color*)dstBits + (i * dstWidth) + j) = out;
		}
	}
}


static void
scale2x(const uint8* srcBits, uint8* dstBits, int32 srcWidth, int32 srcHeight,
	int32 srcBPR, int32 dstBPR)
{
	/*
	 * This implements the AdvanceMAME Scale2x algorithm found on:
	 * http://scale2x.sourceforge.net/
	 *
	 * It is an incredibly simple and powerful image doubling routine that does
	 * an astonishing job of doubling game graphic data while interpolating out
	 * the jaggies.
	 *
	 * Derived from the (public domain) SDL version of the library by Pete
	 * Shinners.
	 */

	// Assume that both src and dst are 4 BPP (B_RGBA32)
	for (int32 y = 0; y < srcHeight; ++y) {
		for (int32 x = 0; x < srcWidth; ++x) {
			uint32 b = *(uint32*)(srcBits + (MAX(0, y - 1) * srcBPR)
				+ (4 * x));
			uint32 d = *(uint32*)(srcBits + (y * srcBPR)
				+ (4 * MAX(0, x - 1)));
			uint32 e = *(uint32*)(srcBits + (y * srcBPR)
				+ (4 * x));
			uint32 f = *(uint32*)(srcBits + (y * srcBPR)
				+ (4 * MIN(srcWidth - 1, x + 1)));
			uint32 h = *(uint32*)(srcBits + (MIN(srcHeight - 1, y + 1)
				* srcBPR) + (4 * x));

			uint32 e0 = d == b && b != f && d != h ? d : e;
			uint32 e1 = b == f && b != d && f != h ? f : e;
			uint32 e2 = d == h && d != b && h != f ? d : e;
			uint32 e3 = h == f && d != h && b != f ? f : e;

			*(uint32*)(dstBits + y * 2 * dstBPR + x * 2 * 4) = e0;
			*(uint32*)(dstBits + y * 2 * dstBPR + (x * 2 + 1) * 4) = e1;
			*(uint32*)(dstBits + (y * 2 + 1) * dstBPR + x * 2 * 4) = e2;
			*(uint32*)(dstBits + (y * 2 + 1) * dstBPR + (x * 2 + 1) * 4) = e3;
		}
	}
}


static void
scale3x(const uint8* srcBits, uint8* dstBits, int32 srcWidth, int32 srcHeight,
	int32 srcBPR, int32 dstBPR)
{
	/*
	 * This implements the AdvanceMAME Scale3x algorithm found on:
	 * http://scale2x.sourceforge.net/
	 *
	 * It is an incredibly simple and powerful image tripling routine that does
	 * an astonishing job of tripling game graphic data while interpolating out
	 * the jaggies.
	 *
	 * Derived from the (public domain) SDL version of the library by Pete
	 * Shinners.
	 */

	// Assume that both src and dst are 4 BPP (B_RGBA32)
	for (int32 y = 0; y < srcHeight; ++y) {
		for (int32 x = 0; x < srcWidth; ++x) {
			uint32 a = *(uint32*)(srcBits + (MAX(0, y - 1) * srcBPR)
				+ (4 * MAX(0, x - 1)));
			uint32 b = *(uint32*)(srcBits + (MAX(0, y - 1) * srcBPR)
				+ (4 * x));
			uint32 c = *(uint32*)(srcBits + (MAX(0, y - 1) * srcBPR)
				+ (4 * MIN(srcWidth - 1, x + 1)));
			uint32 d = *(uint32*)(srcBits + (y * srcBPR)
				+ (4 * MAX(0, x - 1)));
			uint32 e = *(uint32*)(srcBits + (y * srcBPR)
				+ (4 * x));
			uint32 f = *(uint32*)(srcBits + (y * srcBPR)
				+ (4 * MIN(srcWidth - 1,x + 1)));
			uint32 g = *(uint32*)(srcBits + (MIN(srcHeight - 1, y + 1)
				* srcBPR) + (4 * MAX(0, x - 1)));
			uint32 h = *(uint32*)(srcBits + (MIN(srcHeight - 1, y + 1)
				* srcBPR) + (4 * x));
			uint32 i = *(uint32*)(srcBits + (MIN(srcHeight - 1, y + 1)
				* srcBPR) + (4 * MIN(srcWidth - 1, x + 1)));

			uint32 e0 = d == b && b != f && d != h ? d : e;
			uint32 e1 = (d == b && b != f && d != h && e != c)
				|| (b == f && b != d && f != h && e != a) ? b : e;
			uint32 e2 = b == f && b != d && f != h ? f : e;
			uint32 e3 = (d == b && b != f && d != h && e != g)
				|| (d == b && b != f && d != h && e != a) ? d : e;
			uint32 e4 = e;
			uint32 e5 = (b == f && b != d && f != h && e != i)
				|| (h == f && d != h && b != f && e != c) ? f : e;
			uint32 e6 = d == h && d != b && h != f ? d : e;
			uint32 e7 = (d == h && d != b && h != f && e != i)
				|| (h == f && d != h && b != f && e != g) ? h : e;
			uint32 e8 = h == f && d != h && b != f ? f : e;

			*(uint32*)(dstBits + y * 3 * dstBPR + x * 3 * 4) = e0;
			*(uint32*)(dstBits + y * 3 * dstBPR + (x * 3 + 1) * 4) = e1;
			*(uint32*)(dstBits + y * 3 * dstBPR + (x * 3 + 2) * 4) = e2;
			*(uint32*)(dstBits + (y * 3 + 1) * dstBPR + x * 3 * 4) = e3;
			*(uint32*)(dstBits + (y * 3 + 1) * dstBPR + (x * 3 + 1) * 4) = e4;
			*(uint32*)(dstBits + (y * 3 + 1) * dstBPR + (x * 3 + 2) * 4) = e5;
			*(uint32*)(dstBits + (y * 3 + 2) * dstBPR + x * 3 * 4) = e6;
			*(uint32*)(dstBits + (y * 3 + 2) * dstBPR + (x * 3 + 1) * 4) = e7;
			*(uint32*)(dstBits + (y * 3 + 2) * dstBPR + (x * 3 + 2) * 4) = e8;
		}
	}
}


static void
scale4x(const uint8* srcBits, uint8* dstBits, int32 srcWidth, int32 srcHeight,
	int32 srcBPR, int32 dstBPR)
{
	// scale4x is just scale2x twice
	BRect rect = BRect(0, 0, srcWidth * 2 - 1, srcHeight * 2 - 1);
	BBitmap* tmp = new BBitmap(rect, B_BITMAP_NO_SERVER_LINK, B_RGBA32);
	uint8* tmpBits = (uint8*)tmp->Bits();
	int32 tmpBPR = tmp->BytesPerRow();

	scale2x(srcBits, tmpBits, srcWidth, srcHeight, srcBPR, tmpBPR);
	scale2x(tmpBits, dstBits, srcWidth * 2, srcHeight * 2, tmpBPR, dstBPR);

	delete tmp;
}


//	#pragma mark - GetIcon()


status_t
BIconUtils::GetIcon(BNode* node, const char* vectorIconAttrName,
	const char* smallIconAttrName, const char* largeIconAttrName,
	icon_size which, BBitmap* icon)
{
	if (node == NULL || icon == NULL)
		return B_BAD_VALUE;

	status_t result = node->InitCheck();
	if (result != B_OK)
		return result;

	result = icon->InitCheck();
	if (result != B_OK)
		return result;

	switch (icon->ColorSpace()) {
		case B_RGBA32:
		case B_RGB32:
			// prefer vector icon
			result = GetVectorIcon(node, vectorIconAttrName, icon);
			if (result != B_OK) {
				// try to fallback to B_CMAP8 icons
				// (converting to B_RGBA32 is handled)

				// override size
				if (icon->Bounds().IntegerWidth() + 1 >= B_LARGE_ICON)
					which = B_LARGE_ICON;
				else
					which = B_MINI_ICON;

				result = GetCMAP8Icon(node, smallIconAttrName,
					largeIconAttrName, which, icon);
			}
			break;

		case B_CMAP8:
			// prefer old B_CMAP8 icons
			result = GetCMAP8Icon(node, smallIconAttrName, largeIconAttrName,
				which, icon);
			if (result != B_OK) {
				// try to fallback to vector icon
				BBitmap temp(icon->Bounds(), B_BITMAP_NO_SERVER_LINK,
					B_RGBA32);
				result = temp.InitCheck();
				if (result != B_OK)
					break;

				result = GetVectorIcon(node, vectorIconAttrName, &temp);
				if (result != B_OK)
					break;

				uint32 width = temp.Bounds().IntegerWidth() + 1;
				uint32 height = temp.Bounds().IntegerHeight() + 1;
				uint32 bytesPerRow = temp.BytesPerRow();
				result = ConvertToCMAP8((uint8*)temp.Bits(), width, height,
					bytesPerRow, icon);
			}
			break;

		default:
			printf("BIconUtils::GetIcon() - unsupported colorspace\n");
			result = B_ERROR;
			break;
	}

	return result;
}


//	#pragma mark - GetVectorIcon()


status_t
BIconUtils::GetVectorIcon(BNode* node, const char* attrName, BBitmap* icon)
{
	if (node == NULL || attrName == NULL || *attrName == '\0' || icon == NULL)
		return B_BAD_VALUE;

	status_t result = node->InitCheck();
	if (result != B_OK)
		return result;

	result = icon->InitCheck();
	if (result != B_OK)
		return result;

#if TIME_VECTOR_ICONS
	bigtime_t startTime = system_time();
#endif

	// get the attribute info and check type and size of the attr contents
	attr_info attrInfo;
	result = node->GetAttrInfo(attrName, &attrInfo);
	if (result != B_OK)
		return result;

	type_code attrType = B_VECTOR_ICON_TYPE;

	if (attrInfo.type != attrType)
		return B_BAD_TYPE;

	// chicken out on unrealisticly large attributes
	if (attrInfo.size > 512 * 1024)
		return B_BAD_VALUE;

	uint8* buffer = new(std::nothrow) uint8[attrInfo.size];
	if (buffer == NULL)
		return B_NO_MEMORY;

	ArrayDeleter<uint8> deleter(buffer);

	ssize_t bytesRead = node->ReadAttr(attrName, attrType, 0, buffer,
		attrInfo.size);
	if (bytesRead != attrInfo.size)
		return B_ERROR;

#if TIME_VECTOR_ICONS
	bigtime_t importTime = system_time();
#endif

	result = GetVectorIcon(buffer, attrInfo.size, icon);
	if (result != B_OK)
		return result;

#if TIME_VECTOR_ICONS
	bigtime_t finishTime = system_time();
	printf("read: %lld, import: %lld\n", importTime - startTime,
		finishTime - importTime);
#endif

	return B_OK;
}


status_t
BIconUtils::GetVectorIcon(const uint8* buffer, size_t size, BBitmap* icon)
{
	if (buffer == NULL || size <= 0 || icon == NULL)
		return B_BAD_VALUE;

	status_t result = icon->InitCheck();
	if (result != B_OK)
		return result;

	BBitmap* temp = icon;
	ObjectDeleter<BBitmap> deleter;

	if (icon->ColorSpace() != B_RGBA32 && icon->ColorSpace() != B_RGB32) {
		temp = new(std::nothrow) BBitmap(icon->Bounds(),
			B_BITMAP_NO_SERVER_LINK, B_RGBA32);
		deleter.SetTo(temp);
		if (temp == NULL || temp->InitCheck() != B_OK)
			return B_NO_MEMORY;
	}

	Icon vector;
	result = vector.InitCheck();
	if (result != B_OK)
		return result;

	FlatIconImporter importer;
	result = importer.Import(&vector, const_cast<uint8*>(buffer), size);
	if (result != B_OK) {
		// try the message based format used by Icon-O-Matic
		MessageImporter messageImporter;
		BMemoryIO memoryIO(const_cast<uint8*>(buffer), size);
		result = messageImporter.Import(&vector, &memoryIO);
		if (result != B_OK)
			return result;
	}

	IconRenderer renderer(temp);
	renderer.SetIcon(&vector);
	renderer.SetScale((temp->Bounds().Width() + 1.0) / 64.0);
	renderer.Render();

	if (temp != icon) {
		uint8* src = (uint8*)temp->Bits();
		uint32 width = temp->Bounds().IntegerWidth() + 1;
		uint32 height = temp->Bounds().IntegerHeight() + 1;
		uint32 srcBPR = temp->BytesPerRow();
		result = ConvertToCMAP8(src, width, height, srcBPR, icon);
	}

	// TODO: would be nice to get rid of this
	// (B_RGBA32_PREMULTIPLIED or better yet, new blending_mode)
	// NOTE: probably not necessary only because
	// transparent colors are "black" in all existing icons
	// lighter transparent colors should be too dark if
	// app_server uses correct blending
	//renderer.Demultiply();

	return result;
}


//	#pragma mark - GetCMAP8Icon()


status_t
BIconUtils::GetCMAP8Icon(BNode* node, const char* smallIconAttrName,
	const char* largeIconAttrName, icon_size which, BBitmap* icon)
{
	// NOTE: this might be changed if other icon
	// sizes are supported in B_CMAP8 attributes,
	// but this is currently not the case, so we
	// relax the requirement to pass an icon
	// of just the right size
	if (which < B_LARGE_ICON)
		which = B_MINI_ICON;
	else
		which = B_LARGE_ICON;

	// check parameters and initialization
	if (node == NULL || icon == NULL
		|| (which == B_MINI_ICON
			&& (smallIconAttrName == NULL || *smallIconAttrName == '\0'))
		|| (which == B_LARGE_ICON
			&& (largeIconAttrName == NULL || *largeIconAttrName == '\0'))) {
		return B_BAD_VALUE;
	}

	status_t result;
	result = node->InitCheck();
	if (result != B_OK)
		return result;

	result = icon->InitCheck();
	if (result != B_OK)
		return result;

	// set some icon size related variables
	const char* attribute = NULL;
	BRect bounds;
	uint32 attrType = 0;
	off_t attrSize = 0;
	switch (which) {
		case B_MINI_ICON:
			attribute = smallIconAttrName;
			bounds.Set(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1);
			attrType = B_MINI_ICON_TYPE;
			attrSize = B_MINI_ICON * B_MINI_ICON;
			break;

		case B_LARGE_ICON:
			attribute = largeIconAttrName;
			bounds.Set(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1);
			attrType = B_LARGE_ICON_TYPE;
			attrSize = B_LARGE_ICON * B_LARGE_ICON;
			break;

		default:
			// can not happen, see above
			result = B_BAD_VALUE;
			break;
	}

	// get the attribute info and check type and size of the attr contents
	attr_info attrInfo;
	if (result == B_OK)
		result = node->GetAttrInfo(attribute, &attrInfo);

	if (result == B_OK && attrInfo.type != attrType)
		result = B_BAD_TYPE;

	if (result == B_OK && attrInfo.size != attrSize)
		result = B_BAD_DATA;

	// check parameters
	// currently, scaling B_CMAP8 icons is not supported
	if (icon->ColorSpace() == B_CMAP8 && icon->Bounds() != bounds)
		return B_BAD_VALUE;

	// read the attribute
	if (result == B_OK) {
		bool useBuffer = (icon->ColorSpace() != B_CMAP8
			|| icon->Bounds() != bounds);
		uint8* buffer = NULL;
		ssize_t bytesRead;
		if (useBuffer) {
			// other color space or bitmap size than stored in attribute
			buffer = new(std::nothrow) uint8[attrSize];
			if (buffer == NULL)
				bytesRead = result = B_NO_MEMORY;
			else {
				bytesRead = node->ReadAttr(attribute, attrType, 0, buffer,
					attrSize);
			}
		} else {
			bytesRead = node->ReadAttr(attribute, attrType, 0, icon->Bits(),
				attrSize);
		}

		if (result == B_OK) {
			if (bytesRead < 0)
				result = (status_t)bytesRead;
			else if (bytesRead != (ssize_t)attrSize)
				result = B_ERROR;
		}

		if (useBuffer) {
			// other color space than stored in attribute
			if (result == B_OK) {
				result = ConvertFromCMAP8(buffer, (uint32)which, (uint32)which,
					(uint32)which, icon);
			}
			delete[] buffer;
		}
	}

	return result;
}


//	#pragma mark - ConvertFromCMAP8() and ConvertToCMAP8()


status_t
BIconUtils::ConvertFromCMAP8(BBitmap* source, BBitmap* destination)
{
	if (source == NULL || source->ColorSpace() != B_CMAP8)
		return B_BAD_VALUE;

	status_t result = source->InitCheck();
	if (result != B_OK)
		return result;

	result = destination->InitCheck();
	if (result != B_OK)
		return result;

	uint8* src = (uint8*)source->Bits();
	uint32 srcBPR = source->BytesPerRow();
	uint32 width = source->Bounds().IntegerWidth() + 1;
	uint32 height = source->Bounds().IntegerHeight() + 1;

	return ConvertFromCMAP8(src, width, height, srcBPR, destination);
}


status_t
BIconUtils::ConvertToCMAP8(BBitmap* source, BBitmap* destination)
{
	if (source == NULL || source->ColorSpace() != B_RGBA32
		|| destination->ColorSpace() != B_CMAP8) {
		return B_BAD_VALUE;
	}

	status_t result = source->InitCheck();
	if (result != B_OK)
		return result;

	result = destination->InitCheck();
	if (result != B_OK)
		return result;

	uint8* src = (uint8*)source->Bits();
	uint32 srcBPR = source->BytesPerRow();
	uint32 width = source->Bounds().IntegerWidth() + 1;
	uint32 height = source->Bounds().IntegerHeight() + 1;

	return ConvertToCMAP8(src, width, height, srcBPR, destination);
}


status_t
BIconUtils::ConvertFromCMAP8(const uint8* src, uint32 width, uint32 height,
	uint32 srcBPR, BBitmap* icon)
{
	if (src == NULL || icon == NULL || srcBPR == 0)
		return B_BAD_VALUE;

	status_t result = icon->InitCheck();
	if (result != B_OK)
		return result;

	if (icon->ColorSpace() != B_RGBA32 && icon->ColorSpace() != B_RGB32) {
		// TODO: support other color spaces
		return B_BAD_VALUE;
	}

	uint32 dstWidth = icon->Bounds().IntegerWidth() + 1;
	uint32 dstHeight = icon->Bounds().IntegerHeight() + 1;

	uint8* dst = (uint8*)icon->Bits();
	uint32 dstBPR = icon->BytesPerRow();

	// check for downscaling or integer multiple scaling
	if (dstWidth < width || dstHeight < height
		|| (dstWidth == 2 * width && dstHeight == 2 * height)
		|| (dstWidth == 3 * width && dstHeight == 3 * height)
		|| (dstWidth == 4 * width && dstHeight == 4 * height)) {
		BRect rect = BRect(0, 0, width - 1, height - 1);
		BBitmap* converted = new(std::nothrow) BBitmap(rect,
			B_BITMAP_NO_SERVER_LINK, icon->ColorSpace());
		if (converted == NULL)
			return B_NO_MEMORY;

		converted->ImportBits(src, height * srcBPR, srcBPR, 0, B_CMAP8);
		uint8* convertedBits = (uint8*)converted->Bits();
		int32 convertedBPR = converted->BytesPerRow();

		if (dstWidth < width || dstHeight < height)
			scale_down(convertedBits, dst, width, height, dstWidth, dstHeight);
		else if (dstWidth == 2 * width && dstHeight == 2 * height)
			scale2x(convertedBits, dst, width, height, convertedBPR, dstBPR);
		else if (dstWidth == 3 * width && dstHeight == 3 * height)
			scale3x(convertedBits, dst, width, height, convertedBPR, dstBPR);
		else if (dstWidth == 4 * width && dstHeight == 4 * height)
			scale4x(convertedBits, dst, width, height, convertedBPR, dstBPR);

		delete converted;
		return B_OK;
	}

	const rgb_color* colorMap = system_colors()->color_list;
	if (colorMap == NULL)
		return B_NO_INIT;

	const uint8* srcStart = src;
	uint8* dstStart = dst;

	// convert from B_CMAP8 to B_RGB(A)32 without scaling
	for (uint32 y = 0; y < height; y++) {
		uint32* d = (uint32*)dst;
		const uint8* s = src;
		for (uint32 x = 0; x < width; x++, s++, d++) {
			const rgb_color c = colorMap[*s];
			uint8 alpha = 0xff;
			if (*s == B_TRANSPARENT_MAGIC_CMAP8)
				alpha = 0;
			*d = (alpha << 24) | (c.red << 16) | (c.green << 8) | (c.blue);
		}
		src += srcBPR;
		dst += dstBPR;
	}

	if (width == dstWidth && height == dstHeight)
		return B_OK;

	// reset src and dst back to their original locations
	src = srcStart;
	dst = dstStart;

	if (dstWidth > width && dstHeight > height
		&& dstWidth < 2 * width && dstHeight < 2 * height) {
		// scale2x then downscale
		BRect rect = BRect(0, 0, width * 2 - 1, height * 2 - 1);
		BBitmap* temp = new(std::nothrow) BBitmap(rect,
			B_BITMAP_NO_SERVER_LINK, icon->ColorSpace());
		if (temp == NULL)
			return B_NO_MEMORY;

		uint8* tempBits = (uint8*)temp->Bits();
		uint32 tempBPR = temp->BytesPerRow();
		scale2x(dst, tempBits, width, height, dstBPR, tempBPR);
		scale_down(tempBits, dst, width * 2, height * 2, dstWidth, dstHeight);
		delete temp;
	} else if (dstWidth > 2 * width && dstHeight > 2 * height
		&& dstWidth < 3 * width && dstHeight < 3 * height) {
		// scale3x then downscale
		BRect rect = BRect(0, 0, width * 3 - 1, height * 3 - 1);
		BBitmap* temp = new BBitmap(rect, B_BITMAP_NO_SERVER_LINK,
			icon->ColorSpace());
		if (temp == NULL)
			return B_NO_MEMORY;

		uint8* tempBits = (uint8*)temp->Bits();
		uint32 tempBPR = temp->BytesPerRow();
		scale3x(dst, tempBits, width, height, dstBPR, tempBPR);
		scale_down(tempBits, dst, width * 3, height * 3, dstWidth, dstHeight);
		delete temp;
	} else if (dstWidth > 3 * width && dstHeight > 3 * height
		&& dstWidth < 4 * width && dstHeight < 4 * height) {
		// scale4x then downscale
		BRect rect = BRect(0, 0, width * 4 - 1, height * 4 - 1);
		BBitmap* temp = new BBitmap(rect, B_BITMAP_NO_SERVER_LINK,
			icon->ColorSpace());
		if (temp == NULL)
			return B_NO_MEMORY;

		uint8* tempBits = (uint8*)temp->Bits();
		uint32 tempBPR = temp->BytesPerRow();
		scale4x(dst, tempBits, width, height, dstBPR, tempBPR);
		scale_down(tempBits, dst, width * 3, height * 3, dstWidth, dstHeight);
		delete temp;
	} else if (dstWidth > 4 * width && dstHeight > 4 * height) {
		// scale4x then bilinear
		BRect rect = BRect(0, 0, width * 4 - 1, height * 4 - 1);
		BBitmap* temp = new BBitmap(rect, B_BITMAP_NO_SERVER_LINK,
			icon->ColorSpace());
		if (temp == NULL)
			return B_NO_MEMORY;

		uint8* tempBits = (uint8*)temp->Bits();
		uint32 tempBPR = temp->BytesPerRow();
		scale4x(dst, tempBits, width, height, dstBPR, tempBPR);
		icon->ImportBits(tempBits, height * tempBPR, tempBPR, 0,
			temp->ColorSpace());
		scale_bilinear(dst, width, height, dstWidth, dstHeight, dstBPR);
		delete temp;
	} else {
		// fall back to bilinear scaling
		scale_bilinear(dst, width, height, dstWidth, dstHeight, dstBPR);
	}

	return B_OK;
}


status_t
BIconUtils::ConvertToCMAP8(const uint8* src, uint32 width, uint32 height,
	uint32 srcBPR, BBitmap* icon)
{
	if (src == NULL || icon == NULL || srcBPR == 0)
		return B_BAD_VALUE;

	status_t result = icon->InitCheck();
	if (result != B_OK)
		return result;

	if (icon->ColorSpace() != B_CMAP8)
		return B_BAD_VALUE;

	uint32 dstWidth = icon->Bounds().IntegerWidth() + 1;
	uint32 dstHeight = icon->Bounds().IntegerHeight() + 1;

	if (dstWidth < width || dstHeight < height) {
		// TODO: down scaling
		return B_ERROR;
	} else if (dstWidth > width || dstHeight > height) {
		// TODO: up scaling
		// (currently copies bitmap into icon at left-top)
		memset(icon->Bits(), 255, icon->BitsLength());
	}

//#if __HAIKU__
//	return icon->ImportBits(src, height * srcBPR, srcBPR, 0, B_RGBA32);
//#else
	uint8* dst = (uint8*)icon->Bits();
	uint32 dstBPR = icon->BytesPerRow();

	const color_map* colorMap = system_colors();
	if (colorMap == NULL)
		return B_NO_INIT;

	uint16 index;

	for (uint32 y = 0; y < height; y++) {
		uint8* d = dst;
		const uint8* s = src;
		for (uint32 x = 0; x < width; x++) {
			if (s[3] < 128) {
				*d = B_TRANSPARENT_MAGIC_CMAP8;
			} else {
				index = ((s[2] & 0xf8) << 7) | ((s[1] & 0xf8) << 2)
						| (s[0] >> 3);
				*d = colorMap->index_map[index];
			}
			s += 4;
			d += 1;
		}
		src += srcBPR;
		dst += dstBPR;
	}

	return B_OK;
//#endif // __HAIKU__
}


//	#pragma mark - Forbidden


BIconUtils::BIconUtils() {}
BIconUtils::~BIconUtils() {}
BIconUtils::BIconUtils(const BIconUtils&) {}
BIconUtils& BIconUtils::operator=(const BIconUtils&) { return *this; }
