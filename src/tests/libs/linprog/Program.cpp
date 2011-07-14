/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#include <List.h>
#include <SupportDefs.h>

#include "LinearSpec.h"

#include <stdio.h>


using namespace LinearProgramming;


void
PrintResults(const VariableList& list)
{
	for (int32 i = 0; i < list.CountItems(); i++)
		printf("Variable %i = %f\n", (int)i, list.ItemAt(i)->Value());
}


void
Test1()
{
	LinearSpec ls;
	Variable* x1 = ls.AddVariable();
	Variable* x2 = ls.AddVariable();
	
	Constraint* c1 = ls.AddConstraint(1.0, x1, OperatorType(kLE), 108);	
	Constraint* c2 = ls.AddConstraint(1.0, x2, OperatorType(kGE), 113);
	
	ls.Solve();
	printf("ls: %s\n", ls.ToString().String());
	
	ls.RemoveConstraint(c2);
	ls.Solve();
	printf("ls: %s\n", ls.ToString().String());

	c2 = ls.AddConstraint(1.0, x2, OperatorType(kGE), 113);
	ls.Solve();
	printf("ls: %s\n", ls.ToString().String());
}


void
SoftConstraints()
{
	printf("SoftConstraints\n\n");

	LinearSpec ls;
	Variable* x1 = ls.AddVariable();
	x1->SetLabel("label_x1");
	Variable* x2 = ls.AddVariable();
	x2->SetLabel("label_x2");
	Variable* x3 = ls.AddVariable();
	x3->SetLabel("label_x3");

	Constraint* c1 = ls.AddConstraint(1, x1, kEQ, 0);
	Constraint* c2 = ls.AddConstraint(1, x1, -1, x2, kLE, 0);
	Constraint* c3 = ls.AddConstraint(1, x2, -1, x3, kLE, 0);
	Constraint* c4 = ls.AddConstraint(1, x3, -1, x1, kEQ, 20);
	
	Constraint* c5 = ls.AddConstraint(1, x2, -1, x1, kEQ, 10, 5, 5);
	Constraint* c6 = ls.AddConstraint(1, x3, -1, x2, kEQ, 5, 5, 5);

	ls.Solve();
	printf("ls: %s\n", ls.ToString().String());
	PrintResults(ls.AllVariables());

	ls.RemoveConstraint(c6);
	ls.Solve();
	printf("ls: %s\n", ls.ToString().String());
	PrintResults(ls.UsedVariables());
}


void
SoftInequalityConstraints()
{
	printf("SoftInequalityConstraints\n\n");

	LinearSpec ls;
	Variable* x1 = ls.AddVariable();
	x1->SetLabel("label_x1");
	Variable* x2 = ls.AddVariable();
	x2->SetLabel("label_x2");
	Variable* x3 = ls.AddVariable();
	x3->SetLabel("label_x3");

	Constraint* c1 = ls.AddConstraint(1, x1, kEQ, 0);
	Constraint* c2 = ls.AddConstraint(1, x1, -1, x2, kLE, 0);
	Constraint* c3 = ls.AddConstraint(1, x2, -1, x3, kLE, 0);
	Constraint* c4 = ls.AddConstraint(1, x3, -1, x1, kEQ, 20);
	
	Constraint* c5 = ls.AddConstraint(1, x2, -1, x1, kEQ, 15, 5, 5);
	Constraint* c6 = ls.AddConstraint(1, x3, -1, x2, kGE, 10, 5000, 5);

	ls.Solve();
	printf("ls: %s\n", ls.ToString().String());
	PrintResults(ls.AllVariables());

	ls.RemoveConstraint(c6);
	ls.Solve();
	printf("ls: %s\n", ls.ToString().String());
	PrintResults(ls.UsedVariables());
}


int
main()
{
	Test1();
	SoftConstraints();
	SoftInequalityConstraints();	
}

