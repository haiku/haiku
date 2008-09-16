/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_MENU_FIELD_TEST_H
#define WIDGET_LAYOUT_TEST_MENU_FIELD_TEST_H


#include "ControlTest.h"


class BFont;
class BMenuField;
class LabeledCheckBox;


class MenuFieldTest : public ControlTest {
public:
								MenuFieldTest();
	virtual						~MenuFieldTest();

	static	Test*				CreateTest();

	virtual	void				ActivateTest(View* controls);
	virtual	void				DectivateTest();

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_UpdateLabelText();
			void				_UpdateMenuText();
			void				_UpdateLabelFont();

private:
			BMenuField*			fMenuField;
			LabeledCheckBox*	fLongLabelTextCheckBox;
			LabeledCheckBox*	fLongMenuTextCheckBox;
			LabeledCheckBox*	fBigFontCheckBox;
			BFont*				fDefaultFont;
			BFont*				fBigFont;
};


#endif	// WIDGET_LAYOUT_TEST_MENU_FIELD_TEST_H
