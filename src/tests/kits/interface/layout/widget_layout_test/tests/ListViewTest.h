/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_LIST_VIEW_TEST_H
#define WIDGET_LAYOUT_TEST_LIST_VIEW_TEST_H


#include "Test.h"


class BListView;


class ListViewTest : public Test {
public:
								ListViewTest();

	static	Test*				CreateTest();

private:
			BListView*			fListView;
};


#endif	// WIDGET_LAYOUT_TEST_LIST_VIEW_TEST_H
