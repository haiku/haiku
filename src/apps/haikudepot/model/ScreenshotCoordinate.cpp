/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "ScreenshotCoordinate.h"


static const char* kCodeKey = "code";
static const char* kWidthKey = "width";
static const char* kHeightKey = "height";


ScreenshotCoordinate::ScreenshotCoordinate()
	:
	fCode(""),
	fWidth(0),
	fHeight(0)
{
}


ScreenshotCoordinate::ScreenshotCoordinate(const BMessage* from)
{
	from->FindString(kCodeKey, &fCode);
	from->FindUInt16(kWidthKey, &fWidth);
	from->FindUInt16(kHeightKey, &fHeight);
}


ScreenshotCoordinate::ScreenshotCoordinate(BString code, uint16 width, uint16 height)
	:
	fCode(code),
	fWidth(width),
	fHeight(height)
{
}


ScreenshotCoordinate::~ScreenshotCoordinate()
{
}


const BString
ScreenshotCoordinate::Code() const
{
	return fCode;
}


uint16
ScreenshotCoordinate::Width() const
{
	return fWidth;
}


uint16
ScreenshotCoordinate::Height() const
{
	return fHeight;
}


bool
ScreenshotCoordinate::IsValid() const
{
	return !fCode.IsEmpty() && fWidth > 0 && fHeight > 0;
}


bool
ScreenshotCoordinate::operator==(const ScreenshotCoordinate& other) const
{
	return fCode == other.fCode && fHeight == other.fHeight && fWidth == other.fWidth;
}


const BString
ScreenshotCoordinate::Key() const
{
	BString result;
	result.SetToFormat("%s_%" B_PRIu16 "x%" B_PRIu16 , fCode.String(), fWidth, fHeight);
	return result;
}


const BString
ScreenshotCoordinate::CacheFilename() const
{
	return BString() << Key() << ".png";
}


status_t
ScreenshotCoordinate::Archive(BMessage* into, bool deep) const
{
	status_t result = B_OK;
	if (result == B_OK)
		result = into->AddString(kCodeKey, fCode);
	if (result == B_OK)
		result = into->AddUInt16(kWidthKey, fWidth);
	if (result == B_OK)
		result = into->AddUInt16(kHeightKey, fHeight);
	return result;
}
