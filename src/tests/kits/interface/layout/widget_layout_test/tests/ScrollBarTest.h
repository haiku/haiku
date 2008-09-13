/*
 * Copyright 2008, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_SCROLL_BAR_TEST_H
#define WIDGET_LAYOUT_TEST_SCROLL_BAR_TEST_H


#include "Test.h"


class BScrollBar;
class RadioButtonGroup;


class ScrollBarTest : public Test {
public:
								ScrollBarTest();
	virtual						~ScrollBarTest();

	static	Test*				CreateTest();

	virtual	void				ActivateTest(View* controls);
	virtual	void				DectivateTest();

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_UpdateOrientation();

private:
			class OrientationRadioButton;

			BScrollBar*			fScrollBar;
			RadioButtonGroup*	fOrientationRadioGroup;
};


#endif	// WIDGET_LAYOUT_TEST_SCROLL_BAR_TEST_H
