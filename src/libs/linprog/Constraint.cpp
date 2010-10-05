/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#include "Constraint.h"
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
	if (i == -1) {
		STRACE(("Constraint not part of fLS->Constraints()."));
		return -1;
	}
	return i + 1;
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

	fLeftSide = summands;
	UpdateLeftSide();
}


void
Constraint::UpdateLeftSide()
{
	if (!fIsValid)
		return;

	double coeffs[fLeftSide->CountItems() + 2];
	int varIndexes[fLeftSide->CountItems() + 2];
	int32 i;
	for (i = 0; i < fLeftSide->CountItems(); i++) {
		Summand* s = fLeftSide->ItemAt(i);
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
		STRACE(("Error in set_rowex."));

	fLS->UpdateObjectiveFunction();
	fLS->RemovePresolved();
}


void
Constraint::SetLeftSide(double coeff1, Variable* var1)
{
	if (!fIsValid)
		return;

	for (int i=0; i<fLeftSide->CountItems(); i++)
		delete fLeftSide->ItemAt(i);
	fLeftSide->MakeEmpty();
	fLeftSide->AddItem(new Summand(coeff1, var1));
	UpdateLeftSide();
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
	fLeftSide->AddItem(new Summand(coeff1, var1));
	fLeftSide->AddItem(new Summand(coeff2, var2));
	UpdateLeftSide();
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
	if (!fIsValid)
		return;

	for (int i=0; i<fLeftSide->CountItems(); i++)
		delete fLeftSide->ItemAt(i);
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
	if (!fIsValid)
		return;

	fOp = value;
	if (!set_constr_type(fLS->fLP, this->Index(),
			((fOp == OperatorType(EQ)) ? EQ
			: (fOp == OperatorType(GE)) ? GE
			: LE)))
		STRACE(("Error in set_constr_type."));

	fLS->RemovePresolved();
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
	if (!set_rh(fLS->fLP, Index(), fRightSide))
		STRACE(("Error in set_rh."));

	fLS->RemovePresolved();
}


/**
 * Gets the penalty coefficient for negative deviations.
 *
 * @return the penalty coefficient
 */
double
Constraint::PenaltyNeg() const
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
	if (!fIsValid)
		return;

	if (fDNegObjSummand == NULL) {
		fDNegObjSummand = new Summand(value, fLS->AddVariable());
		fLS->ObjectiveFunction()->AddItem(fDNegObjSummand);
		UpdateLeftSide();
		fLS->UpdateObjectiveFunction();
		return;
	}

	if (value == fDNegObjSummand->Coeff())
		return;

	fDNegObjSummand->SetCoeff(value);
	fLS->UpdateObjectiveFunction();
}


/**
 * Gets the penalty coefficient for positive deviations.
 *
 * @return the penalty coefficient
 */
double
Constraint::PenaltyPos() const
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
	if (!fIsValid)
		return;

	if (fDPosObjSummand == NULL) {
		fDPosObjSummand = new Summand(value, fLS->AddVariable());
		fLS->ObjectiveFunction()->AddItem(fDPosObjSummand);
		UpdateLeftSide();
		fLS->UpdateObjectiveFunction();
		return;
	}

	if (value == fDPosObjSummand->Coeff())
		return;

	fDPosObjSummand->SetCoeff(value);
	fLS->UpdateObjectiveFunction();
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


void
Constraint::WriteXML(BFile* file)
{
	if (!file->IsWritable())
		return;

	char buffer[200];

	file->Write(buffer, sprintf(buffer, "\t<constraint>\n"));
	file->Write(buffer, sprintf(buffer, "\t\t<leftside>\n"));

	Summand* summand;
	for (int32 i = 0; i < fLeftSide->CountItems(); i++) {
		summand = (Summand*)fLeftSide->ItemAt(i);
		file->Write(buffer, sprintf(buffer, "\t\t\t<summand>\n"));
		file->Write(buffer, sprintf(buffer, "\t\t\t\t<coeff>%f</coeff>\n",
			summand->Coeff()));
		BString varStr = *(summand->Var());
		file->Write(buffer, sprintf(buffer, "\t\t\t\t<var>%s</var>\n",
			varStr.String()));
		file->Write(buffer, sprintf(buffer, "\t\t\t</summand>\n"));
	}

	file->Write(buffer, sprintf(buffer, "\t\t</leftside>\n"));

	const char* op = "??";
	if (fOp == OperatorType(EQ))
		op = "EQ";
	else if (fOp == OperatorType(LE))
		op = "LE";
	else if (fOp == OperatorType(GE))
		op = "GE";

	file->Write(buffer, sprintf(buffer, "\t\t<op>%s</op>\n", op));
	file->Write(buffer, sprintf(buffer, "\t\t<rightside>%f</rightside>\n", fRightSide));
	//~ file->Write(buffer, sprintf(buffer, "\t\t<penaltyneg>%s</penaltyneg>\n", PenaltyNeg()));
	//~ file->Write(buffer, sprintf(buffer, "\t\t<penaltypos>%s</penaltypos>\n", PenaltyPos()));
	file->Write(buffer, sprintf(buffer, "\t</constraint>\n"));
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

	for (int32 i = 0; i < fLeftSide->CountItems(); i++)
		delete (Summand*)fLeftSide->ItemAt(i);
	delete fLeftSide;
	fLeftSide = NULL;

	if (fDNegObjSummand) {
		fLS->ObjectiveFunction()->RemoveItem(fDNegObjSummand);
		delete fDNegObjSummand->Var();
		delete fDNegObjSummand;
		fDNegObjSummand = NULL;
	}
	if (fDPosObjSummand) {
		fLS->ObjectiveFunction()->RemoveItem(fDPosObjSummand);
		delete fDPosObjSummand->Var();
		delete fDPosObjSummand;
		fDPosObjSummand = NULL;
	}

	del_constraint(fLS->fLP, this->Index());
	const_cast<ConstraintList&>(fLS->Constraints()).RemoveItem(this);
}


Constraint::operator BString() const
{
	BString string;
	GetString(string);
	return string;
}


void
Constraint::GetString(BString& string) const
{
	string << "Constraint ";
	string << fLabel;
	string << "(" << (int32)this << "): ";

	if (fIsValid) {
		for (int i = 0; i < fLeftSide->CountItems(); i++) {
			Summand* s = static_cast<Summand*>(fLeftSide->ItemAt(i));
			string << (float)s->Coeff() << "*";
			s->Var()->GetString(string);
			string << " ";
		}
		string << ((fOp == OperatorType(EQ)) ? "== "
			: (fOp == OperatorType(GE)) ? ">= "
			: (fOp == OperatorType(LE)) ? "<= "
			: "?? ");
		string << (float)fRightSide;
		string << " PenaltyPos=" << (float)PenaltyPos();
		string << " PenaltyNeg=" << (float)PenaltyNeg();
	} else
		string << "invalid";
}


/**
 * Constructor.
 */
Constraint::Constraint(LinearSpec* ls, SummandList* summands, OperatorType op,
	double rightSide, double penaltyNeg, double penaltyPos)
	:
	fLS(ls),
	fLeftSide(summands),
	fOp(op),
	fRightSide(rightSide),
	fIsValid(true)
{
	double coeffs[summands->CountItems() + 2];
	int varIndexes[summands->CountItems() + 2];
	int32 i;
	for (i = 0; i < summands->CountItems(); i++) {
		Summand* s = summands->ItemAt(i);
		coeffs[i] = s->Coeff();
		varIndexes[i] = s->Var()->Index();
	}

	if (penaltyNeg != INFINITY
		&& fOp != OperatorType(LE)) {
		fDNegObjSummand = new Summand(penaltyNeg, ls->AddVariable());
		fLS->fObjFunction->AddItem(fDNegObjSummand);
		varIndexes[i] = fDNegObjSummand->Var()->Index();
		coeffs[i] = 1.0;
		i++;
	}
	else
		fDNegObjSummand = NULL;

	if (penaltyPos != INFINITY
		&& fOp != OperatorType(GE)) {
		fDPosObjSummand = new Summand(penaltyPos, ls->AddVariable());
		fLS->fObjFunction->AddItem(fDPosObjSummand);
		varIndexes[i] = fDPosObjSummand->Var()->Index();
		coeffs[i] = -1.0;
		i++;
	}
	else
		fDPosObjSummand = NULL;

	if (!add_constraintex(fLS->fLP, i, &coeffs[0], &varIndexes[0],
			((fOp == OperatorType(EQ)) ? EQ
			: (fOp == OperatorType(GE)) ? GE
			: LE), rightSide))
		STRACE(("Error in add_constraintex."));

	fLS->UpdateObjectiveFunction();
	const_cast<ConstraintList&>(fLS->Constraints()).AddItem(this);
}


/**
 * Destructor.
 * Removes the constraint from its specification and deletes all the summands.
 */
Constraint::~Constraint()
{
	Invalidate();
}

