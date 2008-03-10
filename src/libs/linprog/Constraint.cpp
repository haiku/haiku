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
 * Gets the left side of the constraint.
 * 
 * @return pointer to a BList containing the summands on the left side of the constraint
 */
BList*
Constraint::LeftSide()
{
	return fLeftSide;
}


/**
 * Sets the summands on the left side of the constraint.
 * The old summands are NOT deleted.
 * 
 * @param summands	a BList containing the Summand objects that make up the new left side
 */
void
Constraint::SetLeftSide(BList* summands)
{
	fLeftSide = summands;
	UpdateLeftSide();
}


void
Constraint::UpdateLeftSide()
{
	double coeffs[fLeftSide->CountItems() + 2];
	int varIndexes[fLeftSide->CountItems() + 2];
	int32 i;
	for (i = 0; i < fLeftSide->CountItems(); i++) {
		Summand* s = (Summand*)fLeftSide->ItemAt(i);
		coeffs[i] = s->Coeff();
		varIndexes[i] = s->Var()->Index();
	}
	
	if (fDNegObjSummand != NULL && fOp != OperatorType(LE)) {
		varIndexes[i] = fDNegObjSummand->Var()->Index();
		coeffs[i] = 1.0;
		i++;
	}
	
	if (fDPosObjSummand != NULL && fOp != OperatorType(GE)) {
		varIndexes[i] = fDPosObjSummand->Var()->Index();
		coeffs[i] = -1.0;
		i++;
	}
	
	if (!set_rowex(fLS->fLP, this->Index(), i, &coeffs[0], &varIndexes[0]))
		printf("Error in set_rowex.");
	
	fLS->UpdateObjFunction();
	fLS->RemovePresolved();
}


void
Constraint::SetLeftSide(double coeff1, Variable* var1)
{
	for (int i=0; i<fLeftSide->CountItems(); i++)
		delete (Summand*)fLeftSide->ItemAt(i);
	fLeftSide->MakeEmpty();
	fLeftSide->AddItem(new Summand(coeff1, var1));
	UpdateLeftSide();
}


void
Constraint::SetLeftSide(double coeff1, Variable* var1, 
	double coeff2, Variable* var2)
{
	for (int i=0; i<fLeftSide->CountItems(); i++)
		delete (Summand*)fLeftSide->ItemAt(i);
	fLeftSide->MakeEmpty();
	fLeftSide->AddItem(new Summand(coeff1, var1));
	fLeftSide->AddItem(new Summand(coeff2, var2));
	UpdateLeftSide();
}

	
void
Constraint::SetLeftSide(double coeff1, Variable* var1, 
	double coeff2, Variable* var2,
	double coeff3, Variable* var3)
{
	for (int i=0; i<fLeftSide->CountItems(); i++)
		delete (Summand*)fLeftSide->ItemAt(i);
	fLeftSide->MakeEmpty();
	fLeftSide->AddItem(new Summand(coeff1, var1));
	fLeftSide->AddItem(new Summand(coeff2, var2));
	fLeftSide->AddItem(new Summand(coeff3, var3));
	UpdateLeftSide();
}


void
Constraint::SetLeftSide(double coeff1, Variable* var1,
	double coeff2, Variable* var2,
	double coeff3, Variable* var3,
	double coeff4, Variable* var4)
{
	for (int i=0; i<fLeftSide->CountItems(); i++)
		delete (Summand*)fLeftSide->ItemAt(i);
	fLeftSide->MakeEmpty();
	fLeftSide->AddItem(new Summand(coeff1, var1));
	fLeftSide->AddItem(new Summand(coeff2, var2));
	fLeftSide->AddItem(new Summand(coeff3, var3));
	fLeftSide->AddItem(new Summand(coeff4, var4));
	UpdateLeftSide();
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
	if (!set_constr_type(fLS->fLP, this->Index(),
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
	if (!set_rh(fLS->fLP, Index(), fRightSide))
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
	if (fDNegObjSummand == NULL)
		return INFINITY;
	return fDNegObjSummand->Coeff();
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
	if (fDNegObjSummand == NULL) {
		fDNegObjSummand = new Summand(value, new Variable(fLS));
		fLS->ObjFunction()->AddItem(fDNegObjSummand);
		UpdateLeftSide();
		fLS->UpdateObjFunction();
		return;
	}
		
	if (value == fDNegObjSummand->Coeff())
		return;

	fDNegObjSummand->SetCoeff(value);
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
	if (fDPosObjSummand == NULL)
		return INFINITY;
	return fDPosObjSummand->Coeff();
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
	if (fDPosObjSummand == NULL) {
		fDPosObjSummand = new Summand(value, new Variable(fLS));
		fLS->ObjFunction()->AddItem(fDPosObjSummand);
		UpdateLeftSide();
		fLS->UpdateObjFunction();
		return;
	}

	if (value == fDPosObjSummand->Coeff())
		return;

	fDPosObjSummand->SetCoeff(value);
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
	if (fDNegObjSummand == NULL)
		return NULL;
	return fDNegObjSummand->Var();
}


/**
 * Gets the slack variable for the positive variations.
 * 
 * @return the slack variable for the positive variations
 */
Variable*
Constraint::DPos() const
{
	if (fDPosObjSummand == NULL)
		return NULL;
	return fDPosObjSummand->Var();
}


/**
 * Constructor.
 */
Constraint::Constraint(LinearSpec* ls, BList* summands, OperatorType op, 
	double rightSide, double penaltyNeg, double penaltyPos)
{
	fLS = ls;
	fLeftSide = summands;
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
		fDNegObjSummand = new Summand(penaltyNeg, new Variable(ls));
		ls->fObjFunction->AddItem(fDNegObjSummand);
		varIndexes[i] = fDNegObjSummand->Var()->Index();
		coeffs[i] = 1.0;
		i++;
	}
	else
		fDNegObjSummand = NULL;
	
	if (penaltyPos != INFINITY
		&& fOp != OperatorType(GE)) {
		fDPosObjSummand = new Summand(penaltyPos, new Variable(ls));
		ls->fObjFunction->AddItem(fDPosObjSummand);
		varIndexes[i] = fDPosObjSummand->Var()->Index();
		coeffs[i] = -1.0;
		i++;
	}
	else
		fDPosObjSummand = NULL;

	if (!add_constraintex(ls->fLP, i, &coeffs[0], &varIndexes[0],
			((fOp == OperatorType(EQ)) ? EQ
			: (fOp == OperatorType(GE)) ? GE
			: LE), rightSide))
		printf("Error in add_constraintex.");

	fLS->UpdateObjFunction();
	ls->Constraints()->AddItem(this);
}


/**
 * Destructor.
 * Removes the constraint from its specification and deletes all the summands.
 */
Constraint::~Constraint()
{
	for (int i=0; i<fLeftSide->CountItems(); i++)
		delete (Summand*)fLeftSide->ItemAt(i);
	delete fLeftSide;
	
	if (fDNegObjSummand != NULL) {
		delete fDNegObjSummand->Var();
		delete fDNegObjSummand;
	}
	if (fDPosObjSummand != NULL) {
		delete fDPosObjSummand->Var();
		delete fDPosObjSummand;
	}

	del_constraint(fLS->fLP, Index());	
	fLS->Constraints()->RemoveItem(this);
}

