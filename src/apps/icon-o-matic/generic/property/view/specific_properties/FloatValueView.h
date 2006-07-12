/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef FLOAT_VALUE_VIEW_H
#define FLOAT_VALUE_VIEW_H

#include "Property.h"
#include "TextInputValueView.h"

class NummericalTextView;

class FloatValueView : public TextInputValueView {
 public:
								FloatValueView(FloatProperty* property);
	virtual						~FloatValueView();

	// TextInputValueView interface
	virtual	InputTextView*		TextView() const;

	// PropertyEditorView interface
	virtual	void				ValueChanged();

	virtual	bool				AdoptProperty(Property* property);
	virtual	Property*			GetProperty() const;

 private:
			FloatProperty*		fProperty;
			NummericalTextView*	fTextView;
};

#endif // FLOAT_VALUE_VIEW_H


