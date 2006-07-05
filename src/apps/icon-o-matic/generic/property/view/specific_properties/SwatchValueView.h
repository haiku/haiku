/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef SWATCH_VALUE_VIEW_H
#define SWATCH_VALUE_VIEW_H

#include "SwatchView.h"

class SwatchValueView : public SwatchView {
 public:
								SwatchValueView(const char* name,
												BMessage* message,
												BHandler* target,
												rgb_color color,
												float width = 24.0,
												float height = 24.0);
	virtual						~SwatchValueView();

								// BView
	virtual	void				MakeFocus(bool focused);

	virtual	void				Draw(BRect updateRect);

	virtual	void				MouseDown(BPoint where);

};

#endif // SWATCH_VALUE_VIEW_H


