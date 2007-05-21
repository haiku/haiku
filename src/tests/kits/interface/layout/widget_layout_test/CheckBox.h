/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_CHECK_BOX_H
#define WIDGET_LAYOUT_TEST_CHECK_BOX_H


#include <Invoker.h>

#include "GroupView.h"


// CheckBox
class CheckBox : public View, public BInvoker {
public:
								CheckBox(BMessage* message = NULL,
									BMessenger target = BMessenger());

			void				SetSelected(bool selected);
			bool				IsSelected() const;

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();

	virtual	void				Draw(BView* container, BRect updateRect);

	virtual	void				MouseDown(BPoint where, uint32 buttons,
									int32 modifiers);
	virtual	void				MouseUp(BPoint where, uint32 buttons,
									int32 modifiers);
	virtual	void				MouseMoved(BPoint where, uint32 buttons,
									int32 modifiers);

private:
			void				_PressedUpdate(BPoint where);

private:
			bool				fSelected;
			bool				fPressed;
			bool				fPressedSelected;
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
