/*
 * Copyright 2006-2007, 2023, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef REFERENCE_IMAGE_H
#define REFERENCE_IMAGE_H

#ifdef ICON_O_MATIC

#include "IconObject.h"
#include "Observer.h"
#include "IconBuild.h"
#include "PathSource.h"
#include "Shape.h"
#include "Transformable.h"
#include "VectorPath.h"

#include <List.h>
#include <Rect.h>


_BEGIN_ICON_NAMESPACE


class Style;

class ReferenceImage : public _ICON_NAMESPACE Shape {
 public:
	enum {
		archive_code = 'shri'
	};

								ReferenceImage(BBitmap* image);
									// transfers ownership of image
								ReferenceImage(const ReferenceImage& other);
								ReferenceImage(BMessage* archive);
	virtual						~ReferenceImage();

	// IconObject interface
	virtual	status_t			Unarchive(BMessage* archive);
	virtual	status_t			Archive(BMessage* into,
										bool deep = true) const;

	virtual	PropertyObject*		MakePropertyObject() const;
	virtual	bool				SetToPropertyObject(
									const PropertyObject* object);
	// Shape
	virtual	status_t			InitCheck() const;
	virtual Shape*				Clone() const
									{ return new ReferenceImage(*this); }

	virtual void				SetImage(BBitmap* image);
									// transfers ownership of image

	virtual bool				Visible(float scale) const
									{ return true; }
 private:
			VectorPath*			fPath;
};


_END_ICON_NAMESPACE


#endif  // ICON_O_MATIC
#endif	// REFERENCE_IMAGE_H
