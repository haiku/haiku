/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef STRING_VALUE_VIEW_H
#define STRING_VALUE_VIEW_H

#include "OptionProperty.h"
#include "Property.h"
#include "TextInputValueView.h"

class StringTextView;

class StringValueView : public TextInputValueView {
 public:
								StringValueView(StringProperty* property);
	virtual						~StringValueView();

	// TextInputValueView interface
	virtual	InputTextView*		TextView() const;

	// PropertyEditorView interface
	virtual	void				ValueChanged();

	virtual	bool				AdoptProperty(Property* property);
	virtual	Property*			GetProperty() const;

 private:
			StringProperty*		fProperty;
			StringTextView*		fTextView;
};

#endif // STRING_VALUE_VIEW_H


