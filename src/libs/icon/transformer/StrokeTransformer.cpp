/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "StrokeTransformer.h"

#ifdef ICON_O_MATIC
# include <Message.h>

# include "CommonPropertyIDs.h"
# include "OptionProperty.h"
# include "Property.h"
# include "PropertyObject.h"
#endif // ICON_O_MATIC

#include <new>


_USING_ICON_NAMESPACE
using std::nothrow;


// constructor
StrokeTransformer::StrokeTransformer(VertexSource& source)
	: Transformer(source, "Stroke"),
	  Stroke(source)
{
}

// constructor
StrokeTransformer::StrokeTransformer(VertexSource& source,
									 BMessage* archive)
	: Transformer(source, archive),
	  Stroke(source)
{
	if (!archive)
		return;

	int32 mode;
	if (archive->FindInt32("line cap", &mode) == B_OK)
		line_cap((agg::line_cap_e)mode);

	if (archive->FindInt32("line join", &mode) == B_OK)
		line_join((agg::line_join_e)mode);

	if (archive->FindInt32("inner join", &mode) == B_OK)
		inner_join((agg::inner_join_e)mode);

	double value;
	if (archive->FindDouble("width", &value) == B_OK)
		width(value);

	if (archive->FindDouble("miter limit", &value) == B_OK)
		miter_limit(value);

	if (archive->FindDouble("inner miter limit", &value) == B_OK)
		inner_miter_limit(value);

	if (archive->FindDouble("shorten", &value) == B_OK)
		shorten(value);
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
	double scale = fSource.ApproximationScale();
	double factor = fabs(width());
	if (factor > 1.0)
		scale *= factor;
	return scale;
}

// #pragma mark -

#ifdef ICON_O_MATIC

// Archive
status_t
StrokeTransformer::Archive(BMessage* into, bool deep) const
{
	status_t ret = Transformer::Archive(into, deep);

	if (ret == B_OK)
		into->what = archive_code;

	if (ret == B_OK)
		ret = into->AddInt32("line cap", line_cap());

	if (ret == B_OK)
		ret = into->AddInt32("line join", line_join());

	if (ret == B_OK)
		ret = into->AddInt32("inner join", inner_join());

	if (ret == B_OK)
		ret = into->AddDouble("width", width());

	if (ret == B_OK)
		ret = into->AddDouble("miter limit", miter_limit());

	if (ret == B_OK)
		ret = into->AddDouble("inner miter limit", inner_miter_limit());

	if (ret == B_OK)
		ret = into->AddDouble("shorten",shorten());

	return ret;
}

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

//	// shorten
//	object->AddProperty(new FloatProperty(PROPERTY_STROKE_SHORTEN,
//										  shorten()));

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

#endif // ICON_O_MATIC


