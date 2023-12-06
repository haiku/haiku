/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2016-2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "ScreenshotInfo.h"


ScreenshotInfo::ScreenshotInfo()
	:
	fCode(),
	fWidth(),
	fHeight(),
	fDataSize()
{
}


ScreenshotInfo::ScreenshotInfo(const BString& code,
		int32 width, int32 height, int32 dataSize)
	:
	fCode(code),
	fWidth(width),
	fHeight(height),
	fDataSize(dataSize)
{
}


ScreenshotInfo::ScreenshotInfo(const ScreenshotInfo& other)
	:
	fCode(other.fCode),
	fWidth(other.fWidth),
	fHeight(other.fHeight),
	fDataSize(other.fDataSize)
{
}


ScreenshotInfo&
ScreenshotInfo::operator=(const ScreenshotInfo& other)
{
	fCode = other.fCode;
	fWidth = other.fWidth;
	fHeight = other.fHeight;
	fDataSize = other.fDataSize;
	return *this;
}


bool
ScreenshotInfo::operator==(const ScreenshotInfo& other) const
{
	return fCode == other.fCode
		&& fWidth == other.fWidth
		&& fHeight == other.fHeight
		&& fDataSize == other.fDataSize;
}


bool
ScreenshotInfo::operator!=(const ScreenshotInfo& other) const
{
	return !(*this == other);
}
