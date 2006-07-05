/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef PROPERTY_EDITOR_VIEW_H
#define PROPERTY_EDITOR_VIEW_H

#include <View.h>

class Property;
class PropertyItemView;

class PropertyEditorView : public BView {
 public:
								PropertyEditorView();
	virtual						~PropertyEditorView();

								// BView
	virtual	void				Draw(BRect updateRect);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
										   const BMessage* dragMessage);

								// PropertyEditorView
	virtual	float				PreferredHeight() const;

			void				SetSelected(bool selected);
			bool				IsSelected() const
									{ return fSelected; }
	virtual	void				SetEnabled(bool enabled) = 0;
	virtual	bool				IsFocused() const
									{ return IsFocus(); }

			void				SetItemView(PropertyItemView* parent);

								// used to trigger an update on the
								// represented object
	virtual	void				ValueChanged();

	virtual	bool				AdoptProperty(Property* property) = 0;
	virtual	Property*			GetProperty() const = 0;

 protected:
	PropertyItemView*			fParent;

 private:
	bool						fSelected;
};

#endif // PROPERTY_EDITOR_VIEW_H


