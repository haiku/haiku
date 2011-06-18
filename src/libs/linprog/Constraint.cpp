/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "Constraint.h"

#include <new>
#include <stdio.h>

#include "LinearSpec.h"
#include "Variable.h"


// Toggle debug output
//#define DEBUG_CONSTRAINT

#ifdef DEBUG_CONSTRAINT
#	define STRACE(x) debug_printf x
#else
#	define STRACE(x) ;
#endif


/**
 * Gets the index of the constraint.
 *
 * @return the index of the constraint
 */
int32
Constraint::Index() const
{
	int32 i = fLS->Constraints().IndexOf(this);
	if (i == -1)
		STRACE(("Constraint not part of fLS->Constraints()."));

	return i;
}


/**
 * Gets the left side of the constraint.
 *
 * @return pointer to a BList containing the summands on the left side of the constraint
 */
SummandList*
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
Constraint::SetLeftSide(SummandList* summands)
{
	if (!fIsValid)
		return;

	// check left side
	for (int32 i = 0; i < summands->CountItems(); i++) {
		Summand* summand = summands->ItemAt(i);
		for (int32 a = i + 1; a < summands->CountItems(); a++) {
			Summand* nextSummand = summands->ItemAt(a);
			if (summand->Var() == nextSummand->Var()) {
				summand->SetCoeff(summand->Coeff() + nextSummand->Coeff());
				summands->RemoveItem(nextSummand);
				delete nextSummand;
				a--;	
			}
		}
	}

	fLeftSide = summands;
	fLS->UpdateLeftSide(this);
}


void
Constraint::SetLeftSide(double coeff1, Variable* var1)
{
	if (!fIsValid)
		return;

	for (int i=0; i<fLeftSide->CountItems(); i++)
		delete fLeftSide->ItemAt(i);
	fLeftSide->MakeEmpty();
	fLeftSide->AddItem(new(std::nothrow) Summand(coeff1, var1));
	SetLeftSide(fLeftSide);
}


void
Constraint::SetLeftSide(double coeff1, Variable* var1,
	double coeff2, Variable* var2)
{
	if (!fIsValid)
		return;

	for (int i=0; i<fLeftSide->CountItems(); i++)
		delete fLeftSide->ItemAt(i);
	fLeftSide->MakeEmpty();
	fLeftSide->AddItem(new(std::nothrow) Summand(coeff1, var1));
	fLeftSide->AddItem(new(std::nothrow) Summand(coeff2, var2));
	SetLeftSide(fLeftSide);
}


void
Constraint::SetLeftSide(double coeff1, Variable* var1,
	double coeff2, Variable* var2,
	double coeff3, Variable* var3)
{
	if (!fIsValid)
		return;

	for (int i=0; i<fLeftSide->CountItems(); i++)
		delete fLeftSide->ItemAt(i);
	fLeftSide->MakeEmpty();
	fLeftSide->AddItem(new(std::nothrow) Summand(coeff1, var1));
	fLeftSide->AddItem(new(std::nothrow) Summand(coeff2, var2));
	fLeftSide->AddItem(new(std::nothrow) Summand(coeff3, var3));
	SetLeftSide(fLeftSide);
}


void
Constraint::SetLeftSide(double coeff1, Variable* var1,
	double coeff2, Variable* var2,
	double coeff3, Variable* var3,
	double coeff4, Variable* var4)
{
	if (!fIsValid)
		return;

	for (int i=0; i<fLeftSide->CountItems(); i++)
		delete fLeftSide->ItemAt(i);
	fLeftSide->MakeEmpty();
	fLeftSide->AddItem(new(std::nothrow) Summand(coeff1, var1));
	fLeftSide->AddItem(new(std::nothrow) Summand(coeff2, var2));
	fLeftSide->AddItem(new(std::nothrow) Summand(coeff3, var3));
	fLeftSide->AddItem(new(std::nothrow) Summand(coeff4, var4));
	SetLeftSide(fLeftSide);
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
	if (!fIsValid)
		return;

	fOp = value;
	fLS->UpdateOperator(this);
}


/**
 * Gets the constant value that is on the right side of the operator.
 *
 * @return the constant value that is on the right side of the operator
 */
double
Constraint::RightSide() const
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
	if (!fIsValid)
		return;

	if (fRightSide == value)
		return;

	fRightSide = value;

	fLS->UpdateRightSide(this);
}


/**
 * Gets the penalty coefficient for negative deviations.
 *
 * @return the penalty coefficient
 */
double
Constraint::PenaltyNeg() const
{
	return fPenaltyNeg;
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
	fPenaltyNeg = value;

	fLS->UpdateLeftSide(this);
}


/**
 * Gets the penalty coefficient for positive deviations.
 *
 * @return the penalty coefficient
 */
double
Constraint::PenaltyPos() const
{
	return fPenaltyPos;
}


/**
 * The penalty coefficient for negative deviations from the soft constraint's
 * exact solution, i.e. if the left side is too small.
 * @param value	coefficient of positive penalty <code>double</code>
 */
void
Constraint::SetPenaltyPos(double value)
{
	fPenaltyPos = value;

	fLS->UpdateLeftSide(this);
}


const char*
Constraint::Label()
{
	return fLabel.String();
}


void
Constraint::SetLabel(const char* label)
{
	fLabel = label;
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


bool
Constraint::IsSoft() const
{
	if (fOp != kEQ)
		return false;
    if (fPenaltyNeg > 0. || fPenaltyPos > 0.)
		return true;
	return false;
}


bool
Constraint::IsValid()
{
	return fIsValid;
}


void
Constraint::Invalidate()
{
	STRACE(("Constraint::Invalidate() on %d\n", this));

	if (!fIsValid)
		return;

	fIsValid = false;
	fLS->RemoveConstraint(this, false);
}


BString
Constraint::ToString() const
{
	BString string;
	string << "Constraint ";
	string << fLabel;
	string << "(" << (int32)this << "): ";

	if (fIsValid) {
		for (int i = 0; i < fLeftSide->CountItems(); i++) {
			Summand* s = static_cast<Summand*>(fLeftSide->ItemAt(i));
			if (i != 0 && s->Coeff() >= 0)
				string << " + ";
			string << (float)s->Coeff() << "*";
			string << "x";
			string << s->Var()->Index();
			string << " ";
		}
		string << ((fOp == kEQ) ? "== "
			: (fOp == kGE) ? ">= "
			: (fOp == kLE) ? "<= "
			: "?? ");
		string << (float)fRightSide;
		string << " PenaltyPos=" << (float)PenaltyPos();
		string << " PenaltyNeg=" << (float)PenaltyNeg();
	} else
		string << "invalid";
	return string;
}


void
Constraint::PrintToStream()
{
	BString string = ToString();
	printf("%s\n", string.String());
}


/**
 * Constructor.
 */
Constraint::Constraint(LinearSpec* ls, SummandList* summands, OperatorType op,
	double rightSide, double penaltyNeg, double penaltyPos)
	:
	fLS(ls),
	fOp(op),
	fRightSide(rightSide),
	fPenaltyNeg(penaltyNeg),
	fPenaltyPos(penaltyPos),
	fDNegObjSummand(NULL),
	fDPosObjSummand(NULL),
	fIsValid(true)
{
	SetLeftSide(summands);
}


/**
 * Destructor.
 * Removes the constraint from its specification and deletes all the summands.
 */
Constraint::~Constraint()
{
	Invalidate();

	for (int32 i = 0; i < fLeftSide->CountItems(); i++)
		delete fLeftSide->ItemAt(i);
	delete fLeftSide;
	fLeftSide = NULL;
}

