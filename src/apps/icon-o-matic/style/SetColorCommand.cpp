/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "SetColorCommand.h"

#include <new>
#include <stdio.h>

#include "Gradient.h"
#include "Style.h"

using std::nothrow;

// constructor
SetColorCommand::SetColorCommand(Style* style,
								 const rgb_color& color)
	: Command(),
	  fStyle(style),
	  fColor(color)
{
}

// destructor
SetColorCommand::~SetColorCommand()
{
}

// InitCheck
status_t
SetColorCommand::InitCheck()
{
	return fStyle ? B_OK : B_NO_INIT;
}

// Perform
status_t
SetColorCommand::Perform()
{
	// toggle the color
	rgb_color previous = fStyle->Color();
	fStyle->SetColor(fColor);
	fColor = previous;

	return B_OK;
}

// Undo
status_t
SetColorCommand::Undo()
{
	return Perform();
}

// GetName
void
SetColorCommand::GetName(BString& name)
{
	name << "Change Color";
}

// CombineWithNext
bool
SetColorCommand::CombineWithNext(const Command* command)
{
	const SetColorCommand* next
		= dynamic_cast<const SetColorCommand*>(command);

	if (next && next->fTimeStamp - fTimeStamp < 1000000) {
		fTimeStamp = next->fTimeStamp;
		// NOTE: next was already performed, but
		// when undoing, we want to use our
		// remembered color
		return true;
	}
	return false;
}

