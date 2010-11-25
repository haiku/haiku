/*
 * Copyright 2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */


#include "WindowBehaviour.h"


WindowBehaviour::WindowBehaviour()
	:
	fIsResizing(false),
	fIsDragging(false)
{
}


WindowBehaviour::~WindowBehaviour()
{
}


void
WindowBehaviour::ModifiersChanged(int32 modifiers)
{
}
