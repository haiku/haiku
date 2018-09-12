/*
 * Copyright 2018, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 */
#include "../common.h"


#include <Application.h>
#include <String.h>
#include <Menu.h>
#include <MenuItem.h>
#include <PopUpMenu.h>


class MenuTestcase : public TestCase {
public:
	void
	SizeTest()
	{
		CPPUNIT_ASSERT_EQUAL(312, sizeof(BMenu));
		CPPUNIT_ASSERT_EQUAL(128, sizeof(BMenuItem));
	}

	void
	ConcurrencyAbuseTest()
	{
		BApplication app("application/x-vnd.Haiku-interfacekit-menutest");
		BPopUpMenu* menu = new BPopUpMenu("Test");
		menu->AddItem(new BMenuItem("One", NULL));
		menu->AddItem(new BMenuItem("Two", NULL));
		menu->AddSeparatorItem();

		BMenuItem* items[10];
		for (int i = 0; i < 10; i++) {
			BString str;
			str.SetToFormat("%d", i);
			items[i] = new BMenuItem(str.String(), NULL);
		}

		// Now for the actual abuse.
		menu->Go(BPoint(), false, true, true);
		snooze(50 * 1000 /* 50 ms */);
		for (int i = 0; i < 100; i++) {
			for (int j = 0; j < (i % 5); j++) {
				BMenuItem* item = items[(i + j) % 10];
				if (item->Menu() != NULL)
					continue;
				menu->AddItem(item);
			}
			if ((i % 3) == 0) {
				for (int j = 0; j < (i % 5); j++)
					menu->RemoveItem((int32)0);
			}
		}

		CPPUNIT_ASSERT_EQUAL(6, menu->CountItems());

		// Cleanup.
		for (int i = 0; i < 10; i++)
			delete items[i];

		// Close the menu.
		char bytes[] = {B_ESCAPE};
		menu->KeyDown(bytes, 1);
		delete menu;
	}
};


Test*
MenuTestSuite()
{
	TestSuite* testSuite = new TestSuite();

	testSuite->addTest(new CppUnit::TestCaller<MenuTestcase>(
		"BMenu_Size", &MenuTestcase::SizeTest));
	testSuite->addTest(new CppUnit::TestCaller<MenuTestcase>(
		"BMenu_ConcurrencyAbuse", &MenuTestcase::ConcurrencyAbuseTest));

	return testSuite;
}
