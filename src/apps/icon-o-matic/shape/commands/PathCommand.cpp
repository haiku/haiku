/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "PathCommand.h"

#include <stdio.h>

#include "VectorPath.h"

// constructor
PathCommand::PathCommand(VectorPath* path)
	: fPath(path)
{
}

// destructor
PathCommand::~PathCommand()
{
}

// InitCheck
status_t
PathCommand::InitCheck()
{
	return fPath ? B_OK : B_NO_INIT;
}

// GetName
void
PathCommand::GetName(BString& name)
{

	name << ("<modify path>");
}

// _Select
void
PathCommand::_Select(const int32* indices, int32 count,
					 bool extend) const
{
	// TODO: ...
}
