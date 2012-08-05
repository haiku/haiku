/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "Variable.h"

#include <float.h>	// for DBL_MAX

#include <File.h>

#include "Constraint.h"
#include "LinearSpec.h"

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
	return fLS->IndexOf(this);
}


int32
Variable::GlobalIndex() const
{
	return fLS->GlobalIndexOf(this);
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
	fMin = min;
	fMax = max;
	if (fIsValid)
		fLS->UpdateRange(this);
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


/**
 * Returns index of the variable as String.
 * E.g. "Var2"
 *
 * @return the <code>String</code> index of the variable
 */
BString
Variable::ToString() const
{
	BString string;
	if (fLabel) {
		string << fLabel;
		if (!fIsValid)
			string << "(invalid)";
	} else {
		string << "Variable ";
		if (!fIsValid)
			string << "(invalid," << (addr_t)this << ")";
		else
			string << Index();
	}
	return string;
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

	return fLS->AddConstraint(1.0, this, -1.0, var, kEQ, 0.0);
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

	return fLS->AddConstraint(1.0, this, -1.0, var, kLE, 0.0);
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

	return fLS->AddConstraint(-1.0, var, 1.0, this, kGE, 0.0);
}


Constraint*
Variable::IsEqual(Variable* var, double penaltyNeg, double penaltyPos)
{
	if (!fIsValid)
		return NULL;

	return fLS->AddConstraint(1.0, this, -1.0, var, kEQ, 0.0,
		penaltyNeg, penaltyPos);
}


Constraint*
Variable::IsSmallerOrEqual(Variable* var, double penaltyNeg, double penaltyPos)
{
	if (!fIsValid)
		return NULL;

	return fLS->AddConstraint(1.0, this, -1.0, var, kLE, 0.0, penaltyNeg,
		penaltyPos);
}


Constraint*
Variable::IsGreaterOrEqual(Variable* var, double penaltyNeg, double penaltyPos)
{
	if (!fIsValid)
		return NULL;

	return fLS->AddConstraint(-1.0, var, 1.0, this, kGE, 0.0, penaltyNeg,
		penaltyPos);
}


bool
Variable::IsValid()
{
	return fIsValid;
}


void
Variable::Invalidate()
{
	STRACE(("Variable::Invalidate() on %s\n", BString(*this).String()));

	if (!fIsValid)
		return;

	fIsValid = false;
	fLS->RemoveVariable(this, false);
}


/**
 * Constructor.
 */
Variable::Variable(LinearSpec* ls)
	:
	fLS(ls),
	fValue(NAN),
	fMin(-20000),
	fMax(20000),
	fLabel(NULL),
	fIsValid(false),
	fReferenceCount(0)
{

}


int32
Variable::AddReference()
{
	fReferenceCount++;
	return fReferenceCount;
}


int32
Variable::RemoveReference()
{
	fReferenceCount--;
	return fReferenceCount;
}


/**
 * Destructor.
 * Removes the variable from its specification.
 */
Variable::~Variable()
{
	if (fLS)
		fLS->RemoveVariable(this, false);
}

