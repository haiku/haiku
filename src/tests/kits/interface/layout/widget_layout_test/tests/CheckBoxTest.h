/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_CHECK_BOX_TEST_H
#define WIDGET_LAYOUT_TEST_CHECK_BOX_TEST_H


#include "ControlTest.h"


class BCheckBox;


class CheckBoxTest : public ControlTest {
public:
								CheckBoxTest();
	virtual						~CheckBoxTest();

	static	Test*				CreateTest();

	virtual	void				ActivateTest(View* controls);
	virtual	void				DectivateTest();

private:
			BCheckBox*			fCheckBox;
};


#endif	// WIDGET_LAYOUT_TEST_CHECK_BOX_TEST_H
