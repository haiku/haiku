/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SCREENSHOT_INFO_H
#define SCREENSHOT_INFO_H


#include <Referenceable.h>
#include <String.h>


class ScreenshotInfo : public BReferenceable
{
public:
								ScreenshotInfo();
								ScreenshotInfo(const BString& code,
									int32 width, int32 height, int32 dataSize);
								ScreenshotInfo(const ScreenshotInfo& other);

			ScreenshotInfo&		operator=(const ScreenshotInfo& other);
			bool				operator==(const ScreenshotInfo& other) const;
			bool				operator!=(const ScreenshotInfo& other) const;

			const BString&		Code() const
									{ return fCode; }
			int32				Width() const
									{ return fWidth; }
			int32				Height() const
									{ return fHeight; }
			int32				DataSize() const
									{ return fDataSize; }

private:
			BString				fCode;
			int32				fWidth;
			int32				fHeight;
			int32				fDataSize;
};


typedef BReference<ScreenshotInfo> ScreenshotInfoRef;


#endif // SCREENSHOT_INFO_H
