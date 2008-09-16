/*
 * Copyright 2008, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_RADIO_BUTTON_TEST_H
#define WIDGET_LAYOUT_TEST_RADIO_BUTTON_TEST_H


#include "ControlTest.h"


class BRadioButton;


class RadioButtonTest : public ControlTest {
public:
								RadioButtonTest();
	virtual						~RadioButtonTest();

	static	Test*				CreateTest();

	virtual	void				ActivateTest(View* controls);
	virtual	void				DectivateTest();

private:
			BRadioButton*		fRadioButton;
};


#endif	// WIDGET_LAYOUT_TEST_RADIO_BUTTON_TEST_H
