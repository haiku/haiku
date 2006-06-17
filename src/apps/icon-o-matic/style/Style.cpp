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
	: Referenceable(),
	  fColor(),
	  fGradient(NULL)
{
}

// destructor
Style::~Style()
{
	delete fGradient;
}

// SetColor
void
Style::SetColor(const rgb_color& color)
{
	fColor = color;
}

// SetGradient
void
Style::SetGradient(::Gradient* gradient)
{
	delete fGradient;
	fGradient = gradient;
}
