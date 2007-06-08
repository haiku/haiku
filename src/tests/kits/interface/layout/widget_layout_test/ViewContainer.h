/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_VIEW_CONTAINER_H
#define WIDGET_LAYOUT_TEST_VIEW_CONTAINER_H


#include <View.h>

#include "View.h"


class ViewContainer : public BView, public View {
public:
								ViewContainer(BRect frame);

	// BView hooks

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				AllAttached();

	virtual	void				Draw(BRect updateRect);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 code,
									const BMessage* message);

	virtual	void				InvalidateLayout(bool descendants);

	// View hooks

	virtual	void				InvalidateLayout();

	virtual	void				Draw(BView* container, BRect updateRect);

	virtual	void				MouseDown(BPoint where, uint32 buttons,
									int32 modifiers);
	virtual	void				MouseUp(BPoint where, uint32 buttons,
									int32 modifiers);
	virtual	void				MouseMoved(BPoint where, uint32 buttons,
									int32 modifiers);

private:
			void				_GetButtonsAndModifiers(uint32& buttons,
									int32& modifiers);

private:
			View*				fMouseFocus;
};

#endif	// WIDGET_LAYOUT_TEST_VIEW_CONTAINER_H
