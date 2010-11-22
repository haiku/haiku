/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "ContourTransformer.h"

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
ContourTransformer::ContourTransformer(VertexSource& source)
	: Transformer(source, "Contour"),
	  Contour(source)
{
	auto_detect_orientation(true);
}

// constructor
ContourTransformer::ContourTransformer(VertexSource& source,
									   BMessage* archive)
	: Transformer(source, archive),
	  Contour(source)
{
	auto_detect_orientation(true);

	if (!archive)
		return;

	int32 mode;
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
}

// destructor
ContourTransformer::~ContourTransformer()
{
}

// Clone
Transformer*
ContourTransformer::Clone(VertexSource& source) const
{
	ContourTransformer* clone = new (nothrow) ContourTransformer(source);
	if (clone) {
		clone->line_join(line_join());
		clone->inner_join(inner_join());
		clone->width(width());
		clone->miter_limit(miter_limit());
		clone->inner_miter_limit(inner_miter_limit());
		clone->auto_detect_orientation(auto_detect_orientation());
	}
	return clone;
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
ContourTransformer::Archive(BMessage* into, bool deep) const
{
	status_t ret = Transformer::Archive(into, deep);

	if (ret == B_OK)
		into->what = archive_code;

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

	return ret;
}

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

#endif // ICON_O_MATIC



