/*
 * Copyright 2002-2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm, darkwyrm@earthlink.net
 *		John Scipione, jscipione@gmail.com
 */
#ifndef COLOR_PREVIEW_H_
#define COLOR_PREVIEW_H_


#include <View.h>
#include <Message.h>
#include <Invoker.h>


class ColorPreview : public BView
{
public:
							ColorPreview(BRect frame, BMessage *msg,
								uint32 resizingMode = B_FOLLOW_LEFT
									| B_FOLLOW_TOP,
								uint32 flags = B_WILL_DRAW);
							~ColorPreview(void);

	virtual	void			Draw(BRect update);
	virtual	void			MessageReceived(BMessage* message);
	virtual	void			SetTarget(BHandler* target);
	virtual	void			SetEnabled(bool value);

			rgb_color		Color(void) const;
			void			SetColor(rgb_color col);
			void			SetColor(uint8 r,uint8 g, uint8 b);

			void			SetMode(bool is_rectangle);

protected:
			BInvoker*		invoker;

			bool			is_enabled;
			bool			is_rect;
			rgb_color		disabledcol;
			rgb_color		currentcol;
};

#endif	// COLOR_PREVIEW_H_
