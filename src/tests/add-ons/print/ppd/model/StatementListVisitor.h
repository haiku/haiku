/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#ifndef _STATEMENT_LIST_VISITOR_H
#define _STATEMENT_LIST_VISITOR_H

#include "StatementWrapper.h"
#include "StatementList.h"

class StatementListVisitor {
private:
	int32 fLevel;
public:
	StatementListVisitor() : fLevel(0) {}
	virtual ~StatementListVisitor() {}
	
	virtual void Visit(StatementList* list);
	
	// the nesting level
	int32 GetLevel() const { return fLevel; }
	
	virtual void BeginGroup(GroupStatement* group) {};
	virtual void DoDefault(Statement* statement) {};
	virtual void DoQuery(Statement* statement) {};
	virtual void DoValue(Statement* statement) {};
	virtual void DoParam(Statement* statement) {};
	virtual void EndGroup(GroupStatement* group) {};
};


#endif
