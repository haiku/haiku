/*
 * Copyright 2006-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Artur Wyszynski <harakash@gmail.com>
 */

#include "Gradient.h"

#include <algorithm>
#include <math.h>
#include <stdio.h>

#include <DataIO.h>
#include <Message.h>

#include <AutoDeleter.h>
#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientDiamond.h>
#include <GradientConic.h>


// constructor
BGradient::ColorStop::ColorStop(const rgb_color c, float o)
{
	color.red = c.red;
	color.green = c.green;
	color.blue = c.blue;
	color.alpha = c.alpha;
	offset = o;
}


// constructor
BGradient::ColorStop::ColorStop(uint8 r, uint8 g, uint8 b, uint8 a, float o)
{
	color.red = r;
	color.green = g;
	color.blue = b;
	color.alpha = a;
	offset = o;
}


// constructor
BGradient::ColorStop::ColorStop(const ColorStop& other)
{
	color.red = other.color.red;
	color.green = other.color.green;
	color.blue = other.color.blue;
	color.alpha = other.color.alpha;
	offset = other.offset;
}


// constructor
BGradient::ColorStop::ColorStop()
{
	color.red = 0;
	color.green = 0;
	color.blue = 0;
	color.alpha = 255;
	offset = 0;
}


// operator!=
bool
BGradient::ColorStop::operator!=(const ColorStop& other) const
{
	return color.red != other.color.red ||
	color.green != other.color.green ||
	color.blue != other.color.blue ||
	color.alpha != other.color.alpha ||
	offset != other.offset;
}


static bool
sort_color_stops_by_offset(const BGradient::ColorStop* left,
	const BGradient::ColorStop* right)
{
	return left->offset < right->offset;
}


// #pragma mark -


// constructor
BGradient::BGradient()
	: BArchivable(),
	fColorStops(4),
	fType(TYPE_NONE)
{
}


BGradient::BGradient(const BGradient& other)
	: BArchivable(),
	fColorStops(std::max((int32)4, other.CountColorStops()))
{
	*this = other;
}


// constructor
BGradient::BGradient(BMessage* archive)
	: BArchivable(archive),
	fColorStops(4),
	fType(TYPE_NONE)
{
	if (!archive)
		return;

	// color stops
	ColorStop stop;
	for (int32 i = 0; archive->FindFloat("offset", i, &stop.offset) >= B_OK; i++) {
		if (archive->FindInt32("color", i, (int32*)&stop.color) >= B_OK)
			AddColorStop(stop, i);
		else
			break;
	}
	if (archive->FindInt32("type", (int32*)&fType) < B_OK)
		fType = TYPE_LINEAR;

	// linear
	if (archive->FindFloat("linear_x1", (float*)&fData.linear.x1) < B_OK)
		fData.linear.x1 = 0.0f;
	if (archive->FindFloat("linear_y1", (float*)&fData.linear.y1) < B_OK)
		fData.linear.y1 = 0.0f;
	if (archive->FindFloat("linear_x2", (float*)&fData.linear.x2) < B_OK)
		fData.linear.x2 = 0.0f;
	if (archive->FindFloat("linear_y2", (float*)&fData.linear.y2) < B_OK)
		fData.linear.y2 = 0.0f;

	// radial
	if (archive->FindFloat("radial_cx", (float*)&fData.radial.cx) < B_OK)
		fData.radial.cx = 0.0f;
	if (archive->FindFloat("radial_cy", (float*)&fData.radial.cy) < B_OK)
		fData.radial.cy = 0.0f;
	if (archive->FindFloat("radial_radius", (float*)&fData.radial.radius) < B_OK)
		fData.radial.radius = 0.0f;

	// radial focus
	if (archive->FindFloat("radial_f_cx", (float*)&fData.radial_focus.cx) < B_OK)
		fData.radial_focus.cx = 0.0f;
	if (archive->FindFloat("radial_f_cy", (float*)&fData.radial_focus.cy) < B_OK)
		fData.radial_focus.cy = 0.0f;
	if (archive->FindFloat("radial_f_fx", (float*)&fData.radial_focus.fx) < B_OK)
		fData.radial_focus.fx = 0.0f;
	if (archive->FindFloat("radial_f_fy", (float*)&fData.radial_focus.fy) < B_OK)
		fData.radial_focus.fy = 0.0f;
	if (archive->FindFloat("radial_f_radius", (float*)&fData.radial_focus.radius) < B_OK)
		fData.radial_focus.radius = 0.0f;

	// diamond
	if (archive->FindFloat("diamond_cx", (float*)&fData.diamond.cx) < B_OK)
		fData.diamond.cx = 0.0f;
	if (archive->FindFloat("diamond_cy", (float*)&fData.diamond.cy) < B_OK)
		fData.diamond.cy = 0.0f;

	// conic
	if (archive->FindFloat("conic_cx", (float*)&fData.conic.cx) < B_OK)
		fData.conic.cx = 0.0f;
	if (archive->FindFloat("conic_cy", (float*)&fData.conic.cy) < B_OK)
		fData.conic.cy = 0.0f;
	if (archive->FindFloat("conic_angle", (float*)&fData.conic.angle) < B_OK)
		fData.conic.angle = 0.0f;
}


// destructor
BGradient::~BGradient()
{
	MakeEmpty();
}


// Archive
status_t
BGradient::Archive(BMessage* into, bool deep) const
{
	status_t ret = BArchivable::Archive(into, deep);

	// color steps
	if (ret >= B_OK) {
		for (int32 i = 0; ColorStop* stop = ColorStopAt(i); i++) {
			ret = into->AddInt32("color", (const uint32&)stop->color);
			if (ret < B_OK)
				break;
			ret = into->AddFloat("offset", stop->offset);
			if (ret < B_OK)
				break;
		}
	}
	// gradient type
	if (ret >= B_OK)
		ret = into->AddInt32("type", (int32)fType);

	// linear
	if (ret >= B_OK)
		ret = into->AddFloat("linear_x1", (float)fData.linear.x1);
	if (ret >= B_OK)
		ret = into->AddFloat("linear_y1", (float)fData.linear.y1);
	if (ret >= B_OK)
		ret = into->AddFloat("linear_x2", (float)fData.linear.x2);
	if (ret >= B_OK)
		ret = into->AddFloat("linear_y2", (float)fData.linear.y2);

	// radial
	if (ret >= B_OK)
		ret = into->AddFloat("radial_cx", (float)fData.radial.cx);
	if (ret >= B_OK)
		ret = into->AddFloat("radial_cy", (float)fData.radial.cy);
	if (ret >= B_OK)
		ret = into->AddFloat("radial_radius", (float)fData.radial.radius);

	// radial focus
	if (ret >= B_OK)
		ret = into->AddFloat("radial_f_cx", (float)fData.radial_focus.cx);
	if (ret >= B_OK)
		ret = into->AddFloat("radial_f_cy", (float)fData.radial_focus.cy);
	if (ret >= B_OK)
		ret = into->AddFloat("radial_f_fx", (float)fData.radial_focus.fx);
	if (ret >= B_OK)
		ret = into->AddFloat("radial_f_fy", (float)fData.radial_focus.fy);
	if (ret >= B_OK)
		ret = into->AddFloat("radial_f_radius", (float)fData.radial_focus.radius);

	// diamond
	if (ret >= B_OK)
		ret = into->AddFloat("diamond_cx", (float)fData.diamond.cx);
	if (ret >= B_OK)
		ret = into->AddFloat("diamond_cy", (float)fData.diamond.cy);

	// conic
	if (ret >= B_OK)
		ret = into->AddFloat("conic_cx", (float)fData.conic.cx);
	if (ret >= B_OK)
		ret = into->AddFloat("conic_cy", (float)fData.conic.cy);
	if (ret >= B_OK)
		ret = into->AddFloat("conic_angle", (float)fData.conic.angle);

	// finish off
	if (ret >= B_OK)
		ret = into->AddString("class", "BGradient");

	return ret;
}


// operator=
BGradient&
BGradient::operator=(const BGradient& other)
{
	if (&other == this)
		return *this;

	SetColorStops(other);
	fType = other.fType;
	switch (fType) {
		case TYPE_LINEAR:
			fData.linear = other.fData.linear;
			break;
		case TYPE_RADIAL:
			fData.radial = other.fData.radial;
			break;
		case TYPE_RADIAL_FOCUS:
			fData.radial_focus = other.fData.radial_focus;
			break;
		case TYPE_DIAMOND:
			fData.diamond = other.fData.diamond;
			break;
		case TYPE_CONIC:
			fData.conic = other.fData.conic;
			break;
		case TYPE_NONE:
			break;
	}
	return *this;
}


// operator==
bool
BGradient::operator==(const BGradient& other) const
{
	return ((other.GetType() == GetType()) && ColorStopsAreEqual(other));
}


// operator!=
bool
BGradient::operator!=(const BGradient& other) const
{
	return !(*this == other);
}


// ColorStopsAreEqual
bool
BGradient::ColorStopsAreEqual(const BGradient& other) const
{
	int32 count = CountColorStops();
	if (count == other.CountColorStops() &&
		fType == other.fType) {

		bool equal = true;
		for (int32 i = 0; i < count; i++) {
			ColorStop* ourStop = ColorStopAtFast(i);
			ColorStop* otherStop = other.ColorStopAtFast(i);
			if (*ourStop != *otherStop) {
				equal = false;
				break;
			}
		}
		return equal;
	}
	return false;
}


// SetColorStops
void
BGradient::SetColorStops(const BGradient& other)
{
	MakeEmpty();
	for (int32 i = 0; ColorStop* stop = other.ColorStopAt(i); i++)
		AddColorStop(*stop, i);
}


// AddColor
int32
BGradient::AddColor(const rgb_color& color, float offset)
{
	// Out of bounds stops would crash the app_server
	if (offset < 0.f || offset > 255.f)
		return -1;

	// find the correct index (sorted by offset)
	ColorStop* stop = new ColorStop(color, offset);
	int32 index = 0;
	int32 count = CountColorStops();
	for (; index < count; index++) {
		ColorStop* s = ColorStopAtFast(index);
		if (s->offset > stop->offset)
			break;
	}
	if (!fColorStops.AddItem((void*)stop, index)) {
		delete stop;
		return -1;
	}
	return index;
}


// AddColorStop
bool
BGradient::AddColorStop(const ColorStop& colorStop, int32 index)
{
	ColorStop* stop = new ColorStop(colorStop);
	if (!fColorStops.AddItem((void*)stop, index)) {
		delete stop;
		return false;
	}
	return true;
}


// RemoveColor
bool
BGradient::RemoveColor(int32 index)
{
	ColorStop* stop = (ColorStop*)fColorStops.RemoveItem(index);
	if (!stop) {
		return false;
	}
	delete stop;
	return true;
}


// SetColorStop
bool
BGradient::SetColorStop(int32 index, const ColorStop& color)
{
	if (ColorStop* stop = ColorStopAt(index)) {
		if (*stop != color) {
			stop->color = color.color;
			stop->offset = color.offset;
			return true;
		}
	}
	return false;
}


// SetColor
bool
BGradient::SetColor(int32 index, const rgb_color& color)
{
	ColorStop* stop = ColorStopAt(index);
	if (stop && stop->color != color) {
		stop->color = color;
		return true;
	}
	return false;
}


// SetOffset
bool
BGradient::SetOffset(int32 index, float offset)
{
	ColorStop* stop = ColorStopAt(index);
	if (stop && stop->offset != offset) {
		stop->offset = offset;
		return true;
	}
	return false;
}


// CountColorStops
int32
BGradient::CountColorStops() const
{
	return fColorStops.CountItems();
}


// ColorStopAt
BGradient::ColorStop*
BGradient::ColorStopAt(int32 index) const
{
	return (ColorStop*)fColorStops.ItemAt(index);
}


// ColorStopAtFast
BGradient::ColorStop*
BGradient::ColorStopAtFast(int32 index) const
{
	return (ColorStop*)fColorStops.ItemAtFast(index);
}


// ColorStops
BGradient::ColorStop*
BGradient::ColorStops() const
{
	if (CountColorStops() > 0) {
		return (ColorStop*) fColorStops.Items();
	}
	return NULL;
}


// SortColorStopsByOffset
void
BGradient::SortColorStopsByOffset()
{
	// Use stable sort: stops with the same offset will retain their original
	// order. This can be used to have sharp color changes in the gradient.
	// BList.SortItems() uses qsort(), which isn't stable, and sometimes swaps
	// such stops.
	const BGradient::ColorStop** first = (const BGradient::ColorStop**)fColorStops.Items();
	const BGradient::ColorStop** last = first + fColorStops.CountItems();
	std::stable_sort(first, last, sort_color_stops_by_offset);
}


// MakeEmpty
void
BGradient::MakeEmpty()
{
	int32 count = CountColorStops();
	for (int32 i = 0; i < count; i++)
		delete ColorStopAtFast(i);
	fColorStops.MakeEmpty();
}


status_t
BGradient::Flatten(BDataIO* stream) const
{
	int32 stopCount = CountColorStops();
	stream->Write(&fType, sizeof(Type));
	stream->Write(&stopCount, sizeof(int32));
	if (stopCount > 0) {
		for (int i = 0; i < stopCount; i++) {
			stream->Write(ColorStopAtFast(i),
				sizeof(ColorStop));
		}
	}

	switch (fType) {
		case TYPE_LINEAR:
			stream->Write(&fData.linear.x1, sizeof(float));
			stream->Write(&fData.linear.y1, sizeof(float));
			stream->Write(&fData.linear.x2, sizeof(float));
			stream->Write(&fData.linear.y2, sizeof(float));
			break;
		case TYPE_RADIAL:
			stream->Write(&fData.radial.cx, sizeof(float));
			stream->Write(&fData.radial.cy, sizeof(float));
			stream->Write(&fData.radial.radius, sizeof(float));
			break;
		case TYPE_RADIAL_FOCUS:
			stream->Write(&fData.radial_focus.cx, sizeof(float));
			stream->Write(&fData.radial_focus.cy, sizeof(float));
			stream->Write(&fData.radial_focus.fx, sizeof(float));
			stream->Write(&fData.radial_focus.fy, sizeof(float));
			stream->Write(&fData.radial_focus.radius, sizeof(float));
			break;
		case TYPE_DIAMOND:
			stream->Write(&fData.diamond.cx, sizeof(float));
			stream->Write(&fData.diamond.cy, sizeof(float));
			break;
		case TYPE_CONIC:
			stream->Write(&fData.conic.cx, sizeof(float));
			stream->Write(&fData.conic.cy, sizeof(float));
			stream->Write(&fData.conic.angle, sizeof(float));
			break;
		case TYPE_NONE:
			break;
	}
	return B_OK;
}


static BGradient*
gradient_for_type(BGradient::Type type)
{
	switch (type) {
		case BGradient::TYPE_LINEAR:
			return new (std::nothrow) BGradientLinear();
		case BGradient::TYPE_RADIAL:
			return new (std::nothrow) BGradientRadial();
		case BGradient::TYPE_RADIAL_FOCUS:
			return new (std::nothrow) BGradientRadialFocus();
		case BGradient::TYPE_DIAMOND:
			return new (std::nothrow) BGradientDiamond();
		case BGradient::TYPE_CONIC:
			return new (std::nothrow) BGradientConic();
		case BGradient::TYPE_NONE:
			return new (std::nothrow) BGradient();
	}
	return NULL;
}


status_t
BGradient::Unflatten(BGradient *&output, BDataIO* stream)
{
	output = NULL;
	Type gradientType;
	int32 colorsCount;
	stream->Read(&gradientType, sizeof(Type));
	status_t status = stream->Read(&colorsCount, sizeof(int32));
	if (status < B_OK)
		return status;

	ObjectDeleter<BGradient> gradient(gradient_for_type(gradientType));
	if (gradient.Get() == NULL)
		return B_NO_MEMORY;

	if (colorsCount > 0) {
		ColorStop stop;
		for (int i = 0; i < colorsCount; i++) {
			if ((status = stream->Read(&stop, sizeof(ColorStop))) < B_OK)
				return status;
			if (!gradient->AddColorStop(stop, i))
				return B_NO_MEMORY;
		}
	}

	switch (gradientType) {
		case TYPE_LINEAR:
			stream->Read(&gradient->fData.linear.x1, sizeof(float));
			stream->Read(&gradient->fData.linear.y1, sizeof(float));
			stream->Read(&gradient->fData.linear.x2, sizeof(float));
			if ((status = stream->Read(&gradient->fData.linear.y2, sizeof(float))) < B_OK)
				return status;
			break;
		case TYPE_RADIAL:
			stream->Read(&gradient->fData.radial.cx, sizeof(float));
			stream->Read(&gradient->fData.radial.cy, sizeof(float));
			if ((stream->Read(&gradient->fData.radial.radius, sizeof(float))) < B_OK)
				return status;
			break;
		case TYPE_RADIAL_FOCUS:
			stream->Read(&gradient->fData.radial_focus.cx, sizeof(float));
			stream->Read(&gradient->fData.radial_focus.cy, sizeof(float));
			stream->Read(&gradient->fData.radial_focus.fx, sizeof(float));
			stream->Read(&gradient->fData.radial_focus.fy, sizeof(float));
			if ((stream->Read(&gradient->fData.radial_focus.radius, sizeof(float))) < B_OK)
				return status;
			break;
		case TYPE_DIAMOND:
			stream->Read(&gradient->fData.diamond.cx, sizeof(float));
			if ((stream->Read(&gradient->fData.diamond.cy, sizeof(float))) < B_OK)
				return status;
			break;
		case TYPE_CONIC:
			stream->Read(&gradient->fData.conic.cx, sizeof(float));
			stream->Read(&gradient->fData.conic.cy, sizeof(float));
			if ((stream->Read(&gradient->fData.conic.angle, sizeof(float))) < B_OK)
				return status;
			break;
		case TYPE_NONE:
			break;
	}

	output = gradient.Detach();
	return B_OK;
}
