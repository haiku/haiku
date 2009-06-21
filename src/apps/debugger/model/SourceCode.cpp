/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "SourceCode.h"

#include <stdlib.h>
#include <string.h>

#include "Statement.h"


SourceCode::SourceCode()
{
}


SourceCode::~SourceCode()
{
	for (int32 i = 0; char* line = fLines.ItemAt(i); i++)
		free(line);
	for (int32 i = 0; Statement* statement = fStatements.ItemAt(i); i++)
		statement->RemoveReference();
}


int32
SourceCode::CountLines() const
{
	return fLines.CountItems();
}


const char*
SourceCode::LineAt(int32 index) const
{
	return fLines.ItemAt(index);
}


int32
SourceCode::CountStatements() const
{
	return fStatements.CountItems();
}


Statement*
SourceCode::StatementAt(int32 index) const
{
	return fStatements.ItemAt(index);
}


Statement*
SourceCode::StatementAtAddress(target_addr_t address) const
{
	// TODO: Optimize!
	for (int32 i = 0; Statement* statement = fStatements.ItemAt(i); i++) {
		if (statement->ContainsAddress(address))
			return statement;
	}

	return NULL;
}


bool
SourceCode::AddLine(const char* _line)
{
	char* line = strdup(_line);
	if (line == NULL)
		return false;
	if (!fLines.AddItem(line)) {
		free(line);
		return false;
	}

	return true;
}


bool
SourceCode::AddStatement(Statement* statement)
{
	if (!fStatements.AddItem(statement)) {
		statement->RemoveReference();
		return false;
	}

	return true;
}
