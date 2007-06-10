/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_BOX_TEST_H
#define WIDGET_LAYOUT_TEST_BOX_TEST_H


#include "Test.h"


class BBox;
class LabeledCheckBox;
class RadioButtonGroup;


class BoxTest : public Test {
public:
								BoxTest();
	virtual						~BoxTest();

	static	Test*				CreateTest();

	virtual	void				ActivateTest(View* controls);
	virtual	void				DectivateTest();

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_UpdateBorderStyle();
			void				_UpdateLabel();
			void				_UpdateLongLabel();
			void				_UpdateChild();

private:
			class BorderStyleRadioButton;
			class LabelRadioButton;

			BBox*				fBox;
			BView*				fChild;
			RadioButtonGroup*	fBorderStyleRadioGroup;
			RadioButtonGroup*	fLabelRadioGroup;
			LabeledCheckBox*	fLongLabelCheckBox;
			LabeledCheckBox*	fChildCheckBox;
};


#endif	// WIDGET_LAYOUT_TEST_BOX_TEST_H
