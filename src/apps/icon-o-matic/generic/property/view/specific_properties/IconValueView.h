/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef ICON_VALUE_VIEW_H
#define ICON_VALUE_VIEW_H

#include "IconProperty.h"
#include "PropertyEditorView.h"

class NummericalTextView;

class IconValueView : public PropertyEditorView {
 public:
								IconValueView(IconProperty* property);
	virtual						~IconValueView();

	// BView interface
	virtual	void				Draw(BRect updateRect);

	// PropertyEditorView interface
	virtual	void				SetEnabled(bool enabled);

	virtual	bool				AdoptProperty(Property* property);
	virtual	Property*			GetProperty() const;

	// IconValueView
			status_t			SetIcon(const unsigned char* bitsFromQuickRes,
										uint32 width, uint32 height,
										color_space format);

 protected:
			IconProperty*		fProperty;
			BBitmap*			fIcon;
};

#endif // ICON_VALUE_VIEW_H


