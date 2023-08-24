/*
 * Copyright 2006, 2023, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "PathSourceShape.h"

#include <new>

#include <agg_bounding_rect.h>
#include <Message.h>

#ifdef ICON_O_MATIC
# include "CommonPropertyIDs.h"
# include "Property.h"
# include "PropertyObject.h"
#endif // ICON_O_MATIC
#include "Style.h"

using std::nothrow;

_USING_ICON_NAMESPACE


PathSourceShape::PathSourceShape(::Style* style)
	: Shape(style),

	  fMinVisibilityScale(0.0),
	  fMaxVisibilityScale(4.0)
{
}


PathSourceShape::PathSourceShape(const PathSourceShape& other)
	: Shape(other),

	  fMinVisibilityScale(other.fMinVisibilityScale),
	  fMaxVisibilityScale(other.fMaxVisibilityScale)
{
}


PathSourceShape::~PathSourceShape()
{
}


// #pragma mark -


status_t
PathSourceShape::Unarchive(BMessage* archive)
{
	Shape::Unarchive(archive);

	// min visibility scale
	if (archive->FindFloat("min visibility scale", &fMinVisibilityScale) < B_OK)
		fMinVisibilityScale = 0.0;

	// max visibility scale
	if (archive->FindFloat("max visibility scale", &fMaxVisibilityScale) < B_OK)
		fMaxVisibilityScale = 4.0;

	if (fMinVisibilityScale < 0.0)
		fMinVisibilityScale = 0.0;
	if (fMinVisibilityScale > 4.0)
		fMinVisibilityScale = 4.0;
	if (fMaxVisibilityScale < 0.0)
		fMaxVisibilityScale = 0.0;
	if (fMaxVisibilityScale > 4.0)
		fMaxVisibilityScale = 4.0;

	return B_OK;
}


#ifdef ICON_O_MATIC
status_t
PathSourceShape::Archive(BMessage* into, bool deep) const
{
	status_t ret = Shape::Archive(into, deep);

	// min visibility scale
	if (ret == B_OK)
		ret = into->AddFloat("min visibility scale", fMinVisibilityScale);

	// max visibility scale
	if (ret == B_OK)
		ret = into->AddFloat("max visibility scale", fMaxVisibilityScale);

	return ret;
}


PropertyObject*
PathSourceShape::MakePropertyObject() const
{
	PropertyObject* object = Shape::MakePropertyObject();
	if (object == NULL)
		return NULL;

//	object->AddProperty(new BoolProperty(PROPERTY_HINTING, fHinting));

	object->AddProperty(new FloatProperty(PROPERTY_MIN_VISIBILITY_SCALE,
		fMinVisibilityScale, 0, 4));

	object->AddProperty(new FloatProperty(PROPERTY_MAX_VISIBILITY_SCALE,
		fMaxVisibilityScale, 0, 4));

	return object;
}


bool
PathSourceShape::SetToPropertyObject(const PropertyObject* object)
{
	AutoNotificationSuspender _(this);
	Shape::SetToPropertyObject(object);

	// hinting
//	SetHinting(object->Value(PROPERTY_HINTING, fHinting));

	// min visibility scale
	SetMinVisibilityScale(object->Value(PROPERTY_MIN_VISIBILITY_SCALE, fMinVisibilityScale));

	// max visibility scale
	SetMaxVisibilityScale(object->Value(PROPERTY_MAX_VISIBILITY_SCALE, fMaxVisibilityScale));

	return HasPendingNotifications();
}
#endif // ICON_O_MATIC


// #pragma mark -


status_t
PathSourceShape::InitCheck() const
{
	return Shape::InitCheck();
}


// #pragma mark -


void
PathSourceShape::SetMinVisibilityScale(float scale)
{
	if (fMinVisibilityScale == scale)
		return;

	fMinVisibilityScale = scale;
	Notify();
}


void
PathSourceShape::SetMaxVisibilityScale(float scale)
{
	if (fMaxVisibilityScale == scale)
		return;

	fMaxVisibilityScale = scale;
	Notify();
}


bool
PathSourceShape::Visible(float scale) const
{
	return scale >= MinVisibilityScale()
		&& (scale <= MaxVisibilityScale() || MaxVisibilityScale() >= 4.0f);
}

