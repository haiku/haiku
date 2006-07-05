/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef ICON_OBJECT_H
#define ICON_OBJECT_H

#include "Observable.h"
#include "Referenceable.h"
#include "Selectable.h"

class PropertyObject;

class IconObject : public Observable,
				   public Referenceable,
				   public Selectable {
 public:
								IconObject();
	virtual						~IconObject();

	// Selectable interface
	virtual	void				SelectedChanged();

	// IconObject
	virtual	PropertyObject*		MakePropertyObject() const;
	virtual	bool				SetToPropertyObject(
									const PropertyObject* object);

 private:
};

#endif // ICON_OBJECT_H
