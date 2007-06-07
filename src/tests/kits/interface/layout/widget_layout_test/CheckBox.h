/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_CHECK_BOX_H
#define WIDGET_LAYOUT_TEST_CHECK_BOX_H


#include <Invoker.h>

#include "AbstractButton.h"
#include "GroupView.h"


// CheckBox
class CheckBox : public AbstractButton {
public:
								CheckBox(BMessage* message = NULL,
									BMessenger target = BMessenger());

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();

	virtual	void				Draw(BView* container, BRect updateRect);
};


// LabeledCheckBox
class LabeledCheckBox : public GroupView {
public:
								LabeledCheckBox(const char* label,
									BMessage* message = NULL,
									BMessenger target = BMessenger());

			void				SetTarget(BMessenger messenger);

			void				SetSelected(bool selected);
			bool				IsSelected() const;

private:
			CheckBox*			fCheckBox;
};


#endif	// WIDGET_LAYOUT_TEST_CHECK_BOX_H
