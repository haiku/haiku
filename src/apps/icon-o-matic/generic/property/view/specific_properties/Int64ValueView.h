/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef INT64_VALUE_VIEW_H
#define INT64_VALUE_VIEW_H

#include "Int64Property.h"
#include "TextInputValueView.h"

class NummericalTextView;

class Int64ValueView : public TextInputValueView {
 public:
								Int64ValueView(Int64Property* property);
	virtual						~Int64ValueView();

	// TextInputValueView interface
	virtual	InputTextView*		TextView() const;

	// PropertyEditorView interface
	virtual	void				ValueChanged();

	virtual	bool				AdoptProperty(Property* property);
	virtual	Property*			GetProperty() const;

 private:
			Int64Property*		fProperty;
			NummericalTextView*	fTextView;
};

#endif // INT64_VALUE_VIEW_H


