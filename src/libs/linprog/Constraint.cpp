/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#include "Constraint.h"
#include "LinearSpec.h"
#include "Variable.h"

#include "lp_lib.h"


/**
 * Gets the index of the constraint.
 * 
 * @return the index of the constraint
 */
int32
Constraint::Index()
{
	int32 i = fLS->Constraints()->IndexOf(this);
	if (i == -1)
		printf("Constraint not part of fLS->Constraints().");
	return i + 1;
}


/**
 * Gets the summands of the constraint.
 * 
 * @return pointer to a BList containing the summands of the constraint
 */
BList*
Constraint::Summands()
{
	return fSummands;
}


void
Constraint::_UpdateLeftSide()
{
	double coeffs[fSummands->CountItems() + 2];
	int varIndexes[fSummands->CountItems() + 2];
	int32 i;
	for (i = 0; i < fSummands->CountItems(); i++) {
		Summand* s = (Summand*)fSummands->ItemAt(i);
		coeffs[i] = s->Coeff();
		varIndexes[i] = s->Var()->Index();
	}
	
	if (fDNegSummand != NULL && fOp != OperatorType(LE)) {
		varIndexes[i] = fDNegSummand->Var()->Index();
		coeffs[i] = 1.0;
		i++;
	}
	
	if (fDPosSummand != NULL && fOp != OperatorType(GE)) {
		varIndexes[i] = fDPosSummand->Var()->Index();
		coeffs[i] = -1.0;
		i++;
	}
	
	if (!set_rowex(fLS->LP(), this->Index(), i, &coeffs[0], &varIndexes[0]))
		printf("Error in set_rowex.");
	
	fLS->RemovePresolved();
}


/**
 * Changes the left side of the constraint, i.e.&nbsp;its coefficients and variables.
 * There must be exactly one coefficient for each variable.
 * 
 * @param summands	a BList containing the Summand objects that make up the new left side
 */
void
Constraint::ChangeLeftSide(BList* summands)
{
	fSummands = summands;
	_UpdateLeftSide();
}


void
Constraint::ChangeLeftSide(double coeff1, Variable* var1)
{
	fSummands->MakeEmpty();
	fSummands->AddItem(new Summand(coeff1, var1));
	_UpdateLeftSide();
}


void
Constraint::ChangeLeftSide(double coeff1, Variable* var1, 
	double coeff2, Variable* var2)
{
	fSummands->MakeEmpty();
	fSummands->AddItem(new Summand(coeff1, var1));
	fSummands->AddItem(new Summand(coeff2, var2));
	_UpdateLeftSide();
}

	
void
Constraint::ChangeLeftSide(double coeff1, Variable* var1, 
	double coeff2, Variable* var2,
	double coeff3, Variable* var3)
{
	fSummands->MakeEmpty();
	fSummands->AddItem(new Summand(coeff1, var1));
	fSummands->AddItem(new Summand(coeff2, var2));
	fSummands->AddItem(new Summand(coeff3, var3));
	_UpdateLeftSide();
}


void
Constraint::ChangeLeftSide(double coeff1, Variable* var1,
	double coeff2, Variable* var2,
	double coeff3, Variable* var3,
	double coeff4, Variable* var4)
{
	fSummands->MakeEmpty();
	fSummands->AddItem(new Summand(coeff1, var1));
	fSummands->AddItem(new Summand(coeff2, var2));
	fSummands->AddItem(new Summand(coeff3, var3));
	fSummands->AddItem(new Summand(coeff4, var4));
	_UpdateLeftSide();
}


/**
 * Gets the operator used for this constraint.
 * 
 * @return the operator used for this constraint
 */
OperatorType
Constraint::Op()
{
	return fOp;
}


/**
 * Sets the operator used for this constraint.
 * 
 * @param value	operator
 */
void
Constraint::SetOp(OperatorType value)
{
	fOp = value;
	if (!set_constr_type(fLS->LP(), this->Index(),
			((fOp == OperatorType(EQ)) ? EQ
			: (fOp == OperatorType(GE)) ? GE
			: LE)))
		printf("Error in set_constr_type.");
	
	fLS->RemovePresolved();
}

	
/**
 * Gets the constant value that is on the right side of the operator.
 * 
 * @return the constant value that is on the right side of the operator
 */
double
Constraint::RightSide()
{
	return fRightSide;
}

	
/**
 * Sets the constant value that is on the right side of the operator.
 * 
 * @param value	constant value that is on the right side of the operator
 */
void
Constraint::SetRightSide(double value)
{
	fRightSide = value;
	if (!set_rh(fLS->LP(), Index(), fRightSide))
		printf("Error in set_rh.");
	
	fLS->RemovePresolved();
}


/**
 * Gets the penalty coefficient for negative deviations.
 * 
 * @return the penalty coefficient
 */
double
Constraint::PenaltyNeg()
{
	if (fDNegSummand == NULL)
		return INFINITY;
	return fDNegSummand->Coeff();
}


/**
 * The penalty coefficient for negative deviations from the soft constraint's exact solution,&nbsp;
 * i.e. if the left side is too large.
 * 
 * @param value	coefficient of negative penalty <code>double</code>
 */
void
Constraint::SetPenaltyNeg(double value)
{
	if (fDNegSummand == NULL) {
		fDNegSummand = fLS->AddObjFunctionSummand(value, new Variable(fLS));
		ChangeLeftSide(fSummands);
		fLS->UpdateObjFunction();
		return;
	}
	
	if (value == fDNegSummand->Coeff())
		return;
	fDNegSummand->SetCoeff(value);
	fLS->UpdateObjFunction();
}


/**
 * Gets the penalty coefficient for positive deviations.
 * 
 * @return the penalty coefficient
 */
double
Constraint::PenaltyPos()
{
	if (fDPosSummand == NULL)
		return INFINITY;
	return fDPosSummand->Coeff();
}


/**
 * The penalty coefficient for negative deviations from the soft constraint's exact solution,
 * i.e. if the left side is too small.
 * 
 * @param value	coefficient of positive penalty <code>double</code>
 */
void
Constraint::SetPenaltyPos(double value)
{
	if (fDPosSummand == NULL) {
		fDPosSummand = fLS->AddObjFunctionSummand(value, new Variable(fLS));
		ChangeLeftSide(fSummands);
		fLS->UpdateObjFunction();
		return;
	}

	if (value == fDPosSummand->Coeff())
		return;
	fDPosSummand->SetCoeff(value);
	fLS->UpdateObjFunction();
}


/**
 * Gets the slack variable for the negative variations.
 * 
 * @return the slack variable for the negative variations
 */
Variable*
Constraint::DNeg() const
{
	if (fDNegSummand == NULL)
		return NULL;
	return fDNegSummand->Var();
}


/**
 * Gets the slack variable for the positive variations.
 * 
 * @return the slack variable for the positive variations
 */
Variable*
Constraint::DPos() const
{
	if (fDPosSummand == NULL)
		return NULL;
	return fDPosSummand->Var();
}


/**
 * Constructor.
 */
Constraint::Constraint(LinearSpec* ls, BList* summands, OperatorType op, 
	double rightSide, double penaltyNeg, double penaltyPos)
{
	fLS = ls;
	fSummands = summands;
	fOp = op;
	fRightSide = rightSide;
		
	double coeffs[summands->CountItems() + 2];
	int varIndexes[summands->CountItems() + 2];
	int32 i;
	for (i = 0; i < summands->CountItems(); i++) {
		Summand* s = (Summand*)summands->ItemAt(i);
		coeffs[i] = s->Coeff();
		varIndexes[i] = s->Var()->Index();
	}
	
	if (penaltyNeg != INFINITY
		&& fOp != OperatorType(LE)) {
		fDNegSummand = ls->AddObjFunctionSummand(penaltyNeg, new Variable(ls));
		varIndexes[i] = fDNegSummand->Var()->Index();
		coeffs[i] = 1.0;
		i++;
	}
	else
		fDNegSummand = NULL;
	
	if (penaltyPos != INFINITY
		&& fOp != OperatorType(GE)) {
		fDPosSummand = ls->AddObjFunctionSummand(penaltyPos, new Variable(ls));
		varIndexes[i] = fDPosSummand->Var()->Index();
		coeffs[i] = -1.0;
		i++;
	}
	else
		fDPosSummand = NULL;

	if (!add_constraintex(ls->LP(), i, &coeffs[0], &varIndexes[0],
			((fOp == OperatorType(EQ)) ? EQ
			: (fOp == OperatorType(GE)) ? GE
			: LE), rightSide))
		printf("Error in add_constraintex.");

	ls->Constraints()->AddItem(this);
}


/**
 * Destructor.
 * Removes the constraint from its specification.
 */
Constraint::~Constraint()
{
	del_constraint(fLS->LP(), Index());
	delete fSummands;
	if(fDNegSummand != NULL) {
		delete fDNegSummand->Var();
		delete fDNegSummand;
	}
	if(fDPosSummand != NULL) {
		delete fDPosSummand->Var();
		delete fDPosSummand;
	}
	fLS->Constraints()->RemoveItem(this);
}

