/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SCREENSHOT_COORDINATE_H
#define SCREENSHOT_COORDINATE_H


#include <Archivable.h>
#include <String.h>


/*!	A screenshot coordinate defines a screenshot to obtain. The `code` is
	recorded against a screenshot in the HDS system and then the width and
	height define the sizing required for that screenshot.
*/

class ScreenshotCoordinate : public BArchivable {
public:
								ScreenshotCoordinate(const BMessage* from);
								ScreenshotCoordinate(BString code, uint32 width, uint32 height);
								ScreenshotCoordinate();
	virtual						~ScreenshotCoordinate();

	const	BString				Code() const;
			uint32				Width() const;
			uint32				Height() const;

			bool				operator==(const ScreenshotCoordinate& other) const;

			bool				IsValid() const;
	const	BString				Key() const;
	const	BString				CacheFilename() const;

			status_t			Archive(BMessage* into, bool deep = true) const;

private:
			BString				fCode;
			uint32				fWidth;
			uint32				fHeight;
};


#endif // SCREENSHOT_COORDINATE_H