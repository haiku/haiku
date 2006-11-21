/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef ICON_OBJECT_LIST_VIEW_H
#define ICON_OBJECT_LIST_VIEW_H

#include "Observer.h"
#include "PropertyListView.h"

class CommandStack;
class IconObject;
class Selection;

class IconObjectListView : public PropertyListView,
						   public Observer {
 public:
								IconObjectListView();
	virtual						~IconObjectListView();

	// PropertyListView interface
	virtual	void				Draw(BRect updateRect);

	virtual	void				PropertyChanged(const Property* previous,
												const Property* current);
	virtual	void				PasteProperties(const PropertyObject* object);
	virtual	bool				IsEditingMultipleObjects();

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// IconObjectListView
			void				SetSelection(Selection* selection);
			void				SetCommandStack(CommandStack* stack);

 private:
			void				_SetObject(IconObject* object);

			Selection*			fSelection;
			CommandStack*		fCommandStack;

			IconObject*			fObject;
			bool				fIgnoreObjectChange;
};

#endif // ICON_OBJECT_LIST_VIEW_H
