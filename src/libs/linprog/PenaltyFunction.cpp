/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#include "PenaltyFunction.h"
#include "Constraint.h"
#include "Summand.h"
#include "OperatorType.h"
#include "Variable.h"
#include "LinearSpec.h"


/**
 * Constructor.
 */
PenaltyFunction::PenaltyFunction(LinearSpec* ls, Variable* var, BList* xs, BList* gs)
{
	int32 sizeXs = xs->CountItems();
	int32 sizeGs = gs->CountItems();
	if (var->LS() != ls)
		printf("The variable must belong to the same linear specification as the penalty function.");
	if (sizeXs + 1 != sizeGs)
		printf("The number of sampling points must be exactly one less than the number of gradients.");
	for (int32 i = 1; i < sizeGs; i++) {
		if (*(double*)(gs->ItemAt(i - 1)) > *(double*)(gs->ItemAt(i)))
			printf("Penalty function must be concave.");
	}
	
	fVar = var;
	fXs = xs;
	fGs = gs;
	fConstraints = new BList(sizeGs + 1);
	fObjFunctionSummands = new BList(sizeGs);
		
	fConstraints->AddItem(ls->AddConstraint(1.0, var, OperatorType(EQ), 
		*(double*)(xs->ItemAt(0)), -*(double*)(gs->ItemAt(0)), 
		*(double*)(gs->ItemAt(1))));
	
	for (int32 i = 1; i < sizeGs; i++) {
		Variable* dPos = ls->AddVariable();
			
		fConstraints->AddItem(ls->AddConstraint(1.0, var, -1.0, dPos, OperatorType(LE), 
			*(double*)(xs->ItemAt(i))));
		
		Summand* objSummand =  new Summand(*(double*)(gs->ItemAt(i + 1)) - *(double*)(gs->ItemAt(i)), dPos);
		ls->ObjectiveFunction()->AddItem(objSummand);
		fObjFunctionSummands->AddItem(objSummand);
	}
	ls->UpdateObjectiveFunction();
}


/**
 * Destructor.
 * Removes all constraints and summands from the penalty function.
 */
PenaltyFunction::~PenaltyFunction()
{
	for (int32 i = 0; i < fConstraints->CountItems(); i++)
		delete (Constraint*)fConstraints->ItemAt(i);
	
	for (int32 i = 0; i < fObjFunctionSummands->CountItems(); i++)
		delete (Summand*)fObjFunctionSummands->ItemAt(i);
}

