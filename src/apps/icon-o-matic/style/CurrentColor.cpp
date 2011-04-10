/*
 * Copyright 2006, 2011, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "CurrentColor.h"

#include <stdio.h>

#include <OS.h>

#include "ui_defines.h"


CurrentColor::CurrentColor()
	: Observable(),
	  fColor(kBlack)
{
}


CurrentColor::~CurrentColor()
{
}


void
CurrentColor::SetColor(rgb_color color)
{
	if ((uint32&)fColor == (uint32&)color)
		return;

	fColor = color;
	Notify();
}

