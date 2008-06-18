/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#include "StatementList.h"
#include "Statement.h"

#include <stdio.h>

StatementList::StatementList(bool ownsStatements)
	: fOwnsStatements(ownsStatements)
{
}

StatementList::~StatementList()
{
	if (fOwnsStatements) {
		for (int32 i = 0; i < Size(); i ++) {
			Statement* statement = StatementAt(i);
			delete statement;
		}
	}
	fList.MakeEmpty();
}

void StatementList::Add(Statement* statement)
{
	fList.AddItem(statement);
}

void StatementList::Remove(Statement* statement)
{
	fList.RemoveItem(statement);
}

int32 StatementList::Size()
{
	return fList.CountItems();
}

Statement* StatementList::StatementAt(int32 index)
{
	return (Statement*)fList.ItemAt(index);
}

Statement* StatementList::GetStatement(const char* keyword)
{
	for (int32 i = 0; i < fList.CountItems(); i ++) {
		if (strcmp(keyword, StatementAt(i)->GetKeywordString()) == 0) {
			return StatementAt(i);
		}
	}
	return NULL;
}

const char* StatementList::GetValue(const char* keyword)
{
	Statement* statement = GetStatement(keyword);
	if (statement != NULL) {
		return statement->GetValueString();
	}
	return NULL;
}

void StatementList::Print()
{
	for (int32 i = 0; i < Size(); i ++) {
		StatementAt(i)->Print();
	}	
}
