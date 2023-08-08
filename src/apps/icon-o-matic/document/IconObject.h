/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef ICON_OBJECT_H
#define ICON_OBJECT_H

#include <Referenceable.h>
#include <String.h>

#include "Observable.h"
#include "Selectable.h"

class BMessage;
class PropertyObject;

class IconObject : public Observable,
				   public BReferenceable,
				   public Selectable {
 public:
								IconObject(const char* name);
								IconObject(const IconObject& other);
								IconObject(BMessage* archive);
	virtual						~IconObject();

	// Selectable interface
	virtual	void				SelectedChanged();

	// IconObject
	virtual	status_t			Unarchive(BMessage* archive);
	virtual status_t			Archive(BMessage* into,
										bool deep = true) const;

	virtual	PropertyObject*		MakePropertyObject() const;
	virtual	bool				SetToPropertyObject(
									const PropertyObject* object);

			void				SetName(const char* name);
			const char*			Name() const
									{ return fName.String(); }

	// TODO: let IconObject control its own manipulators?
	// This would allow VectorPaths to control their own PathManipulator,
	// Styles to control their own TransformGradientBox, etc.
 private:
			BString				fName;
};

#endif // ICON_OBJECT_H
