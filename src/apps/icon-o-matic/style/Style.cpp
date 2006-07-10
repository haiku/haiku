/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Style.h"

#include <new>

#include "Gradient.h"

using std::nothrow;

// constructor
Style::Style()
	: IconObject("<style>"),
	  Observer(),

	  fColor(),
	  fGradient(NULL),
	  fColors(NULL)
{
}

// destructor
Style::~Style()
{
	SetGradient(NULL);
}

// ObjectChanged
void
Style::ObjectChanged(const Observable* object)
{
	if (object == fGradient && fColors) {
		fGradient->MakeGradient((uint32*)fColors, 256);
		Notify();
	}
}

// #pragma mark -

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
				fGradient->AddObserver(this);
				// generate gradient
				fColors = new agg::rgba8[256];
				fGradient->MakeGradient((uint32*)fColors, 256);
				Notify();
			}
		} else {
			if (*fGradient != *gradient) {
				*fGradient = *gradient;
			}
		}
	} else {
		fGradient->RemoveObserver(this);
		delete[] fColors;
		fColors = NULL;
		fGradient = NULL;
		Notify();
	}
}
