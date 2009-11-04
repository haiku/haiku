/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TERM_SCROLL_VIEW_H
#define TERM_SCROLL_VIEW_H

#include <ScrollView.h>


class TermScrollView : public BScrollView {
public:
								TermScrollView(const char* name, BView* child,
									BView* target, bool overlapTop,
									uint32 resizingMode = B_FOLLOW_ALL);
	virtual						~TermScrollView();
};


#endif	// TERM_SCROLL_VIEW_H
