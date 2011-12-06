/*
 * Copyright 2010 Wim van der Meer <WPJvanderMeer@gmail.com>
 * Distributed under the terms of the MIT License.
 */
#ifndef UTILITY_H
#define UTILITY_H


#include <Point.h>
#include <Rect.h>
#include <String.h>


class BBitmap;


// Command constant for sending utility data to the GUI app
const int32 SS_UTILITY_DATA = 'SSUD';


class Utility {
public:
						Utility();
						~Utility();

			void		CopyToClipboard(const BBitmap& screenshot) const;
			status_t	Save(BBitmap** screenshot, const char* fileName,
							uint32 imageType) const;
			BBitmap*	MakeScreenshot(bool includeCursor, bool activeWindow,
							bool includeBorder) const;
			BString		GetFileNameExtension(uint32 imageType) const;

			BBitmap*	wholeScreen;
			BBitmap*	cursorBitmap;
			BBitmap*	cursorAreaBitmap;
			BPoint		cursorPosition;
			BRect		activeWindowFrame;
			BRect		tabFrame;
			float		borderSize;

	static	const char*	sDefaultFileNameBase;

private:
			void		_MakeTabSpaceTransparent(BBitmap* screenshot,
							BRect frame) const;
			BString		_GetMimeString(uint32 imageType) const;
};


#endif // UTILITY_H
