/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "IconUtils.h"

#include <fs_attr.h>

#include <Bitmap.h>
#include <Node.h>
#include <TypeConstants.h>

#include "Icon.h"
#include "IconRenderer.h"
#include "FlatIconImporter.h"

// GetVectorIcon
status_t
BIconUtils::GetVectorIcon(BNode* node, const char* attrName,
						  BBitmap* result)
{
	if (!node || !attrName)
		return B_BAD_VALUE;

#if TIME_VECTOR_ICONS
bigtime_t startTime = system_time();
#endif

	// get the attribute info and check type and size of the attr contents
	attr_info attrInfo;
	status_t ret = node->GetAttrInfo(attrName, &attrInfo);
	if (ret < B_OK)
		return ret;

	type_code attrType = B_RAW_TYPE;
		// TODO: introduce special type?

	if (attrInfo.type != attrType)
		return B_BAD_TYPE;

	// chicken out on unrealisticly large attributes
	if (attrInfo.size > 16 * 1024)
		return B_BAD_VALUE;

	uint8 buffer[attrInfo.size];
	node->ReadAttr(attrName, attrType, 0, buffer, attrInfo.size);

#if TIME_VECTOR_ICONS
bigtime_t importTime = system_time();
#endif

	ret = GetVectorIcon(buffer, attrInfo.size, result);
	if (ret < B_OK)
		return ret;

#if TIME_VECTOR_ICONS
bigtime_t finishTime = system_time();
printf("read: %lld, import: %lld\n", importTime - startTime, finishTime - importTime);
#endif

	return B_OK;
}

// GetVectorIcon
status_t
BIconUtils::GetVectorIcon(const uint8* buffer, size_t size,
						  BBitmap* result)
{
	if (!result)
		return B_BAD_VALUE;

	status_t ret = result->InitCheck();
	if (ret < B_OK)
		return ret;

	if (result->ColorSpace() != B_RGBA32 && result->ColorSpace() != B_RGB32)
		return B_BAD_VALUE;

	Icon icon;
	ret = icon.InitCheck();
	if (ret < B_OK)
		return ret;

	FlatIconImporter importer;
	ret = importer.Import(&icon, const_cast<uint8*>(buffer), size);
	if (ret < B_OK)
		return ret;

	IconRenderer renderer(result);
	renderer.SetIcon(&icon);
	renderer.SetScale((result->Bounds().Width() + 1.0) / 64.0);
	renderer.Render();

	// TODO: would be nice to get rid of this
	// (B_RGBA32_PREMULTIPLIED or better yes, new blending_mode)
	// NOTE: probably not necessary only because
	// transparent colors are "black" in all existing icons
	// lighter transparent colors should be too dark if
	// app_server uses correct blending
//	renderer.Demultiply();

	return B_OK;
}

// ConvertFromCMAP8
status_t
BIconUtils::ConvertFromCMAP8(BBitmap* source, BBitmap* result)
{
	if (!source)
		return B_BAD_VALUE;

	status_t ret = source->InitCheck();
	if (ret < B_OK)
		return ret;

	uint8* src = (uint8*)source->Bits();
	uint32 srcBPR = source->BytesPerRow();
	uint32 width = source->Bounds().IntegerWidth() + 1;
	uint32 height = source->Bounds().IntegerHeight() + 1;

	return ConvertFromCMAP8(src, width, height, srcBPR, result);
}

// ConvertFromCMAP8
status_t
BIconUtils::ConvertFromCMAP8(const uint8* src,
							 uint32 width, uint32 height, uint32 srcBPR,
							 BBitmap* result)
{
	if (!src || !result || srcBPR == 0)
		return B_BAD_VALUE;

	status_t ret = result->InitCheck();
	if (ret < B_OK)
		return ret;

	uint32 dstWidth = result->Bounds().IntegerWidth() + 1;
	uint32 dstHeight = result->Bounds().IntegerHeight() + 1;

	if (dstWidth < width || dstHeight < height) {
		// TODO: down scaling
		return B_ERROR;
	} else if (dstWidth > width || dstHeight > height) {
		// TODO: up scaling
		// (currently copies bitmap into result at left-top)
	}

#if __HAIKU__

	return result->ImportBits(src, height * srcBPR, srcBPR, 0, B_CMAP8);

#else

	if (result->ColorSpace() != B_RGBA32 && result->ColorSpace() != B_RGB32) {
		// TODO: support other color spaces
		return B_BAD_VALUE;
	}

	uint8* dst = (uint8*)result->Bits();
	uint32 dstBPR = result->BytesPerRow();

	const rgb_color* colorMap = system_colors()->color_list;

	for (uint32 y = 0; y < dstHeight; y++) {
		uint32* d = (uint32*)dst;
		const uint8* s = src;
		for (uint32 x = 0; x < dstWidth; x++) {
			const rgb_color c = colorMap[*s];
			uint8 alpha = 255;
			if (*s == B_TRANSPARENT_MAGIC_CMAP8)
				alpha = 0;
			*d = (alpha << 24) | (c.red << 16) | (c.green << 8) | (c.blue);
			s++;
			d++;
		}
		src += srcBPR;
		dst += dstBPR;
	}

	return B_OK;

#endif // __HAIKU__
}

// #pragma mark - forbidden

BIconUtils::BIconUtils() {}
BIconUtils::~BIconUtils() {}
BIconUtils::BIconUtils(const BIconUtils&) {}
BIconUtils& BIconUtils::operator=(const BIconUtils&) { return *this; }

