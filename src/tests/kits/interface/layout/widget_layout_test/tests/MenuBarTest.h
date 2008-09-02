/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_MENU_BAR_TEST_H
#define WIDGET_LAYOUT_TEST_MENU_BAR_TEST_H


#include "Test.h"


class BMenu;
class BMenuBar;
class BMenuItem;
class LabeledCheckBox;


class MenuBarTest : public Test {
public:
								MenuBarTest();

	static	Test*				CreateTest();

	virtual	void				ActivateTest(View* controls);
	virtual	void				DectivateTest();

	virtual	void				MessageReceived(BMessage* message);

private:
			void				UpdateThirdItem();
			void				UpdateChildMenu();
			void				UpdateLongText();

private:
			BMenuBar*			fMenuBar;
			BMenuItem*			fFirstItem;
			BMenuItem*			fThirdItem;
			BMenu*				fChildMenu;
			LabeledCheckBox*	fThirdItemCheckBox;
			LabeledCheckBox*	fChildMenuCheckBox;
			LabeledCheckBox*	fLongTextCheckBox;
};


#endif	// WIDGET_LAYOUT_TEST_MENU_BAR_TEST_H
