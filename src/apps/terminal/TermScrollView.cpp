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

#include <Message.h>


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
		bool overlapTop, uint32 resizingMode)
	:
	BScrollView(name, child, resizingMode, 0, false, true, B_NO_BORDER)
{
	child->TargetedByScrollView(NULL);

	// replace the vertical scroll bar with our own
	if (fVerticalScrollBar != NULL) {
		BRect frame(fVerticalScrollBar->Frame());
		RemoveChild(fVerticalScrollBar);
		delete fVerticalScrollBar;

		// Overlap one pixel at the top (if required) with the menu for
		// aesthetical reasons.
		if (overlapTop)
			frame.top--;

		TermScrollBar* scrollBar = new TermScrollBar(frame, "_VSB_", target, 0,
			1000, B_VERTICAL);
		AddChild(scrollBar);
		fVerticalScrollBar = scrollBar;
	}

	target->TargetedByScrollView(this);
}


TermScrollView::~TermScrollView()
{
}
