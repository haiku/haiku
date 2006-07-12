/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "StrokeTransformer.h"

#include <new>

#include "CommonPropertyIDs.h"
#include "OptionProperty.h"
#include "Property.h"
#include "PropertyObject.h"

using std::nothrow;

// constructor
StrokeTransformer::StrokeTransformer(VertexSource& source)
	: Transformer(source, "Stroke"),
	  Stroke(source)
{
}

// destructor
StrokeTransformer::~StrokeTransformer()
{
}

// Clone
Transformer*
StrokeTransformer::Clone(VertexSource& source) const
{
	StrokeTransformer* clone = new (nothrow) StrokeTransformer(source);
	if (clone) {
		clone->line_cap(line_cap());
		clone->line_join(line_join());
		clone->inner_join(inner_join());
		clone->width(width());
		clone->miter_limit(miter_limit());
		clone->inner_miter_limit(inner_miter_limit());
		clone->shorten(shorten());
	}
	return clone;
}

// rewind
void
StrokeTransformer::rewind(unsigned path_id)
{
	Stroke::rewind(path_id);
}

// vertex
unsigned
StrokeTransformer::vertex(double* x, double* y)
{
	return Stroke::vertex(x, y);
}

// SetSource
void
StrokeTransformer::SetSource(VertexSource& source)
{
	Transformer::SetSource(source);
	Stroke::attach(source);
}

// WantsOpenPaths
bool
StrokeTransformer::WantsOpenPaths() const
{
	return true;
}

// ApproximationScale
double
StrokeTransformer::ApproximationScale() const
{
	return fSource.ApproximationScale() * width();
}

// #pragma mark -

// MakePropertyObject
PropertyObject*
StrokeTransformer::MakePropertyObject() const
{
	PropertyObject* object = Transformer::MakePropertyObject();
	if (!object)
		return NULL;

	// width
	object->AddProperty(new FloatProperty(PROPERTY_WIDTH, width()));

	// cap mode
	OptionProperty* property = new OptionProperty(PROPERTY_CAP_MODE);
	property->AddOption(agg::butt_cap, "Butt");
	property->AddOption(agg::square_cap, "Square");
	property->AddOption(agg::round_cap, "Round");
	property->SetCurrentOptionID(line_cap());

	object->AddProperty(property);

	// join mode
	property = new OptionProperty(PROPERTY_JOIN_MODE);
	property->AddOption(agg::miter_join, "Miter");
	property->AddOption(agg::round_join, "Round");
	property->AddOption(agg::bevel_join, "Bevel");
	property->SetCurrentOptionID(line_join());

	object->AddProperty(property);

	// miter limit
	if (line_join() == agg::miter_join) {
		object->AddProperty(new FloatProperty(PROPERTY_MITER_LIMIT,
											  miter_limit()));
	}

	// shorten
	object->AddProperty(new FloatProperty(PROPERTY_STROKE_SHORTEN,
										  shorten()));

	return object;
}

// SetToPropertyObject
bool
StrokeTransformer::SetToPropertyObject(const PropertyObject* object)
{
	AutoNotificationSuspender _(this);
	Transformer::SetToPropertyObject(object);

	// width
	float w = object->Value(PROPERTY_WIDTH, (float)width());
	if (w != width()) {
		width(w);
		Notify();
	}

	// cap mode
	OptionProperty* property = dynamic_cast<OptionProperty*>(
			object->FindProperty(PROPERTY_CAP_MODE));
	if (property && line_cap() != property->CurrentOptionID()) {
		line_cap((agg::line_cap_e)property->CurrentOptionID());
		Notify();
	}
	// join mode
	property = dynamic_cast<OptionProperty*>(
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

	// shorten
	float s = object->Value(PROPERTY_STROKE_SHORTEN, (float)shorten());
	if (s != shorten()) {
		shorten(s);
		Notify();
	}

	return HasPendingNotifications();
}


