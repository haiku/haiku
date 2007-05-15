/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_TWO_DIMENSIONAL_SLIDER_VIEW_H
#define WIDGET_LAYOUT_TEST_TWO_DIMENSIONAL_SLIDER_VIEW_H


#include <Invoker.h>

#include "View.h"


class TwoDimensionalSliderView : public View, public BInvoker {
public:
								TwoDimensionalSliderView(
									BMessage* message = NULL,
									BMessenger target = BMessenger());

			void				SetLocationRange(BPoint minLocation,
									BPoint maxLocation);

			BPoint				MinLocation() const;
			BPoint				MaxLocation() const;

			BPoint				Value() const;
			void				SetValue(BPoint value);

	virtual	void				MouseDown(BPoint where, uint32 buttons,
									int32 modifiers);
	virtual	void				MouseUp(BPoint where, uint32 buttons,
									int32 modifiers);
	virtual	void				MouseMoved(BPoint where, uint32 buttons,
									int32 modifiers);

private:
			BPoint				fMinLocation;
			BPoint				fMaxLocation;
			bool				fDragging;
			BPoint				fOriginalPoint;
			BPoint				fOriginalLocation;
};

#endif	// WIDGET_LAYOUT_TEST_TWO_DIMENSIONAL_SLIDER_VIEW_H
