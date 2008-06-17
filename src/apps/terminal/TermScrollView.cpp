/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

// NOTE: Nasty hack to get access to BScrollView's private parts.
#include <ScrollBar.h>
#define private	protected
#include <ScrollView.h>
#undef private

#include "TermScrollView.h"


class TermScrollBar : public BScrollBar {
public:
	TermScrollBar(BRect frame, const char *name, BView *target,
			float min, float max, orientation direction)
		:
		BScrollBar(frame, name, target, min, max, direction)
	{
	}

	virtual void ValueChanged(float newValue)
	{
		if (BView* target = Target())
			target->ScrollTo(0, newValue);
	}
};


TermScrollView::TermScrollView(const char* name, BView* child, BView* target,
		uint32 resizingMode)
	:
	BScrollView(name, child, resizingMode, 0, false, true, B_NO_BORDER)
{
	// replace the vertical scroll bar with our own
	if (fVerticalScrollBar != NULL) {
		BRect frame(fVerticalScrollBar->Frame());
		RemoveChild(fVerticalScrollBar);

		// Overlap one pixel at the top and the bottom of the scroll bar with
		// the menu respectively resize knob for aesthetical reasons.
		frame.top -= 1;
		frame.bottom -= B_H_SCROLL_BAR_HEIGHT - 1;

		TermScrollBar* scrollBar = new TermScrollBar(frame, "_VSB_", target, 0,
			1000, B_VERTICAL);
		AddChild(scrollBar);
		fVerticalScrollBar = scrollBar;
	}
}


TermScrollView::~TermScrollView()
{
}
