/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef GRAVITY_CONFIG_VIEW_H
#define GRAVITY_CONFIG_VIEW_H


#include <View.h>

class Gravity;

class BListView;
class BScrollView;
class BSlider;
class BStringView;

class ConfigView : public BView
{
public:
					ConfigView(Gravity* parent, BRect rect);

	void 			AttachedToWindow();
	void 			MessageReceived(BMessage* pbmMessage);

private:
	Gravity*		fParent;

	BStringView* 	fTitleString;
	BStringView*	fAuthorString;

	BListView*		fShadeList;
	BStringView*	fShadeString;
	BScrollView*	fShadeScroll;

	BSlider* 		fCountSlider;

};


#endif
