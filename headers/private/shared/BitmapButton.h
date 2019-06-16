/*
 * Copyright 2010 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef BITMAP_BUTTON_H
#define BITMAP_BUTTON_H

#include <Button.h>
#include <Size.h>


class BBitmap;


namespace BPrivate {

class BBitmapButton : public BButton {
public:
	enum {
		BUTTON_BACKGROUND = 0,
		MENUBAR_BACKGROUND,
	};

								BBitmapButton(const char* resourceName,
									BMessage* message);

								BBitmapButton(const uint8* bits, uint32 width,
									uint32 height, color_space format,
									BMessage* message);

	virtual						~BBitmapButton();

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

	virtual	void				Draw(BRect updateRect);

			void				SetBackgroundMode(uint32 mode);

private:
			BBitmap*			fBitmap;
			uint32				fBackgroundMode;
};

};

using BPrivate::BBitmapButton;


#endif // BITMAP_BUTTON_H
