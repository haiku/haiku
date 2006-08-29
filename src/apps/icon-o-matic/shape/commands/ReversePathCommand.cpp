#include "ReversePathCommand.h"

#include <stdio.h>

#include "VectorPath.h"

// constructor
ReversePathCommand::ReversePathCommand(VectorPath* path)
	: PathCommand(path)
{
}

// destructor
ReversePathCommand::~ReversePathCommand()
{
}

// Perform
status_t
ReversePathCommand::Perform()
{
	fPath->Reverse();

	return B_OK;
}

// Undo
status_t
ReversePathCommand::Undo()
{
	return Perform();
}

// GetName
void
ReversePathCommand::GetName(BString& name)
{
	name <<"Reverse Path";
}
