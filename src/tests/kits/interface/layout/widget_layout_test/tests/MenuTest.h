/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_MENU_TEST_H
#define WIDGET_LAYOUT_TEST_MENU_TEST_H


#include "Test.h"


class BMenu;


class MenuTest : public Test {
public:
								MenuTest();

	static	Test*				CreateTest();

private:
			BMenu*				fMenu;
};


#endif	// WIDGET_LAYOUT_TEST_LIST_VIEW_TEST_H
