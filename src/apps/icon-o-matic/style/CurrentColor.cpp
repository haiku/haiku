/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "CurrentColor.h"

#include <stdio.h>

#include <OS.h>

#include "ui_defines.h"

// init global CurrentColor instance
CurrentColor
CurrentColor::fDefaultInstance;


// constructor
CurrentColor::CurrentColor()
	: Observable(),
	  fColor(kBlack)
{
}

// destructor
CurrentColor::~CurrentColor()
{
}

// Default
CurrentColor*
CurrentColor::Default()
{
	return &fDefaultInstance;
}

// SetColor
void
CurrentColor::SetColor(rgb_color color)
{
	if ((uint32&)fColor == (uint32&)color)
		return;

	fColor = color;
	Notify();
}

