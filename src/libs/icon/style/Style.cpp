/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Style.h"

#include <new>

#ifdef ICON_O_MATIC
# include <Message.h>

# include "ui_defines.h"
#else
# define kWhite (rgb_color){ 255, 255, 255, 255 }
#endif // ICON_O_MATIC

#include "Gradient.h"

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

#ifdef ICON_O_MATIC
// constructor
Style::Style(BMessage* archive)
	: IconObject(archive),
	  Observer(),

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
#endif // ICON_O_MATIC

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
#endif // ICON_O_MATIC

// SetColor
void
Style::SetColor(const rgb_color& color)
{
	if ((uint32&)fColor == (uint32&)color)
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

