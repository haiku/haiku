/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#ifndef _STATEMENT_LIST_H
#define _STATEMENT_LIST_H

#include <List.h>

class Statement;

class StatementList {
private:
	BList fList;
	bool  fOwnsStatements;
	
public:
	StatementList(bool ownsStatements);
	~StatementList();
	
	void Add(Statement* statement);
	void Remove(Statement* statement);
	int32 Size();
	Statement* StatementAt(int32 index);

	Statement* GetStatement(const char* keyword);
	const char* GetValue(const char* keyword);
	
	void Print();
};

#endif
