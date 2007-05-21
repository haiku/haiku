/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_BUTTON_TEST_H
#define WIDGET_LAYOUT_TEST_BUTTON_TEST_H


#include "Test.h"


class BButton;
class BFont;
class LabeledCheckBox;
class View;


class ButtonTest : public Test {
public:
								ButtonTest();
	virtual						~ButtonTest();

	virtual	void				ActivateTest(View* controls);
	virtual	void				DectivateTest();

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_SetButtonText(bool longText);
			void				_SetButtonFont(bool bigFont);

private:
			BButton*			fButton;
			LabeledCheckBox*	fLongTextCheckBox;
			LabeledCheckBox*	fBigFontCheckBox;
			BFont*				fDefaultFont;
			BFont*				fBigFont;
};


#endif	// WIDGET_LAYOUT_TEST_BUTTON_TEST_H
