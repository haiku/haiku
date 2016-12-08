/*
 * Copyright 2007-2016 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Leavengood <leavengood@gmail.com>
 *		John Scipione <jscipione@gmail.com>
 *		Joseph Groover <looncraz@looncraz.net>
 */
#ifndef _STRIPE_VIEW_H
#define _STRIPE_VIEW_H


#include <Bitmap.h>
#include <View.h>


class StripeView : public BView {
public:
							StripeView(BBitmap* icon);
							~StripeView();

	virtual void			Draw(BRect updateRect);
		
			BBitmap*		Icon() const { return fIcon; };
			void			SetIcon(BBitmap* icon);

private:
			BBitmap*		fIcon;
};


#endif /* _STRIPE_VIEW_H */
