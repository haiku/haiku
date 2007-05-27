/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_BOX_TEST_H
#define WIDGET_LAYOUT_TEST_BOX_TEST_H


#include "Test.h"


class BBox;
class View;


class BoxTest : public Test {
public:
								BoxTest();
	virtual						~BoxTest();

	virtual	void				ActivateTest(View* controls);
	virtual	void				DectivateTest();

private:
			BBox*				fBox;
};


#endif	// WIDGET_LAYOUT_TEST_BOX_TEST_H
