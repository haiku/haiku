/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "CompoundCommand.h"

#include <stdio.h>

// constructor
CompoundCommand::CompoundCommand(Command** commands,
								 int32 count,
								 const char* name,
								 int32 nameIndex)
	: Command(),
	  fCommands(commands),
	  fCount(count),
	  fName(name),
	  fNameIndex(nameIndex)
{
}

// destructor
CompoundCommand::~CompoundCommand()
{
	for (int32 i = 0; i < fCount; i++)
		delete fCommands[i];
	delete[] fCommands;
}

// InitCheck
status_t
CompoundCommand::InitCheck()
{
	status_t status = fCommands && fCount > 0 ? B_OK : B_BAD_VALUE;
	return status;
}

// Perform
status_t
CompoundCommand::Perform()
{
	status_t status = InitCheck();
	if (status >= B_OK) {
		int32 i = 0;
		for (; i < fCount; i++) {
			if (fCommands[i])
				status = fCommands[i]->Perform();
			if (status < B_OK)
				break;
		}
/*		if (status < B_OK) {
			// roll back
			i--;
			for (; i >= 0; i--) {
				if (fCommands[i])
					fCommands[i]->Undo();
			}
		}*/
	}
	return status;
}

// Undo
status_t
CompoundCommand::Undo()
{
	status_t status = InitCheck();
	if (status >= B_OK) {
		int32 i = fCount - 1;
		for (; i >= 0; i--) {
			if (fCommands[i])
				status = fCommands[i]->Undo();
			if (status < B_OK)
				break;
		}
	}
	return status;
}

// Redo
status_t
CompoundCommand::Redo()
{
	return Perform();
}

// GetName
void
CompoundCommand::GetName(BString& name)
{
	name << _GetString(fNameIndex, fName.String());
}
