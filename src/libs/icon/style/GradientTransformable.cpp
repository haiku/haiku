/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "GradientTransformable.h"

#include <math.h>
#include <stdio.h>

#include <Message.h>

#ifdef ICON_O_MATIC
#  include "support.h"
#endif

// constructor
Gradient::Gradient(bool empty)
#ifdef ICON_O_MATIC
	: BArchivable(),
	  Observable(),
	  BReferenceable(),
	  Transformable(),
#else
	: Transformable(),
#endif

	  fColors(4),
	  fType(GRADIENT_LINEAR),
	  fInterpolation(INTERPOLATION_SMOOTH),
	  fInheritTransformation(true)
{
	if (!empty) {
		AddColor(BGradient::ColorStop(0, 0, 0, 255, 0.0), 0);
		AddColor(BGradient::ColorStop(255, 255, 255, 255, 1.0), 1);
	}
}

// constructor
Gradient::Gradient(BMessage* archive)
#ifdef ICON_O_MATIC
	: BArchivable(archive),
	  Observable(),
	  BReferenceable(),
	  Transformable(),
#else
	: Transformable(),
#endif

	  fColors(4),
	  fType(GRADIENT_LINEAR),
	  fInterpolation(INTERPOLATION_SMOOTH),
	  fInheritTransformation(true)
{
	if (!archive)
		return;

	// read transformation
	int32 size = Transformable::matrix_size;
	const void* matrix;
	ssize_t dataSize = size * sizeof(double);
	if (archive->FindData("transformation", B_DOUBLE_TYPE,
						  &matrix, &dataSize) == B_OK
		&& dataSize == (ssize_t)(size * sizeof(double)))
		LoadFrom((const double*)matrix);

	// color steps
	BGradient::ColorStop step;
	for (int32 i = 0; archive->FindFloat("offset", i, &step.offset) >= B_OK; i++) {
		if (archive->FindInt32("color", i, (int32*)&step.color) >= B_OK) {
			// Use the slower method of adding by offset in case the gradient
			// was not stored with steps in the correct order
			AddColor(step.color, step.offset);
		} else
			break;
	}
	if (archive->FindInt32("type", (int32*)&fType) < B_OK)
		fType = GRADIENT_LINEAR;

	if (archive->FindInt32("interpolation", (int32*)&fInterpolation) < B_OK)
		fInterpolation = INTERPOLATION_SMOOTH;

	if (archive->FindBool("inherit transformation",
						  &fInheritTransformation) < B_OK)
		fInheritTransformation = true;
}

// constructor
Gradient::Gradient(const Gradient& other)
#ifdef ICON_O_MATIC
	: BArchivable(other),
	  Observable(),
	  BReferenceable(),
	  Transformable(other),
#else
	: Transformable(other),
#endif

	  fColors(4),
	  fType(other.fType),
	  fInterpolation(other.fInterpolation),
	  fInheritTransformation(other.fInheritTransformation)
{
	for (int32 i = 0; BGradient::ColorStop* step = other.ColorAt(i); i++) {
		AddColor(*step, i);
	}
}

// destructor
Gradient::~Gradient()
{
	_MakeEmpty();
}

#ifdef ICON_O_MATIC
// Archive
status_t
Gradient::Archive(BMessage* into, bool deep) const
{
	status_t ret = BArchivable::Archive(into, deep);

	// transformation
	if (ret == B_OK) {
		int32 size = Transformable::matrix_size;
		double matrix[size];
		StoreTo(matrix);
		ret = into->AddData("transformation", B_DOUBLE_TYPE,
							matrix, size * sizeof(double));
	}

	// color steps
	if (ret >= B_OK) {
		for (int32 i = 0; BGradient::ColorStop* step = ColorAt(i); i++) {
			ret = into->AddInt32("color", (const uint32&)step->color);
			if (ret < B_OK)
				break;
			ret = into->AddFloat("offset", step->offset);
			if (ret < B_OK)
				break;
		}
	}
	// gradient and interpolation type
	if (ret >= B_OK)
		ret = into->AddInt32("type", (int32)fType);
	if (ret >= B_OK)
		ret = into->AddInt32("interpolation", (int32)fInterpolation);
	if (ret >= B_OK)
		ret = into->AddBool("inherit transformation", fInheritTransformation);

	// finish off
	if (ret >= B_OK)
		ret = into->AddString("class", "Gradient");

	return ret;
}
#endif // ICON_O_MATIC

// #pragma mark -

// operator=
Gradient&
Gradient::operator=(const Gradient& other)
{
#ifdef ICON_O_MATIC
	AutoNotificationSuspender _(this);
#endif

	SetTransform(other);
	SetColors(other);
	SetType(other.fType);
	SetInterpolation(other.fInterpolation);
	SetInheritTransformation(other.fInheritTransformation);

	return *this;
}

// operator==
bool
Gradient::operator==(const Gradient& other) const
{
	if (Transformable::operator==(other))
		return ColorStepsAreEqual(other);
	return false;
}

// operator!=
bool
Gradient::operator!=(const Gradient& other) const
{
	return !(*this == other);
}

// ColorStepsAreEqual
bool
Gradient::ColorStepsAreEqual(const Gradient& other) const
{
	int32 count = CountColors();
	if (count == other.CountColors() &&
		fType == other.fType &&
		fInterpolation == other.fInterpolation &&
		fInheritTransformation == other.fInheritTransformation) {

		bool equal = true;
		for (int32 i = 0; i < count; i++) {
			BGradient::ColorStop* ourStep = ColorAtFast(i);
			BGradient::ColorStop* otherStep = other.ColorAtFast(i);
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
Gradient::SetColors(const Gradient& other)
{
#ifdef ICON_O_MATIC
	AutoNotificationSuspender _(this);
#endif

	_MakeEmpty();
	for (int32 i = 0; BGradient::ColorStop* step = other.ColorAt(i); i++)
		AddColor(*step, i);

	Notify();
}

// #pragma mark -

// AddColor
int32
Gradient::AddColor(const rgb_color& color, float offset)
{
	// find the correct index (sorted by offset)
	BGradient::ColorStop* step = new BGradient::ColorStop(color, offset);
	int32 index = 0;
	int32 count = CountColors();
	for (; index < count; index++) {
		BGradient::ColorStop* s = ColorAtFast(index);
		if (s->offset > step->offset)
			break;
	}
	if (!fColors.AddItem((void*)step, index)) {
		delete step;
		return -1;
	}
	Notify();
	return index;
}

// AddColor
bool
Gradient::AddColor(const BGradient::ColorStop& color, int32 index)
{
	BGradient::ColorStop* step = new BGradient::ColorStop(color);
	if (!fColors.AddItem((void*)step, index)) {
		delete step;
		return false;
	}
	Notify();
	return true;
}

// RemoveColor
bool
Gradient::RemoveColor(int32 index)
{
	BGradient::ColorStop* step
		= (BGradient::ColorStop*)fColors.RemoveItem(index);
	if (!step) {
		return false;
	}
	delete step;
	Notify();
	return true;
}

// #pragma mark -

// SetColor
bool
Gradient::SetColor(int32 index, const BGradient::ColorStop& color)
{
	if (BGradient::ColorStop* step = ColorAt(index)) {
		if (*step != color) {
			step->color = color.color;
			step->offset = color.offset;
			Notify();
			return true;
		}
	}
	return false;
}

// SetColor
bool
Gradient::SetColor(int32 index, const rgb_color& color)
{
	if (BGradient::ColorStop* step = ColorAt(index)) {
		if ((uint32&)step->color != (uint32&)color) {
			step->color = color;
			Notify();
			return true;
		}
	}
	return false;
}

// SetOffset
bool
Gradient::SetOffset(int32 index, float offset)
{
	BGradient::ColorStop* step = ColorAt(index);
	if (step && step->offset != offset) {
		step->offset = offset;
		Notify();
		return true;
	}
	return false;
}

// #pragma mark -

// CountColors
int32
Gradient::CountColors() const
{
	return fColors.CountItems();
}

// ColorAt
BGradient::ColorStop*
Gradient::ColorAt(int32 index) const
{
	return (BGradient::ColorStop*)fColors.ItemAt(index);
}

// ColorAtFast
BGradient::ColorStop*
Gradient::ColorAtFast(int32 index) const
{
	return (BGradient::ColorStop*)fColors.ItemAtFast(index);
}

// #pragma mark -

// SetType
void
Gradient::SetType(gradients_type type)
{
	if (fType != type) {
		fType = type;
		Notify();
	}
}

// SetInterpolation
void
Gradient::SetInterpolation(interpolation_type type)
{
	if (fInterpolation != type) {
		fInterpolation = type;
		Notify();
	}
}

// SetInheritTransformation
void
Gradient::SetInheritTransformation(bool inherit)
{
	if (fInheritTransformation != inherit) {
		fInheritTransformation = inherit;
		Notify();
	}
}

// #pragma mark -

// gauss
inline double
gauss(double f)
{ 
	// this aint' a real gauss function
	if (f > 0.0) {
		if (f < 0.5)
			return (1.0 - 2.0 * f*f);

		f = 1.0 - f;
		return (2.0 * f*f);
	}
	return 1.0;
}

// MakeGradient
void
Gradient::MakeGradient(uint32* colors, int32 count) const
{
	BGradient::ColorStop* from = ColorAt(0);
	
	if (!from)
		return;

	// find the step with the lowest offset
	for (int32 i = 0; BGradient::ColorStop* step = ColorAt(i); i++) {
		if (step->offset < from->offset)
			from = step;
	}

	// current index into "colors" array
	int32 index = (int32)floorf(count * from->offset + 0.5);
	if (index < 0)
		index = 0;
	if (index > count)
		index = count;
	//  make sure we fill the entire array
	if (index > 0) {
		uint8* c = (uint8*)&colors[0];
		for (int32 i = 0; i < index; i++) {
			c[0] = from->color.red;
			c[1] = from->color.green;
			c[2] = from->color.blue;
			c[3] = from->color.alpha;
			c += 4;
		}
	}

	// put all steps that we need to interpolate to into a list
	BList nextSteps(fColors.CountItems() - 1);
	for (int32 i = 0; BGradient::ColorStop* step = ColorAt(i); i++) {
		if (step != from)
			nextSteps.AddItem((void*)step);
	}

	// interpolate "from" to "to"
	while (!nextSteps.IsEmpty()) {

		// find the step with the next offset
		BGradient::ColorStop* to = NULL;
		float nextOffsetDist = 2.0;
		for (int32 i = 0; BGradient::ColorStop* step
				= (BGradient::ColorStop*)nextSteps.ItemAt(i); i++) {
			float d = step->offset - from->offset;
			if (d < nextOffsetDist && d >= 0) {
				to = step;
				nextOffsetDist = d;
			}
		}
		if (!to)
			break;

		nextSteps.RemoveItem((void*)to);

		// interpolate
		int32 offset = (int32)floorf((count - 1) * to->offset + 0.5);
		if (offset >= count)
			offset = count - 1;
		int32 dist = offset - index;
		if (dist >= 0) {
			uint8* c = (uint8*)&colors[index];
#if GAMMA_BLEND
			uint16 fromRed = kGammaTable[from->color.red];
			uint16 fromGreen = kGammaTable[from->color.green];
			uint16 fromBlue = kGammaTable[from->color.blue];
			uint16 toRed = kGammaTable[to->color.red];
			uint16 toGreen = kGammaTable[to->color.green];
			uint16 toBlue = kGammaTable[to->color.blue];

			for (int32 i = index; i <= offset; i++) {
				float f = (float)(offset - i) / (float)(dist + 1);
				if (fInterpolation == INTERPOLATION_SMOOTH)
					f = gauss(1.0 - f);
				float t = 1.0 - f;
				c[0] = kInverseGammaTable[(uint16)floor(fromBlue * f + toBlue * t + 0.5)];
				c[1] = kInverseGammaTable[(uint16)floor(fromGreen * f + toGreen * t + 0.5)];
				c[2] = kInverseGammaTable[(uint16)floor(fromRed * f + toRed * t + 0.5)];
				c[3] = (uint8)floor(from->color.alpha * f + to->color.alpha * t + 0.5);
				c += 4;
			}
#else // GAMMA_BLEND
			for (int32 i = index; i <= offset; i++) {
				float f = (float)(offset - i) / (float)(dist + 1);
				if (fInterpolation == INTERPOLATION_SMOOTH)
					f = gauss(1.0 - f);
				float t = 1.0 - f;
				c[0] = (uint8)floor(from->color.red * f + to->color.red * t + 0.5);
				c[1] = (uint8)floor(from->color.green * f + to->color.green * t + 0.5);
				c[2] = (uint8)floor(from->color.blue * f + to->color.blue * t + 0.5);
				c[3] = (uint8)floor(from->color.alpha * f + to->color.alpha * t + 0.5);
				c += 4;
			}
#endif // GAMMA_BLEND
		}
		index = offset + 1;
		// the current "to" will be the "from" in the next interpolation
		from = to;
	}
	//  make sure we fill the entire array
	if (index < count) {
		uint8* c = (uint8*)&colors[index];
		for (int32 i = index; i < count; i++) {
			c[0] = from->color.red;
			c[1] = from->color.green;
			c[2] = from->color.blue;
			c[3] = from->color.alpha;
			c += 4;
		}
	}
}

// FitToBounds
void
Gradient::FitToBounds(const BRect& bounds)
{
	double parl[6];
	parl[0] = bounds.left;
	parl[1] = bounds.top;
	parl[2] = bounds.right;
	parl[3] = bounds.top;
	parl[4] = bounds.right;
	parl[5] = bounds.bottom;
	agg::trans_affine transform(-200.0, -200.0, 200.0, 200.0, parl);
	multiply(transform);
}

// string_for_type
static const char*
string_for_type(gradients_type type)
{
	switch (type) {
		case GRADIENT_LINEAR:
			return "GRADIENT_LINEAR";
		case GRADIENT_CIRCULAR:
			return "GRADIENT_CIRCULAR";
		case GRADIENT_DIAMOND:
			return "GRADIENT_DIAMOND";
		case GRADIENT_CONIC:
			return "GRADIENT_CONIC";
		case GRADIENT_XY:
			return "GRADIENT_XY";
		case GRADIENT_SQRT_XY:
			return "GRADIENT_SQRT_XY";
	}
	return "<unkown>";
}

//string_for_interpolation
static const char*
string_for_interpolation(interpolation_type type)
{
	switch (type) {
		case INTERPOLATION_LINEAR:
			return "INTERPOLATION_LINEAR";
		case INTERPOLATION_SMOOTH:
			return "INTERPOLATION_SMOOTH";
	}
	return "<unkown>";
}

// GradientArea
BRect
Gradient::GradientArea() const
{
	BRect area(0, 0, 64, 64);
	switch (fType) {
		case GRADIENT_LINEAR:
		case GRADIENT_CIRCULAR:
		case GRADIENT_DIAMOND:
		case GRADIENT_CONIC:
		case GRADIENT_XY:
		case GRADIENT_SQRT_XY:
			break;
	}
	return area;
}

// TransformationChanged()
void
Gradient::TransformationChanged()
{
	Notify();
}

// PrintToStream
void
Gradient::PrintToStream() const
{
	printf("Gradient: type: %s, interpolation: %s, inherits transform: %d\n",
		   string_for_type(fType),
		   string_for_interpolation(fInterpolation),
		   fInheritTransformation);
	for (int32 i = 0; BGradient::ColorStop* step = ColorAt(i); i++) {
		printf("  %" B_PRId32 ": offset: %.1f -> color(%d, %d, %d, %d)\n",
			   i, step->offset,
			   step->color.red,
			   step->color.green,
			   step->color.blue,
			   step->color.alpha);
	}

	Transformable::PrintToStream();
}

// _MakeEmpty
void
Gradient::_MakeEmpty()
{
	int32 count = CountColors();
	for (int32 i = 0; i < count; i++)
		delete ColorAtFast(i);
	fColors.MakeEmpty();
}
