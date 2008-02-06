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
 * Gets the coefficients of the constraint.
 * 
 * @return the coefficients of the constraint
 */
BList*
Constraint::Coeffs()
{
	return fCoeffs;
}


/**
 * Gets the variables of the constraint.
 * 
 * @return the variables of the constraint
 */
BList*
Constraint::Vars()
{
	return fVars;
}


/**
 * Changes the left side of the constraint, i.e.&nbsp;its coefficients and variables.
 * There must be exactly one coefficient for each variable.
 * 
 * @param coeffs	the new coefficients
 * @param vars		the new variables
 */
void
Constraint::ChangeLeftSide(BList* coeffs, BList* vars)
{
	int32 sizeCoeffs = coeffs->CountItems();
	int32 sizeVars = vars->CountItems();
	if (sizeCoeffs != sizeVars)
		printf("Number of coefficients and number of variables in a constraint must be equal.");
	
	fCoeffs = coeffs;
	fVars = vars;
	
	double coeffsArray[sizeCoeffs];
	for (int32 i = 0; i < sizeCoeffs; i++)
	coeffsArray[i] = *(double*)coeffs->ItemAt(i);
	
	int vIndexes[sizeCoeffs];
	for (int32 i = 0; i < sizeVars; i++)
		vIndexes[i] = ((Variable*)vars->ItemAt(i))->Index();

	if (!set_rowex(fLS->LP(), this->Index(), sizeCoeffs, &coeffsArray[0], &vIndexes[0]))
		printf("Error in set_rowex.");
	
	fLS->RemovePresolved();
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


// Needs to be fixed. Use BString. Substring?
//~ string Constraint::ToString() {
	//~ string s = "";
	//~ for (int32 i = 0; i < fVars->CountItems(); i++)
		//~ s += (double)fCoeffs->ItemAt(i) + "*" + (Variable*)fVars->ItemAt(i).ToString() + " + ";
	//~ s = s.Substring(0, s.Length - 2);
	//~ if (Op == OperatorType.EQ) s += "= ";
	//~ else if (Op == OperatorType.GE) s += ">= ";
	//~ else s += "<= ";
	//~ s += rightSide.ToString();
	//~ return s;
//~ }


/**
 * Destructor.
 * Removes the constraint from its specification.
 */
Constraint::~Constraint()
{
	del_constraint(fLS->LP(), Index());
	delete fCoeffs;
	delete fVars;
	fLS->Constraints()->RemoveItem(this);
}


/**
 * Default constructor.
 */
Constraint::Constraint() {}


/**
 * Constructor.
 */
Constraint::Constraint(LinearSpec* ls, BList* coeffs, BList* vars, OperatorType op, 
	double rightSide)
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
	
	double coeffsArray[sizeCoeffs];
	for (int32 i = 0; i < sizeCoeffs; i++)
		coeffsArray[i] = *(double*)coeffs->ItemAt(i);
	
	int vIndexes[sizeCoeffs];
	for (int32 i = 0; i < sizeVars; i++)
		vIndexes[i] = ((Variable*)vars->ItemAt(i))->Index();
	
	if (!add_constraintex(ls->LP(), sizeCoeffs, &coeffsArray[0], &vIndexes[0],
			((op == OperatorType(EQ)) ? EQ
			: (op == OperatorType(GE)) ? GE
			: LE), rightSide))
		printf("Error in add_constraintex.");
	
	ls->Constraints()->AddItem(this);
}

