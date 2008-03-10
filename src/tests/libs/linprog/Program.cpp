/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#include <List.h>
#include <SupportDefs.h>

#include "LinearSpec.h"

#include <stdio.h>


void
PrintVars(LinearSpec* ls)
{
	int32 size = ls->Variables()->CountItems();
	Variable* variable;
	for (int i = 0; i < size; i++) {
		variable = (Variable*)ls->Variables()->ItemAt(i);
		printf("%f ", variable->Value());
	}
	printf("%d\n", ls->Result());
}


void
Test1()
{
	LinearSpec* ls = new LinearSpec();
	Variable* x1 = ls->AddVariable();
	Variable* x2 = ls->AddVariable();
	
	Constraint* c1 = ls->AddConstraint(1.0, x1, OperatorType(LE), 108);	
	Constraint* c2 = ls->AddConstraint(1.0, x2, OperatorType(GE), 113);
	
	BList* summands = new BList(2);
	summands->AddItem(new Summand(1.0, x1));
	summands->AddItem(new Summand(1.0, x2));
	ls->SetObjFunction(summands);
	
	ls->Solve();
	PrintVars(ls);
	
	delete c2;
	ls->Solve();
	PrintVars(ls);
	
	c2 = ls->AddConstraint(1.0, x2, OperatorType(GE), 113);
	ls->Solve();
	PrintVars(ls);
}


int
main()
{
	Test1();
}

