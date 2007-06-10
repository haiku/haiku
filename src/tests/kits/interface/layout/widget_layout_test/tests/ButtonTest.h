/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_BUTTON_TEST_H
#define WIDGET_LAYOUT_TEST_BUTTON_TEST_H


#include "ControlTest.h"


class BButton;


class ButtonTest : public ControlTest {
public:
								ButtonTest();
	virtual						~ButtonTest();

	static	Test*				CreateTest();
	virtual	void				ActivateTest(View* controls);
	virtual	void				DectivateTest();

private:
			BButton*			fButton;
};


#endif	// WIDGET_LAYOUT_TEST_BUTTON_TEST_H
