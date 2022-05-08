/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2019-2022, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef GENERAL_CONTENT_SCROLL_VIEW_H
#define GENERAL_CONTENT_SCROLL_VIEW_H

#include <ScrollView.h>


/*!	Layouts the scrollbar so it looks nice with no border and the
	document window look.
*/

class GeneralContentScrollView : public BScrollView {
public:
	GeneralContentScrollView(const char* name, BView* target);

	virtual void DoLayout();
};

#endif // GENERAL_CONTENT_SCROLL_VIEW_H
