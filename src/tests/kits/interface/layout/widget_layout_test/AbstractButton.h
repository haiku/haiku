/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_ABSTRACT_BUTTON_H
#define WIDGET_LAYOUT_TEST_ABSTRACT_BUTTON_H


#include <Invoker.h>
#include <List.h>

#include "View.h"


// button behavior policy
enum button_policy {
	BUTTON_POLICY_TOGGLE_ON_RELEASE,	// check box
	BUTTON_POLICY_SELECT_ON_RELEASE,	// radio button
	BUTTON_POLICY_INVOKE_ON_RELEASE		// button
};


// AbstractButton
class AbstractButton : public View, public BInvoker {
public:
								AbstractButton(button_policy policy,
									BMessage* message = NULL,
									BMessenger target = BMessenger());

			void				SetPolicy(button_policy policy);
			button_policy		Policy() const;

			void				SetSelected(bool selected);
			bool				IsSelected() const;

	virtual	void				MouseDown(BPoint where, uint32 buttons,
									int32 modifiers);
	virtual	void				MouseUp(BPoint where, uint32 buttons,
									int32 modifiers);
	virtual	void				MouseMoved(BPoint where, uint32 buttons,
									int32 modifiers);

public:
			class Listener;

			void				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

protected:
			bool				IsPressed() const;

private:
			void				_PressedUpdate(BPoint where);
			void				_NotifyListeners();

private:
			button_policy		fPolicy;
			bool				fSelected;
			bool				fPressed;
			bool				fPressedInBounds;
			BList				fListeners;
};


// synchronous listener interface
class AbstractButton::Listener {
public:
	virtual						~Listener();

	virtual	void				SelectionChanged(AbstractButton* button) = 0;
};


#endif	// WIDGET_LAYOUT_TEST_ABSTRACT_BUTTON_H
