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


// Toggle debug output
//#define DEBUG_VARIABLE

#ifdef DEBUG_VARIABLE
#	define STRACE(x) debug_printf x
#else
#	define STRACE(x) ;
#endif


/**
 * Gets index of the variable.
 *
 * @return the index of the variable
 */
int32
Variable::Index() const
{
	int32 i = fLS->Variables()->IndexOf(this);
	if (i == -1) {
		printf("Variable not part of fLS->Variables().");
		return -1;
	}
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
	SetRange(min, fMax);
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
	SetRange(fMin, max);
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
	if (!fIsValid)
		return;

	fMin = min;
	fMax = max;
	set_bounds(fLS->fLP, this->Index(), fMin, fMax);
}


const char*
Variable::Label()
{
	return fLabel.String();
}


void
Variable::SetLabel(const char* label)
{
	fLabel = label;
}


Variable::operator BString() const
{
	BString string;
	GetString(string);
	return string;
}


/**
 * Returns index of the variable as String.
 * E.g. "Var2"
 *
 * @return the <code>String</code> index of the variable
 */
void
Variable::GetString(BString& string) const
{
	if (fLabel) {
		string << fLabel;
		if (!fIsValid)
			string << "(invalid)";
	} else {
		string << "Var";
		if (!fIsValid)
			string << "(invalid," << (int32)this << ")";
		else
			string << Index();
	}
}


/**
 * Adds a constraint that sets this variable equal to the given one.
 *
 * @param var	variable that should have the same value
 * @return the new equality constraint
 */
Constraint*
Variable::IsEqual(Variable* var)
{
	if (!fIsValid)
		return NULL;

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
	if (!fIsValid)
		return NULL;

	return fLS->AddConstraint(1.0, this, -1.0, var, OperatorType(LE), 0.0);
}


/**
 * Adds a constraint that sets this variable greater or equal to the given one.
 *
 * @param var	variable that should have a smaller or equal value
 * @return the new constraint
 */
Constraint*
Variable::IsGreaterOrEqual(Variable* var)
{
	if (!fIsValid)
		return NULL;

	return fLS->AddConstraint(-1.0, var, 1.0, this, OperatorType(GE), 0.0);
}


Constraint*
Variable::IsEqual(Variable* var, double penaltyNeg, double penaltyPos)
{
	if (!fIsValid)
		return NULL;

	return fLS->AddConstraint(1.0, this, -1.0, var, OperatorType(EQ), 0.0,
		penaltyNeg, penaltyPos);
}


Constraint*
Variable::IsSmallerOrEqual(Variable* var, double penaltyNeg, double penaltyPos)
{
	if (!fIsValid)
		return NULL;

	return fLS->AddConstraint(1.0, this, -1.0, var, OperatorType(LE), 0.0,
		penaltyNeg, penaltyPos);
}


Constraint*
Variable::IsGreaterOrEqual(Variable* var, double penaltyNeg, double penaltyPos)
{
	if (!fIsValid)
		return NULL;

	return fLS->AddConstraint(-1.0, var, 1.0, this, OperatorType(GE), 0.0,
		penaltyNeg, penaltyPos);
}


bool
Variable::IsValid()
{
	return fIsValid;
}


void
Variable::Invalidate()
{
	STRACE(("Variable::Invalidate() on %s\n", ToString()));

	if (!fIsValid)
		return;

	fIsValid = false;
	del_column(fLS->fLP, Index());
	fLS->Variables()->RemoveItem(this);

	// invalidate all constraints that use this variable
	BList markedForInvalidation;
	BList* constraints = fLS->Constraints();
	for (int i = 0; i < constraints->CountItems(); i++) {
		Constraint* constraint = static_cast<Constraint*>(
			constraints->ItemAt(i));

		if (!constraint->IsValid())
			continue;

		BList* summands = constraint->LeftSide();
		for (int j = 0; j < summands->CountItems(); j++) {
			Summand* summand = static_cast<Summand*>(summands->ItemAt(j));
			if (summand->Var() == this) {
				markedForInvalidation.AddItem(constraint);
				break;
			}
		}
	}
	for (int i = 0; i < markedForInvalidation.CountItems(); i++)
		static_cast<Constraint*>(markedForInvalidation.ItemAt(i))
			->Invalidate();
}


/**
 * Constructor.
 */
Variable::Variable(LinearSpec* ls)
	:
	fLS(ls),

	fValue(NAN),
	fMin(0),
	fMax(DBL_MAX),
	fLabel(NULL),
	fIsValid(true)
{
	fLS->Variables()->AddItem(this);

	if (fLS->Variables()->CountItems() > fLS->CountColumns()) {
		double d = 0;
		int i = 0;
		if (!add_columnex(fLS->fLP, 0, &d, &i))
			printf("Error in add_columnex.");
	}

	SetRange(-20000, 20000);
}


/**
 * Destructor.
 * Removes the variable from its specification.
 */
Variable::~Variable()
{
	Invalidate();
}

