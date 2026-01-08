/*
 * Copyright 2026, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "ScreenshotCoordinate.h"


#include "Logger.h"


static const char* const kKeyCode = "code";
static const char* const kKeyWidth = "width";
static const char* const kKeyHeight = "height";


ScreenshotCoordinate::ScreenshotCoordinate()
	:
	fCode(""),
	fWidth(0),
	fHeight(0)
{
}


ScreenshotCoordinate::ScreenshotCoordinate(const BMessage* from)
{
	if (from->FindString(kKeyCode, &fCode) != B_OK)
		HDERROR("expected key [%s] in the message data", kKeyCode);
	if (from->FindUInt32(kKeyWidth, &fWidth) != B_OK)
		HDERROR("expected key [%s] in the message data", kKeyWidth);
	if (from->FindUInt32(kKeyHeight, &fHeight) != B_OK)
		HDERROR("expected key [%s] in the message data", kKeyHeight);
}


ScreenshotCoordinate::ScreenshotCoordinate(BString code, uint32 width, uint32 height)
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


uint32
ScreenshotCoordinate::Width() const
{
	return fWidth;
}


uint32
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
	result.SetToFormat("%s_%" B_PRIu32 "x%" B_PRIu32, fCode.String(), fWidth, fHeight);
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
		result = into->AddString(kKeyCode, fCode);
	if (result == B_OK)
		result = into->AddUInt32(kKeyWidth, fWidth);
	if (result == B_OK)
		result = into->AddUInt32(kKeyHeight, fHeight);
	return result;
}
