/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "ContourTransformer.h"

#include "CommonPropertyIDs.h"
#include "OptionProperty.h"
#include "Property.h"
#include "PropertyObject.h"

// constructor
ContourTransformer::ContourTransformer(VertexSource& source)
	: Transformer(source, "Contour"),
	  Contour(source)
{
	auto_detect_orientation(true);
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

// ApproximationScale
double
ContourTransformer::ApproximationScale() const
{
	return fSource.ApproximationScale() * width();
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

	// auto detect orientation
	object->AddProperty(new BoolProperty(PROPERTY_DETECT_ORIENTATION,
										 auto_detect_orientation()));

	// join mode
	OptionProperty* property = new OptionProperty(PROPERTY_JOIN_MODE);
	property->AddOption(agg::miter_join, "Miter");
	property->AddOption(agg::round_join, "Round");
	property->AddOption(agg::bevel_join, "Bevel");
	property->SetCurrentOptionID(line_join());

	object->AddProperty(property);

	// miter limit
	object->AddProperty(new FloatProperty(PROPERTY_MITER_LIMIT,
										  miter_limit()));

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

	// auto detect orientation
	bool ado = object->Value(PROPERTY_DETECT_ORIENTATION,
							 auto_detect_orientation());
	if (ado != auto_detect_orientation()) {
		auto_detect_orientation(ado);
		Notify();
	}

	// join mode
	OptionProperty* property = dynamic_cast<OptionProperty*>(
		object->FindProperty(PROPERTY_JOIN_MODE));
	if (property && line_join() != property->CurrentOptionID()) {
		line_join((agg::line_join_e)property->CurrentOptionID());
		Notify();
	}

	// miter limit
	float l = object->Value(PROPERTY_MITER_LIMIT, (float)miter_limit());
	if (l != miter_limit()) {
		miter_limit(l);
		Notify();
	}

	return HasPendingNotifications();
}



