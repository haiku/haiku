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


Constraint::Constraint()
	:
	fLS(NULL),
	fLeftSide(new SummandList),
	fOp(kEQ),
	fRightSide(0),
	fPenaltyNeg(-1),
	fPenaltyPos(-1)
{
}


Constraint::Constraint(Constraint* constraint)
	:
	fLS(NULL),
	fLeftSide(new SummandList),
	fOp(constraint->Op()),
	fRightSide(constraint->RightSide()),
	fPenaltyNeg(constraint->PenaltyNeg()),
	fPenaltyPos(constraint->PenaltyPos()),
	fLabel(constraint->Label())
{
	SummandList* orgSummands = constraint->LeftSide();
	for (int32 i = 0; i < orgSummands->CountItems(); i++) {
		Summand* summand = orgSummands->ItemAt(i);
		fLeftSide->AddItem(new Summand(summand));
	}
}


/**
 * Gets the index of the constraint.
 *
 * @return the index of the constraint
 */
int32
Constraint::Index() const
{
	if (fLS == NULL)
		return -1;
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
bool
Constraint::SetLeftSide(SummandList* summands, bool deleteOldSummands)
{
	if (summands == NULL)
		debugger("Invalid summands");

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

	if (fLS == NULL) {
		if (deleteOldSummands)
			delete fLeftSide;
		fLeftSide = summands;
		return true;
	}

	// notify the solver
	SummandList oldSummands;
	if (fLeftSide != NULL)
		oldSummands = *fLeftSide;
	if (deleteOldSummands)
		delete fLeftSide;
	fLeftSide = summands;
	if (fLS != NULL)
		fLS->UpdateLeftSide(this, &oldSummands);

	if (deleteOldSummands) {
		for (int32 i = 0; i < oldSummands.CountItems(); i++)
			delete oldSummands.ItemAt(i);
	}
	return true;
}


bool
Constraint::SetLeftSide(double coeff1, Variable* var1)
{
	SummandList* list = new SummandList;
	list->AddItem(new(std::nothrow) Summand(coeff1, var1));
	return SetLeftSide(list, true);
}


bool
Constraint::SetLeftSide(double coeff1, Variable* var1,
	double coeff2, Variable* var2)
{
	SummandList* list = new SummandList;
	list->AddItem(new(std::nothrow) Summand(coeff1, var1));
	list->AddItem(new(std::nothrow) Summand(coeff2, var2));
	return SetLeftSide(list, true);
}


bool
Constraint::SetLeftSide(double coeff1, Variable* var1,
	double coeff2, Variable* var2,
	double coeff3, Variable* var3)
{
	SummandList* list = new SummandList;
	list->AddItem(new(std::nothrow) Summand(coeff1, var1));
	list->AddItem(new(std::nothrow) Summand(coeff2, var2));
	list->AddItem(new(std::nothrow) Summand(coeff3, var3));
	return SetLeftSide(list, true);
}


bool
Constraint::SetLeftSide(double coeff1, Variable* var1,
	double coeff2, Variable* var2,
	double coeff3, Variable* var3,
	double coeff4, Variable* var4)
{
	SummandList* list = new SummandList;
	list->AddItem(new(std::nothrow) Summand(coeff1, var1));
	list->AddItem(new(std::nothrow) Summand(coeff2, var2));
	list->AddItem(new(std::nothrow) Summand(coeff3, var3));
	list->AddItem(new(std::nothrow) Summand(coeff4, var4));
	return SetLeftSide(list, true);
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
	if (fLS != NULL)
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
	if (fRightSide == value)
		return;

	fRightSide = value;

	if (fLS != NULL)
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

	if (fLS != NULL)
		fLS->UpdatePenalties(this);
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

	if (fLS != NULL)
		fLS->UpdatePenalties(this);
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


bool
Constraint::IsValid()
{
	return fLS != NULL;
}


void
Constraint::Invalidate()
{
	STRACE(("Constraint::Invalidate() on %d\n", this));

	if (fLS == NULL)
		return;

	fLS->RemoveConstraint(this, false);
	fLS = NULL;
}


BString
Constraint::ToString() const
{
	BString string;
	string << "Constraint ";
	string << fLabel;
	string << "(" << (int32)this << "): ";

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
	fLeftSide(NULL),
	fOp(op),
	fRightSide(rightSide),
	fPenaltyNeg(penaltyNeg),
	fPenaltyPos(penaltyPos)
{
	SetLeftSide(summands, false);
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

