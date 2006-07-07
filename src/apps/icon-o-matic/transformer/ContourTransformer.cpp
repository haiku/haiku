/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "ContourTransformer.h"

#include "CommonPropertyIDs.h"
#include "Property.h"
#include "PropertyObject.h"

// constructor
ContourTransformer::ContourTransformer(VertexSource& source)
	: Transformer(source, "Contour"),
	  Contour(source)
{
}

// destructor
ContourTransformer::~ContourTransformer()
{
}

// rewind
void
ContourTransformer::rewind(unsigned path_id)
{
	Contour::rewind(path_id);
}

// vertex
unsigned
ContourTransformer::vertex(double* x, double* y)
{
	return Contour::vertex(x, y);
}

// SetSource
void
ContourTransformer::SetSource(VertexSource& source)
{
	Transformer::SetSource(source);
	Contour::attach(source);
}

// #pragma mark -

// MakePropertyObject
PropertyObject*
ContourTransformer::MakePropertyObject() const
{
	PropertyObject* object = Transformer::MakePropertyObject();
	if (!object)
		return NULL;

	// width
	object->AddProperty(new FloatProperty(PROPERTY_WIDTH, width()));

	return object;
}

// SetToPropertyObject
bool
ContourTransformer::SetToPropertyObject(const PropertyObject* object)
{
	AutoNotificationSuspender _(this);
	Transformer::SetToPropertyObject(object);

	// width
	float w = object->Value(PROPERTY_WIDTH, (float)width());
	if (w != width()) {
		width(w);
		Notify();
	}

	return HasPendingNotifications();
}



