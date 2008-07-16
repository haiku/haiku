/*
 * Copyright 2008, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_SLIDER_TEST_H
#define WIDGET_LAYOUT_TEST_SLIDER_TEST_H


#include "Test.h"


class BSlider;
class RadioButtonGroup;


class SliderTest : public Test {
public:
								SliderTest();
	virtual						~SliderTest();

	static	Test*				CreateTest();

	virtual	void				ActivateTest(View* controls);
	virtual	void				DectivateTest();

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_UpdateOrientation();
			void				_UpdateThumbStyle();
			void				_UpdateHashMarkLocation();
			void				_UpdateBarThickness();
			void				_UpdateLabel();
			void				_UpdateLimitLabels();

private:
			class OrientationRadioButton;
			class ThumbStyleRadioButton;
			class HashMarkLocationRadioButton;
			class ThicknessRadioButton;
			class LabelRadioButton;
			class LimitLabelsRadioButton;

			BSlider*			fSlider;
			RadioButtonGroup*	fOrientationRadioGroup;
			RadioButtonGroup*	fThumbStyleRadioGroup;
			RadioButtonGroup*	fHashMarkLocationRadioGroup;
			RadioButtonGroup*	fBarThicknessRadioGroup;
			RadioButtonGroup*	fLabelRadioGroup;
			RadioButtonGroup*	fLimitLabelsRadioGroup;
};


#endif	// WIDGET_LAYOUT_TEST_SLIDER_TEST_H
