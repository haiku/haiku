/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "UnassignPathCommand.h"

#include "PathContainer.h"
#include "Shape.h"
#include "VectorPath.h"

// constructor
UnassignPathCommand::UnassignPathCommand(Shape* shape,
										 VectorPath* path)
	: Command(),
	  fShape(shape),
	  fPath(path),
	  fPathRemoved(false)
{
}

// destructor
UnassignPathCommand::~UnassignPathCommand()
{
	if (fPathRemoved && fPath)
		fPath->Release();
}

// InitCheck
status_t
UnassignPathCommand::InitCheck()
{
	return fShape && fPath ? B_OK : B_NO_INIT;
}

// Perform
status_t
UnassignPathCommand::Perform()
{
	// remove path from shape
	fShape->Paths()->RemovePath(fPath);
	fPathRemoved = true;

	return B_OK;
}

// Undo
status_t
UnassignPathCommand::Undo()
{
	// add path to shape
	fShape->Paths()->AddPath(fPath);
	fPathRemoved = false;

	return B_OK;
}

// GetName
void
UnassignPathCommand::GetName(BString& name)
{
	name << "Unassign Path";
}
