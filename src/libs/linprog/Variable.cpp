/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#include "Variable.h"
#include "Constraint.h"
#include "LinearSpec.h"
#include "OperatorType.h"

#include "lp_lib.h"

#include <float.h>	// for DBL_MAX


/**
 * Gets index of the variable.
 * 
 * @return the index of the variable
 */
int32
Variable::Index()
{
	int32 i = fLS->Variables()->IndexOf(this);
	if (i == -1)
		printf("Variable not part of fLS->Variables().");
	return i + 1;
}


/**
 * Gets the current linear specification.
 * 
 * @return the current linear specification
 */
LinearSpec*
Variable::LS() const
{
	return fLS;
}


/**
 * Sets the current linear specification.
 * 
 * @param value	the current linear specification
 */
void
Variable::SetLS(LinearSpec* value)
{
	fLS = value;
}


/**
 * Gets the value.
 * 
 * @return the value
 */
double
Variable::Value() const
{
	return fValue;
}


/**
 * Sets the value.
 * 
 * @param value	the value
 */
void
Variable::SetValue(double value)
{
	fValue = value;
}


/**
 * Gets the minimum value of the variable.
 * 
 * @return the minimum value of variable
 */
double
Variable::Min() const
{
	return fMin;
}


/**
 * Sets the minimum value of the variable.
 * 
 * @param min	minimum value
 */
void
Variable::SetMin(double min)
{
	fMin = min;
	set_bounds(fLS->fLP, this->Index(), fMin, fMax);
}


/**
 * Gets the maximum value of the variable.
 * 
 * @return the maximum value of variable
 */
double
Variable::Max() const
{
	return fMax;
}


/**
 * Sets the maximum value of the variable.
 * 
 * @param max	maximum value
 */
void
Variable::SetMax(double max)
{
	fMax = max;
	set_bounds(fLS->fLP, this->Index(), fMin, fMax);
}


/**
 * Sets the minimum and maximum values of the variable.
 * 
 * @param min	minimum value
 * @param max	maximum value
 */
void
Variable::SetRange(double min, double max)
{
	fMin = min;
	fMax = max;
	set_bounds(fLS->fLP, this->Index(), fMin, fMax);
}


/**
 * Returns index of the variable as String.
 * E.g. "Var2"
 * 
 * @return the <code>String</code> index of the variable
 */
//~ string Variable::ToString() {
	//~ return "Var" + Index();
//~ }


/**
 * Adds a constraint that sets this variable equal to the given one.
 * 
 * @param var	variable that should have the same value
 * @return the new equality constraint
 */
Constraint*
Variable::IsEqual(Variable* var)
{
	return fLS->AddConstraint(1.0, this, -1.0, var, OperatorType(EQ), 0.0);
}


/**
 * Adds a constraint that sets this variable smaller or equal to the given one.
 * 
 * @param var	variable that should have a larger or equal value
 * @return the new constraint
 */
Constraint*
Variable::IsSmallerOrEqual(Variable* var)
{
	return fLS->AddConstraint(1.0, this, -1.0, var, OperatorType(LE), 0.0);
}


/**
 * Adds a constraint that sets this variable greater or equal to the given one.
 * 
 * @param var	variable that should have a smaller or equal value
 * @return the new constraint
 */
Constraint*
Variable::IsGreaterorEqual(Variable* var)
{
	return fLS->AddConstraint(-1.0, var, 1.0, this, OperatorType(GE), 0.0);
}


/**
 * Constructor.
 */
Variable::Variable(LinearSpec* ls)
{
	fMin = 0;
	fMax = DBL_MAX;
	fValue = NULL;
	fLS = ls;
	
	ls->Variables()->AddItem(this);
	
	if (ls->Variables()->CountItems() > ls->CountColumns()) {
		double d = 0;
		int i = 0;
		if (!add_columnex(ls->fLP, 0, &d, &i))
			printf("Error in add_columnex.");
	}
}


/**
 * Destructor.
 * Removes the variable from its specification.
 */
Variable::~Variable()
{
	del_column(fLS->fLP, this->Index());
	fLS->Variables()->RemoveItem(this);
}

