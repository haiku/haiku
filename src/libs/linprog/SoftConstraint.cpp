#include "SoftConstraint.h"
#include "ObjFunctionSummand.h"
#include "Variable.h"
#include "LinearSpec.h"

#include "lp_lib.h"


/**
 * Changes the left side of the soft constraint,&nbsp;i.e. its coefficients and variables.
 * There must be exactly one coefficient for each variable.
 * 
 * @param coeffs	the constraint's coefficients
 * @param vars	the constraint's variables
 */
void
SoftConstraint::ChangeLeftSide(BList* coeffs, BList* vars)
{
	int32 sizeCoeffs = coeffs->CountItems();
	int32 sizeVars = vars->CountItems();
	if (sizeCoeffs != sizeVars)
		printf("Number of coefficients and number of variables in a constraint must be equal.");
	
	fCoeffs = coeffs;
	fVars = vars;
	
	int vIndexes[sizeCoeffs + 2];
	double coeffs2[sizeCoeffs + 2];
	
	int32 i;
	for (i = 0; i < sizeVars; i++) {
		vIndexes[i] = ((Variable*)vars->ItemAt(i))->Index();
		coeffs2[i] = *(double*)coeffs->ItemAt(i);
	}
	
	if (fOp != OperatorType(LE)) {
		vIndexes[i] = fDNegSummand->Var()->Index();
		coeffs2[i] = 1.0;
		i++;
	}
	
	if (fOp != OperatorType(GE)) {
		vIndexes[i] = fDPosSummand->Var()->Index();
		coeffs2[i] = -1.0;
		i++;
	}
	
	if (!set_rowex(fLS->LP(), this->Index(), i, &coeffs2[0], &vIndexes[0]))
		printf("Error in set_rowex.");
	
	fLS->RemovePresolved();
}


/**
 * Gets the operator used for this constraint.
 * 
 * @return the operator used for this constraint
 */
OperatorType
SoftConstraint::Op()
{
	return fOp;
}


/**
 * Sets the operator used for this constraint.
 * 
 * @param value	operator type
 */
void
SoftConstraint::SetOp(OperatorType value)
{
	fOp = value;
	ChangeLeftSide(fCoeffs, fVars);
}


/**
 * Gets the coefficient of negative summand.
 * 
 * @return the coefficient of negative summand.
 */
double
SoftConstraint::PenaltyNeg()
{
	return fDNegSummand->Coeff();
}


/**
 * The penalty coefficient for positive deviations from the soft constraint's exact solution,&nbsp;
 * i.e. if the left side is too large.
 * 
 * @param value	coefficient of negative penalty <code>double</code>
 */
void
SoftConstraint::SetPenaltyNeg(double value)
{
	if (value == fDNegSummand->Coeff())
		return;
	fDNegSummand->SetCoeff(value);
	fLS->UpdateObjFunction();
}


/**
 * Gets the coefficient of positive summand.
 * 
 * @return the coefficient of positive summand.
 */
double
SoftConstraint::PenaltyPos()
{
	return fDPosSummand->Coeff();
}


/**
 * The penalty coefficient for negative deviations from the soft constraint's exact solution,
 * i.e. if the left side is too small.
 * 
 * @param value	coefficient of positive penalty <code>double</code>
 */
void
SoftConstraint::SetPenaltyPos(double value)
{
	if (value == fDPosSummand->Coeff())
		return;
	fDPosSummand->SetCoeff(value);
	fLS->UpdateObjFunction();
}


/**
 * Returns positive and negative penalty coefficient as string.
 */
// Needs to be fixed.
//~ string SoftConstraint::ToString() {
	//~ return base.ToString()
		//~ + "; PenaltyNeg=" + PenaltyNeg()
		//~ + "; PenaltyPos=" + PenaltyPos();
//~ }


/**
 * Gets the slack variable for the negative variations.
 * 
 * @return the slack variable for the negative variations
 */
Variable*
SoftConstraint::DNeg() const
{
	return fDNeg;
}


/**
 * Gets the slack variable for the positive variations.
 * 
 * @return the slack variable for the positive variations
 */
Variable*
SoftConstraint::DPos() const
{
	return fDPos;
}


/**
 * Destructor.
 * Removes the soft constraint from its specification.
 */
SoftConstraint::~SoftConstraint()
{
	delete fDNegSummand->Var();
	delete fDPosSummand->Var();
	delete fDNegSummand;
	delete fDPosSummand;
}


/**
 * Constructor.
 */
SoftConstraint::SoftConstraint(LinearSpec* ls, BList* coeffs, BList* vars, OperatorType op, 
		double rightSide, double penaltyNeg, double penaltyPos)
{	
	int32 sizeCoeffs = coeffs->CountItems();
	int32 sizeVars = vars->CountItems();
	if (sizeCoeffs != sizeVars)
		printf("Number of coefficients and number of variables in a constraint must be equal.");
	
	fLS = ls;
	fCoeffs = coeffs;
	fVars = vars;
	fOp = op;
	fRightSide = rightSide;
	
	fDNeg = new Variable(ls);
	fDPos = new Variable(ls);
	fDNegSummand = ls->AddObjFunctionSummand(penaltyNeg, DNeg());
	fDPosSummand = ls->AddObjFunctionSummand(penaltyPos, DPos());
	
	int vIndexes[sizeCoeffs + 2];
	double coeffs2[sizeCoeffs + 2];
	
	int32 i;
	for (i=0; i < sizeVars; i++) {
		vIndexes[i] = ((Variable*)vars->ItemAt(i))->Index();
		coeffs2[i] = *(double*)coeffs->ItemAt(i);
	}
	
	if (fOp != OperatorType(LE)) {
		vIndexes[i] = fDNegSummand->Var()->Index();
		coeffs2[i] = 1.0;
		i++;
	}

	if (fOp != OperatorType(GE)) {
		vIndexes[i] = fDPosSummand->Var()->Index();
		coeffs2[i] = -1.0;
		i++;
	}

	if (!add_constraintex(ls->LP(), i, &coeffs2[0], &vIndexes[0],
			((fOp == OperatorType(EQ)) ? EQ
			: (fOp == OperatorType(GE)) ? GE
			: LE), rightSide))
		printf("Error in add_constraintex.");

	ls->Constraints()->AddItem(this);
}

