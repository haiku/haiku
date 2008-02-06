#include "PenaltyFunction.h"
#include "Constraint.h"
#include "ObjFunctionSummand.h"
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
	fConstraints = new BList(1);
	fObjFunctionSummands = new BList(1);
	
	BList* coeffs = new BList(1);
	coeffs->AddItem(new double(1.0));
	
	BList* vars = new BList(1);
	vars->AddItem(var);
	
	fConstraints->AddItem(ls->AddSoftConstraint(coeffs, vars, OperatorType(EQ), 
		*(double*)(xs->ItemAt(0)), -*(double*)(gs->ItemAt(0)), 
		*(double*)(gs->ItemAt(1))));
	
	for (int32 i = 1; i < sizeGs; i++) {
		Variable* dPos = ls->AddVariable();
		
		BList* coeffs = new BList(2);
		coeffs->AddItem(new double(1.0));
		coeffs->AddItem(new double(-1.0));
		
		BList* vars = new BList(2);
		vars->AddItem(var);
		vars->AddItem(dPos);
		
		fConstraints->AddItem(ls->AddConstraint(coeffs, vars, OperatorType(LE), 
			*(double*)(xs->ItemAt(i))));
		
		fObjFunctionSummands->AddItem(ls->AddObjFunctionSummand(
			*(double*)(gs->ItemAt(i + 1)) - *(double*)(gs->ItemAt(i)), dPos));
	}
}


/**
 * Destructor.
 * Removes all constraints and summands from the penalty function.
 */
PenaltyFunction::~PenaltyFunction()
{
	int32 sizeConstraints = fConstraints->CountItems();
	for (int32 i = 0; i < sizeConstraints; i++) {
		Constraint* currentConstraint = (Constraint*)fConstraints->ItemAt(i);
		delete currentConstraint;
	}
	
	int32 sizeObjFunctionSummands = fObjFunctionSummands->CountItems();
	for (int32 i = 0; i < sizeObjFunctionSummands; i++) {
		ObjFunctionSummand* currentObjFunctionSummand = 
			(ObjFunctionSummand*)fObjFunctionSummands->ItemAt(i);
		delete currentObjFunctionSummand;
	}
}


/**
 * Gets the variable.
 * 
 * @return the variable
 */
const Variable*
PenaltyFunction::Var() const
{
	return fVar;
}


/**
 * Gets the sampling points.
 * 
 * @return the sampling points
 */
const BList*
PenaltyFunction::Xs() const
{
	return fXs;
}


/**
 * Gets the sampling gradients.
 * 
 * @return the sampling gradients
 */
const
BList* PenaltyFunction::Gs() const
{
	return fGs;
}

