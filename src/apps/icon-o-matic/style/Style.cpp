/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Style.h"

#include "Gradient.h"

// constructor
Style::Style()
	: IconObject(),
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
Style::SetGradient(::Gradient* gradient)
{
	if (fGradient) {
		fGradient->RemoveObserver(this);
		delete fGradient;
	}

	fGradient = gradient;

	if (fGradient) {
		fGradient->AddObserver(this);
		// generate gradient
		if (!fColors)
			fColors = new agg::rgba8[256];
		fGradient->MakeGradient((uint32*)fColors, 256);
	} else {
		delete[] fColors;
		fColors = NULL;
	}
}
