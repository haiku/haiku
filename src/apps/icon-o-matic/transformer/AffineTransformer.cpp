/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "AffineTransformer.h"

#include "CommonPropertyIDs.h"
#include "Property.h"
#include "PropertyObject.h"

// constructor
AffineTransformer::AffineTransformer(VertexSource& source)
	: Transformer(source, "Transformation"),
	  Affine(source, *this)
{
}

// destructor
AffineTransformer::~AffineTransformer()
{
}

// rewind
void
AffineTransformer::rewind(unsigned path_id)
{
	Affine::rewind(path_id);
}

// vertex
unsigned
AffineTransformer::vertex(double* x, double* y)
{
	return Affine::vertex(x, y);
}

// SetSource
void
AffineTransformer::SetSource(VertexSource& source)
{
	Transformer::SetSource(source);
	Affine::attach(source);
}

// ApproximationScale
double
AffineTransformer::ApproximationScale() const
{
	return fSource.ApproximationScale() * scale();
}

// #pragma mark -

// MakePropertyObject
PropertyObject*
AffineTransformer::MakePropertyObject() const
{
	PropertyObject* object = Transformer::MakePropertyObject();
	if (!object)
		return NULL;

	// translation
	double tx;
	double ty;
	translation(&tx, &ty);
	object->AddProperty(new FloatProperty(PROPERTY_TRANSLATION_X, tx));
	object->AddProperty(new FloatProperty(PROPERTY_TRANSLATION_Y, ty));

	// rotation
	object->AddProperty(new FloatProperty(PROPERTY_ROTATION,
										  agg::rad2deg(rotation())));

	// scale
	double scaleX;
	double scaleY;
	scaling(&scaleX, &scaleY);
	object->AddProperty(new FloatProperty(PROPERTY_SCALE_X, scaleX));
	object->AddProperty(new FloatProperty(PROPERTY_SCALE_Y, scaleY));

	return object;
}

// SetToPropertyObject
bool
AffineTransformer::SetToPropertyObject(const PropertyObject* object)
{
	AutoNotificationSuspender _(this);
	Transformer::SetToPropertyObject(object);

	// current affine parameters
	double tx;
	double ty;
	translation(&tx, &ty);
	double r = rotation();
	double scaleX;
	double scaleY;
	scaling(&scaleX, &scaleY);

	// properties
	double newTX = object->Value(PROPERTY_TRANSLATION_X, (float)tx);
	double newTY = object->Value(PROPERTY_TRANSLATION_Y, (float)ty);

	double newR = object->Value(PROPERTY_ROTATION,
								(float)agg::rad2deg(r));
	newR = agg::deg2rad(newR);

	double newScaleX = object->Value(PROPERTY_SCALE_X, (float)scaleX);
	double newScaleY = object->Value(PROPERTY_SCALE_Y, (float)scaleY);

	if (newTX != tx || newTY != ty
		|| newR != r
		|| newScaleX != scaleX
		|| newScaleY != scaleY) {

		reset();
		
		multiply(agg::trans_affine_scaling(newScaleX, newScaleY));
		multiply(agg::trans_affine_rotation(newR));
		multiply(agg::trans_affine_translation(newTX, newTY));

		Notify();
	}

	return HasPendingNotifications();
}


