/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_BOX_TEST_H
#define WIDGET_LAYOUT_TEST_BOX_TEST_H


#include "Test.h"


class BBox;
class RadioButtonGroup;


class BoxTest : public Test {
public:
								BoxTest();
	virtual						~BoxTest();

	virtual	void				ActivateTest(View* controls);
	virtual	void				DectivateTest();

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_UpdateBorderStyle();

private:
			class BorderStyleRadioButton;

			BBox*				fBox;
			RadioButtonGroup*	fBorderStyleRadioGroup;
};


#endif	// WIDGET_LAYOUT_TEST_BOX_TEST_H
