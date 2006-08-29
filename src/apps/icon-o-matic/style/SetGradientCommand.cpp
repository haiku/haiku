/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "SetGradientCommand.h"

#include <new>
#include <stdio.h>

#include "Gradient.h"
#include "Style.h"

using std::nothrow;

// constructor
SetGradientCommand::SetGradientCommand(Style* style,
									   const Gradient* gradient)
	: Command(),
	  fStyle(style),
	  fGradient(gradient ? new (nothrow) Gradient(*gradient) : NULL)
{
}

// destructor
SetGradientCommand::~SetGradientCommand()
{
	delete fGradient;
}

// InitCheck
status_t
SetGradientCommand::InitCheck()
{
	if (!fStyle)
		return B_NO_INIT;
	if (fGradient && fStyle->Gradient()) {
		if (*fGradient == *fStyle->Gradient()) {
			printf("SetGradientCommand::InitCheck() - same gradients\n");
			return B_ERROR;
		}
	}
	return B_OK;
}

// Perform
status_t
SetGradientCommand::Perform()
{
	// clone the new gradient for handling over to the style
	Gradient* clone = NULL;
	if (fGradient) {
		clone = new (nothrow) Gradient(*fGradient);
		if (!clone)
			return B_NO_MEMORY;
	}

	if (fStyle->Gradient()) {
		// remember the current gradient of the style
		if (fGradient)
			*fGradient = *fStyle->Gradient();
		else {
			fGradient = new (nothrow) Gradient(*fStyle->Gradient());
			if (!fGradient)
				return B_NO_MEMORY;
		}
	} else {
		// the style didn't have a gradient set
		delete fGradient;
		fGradient = NULL;
	}

	// remove the gradient or set the new one
	fStyle->SetGradient(clone);
	delete clone;

	return B_OK;
}

// Undo
status_t
SetGradientCommand::Undo()
{
	return Perform();
}

// GetName
void
SetGradientCommand::GetName(BString& name)
{
	name << _GetString(/*EDIT_GRADIENT*/0, "Edit Gradient");
}

// CombineWithNext
bool
SetGradientCommand::CombineWithNext(const Command* command)
{
	const SetGradientCommand* next
		= dynamic_cast<const SetGradientCommand*>(command);

	if (next && next->fTimeStamp - fTimeStamp < 1000000) {
		fTimeStamp = next->fTimeStamp;
		// NOTE: next was already performed, but
		// when undoing, we want to use our
		// remembered gradient
		return true;
	}
	return false;
}

