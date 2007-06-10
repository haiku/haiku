/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ListViewTest.h"

#include <ListView.h>
#include <StringItem.h>


ListViewTest::ListViewTest()
	: Test("ListView", NULL),
	  fListView(new BListView(BRect(0, 0, -1, -1), NULL))
{
	SetView(fListView);

	// add a view items
	for (int32 i = 0; i < 15; i++) {
		BString itemText("list item ");
		itemText << i;
		fListView->AddItem(new BStringItem(itemText.String()));
	}
}
