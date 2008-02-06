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
	set_bounds(fLS->LP(), this->Index(), fMin, fMax);
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
	set_bounds(fLS->LP(), this->Index(), fMin, fMax);
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
	set_bounds(fLS->LP(), this->Index(), fMin, fMax);
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
	BList* coeffs = new BList(2);
	coeffs->AddItem(new double(1.0));
	coeffs->AddItem(new double(-1.0));
	
	BList* vars = new BList(2);
	vars->AddItem(this);
	vars->AddItem(var);
	
	return fLS->AddConstraint(coeffs, vars, OperatorType(EQ), 0.0);
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
	BList* coeffs = new BList(2);
	coeffs->AddItem(new double(1.0));
	coeffs->AddItem(new double(-1.0));
	
	BList* vars = new BList(2);
	vars->AddItem(this);
	vars->AddItem(var);
	
	return fLS->AddConstraint(coeffs, vars, OperatorType(LE), 0.0);
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
	BList* coeffs = new BList(2);
	coeffs->AddItem(new double(-1.0));
	coeffs->AddItem(new double(1.0));
	
	BList* vars = new BList(2);
	vars->AddItem(var);
	vars->AddItem(this);
	
	return fLS->AddConstraint(coeffs, vars, OperatorType(GE), 0.0);
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
	
	int32 size = ls->Variables()->CountItems();
	if (size > ls->Columns()) {
		double d = 0;
		int i = 0;
		if (!add_columnex(ls->LP(), 0, &d, &i))
			printf("Error in add_columnex.");
	}
}


/**
 * Destructor.
 * Removes the variable from its specification.
 */
Variable::~Variable()
{
	del_column(fLS->LP(), this->Index());
	fLS->Variables()->RemoveItem(this);
}

