/*
 * Copyright 2006-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Artur Wyszynski <harakash@gmail.com>
 */

#include "Gradient.h"

#include <math.h>
#include <stdio.h>

#include <Message.h>


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


static int
sort_color_stops_by_offset(const void* _left, const void* _right)
{
	const BGradient::ColorStop** left = (const BGradient::ColorStop**)_left;
	const BGradient::ColorStop** right = (const BGradient::ColorStop**)_right;
	if ((*left)->offset > (*right)->offset)
		return 1;
	else if ((*left)->offset < (*right)->offset)
		return -1;
	return 0;
}


// #pragma mark -


// constructor
BGradient::BGradient()
	: BArchivable(),
	fColorStops(4),
	fType(TYPE_NONE)
{
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
	SetColorStops(other);
	fType = other.fType;
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
	fColorStops.SortItems(sort_color_stops_by_offset);
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
