#include "ObjFunctionSummand.h"
#include "Variable.h"
#include "LinearSpec.h"


/**
 * Gets the summmand's coefficient.
 * 
 * @return the summand's coefficient
 */
double
ObjFunctionSummand::Coeff()
{
	return fCoeff;
}


/**
 * Sets the summmand's coefficient.
 * 
 * @param coeff	coefficient
 */
void
ObjFunctionSummand::SetCoeff(double coeff)
{
	fCoeff = coeff;
	fLS->UpdateObjFunction();
}


/**
 * Gets the summand's variable.
 * 
 * @return the summand's variable
 */
Variable*
ObjFunctionSummand::Var()
{
	return fVar;
}


/**
 * Sets the summand's variable.
 * 
 * @param var	variable
 */
void
ObjFunctionSummand::SetVar(Variable* var)
{
	fVar = var;
	fLS->UpdateObjFunction();
}


/**
 * Destructor.
 * Removes the summand from the objective function.
 */
ObjFunctionSummand::~ObjFunctionSummand()
{
	fLS->ObjFunctionSummands()->RemoveItem(this);
	fLS->UpdateObjFunction();
}


/**
 * Constructor.
 */
ObjFunctionSummand::ObjFunctionSummand(LinearSpec* ls, double coeff, Variable* var)
{
	fLS = ls;
	fCoeff = coeff;
	fVar = var;
}

