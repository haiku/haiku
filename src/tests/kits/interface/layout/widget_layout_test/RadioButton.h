/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_RADIO_BUTTON_H
#define WIDGET_LAYOUT_TEST_RADIO_BUTTON_H


#include <Invoker.h>
#include <List.h>

#include "AbstractButton.h"
#include "GroupView.h"


// RadioButton
class RadioButton : public AbstractButton {
public:
								RadioButton(BMessage* message = NULL,
									BMessenger target = BMessenger());

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();

	virtual	void				Draw(BView* container, BRect updateRect);
};


// LabeledRadioButton
class LabeledRadioButton : public GroupView {
public:
								LabeledRadioButton(const char* label,
									BMessage* message = NULL,
									BMessenger target = BMessenger());

			RadioButton*		GetRadioButton() const	{ return fRadioButton; }

			void				SetTarget(BMessenger messenger);

			void				SetSelected(bool selected);
			bool				IsSelected() const;

private:
			RadioButton*		fRadioButton;
};


// RadioButtonGroup
class RadioButtonGroup : public BInvoker, private AbstractButton::Listener {
public:
								RadioButtonGroup(BMessage* message = NULL,
									BMessenger target = BMessenger());
	virtual						~RadioButtonGroup();

			void				AddButton(AbstractButton* button);
			bool				RemoveButton(AbstractButton* button);
			AbstractButton*		RemoveButton(int32 index);

			int32				CountButtons() const;
			AbstractButton*		ButtonAt(int32 index) const;
			int32				IndexOfButton(AbstractButton* button) const;

			void				SelectButton(AbstractButton* button);
			void				SelectButton(int32 index);
			AbstractButton*		SelectedButton() const;
			int32				SelectedIndex() const;

private:
	virtual	void				SelectionChanged(AbstractButton* button);

			void				_SelectionChanged();

private:
			BList				fButtons;
			AbstractButton*		fSelected;
};


#endif	// WIDGET_LAYOUT_TEST_RADIO_BUTTON_H
