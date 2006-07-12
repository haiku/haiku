/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef OPTION_VALUE_VIEW_H
#define OPTION_VALUE_VIEW_H

#include <String.h>

#include "OptionProperty.h"
#include "PropertyEditorView.h"

class OptionValueView : public PropertyEditorView {
 public:
								OptionValueView(OptionProperty* property);
	virtual						~OptionValueView();

	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);

	virtual	void				MakeFocus(bool focused);

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				MouseDown(BPoint where);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);

	virtual	void				SetEnabled(bool enabled);

	virtual	void				ValueChanged();

	virtual	bool				AdoptProperty(Property* property);
	virtual	Property*			GetProperty() const;

 private:
			OptionProperty*		fProperty;

			BString				fCurrentOption;
			bool				fEnabled;
};

#endif // OPTION_VALUE_VIEW_H


