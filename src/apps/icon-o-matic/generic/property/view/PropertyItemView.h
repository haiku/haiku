/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef PROPERTY_ITEM_VIEW_H
#define PROPERTY_ITEM_VIEW_H

#include <View.h>

class Property;
class PropertyEditorView;
class PropertyListView;

class PropertyItemView : public BView {
 public:
								PropertyItemView(Property* property);
	virtual						~PropertyItemView();

	// BView interface
	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);
	virtual	void				MakeFocus(bool focused);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
										   const BMessage* dragMessage);

	// PropertyItemView
			float				PreferredHeight() const;
			float				PreferredLabelWidth() const;
			void				SetLabelWidth(float width);

			Property*			GetProperty() const;
			bool				AdoptProperty(Property* property);

			void				SetSelected(bool selected);
			bool				IsSelected() const
									{ return fSelected; }
			void				SetEnabled(bool enabled);
			bool				IsEnabled() const
									{ return fEnabled; }
	virtual	bool				IsFocused() const;

			void				SetListView(PropertyListView* parent);

	virtual	void				UpdateObject();

 private:
			void				_UpdateLowColor();

	PropertyListView*			fParent;
	PropertyEditorView*			fEditorView;
	bool						fSelected;
	bool						fEnabled;
	float						fLabelWidth;
};

#endif // PROPERTY_ITEM_VIEW_H


