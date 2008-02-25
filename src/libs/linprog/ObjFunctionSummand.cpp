/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#include "ObjFunctionSummand.h"
#include "Variable.h"
#include "LinearSpec.h"


/**
 * Sets the summmand's coefficient.
 * 
 * @param coeff	coefficient
 */
void
ObjFunctionSummand::SetCoeff(double coeff)
{
	Summand::SetCoeff(coeff);
	fLS->UpdateObjFunction();
}


/**
 * Sets the summand's variable.
 * 
 * @param var	variable
 */
void
ObjFunctionSummand::SetVar(Variable* var)
{
	Summand::SetVar(var);
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
	: Summand(coeff, var)
{
	fLS = ls;
}

