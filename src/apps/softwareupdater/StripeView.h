/*
 * Copyright 2007-2016 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Leavengood <leavengood@gmail.com>
 *		John Scipione <jscipione@gmail.com>
 *		Joseph Groover <looncraz@looncraz.net>
 *		Brian Hill <supernova@warpmail.net>
 */
#ifndef _STRIPE_VIEW_H
#define _STRIPE_VIEW_H


#include <Bitmap.h>
#include <View.h>


class StripeView : public BView {
public:
							StripeView(BBitmap* icon);

	virtual void			Draw(BRect updateRect);
	virtual BSize			PreferredSize();
	virtual	void			GetPreferredSize(float* _width, float* _height);
	virtual	BSize			MaxSize();

private:
			BBitmap*		fIcon;
			float			fWidth;
			float			fStripeWidth;
};


#endif /* _STRIPE_VIEW_H */
