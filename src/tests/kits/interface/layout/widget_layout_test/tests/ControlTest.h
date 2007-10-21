/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_CONTROL_TEST_H
#define WIDGET_LAYOUT_TEST_CONTROL_TEST_H


#include "Test.h"


class BButton;
class BControl;
class BFont;
class LabeledCheckBox;
class View;


class ControlTest : public Test {
public:
								ControlTest(const char* name);
	virtual						~ControlTest();

	virtual	void				SetView(BView* view);

	virtual	void				ActivateTest(View* controls);
	virtual	void				DectivateTest();

	virtual	void				MessageReceived(BMessage* message);

protected:
			enum {
				MSG_CHANGE_CONTROL_TEXT	= 'chct',
				MSG_CHANGE_CONTROL_FONT	= 'chcf'
			};

protected:
			void				UpdateControlText();
			void				UpdateControlFont();

protected:
			BControl*			fControl;
			LabeledCheckBox*	fLongTextCheckBox;
			LabeledCheckBox*	fBigFontCheckBox;
			BFont*				fDefaultFont;
			BFont*				fBigFont;
};


#endif	// WIDGET_LAYOUT_TEST_CONTROL_TEST_H
