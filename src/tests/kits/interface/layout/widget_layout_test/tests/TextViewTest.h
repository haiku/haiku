/*
 * Copyright 2008, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_TEXT_VIEW_TEST_H
#define WIDGET_LAYOUT_TEST_TEXT_VIEW_TEST_H


#include "Test.h"


class BTextView;
class LabeledCheckBox;


class TextViewTest : public Test {
public:
								TextViewTest();

	static	Test*				CreateTest();

	virtual	void				ActivateTest(View* controls);
	virtual	void				DectivateTest();

	virtual	void				MessageReceived(BMessage* message);


private:
			void				_UpdateInsets();
			void				_UpdateText();
			void				_UpdateFont();

private:
			BTextView*			fTextView;
			LabeledCheckBox*	fUseInsetsCheckBox;
			LabeledCheckBox*	fTextCheckBox;
			LabeledCheckBox*	fFontCheckBox;
};


#endif	// WIDGET_LAYOUT_TEST_TEXT_VIEW_TEST_H
