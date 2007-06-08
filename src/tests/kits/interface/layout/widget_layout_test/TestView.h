/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_TEST_VIEW_H
#define WIDGET_LAYOUT_TEST_TEST_VIEW_H


#include <View.h>


class TestView : public BView {
public:
								TestView(BSize minSize, BSize maxSize,
									BSize preferredSize);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

	virtual	void				Draw(BRect updateRect);

private:
			BSize				fMinSize;
			BSize				fMaxSize;
			BSize				fPreferredSize;
};


#endif	// WIDGET_LAYOUT_TEST_TEST_VIEW_H
