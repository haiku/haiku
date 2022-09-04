/*
 * Copyright 2002-2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm, darkwyrm@earthlink.net
 *		John Scipione, jscipione@gmail.com
 */
#ifndef COLOR_PREVIEW_H_
#define COLOR_PREVIEW_H_


#include <Control.h>


class BMessage;
class BMessageRunner;


class ColorPreview : public BControl
{
public:
							ColorPreview(BMessage* message,
								uint32 flags = B_WILL_DRAW);
							~ColorPreview(void);

	virtual	void			Draw(BRect updateRect);
	virtual	void			MessageReceived(BMessage* message);
	virtual	void			MouseDown(BPoint where);
	virtual	void			MouseMoved(BPoint where, uint32 transit,
								const BMessage* message);
	virtual	void			MouseUp(BPoint where);

			rgb_color		Color(void) const;
			void			SetColor(rgb_color color);
			void			SetColor(uint8 r, uint8 g, uint8 b);

			void			SetMode(bool rectangle);

private:
			void			_DragColor(BPoint where);

			rgb_color		fColor;
			rgb_color		fDisabledColor;

			BMessageRunner*	fMessageRunner;

			bool			fIsRectangle;
};

#endif	// COLOR_PREVIEW_H_
