/*
 * Copyright 2006-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef PROPERTY_LIST_VIEW_H
#define PROPERTY_LIST_VIEW_H

#include <List.h>
#include <View.h>

#include "MouseWheelFilter.h"
#include "Scrollable.h"

class BClipboard;
class BMenu;
class BMenuItem;
class CommandStack;
class Property;
class PropertyItemView;
class PropertyObject;
class ScrollView;
class TabFilter;

class PropertyListView : public BView,
						 public Scrollable,
						 private BList,
						 public MouseWheelTarget {
 public:
								PropertyListView();
	virtual						~PropertyListView();

	// BView interface
	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				FrameResized(float width, float height);
	virtual	void				Draw(BRect updateRect);
	virtual	void				MakeFocus(bool focus);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MessageReceived(BMessage* message);

#ifdef __HAIKU__
	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();
#endif

	// Scrollable interface
	virtual	void				ScrollOffsetChanged(BPoint oldOffset,
													BPoint newOffset);

	// MouseWheelTarget interface
	virtual	bool				MouseWheelChanged(float x, float y) { return false; }

	// PropertyListView
			void				SetTo(PropertyObject* object);
				// takes ownership of the object
	virtual	void				PropertyChanged(const Property* previous,
												const Property* current);
				// implement to know when a property changed
	virtual	void				PasteProperties(const PropertyObject* object);
				// implement to know when a property changed
	virtual	bool				IsEditingMultipleObjects();

			void				SetMenu(BMenu* menu);
			::ScrollView*		ScrollView() const;
			void				UpdateStrings();

	// interface for Property framework
			void				UpdateObject(uint32 propertyID);

			bool				TabFocus(bool shift);

			void				Select(PropertyItemView* item);
			void				DeselectAll();

			void				Clicked(PropertyItemView* item);
			void				DoubleClicked(PropertyItemView* item);

protected:
			PropertyItemView*	_ItemAt(int32 index) const;
			int32				_CountItems() const;

private:
			void				_UpdateSavedProperties();

			bool				_AddItem(PropertyItemView* item);
			PropertyItemView*	_RemoveItem(int32 index);

			void				_MakeEmpty();

			BRect				_ItemsRect() const;
			void				_LayoutItems();

			void				_CheckMenuStatus();

			BClipboard*			fClipboard;

			BMenu*				fSelectM;
			BMenuItem*			fSelectAllMI;
			BMenuItem*			fSelectNoneMI;
			BMenuItem*			fInvertSelectionMI;

			BMenu*				fPropertyM;
			BMenuItem*			fCopyMI;
			BMenuItem*			fPasteMI;

			PropertyObject*		fPropertyObject;
			PropertyObject*		fSavedProperties;

			PropertyItemView*	fLastClickedItem;

			bool				fSuspendUpdates;

			MouseWheelFilter*	fMouseWheelFilter;
			TabFilter*			fTabFilter;
};

#endif // PROPERTY_LIST_VIEW_H
