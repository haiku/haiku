/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "AssignStyleCommand.h"

#include <Catalog.h>
#include <Locale.h>

#include "Shape.h"
#include "Style.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-AssignStyleCmd"


// constructor
AssignStyleCommand::AssignStyleCommand(Shape* shape,
									   Style* style)
	: Command(),
	  fShape(shape),
	  fOldStyle(shape ? shape->Style() : NULL),
	  fNewStyle(style)
{
	if (fOldStyle)
		fOldStyle->Acquire();
	if (fNewStyle)
		fNewStyle->Acquire();
}

// destructor
AssignStyleCommand::~AssignStyleCommand()
{
	if (fOldStyle)
		fOldStyle->Release();
	if (fNewStyle)
		fNewStyle->Release();
}

// InitCheck
status_t
AssignStyleCommand::InitCheck()
{
	return fShape && fNewStyle ? B_OK : B_NO_INIT;
}

// Perform
status_t
AssignStyleCommand::Perform()
{
	fShape->SetStyle(fNewStyle);

	return B_OK;
}

// Undo
status_t
AssignStyleCommand::Undo()
{
	fShape->SetStyle(fOldStyle);

	return B_OK;
}

// GetName
void
AssignStyleCommand::GetName(BString& name)
{
	name << B_TRANSLATE("Assign Style");
	if (fNewStyle)
		name << " \"" << fNewStyle->Name() << "\"";
}

