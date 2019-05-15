/*
 * Copyright 2007-2016 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Leavengood <leavengood@gmail.com>
 *		John Scipione <jscipione@gmail.com>
 *		Joseph Groover <looncraz@looncraz.net>
 *		Brian Hill <supernova@tycho.email>
 */
#ifndef _STRIPE_VIEW_H
#define _STRIPE_VIEW_H


#include <Bitmap.h>
#include <View.h>


namespace BPrivate {


class BStripeView : public BView {
public:
							BStripeView(BBitmap& icon);

	virtual void			Draw(BRect updateRect);
	virtual BSize			PreferredSize();
	virtual	void			GetPreferredSize(float* _width, float* _height);
	virtual	BSize			MaxSize();

private:
			BBitmap			fIcon;
			float			fIconSize;
			float			fPreferredWidth;
			float			fPreferredHeight;
};


static inline int32
icon_layout_scale()
{
	return max_c(1, ((int32)be_plain_font->Size() + 15) / 16);
}


};


using namespace BPrivate;


#endif /* _STRIPE_VIEW_H */
