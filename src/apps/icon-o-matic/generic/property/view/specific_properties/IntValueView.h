/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef INT_VALUE_VIEW_H
#define INT_VALUE_VIEW_H

#include "Property.h"
#include "TextInputValueView.h"

class NummericalTextView;

class IntValueView : public TextInputValueView {
 public:
								IntValueView(IntProperty* property);
	virtual						~IntValueView();

	// TextInputValueView interface
	virtual	InputTextView*		TextView() const;

	// PropertyEditorView interface
	virtual	void				ValueChanged();

	virtual	bool				AdoptProperty(Property* property);
	virtual	Property*			GetProperty() const;

 private:
			IntProperty*		fProperty;
			NummericalTextView*	fTextView;
};

#endif // INT_VALUE_VIEW_H


