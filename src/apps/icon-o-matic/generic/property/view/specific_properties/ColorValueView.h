/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef COLOR_VALUE_VIEW_H
#define COLOR_VALUE_VIEW_H

#include "PropertyEditorView.h"

class ColorProperty;
class SwatchValueView;

class ColorValueView : public PropertyEditorView {
 public:
								ColorValueView(ColorProperty* property);
	virtual						~ColorValueView();

	// BView interface
	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);

	virtual	void				MakeFocus(bool focused);

	virtual	void				MessageReceived(BMessage* message);

	// PropertyEditorView interface
	virtual	void				SetEnabled(bool enabled);
	virtual	bool				IsFocused() const;

	virtual	bool				AdoptProperty(Property* property);

 protected:
			ColorProperty*		fProperty;

			SwatchValueView*	fSwatchView;
};

#endif // COLOR_VALUE_VIEW_H


