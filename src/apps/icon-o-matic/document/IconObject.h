/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef ICON_OBJECT_H
#define ICON_OBJECT_H

#include <String.h>

#include "Observable.h"
#include "Referenceable.h"
#include "Selectable.h"

class BMessage;
class PropertyObject;

class IconObject : public Observable,
				   public Referenceable,
				   public Selectable {
 public:
								IconObject(const char* name);
								IconObject(const IconObject& other);
								IconObject(BMessage* archive);
	virtual						~IconObject();

	// Selectable interface
	virtual	void				SelectedChanged();

	// IconObject
	virtual	status_t			Unarchive(const BMessage* archive);
	virtual status_t			Archive(BMessage* into,
										bool deep = true) const;

	virtual	PropertyObject*		MakePropertyObject() const;
	virtual	bool				SetToPropertyObject(
									const PropertyObject* object);

			void				SetName(const char* name);
			const char*			Name() const
									{ return fName.String(); }

 private:
			BString				fName;
};

#endif // ICON_OBJECT_H
