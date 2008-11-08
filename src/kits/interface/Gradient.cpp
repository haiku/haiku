/*
 * Copyright 2006-2008, Haiku.
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
BGradient::color_step::color_step(const rgb_color c, float o)
{
	color.red = c.red;
	color.green = c.green;
	color.blue = c.blue;
	color.alpha = c.alpha;
	offset = o;
}


// constructor
BGradient::color_step::color_step(uint8 r, uint8 g, uint8 b, uint8 a, float o)
{
	color.red = r;
	color.green = g;
	color.blue = b;
	color.alpha = a;
	offset = o;
}


// constructor
BGradient::color_step::color_step(const color_step& other)
{
	color.red = other.color.red;
	color.green = other.color.green;
	color.blue = other.color.blue;
	color.alpha = other.color.alpha;
	offset = other.offset;
}


// constructor
BGradient::color_step::color_step()
{
	color.red = 0;
	color.green = 0;
	color.blue = 0;
	color.alpha = 255;
	offset = 0;
}


// operator!=
bool
BGradient::color_step::operator!=(const color_step& other) const
{
	return color.red != other.color.red ||
	color.green != other.color.green ||
	color.blue != other.color.blue ||
	color.alpha != other.color.alpha ||
	offset != other.offset;
}


static int
sort_color_steps_by_offset(const void* _left, const void* _right)
{
	const BGradient::color_step** left = (const BGradient::color_step**)_left;
	const BGradient::color_step** right = (const BGradient::color_step**)_right;
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
	fColors(4),
	fType(TYPE_NONE)
{
}


// constructor
BGradient::BGradient(BMessage* archive)
	: BArchivable(archive),
	fColors(4),
	fType(TYPE_NONE)
{
	if (!archive)
		return;

	// color steps
	color_step step;
	for (int32 i = 0; archive->FindFloat("offset", i, &step.offset) >= B_OK; i++) {
		if (archive->FindInt32("color", i, (int32*)&step.color) >= B_OK)
			AddColor(step, i);
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
		for (int32 i = 0; color_step* step = ColorAt(i); i++) {
			ret = into->AddInt32("color", (const uint32&)step->color);
			if (ret < B_OK)
				break;
			ret = into->AddFloat("offset", step->offset);
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
	SetColors(other);
	fType = other.fType;
	return *this;
}


// operator==
bool
BGradient::operator==(const BGradient& other) const
{
	return ((other.Type() == Type()) && ColorStepsAreEqual(other));
}


// operator!=
bool
BGradient::operator!=(const BGradient& other) const
{
	return !(*this == other);
}


// ColorStepsAreEqual
bool
BGradient::ColorStepsAreEqual(const BGradient& other) const
{
	int32 count = CountColors();
	if (count == other.CountColors() &&
		fType == other.fType) {
		
		bool equal = true;
		for (int32 i = 0; i < count; i++) {
			color_step* ourStep = ColorAtFast(i);
			color_step* otherStep = other.ColorAtFast(i);
			if (*ourStep != *otherStep) {
				equal = false;
				break;
			}
		}
		return equal;
	}
	return false;
}


// SetColors
void
BGradient::SetColors(const BGradient& other)
{
	MakeEmpty();
	for (int32 i = 0; color_step* step = other.ColorAt(i); i++)
		AddColor(*step, i);
}


// AddColor
int32
BGradient::AddColor(const rgb_color& color, float offset)
{
	// find the correct index (sorted by offset)
	color_step* step = new color_step(color, offset);
	int32 index = 0;
	int32 count = CountColors();
	for (; index < count; index++) {
		color_step* s = ColorAtFast(index);
		if (s->offset > step->offset)
			break;
	}
	if (!fColors.AddItem((void*)step, index)) {
		delete step;
		return -1;
	}
	return index;
}


// AddColor
bool
BGradient::AddColor(const color_step& color, int32 index)
{
	color_step* step = new color_step(color);
	if (!fColors.AddItem((void*)step, index)) {
		delete step;
		return false;
	}
	return true;
}


// RemoveColor
bool
BGradient::RemoveColor(int32 index)
{
	color_step* step = (color_step*)fColors.RemoveItem(index);
	if (!step) {
		return false;
	}
	delete step;
	return true;
}


// SetColor
bool
BGradient::SetColor(int32 index, const color_step& color)
{
	if (color_step* step = ColorAt(index)) {
		if (*step != color) {
			step->color = color.color;
			step->offset = color.offset;
			return true;
		}
	}
	return false;
}


// SetColor
bool
BGradient::SetColor(int32 index, const rgb_color& color)
{
	if (color_step* step = ColorAt(index)) {
		if ((uint32&)step->color != (uint32&)color) {
			step->color = color;
			return true;
		}
	}
	return false;
}


// SetOffset
bool
BGradient::SetOffset(int32 index, float offset)
{
	color_step* step = ColorAt(index);
	if (step && step->offset != offset) {
		step->offset = offset;
		return true;
	}
	return false;
}


// CountColors
int32
BGradient::CountColors() const
{
	return fColors.CountItems();
}


// ColorAt
BGradient::color_step*
BGradient::ColorAt(int32 index) const
{
	return (color_step*)fColors.ItemAt(index);
}


// ColorAtFast
BGradient::color_step*
BGradient::ColorAtFast(int32 index) const
{
	return (color_step*)fColors.ItemAtFast(index);
}


// Colors
BGradient::color_step*
BGradient::Colors() const
{
	if (CountColors() > 0) {
		return (color_step*) fColors.Items();
	}
	return NULL;
}


// SortColorStepsByOffset
void
BGradient::SortColorStepsByOffset()
{
	fColors.SortItems(sort_color_steps_by_offset);
}


// MakeEmpty
void
BGradient::MakeEmpty()
{
	int32 count = CountColors();
	for (int32 i = 0; i < count; i++)
		delete ColorAtFast(i);
	fColors.MakeEmpty();
}
