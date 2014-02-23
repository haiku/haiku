/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Tri-Edge AI
 *		John Scipione, jscipione@gmail.com
 */
#ifndef GRAVITY_CONFIG_VIEW_H
#define GRAVITY_CONFIG_VIEW_H


#include <View.h>


class Gravity;

class BListView;
class BScrollView;
class BSlider;
class BStringView;


class ConfigView : public BView {
public:
								ConfigView(BRect frame, Gravity* parent);

			void	 			AttachedToWindow();
			void	 			MessageReceived(BMessage* message);

private:
			Gravity*			fParent;

			BStringView*		fTitleString;
			BStringView*		fAuthorString;

			BSlider*			fCountSlider;

			BStringView*		fShadeString;
			BListView*			fShadeList;
			BScrollView*		fShadeScroll;
};


#endif	// GRAVITY_CONFIG_VIEW_H
