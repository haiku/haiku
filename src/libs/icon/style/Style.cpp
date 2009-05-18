/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Style.h"

#include <new>

# include <Message.h>

#ifdef ICON_O_MATIC
# include "ui_defines.h"
#else
# define kWhite (rgb_color){ 255, 255, 255, 255 }
#endif // ICON_O_MATIC

#include "GradientTransformable.h"

using std::nothrow;

// constructor
Style::Style()
#ifdef ICON_O_MATIC
	: IconObject("<style>"),
	  Observer(),
#else
	:
#endif

	  fColor(kWhite),
	  fGradient(NULL),
	  fColors(NULL),

	  fGammaCorrectedColors(NULL),
	  fGammaCorrectedColorsValid(false)
{
}

// constructor
Style::Style(const rgb_color& color)
#ifdef ICON_O_MATIC
	: IconObject("<style>"),
	  Observer(),
#else
	:
#endif

	  fColor(color),
	  fGradient(NULL),
	  fColors(NULL),

	  fGammaCorrectedColors(NULL),
	  fGammaCorrectedColorsValid(false)
{
}

// constructor
Style::Style(const Style& other)
#ifdef ICON_O_MATIC
	: IconObject(other),
	  Observer(),
#else
	:
#endif

	  fColor(other.fColor),
	  fGradient(NULL),
	  fColors(NULL),

	  fGammaCorrectedColors(NULL),
	  fGammaCorrectedColorsValid(false)
{
	SetGradient(other.fGradient);
}

// constructor
Style::Style(BMessage* archive)
#ifdef ICON_O_MATIC
	: IconObject(archive),
	  Observer(),
#else
	:
#endif

	  fColor(kWhite),
	  fGradient(NULL),
	  fColors(NULL),

	  fGammaCorrectedColors(NULL),
	  fGammaCorrectedColorsValid(false)
{
	if (!archive)
		return;

	if (archive->FindInt32("color", (int32*)&fColor) < B_OK)
		fColor = kWhite;

	BMessage gradientArchive;
	if (archive->FindMessage("gradient", &gradientArchive) == B_OK) {
		::Gradient gradient(&gradientArchive);
		SetGradient(&gradient);
	}
}

// destructor
Style::~Style()
{
	SetGradient(NULL);
}

#ifdef ICON_O_MATIC
// ObjectChanged
void
Style::ObjectChanged(const Observable* object)
{
	if (object == fGradient && fColors) {
		fGradient->MakeGradient((uint32*)fColors, 256);
		fGammaCorrectedColorsValid = false;
		Notify();
	}
}

// #pragma mark -

// Archive
status_t
Style::Archive(BMessage* into, bool deep) const
{
	status_t ret = IconObject::Archive(into, deep);

	if (ret == B_OK)
		ret = into->AddInt32("color", (uint32&)fColor);

	if (ret == B_OK && fGradient) {
		BMessage gradientArchive;
		ret = fGradient->Archive(&gradientArchive, deep);
		if (ret == B_OK)
			ret = into->AddMessage("gradient", &gradientArchive);
	}

	return ret;
}

// operator ==
bool
Style::operator==(const Style& other) const
{
	if (fGradient) {
		if (other.fGradient)
			return *fGradient == *other.fGradient;
		else
			return false;
	} else {
		if (!other.fGradient)
			return *(uint32*)&fColor == *(uint32*)&other.fColor;
		else
			return false;
	}
}

#endif // ICON_O_MATIC

// HasTransparency
bool
Style::HasTransparency() const
{
	if (fGradient) {
		int32 count = fGradient->CountColors();
		for (int32 i = 0; i < count; i++) {
			BGradient::ColorStop* step = fGradient->ColorAtFast(i);
			if (step->color.alpha < 255)
				return true;
		}
		return false;
	}
	return fColor.alpha < 255;
}

// SetColor
void
Style::SetColor(const rgb_color& color)
{
	if (*(uint32*)&fColor == *(uint32*)&color)
		return;

	fColor = color;
	Notify();
}

// SetGradient
void
Style::SetGradient(const ::Gradient* gradient)
{
	if (!fGradient && !gradient)
		return;

	if (gradient) {
		if (!fGradient) {
			fGradient = new (nothrow) ::Gradient(*gradient);
			if (fGradient) {
#ifdef ICON_O_MATIC
				fGradient->AddObserver(this);
#endif
				// generate gradient
				fColors = new agg::rgba8[256];
				fGradient->MakeGradient((uint32*)fColors, 256);
				fGammaCorrectedColorsValid = false;

				Notify();
			}
		} else {
			if (*fGradient != *gradient) {
				*fGradient = *gradient;
			}
		}
	} else {
#ifdef ICON_O_MATIC
		fGradient->RemoveObserver(this);
#endif
		delete[] fColors;
		delete[] fGammaCorrectedColors;
		fColors = NULL;
		fGammaCorrectedColors = NULL;
		fGradient = NULL;
		Notify();
	}
}

// GammaCorrectedColors
const agg::rgba8*	
Style::GammaCorrectedColors(const GammaTable& table) const
{
	if (!fColors)
		return NULL;

	if (!fGammaCorrectedColors)
		fGammaCorrectedColors = new agg::rgba8[256];

	if (!fGammaCorrectedColorsValid) {
		for (int32 i = 0; i < 256; i++) {
			fGammaCorrectedColors[i].r = table.dir(fColors[i].r);
			fGammaCorrectedColors[i].g = table.dir(fColors[i].g);
			fGammaCorrectedColors[i].b = table.dir(fColors[i].b);
			fGammaCorrectedColors[i].a = fColors[i].a;
			fGammaCorrectedColors[i].premultiply();
		}
		fGammaCorrectedColorsValid = true;
	}

	return fGammaCorrectedColors;
}

