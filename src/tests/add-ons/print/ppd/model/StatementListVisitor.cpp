/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#include "StatementListVisitor.h"

void StatementListVisitor::Visit(StatementList* list)
{
	if (list == NULL) return;

	const int32 n = list->Size();
	for (int32 i = 0; i < n; i ++) {
		Statement* statement = list->StatementAt(i);
		GroupStatement group(statement);
		if (group.IsOpenGroup()) {
			BeginGroup(&group);
			fLevel ++;
		} else if (statement->IsValueStatement()) {
			DoValue(statement);
		} else if (statement->IsDefaultStatement()) {
			DoDefault(statement);
		} else if (statement->IsQueryStatement()) {
			DoQuery(statement);
		} else if (statement->IsParamStatement()) {
			DoParam(statement);
		}		
		
		StatementList* children = statement->GetChildren();
		if (children != NULL) {
			Visit(children);
		}
		
		// Close statements have been removed
		if (group.IsOpenGroup()) {
			fLevel --;
			EndGroup(&group);
		}
	}
}
