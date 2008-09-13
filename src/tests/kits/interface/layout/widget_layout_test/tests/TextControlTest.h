/*
 * Copyright 2008, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_TEXT_CONTROL_TEST_H
#define WIDGET_LAYOUT_TEST_TEXT_CONTROL_TEST_H


#include "ControlTest.h"


class BFont;
class BTextControl;
class LabeledCheckBox;


class TextControlTest : public ControlTest {
public:
								TextControlTest();
	virtual						~TextControlTest();

	static	Test*				CreateTest();

	virtual	void				ActivateTest(View* controls);
	virtual	void				DectivateTest();

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_UpdateLabelText();
			void				_UpdateLabelFont();

private:
			BTextControl*		fTextControl;
			LabeledCheckBox*	fLongTextCheckBox;
			LabeledCheckBox*	fBigFontCheckBox;
			BFont*				fDefaultFont;
			BFont*				fBigFont;
};


#endif	// WIDGET_LAYOUT_TEST_TEXT_CONTROL_TEST_H
