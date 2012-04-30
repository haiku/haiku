/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "LinearSpec.h"

#include <new>
#include <stdio.h>

#include "ActiveSetSolver.h"


using namespace LinearProgramming;


// #define DEBUG_LINEAR_SPECIFICATIONS

#ifdef DEBUG_LINEAR_SPECIFICATIONS
#include <stdio.h>
#define TRACE(x...) printf(x)
#else
#define TRACE(x...) /* nothing */
#endif


SolverInterface::SolverInterface(LinearSpec* linSpec)
	:
	fLinearSpec(linSpec)
{

}


/**
 * Constructor.
 * Creates a new specification for a linear programming problem.
 */
LinearSpec::LinearSpec()
	:
	fResult(kError),
	fSolvingTime(0)
{
	fSolver = new ActiveSetSolver(this);
}


/**
 * Destructor.
 * Removes the specification and deletes all constraints,
 * objective function summands and variables.
 */
LinearSpec::~LinearSpec()
{
	for (int32 i = 0; i < fConstraints.CountItems(); i++)
		delete (Constraint*)fConstraints.ItemAt(i);
	while (fVariables.CountItems() > 0)
		RemoveVariable(fVariables.ItemAt(0));

	delete fSolver;
}


/**
 * Adds a new variable to the specification.
 *
 * @return the new variable
 */
Variable*
LinearSpec::AddVariable()
{
	Variable* variable = new(std::nothrow) Variable(this);
	if (!variable)
		return NULL;
	if (!AddVariable(variable)) {
		delete variable;
		return NULL;
	}

	return variable;
}


bool
LinearSpec::AddVariable(Variable* variable)
{
	if (variable->IsValid())
		return false;

	if (!fVariables.AddItem(variable))
		return false;

	if (variable->fLS == NULL)
		variable->fLS = this;

	if (!fSolver->VariableAdded(variable)) {
		fVariables.RemoveItem(variable);
		return false;
	}
	variable->fIsValid = true;

	if (!UpdateRange(variable)) {
		RemoveVariable(variable, false);
		return false;
	}
	return true;
}


bool
LinearSpec::RemoveVariable(Variable* variable, bool deleteVariable)
{
	// must be called first otherwise the index is invalid
	if (fSolver->VariableRemoved(variable) == false)
		return false;

	// do we know the variable?
	if (fVariables.RemoveItem(variable) == false)
		return false;
	fUsedVariables.RemoveItem(variable);

	variable->fIsValid = false;

	// invalidate all constraints that use this variable
	ConstraintList markedForInvalidation;
	const ConstraintList& constraints = Constraints();
	for (int i = 0; i < constraints.CountItems(); i++) {
		Constraint* constraint = constraints.ItemAt(i);

		if (!constraint->IsValid())
			continue;

		SummandList* summands = constraint->LeftSide();
		for (int j = 0; j < summands->CountItems(); j++) {
			Summand* summand = summands->ItemAt(j);
			if (summand->Var() == variable) {
				markedForInvalidation.AddItem(constraint);
				break;
			}
		}
	}
	for (int i = 0; i < markedForInvalidation.CountItems(); i++)
		RemoveConstraint(markedForInvalidation.ItemAt(i));


	if (deleteVariable)
		delete variable;
	return true;
}


int32
LinearSpec::IndexOf(const Variable* variable) const
{
	return fUsedVariables.IndexOf(variable);
}


int32
LinearSpec::GlobalIndexOf(const Variable* variable) const
{
	return fVariables.IndexOf(variable);
}


bool
LinearSpec::UpdateRange(Variable* variable)
{
	if (!fSolver->VariableRangeChanged(variable))
		return false;
	return true;
}


bool
LinearSpec::AddConstraint(Constraint* constraint)
{
	if (!fConstraints.AddItem(constraint))
		return false;

	// ref count the used variables
	SummandList* leftSide = constraint->LeftSide();
	for (int i = 0; i < leftSide->CountItems(); i++) {
		Variable* var = leftSide->ItemAt(i)->Var();
		if (var->AddReference() == 1)
			fUsedVariables.AddItem(var);
	}

	if (!fSolver->ConstraintAdded(constraint)) {
		RemoveConstraint(constraint, false);
		return false;
	}

	constraint->fIsValid = true;
	return true;
}


bool
LinearSpec::RemoveConstraint(Constraint* constraint, bool deleteConstraint)
{
	fSolver->ConstraintRemoved(constraint);
	if (!fConstraints.RemoveItem(constraint))
		return false;
	constraint->fIsValid = false;

	SummandList* leftSide = constraint->LeftSide();
	for (int i = 0; i < leftSide->CountItems(); i++) {
		Variable* var = leftSide->ItemAt(i)->Var();
		if (var->RemoveReference() == 0)
			fUsedVariables.RemoveItem(var);
	}

	if (deleteConstraint)
		delete constraint;
	return true;
}


bool
LinearSpec::UpdateLeftSide(Constraint* constraint)
{
	if (!fSolver->LeftSideChanged(constraint))
		return false;
	return true;
}


bool
LinearSpec::UpdateRightSide(Constraint* constraint)
{
	if (!fSolver->RightSideChanged(constraint))
		return false;
	return true;
}


bool
LinearSpec::UpdateOperator(Constraint* constraint)
{
	if (!fSolver->OperatorChanged(constraint))
		return false;
	return true;
}


/**
 * Adds a new hard linear constraint to the specification.
 *
 * @param coeffs		the constraint's coefficients
 * @param vars			the constraint's variables
 * @param op			the constraint's operand
 * @param rightSide	the constant value on the constraint's right side
 * @return the new constraint
 */
Constraint*
LinearSpec::AddConstraint(SummandList* summands, OperatorType op,
	double rightSide)
{
	return AddConstraint(summands, op, rightSide, -1, -1);
}


/**
 * Adds a new hard linear constraint to the specification with a single summand.
 *
 * @param coeff1		the constraint's first coefficient
 * @param var1			the constraint's first variable
 * @param op			the constraint's operand
 * @param rightSide	the constant value on the constraint's right side
 * @return the new constraint
 */
Constraint*
LinearSpec::AddConstraint(double coeff1, Variable* var1,
	OperatorType op, double rightSide)
{
	return AddConstraint(coeff1, var1, op, rightSide, -1, -1);
}


/**
 * Adds a new hard linear constraint to the specification with two summands.
 *
 * @param coeff1		the constraint's first coefficient
 * @param var1			the constraint's first variable
 * @param coeff2		the constraint's second coefficient
 * @param var2			the constraint's second variable
 * @param op			the constraint's operand
 * @param rightSide	the constant value on the constraint's right side
 * @return the new constraint
 */
Constraint*
LinearSpec::AddConstraint(double coeff1, Variable* var1,
	double coeff2, Variable* var2, OperatorType op, double rightSide)
{
	return AddConstraint(coeff1, var1, coeff2, var2, op, rightSide, -1,
		-1);
}


/**
 * Adds a new hard linear constraint to the specification with three summands.
 *
 * @param coeff1		the constraint's first coefficient
 * @param var1			the constraint's first variable
 * @param coeff2		the constraint's second coefficient
 * @param var2			the constraint's second variable
 * @param coeff3		the constraint's third coefficient
 * @param var3			the constraint's third variable
 * @param op			the constraint's operand
 * @param rightSide	the constant value on the constraint's right side
 * @return the new constraint
 */
Constraint*
LinearSpec::AddConstraint(double coeff1, Variable* var1,
		double coeff2, Variable* var2, double coeff3, Variable* var3,
		OperatorType op, double rightSide)
{
	return AddConstraint(coeff1, var1, coeff2, var2, coeff3, var3, op,
		rightSide, -1, -1);
}


/**
 * Adds a new hard linear constraint to the specification with four summands.
 *
 * @param coeff1		the constraint's first coefficient
 * @param var1			the constraint's first variable
 * @param coeff2		the constraint's second coefficient
 * @param var2			the constraint's second variable
 * @param coeff3		the constraint's third coefficient
 * @param var3			the constraint's third variable
 * @param coeff4		the constraint's fourth coefficient
 * @param var4			the constraint's fourth variable
 * @param op			the constraint's operand
 * @param rightSide	the constant value on the constraint's right side
 * @return the new constraint
 */
Constraint*
LinearSpec::AddConstraint(double coeff1, Variable* var1,
		double coeff2, Variable* var2, double coeff3, Variable* var3,
		double coeff4, Variable* var4, OperatorType op, double rightSide)
{
	return AddConstraint(coeff1, var1, coeff2, var2, coeff3, var3, coeff4, var4,
		op, rightSide, -1, -1);
}


/**
 * Adds a new soft linear constraint to the specification.
 * i.e. a constraint that does not always have to be satisfied.
 *
 * @param coeffs		the constraint's coefficients
 * @param vars			the constraint's variables
 * @param op			the constraint's operand
 * @param rightSide		the constant value on the constraint's right side
 * @param penaltyNeg	the coefficient penalizing negative deviations from the exact solution
 * @param penaltyPos	the coefficient penalizing positive deviations from the exact solution
 */
Constraint*
LinearSpec::AddConstraint(SummandList* summands, OperatorType op,
		double rightSide, double penaltyNeg, double penaltyPos)
{
	return _AddConstraint(summands, op, rightSide, penaltyNeg, penaltyPos);
}


/**
 * Adds a new soft linear constraint to the specification with a single summand.
 *
 * @param coeff1		the constraint's first coefficient
 * @param var1			the constraint's first variable
 * @param op			the constraint's operand
 * @param rightSide		the constant value on the constraint's right side
 * @param penaltyNeg	the coefficient penalizing negative deviations from the exact solution
 * @param penaltyPos	the coefficient penalizing positive deviations from the exact solution
 */
Constraint*
LinearSpec::AddConstraint(double coeff1, Variable* var1,
		OperatorType op, double rightSide, double penaltyNeg, double penaltyPos)
{
	SummandList* summands = new(std::nothrow) SummandList(1);
	if (!summands)
		return NULL;
	summands->AddItem(new(std::nothrow) Summand(coeff1, var1));
	if (!_CheckSummandList(summands))
		return NULL;
	return _AddConstraint(summands, op, rightSide, penaltyNeg, penaltyPos);
}


/**
 * Adds a new soft linear constraint to the specification with two summands.
 *
 * @param coeff1		the constraint's first coefficient
 * @param var1			the constraint's first variable
 * @param coeff2		the constraint's second coefficient
 * @param var2			the constraint's second variable
 * @param op			the constraint's operand
 * @param rightSide		the constant value on the constraint's right side
 * @param penaltyNeg	the coefficient penalizing negative deviations from the exact solution
 * @param penaltyPos	the coefficient penalizing positive deviations from the exact solution
 */
Constraint*
LinearSpec::AddConstraint(double coeff1, Variable* var1,
	double coeff2, Variable* var2, OperatorType op, double rightSide,
	double penaltyNeg, double penaltyPos)
{
	SummandList* summands = new(std::nothrow) SummandList(2);
	if (!summands)
		return NULL;
	summands->AddItem(new(std::nothrow) Summand(coeff1, var1));
	summands->AddItem(new(std::nothrow) Summand(coeff2, var2));
	if (!_CheckSummandList(summands))
		return NULL;
	return _AddConstraint(summands, op, rightSide, penaltyNeg, penaltyPos);
}


/**
 * Adds a new soft linear constraint to the specification with three summands.
 *
 * @param coeff1		the constraint's first coefficient
 * @param var1			the constraint's first variable
 * @param coeff2		the constraint's second coefficient
 * @param var2			the constraint's second variable
 * @param coeff3		the constraint's third coefficient
 * @param var3			the constraint's third variable
 * @param op			the constraint's operand
 * @param rightSide		the constant value on the constraint's right side
 * @param penaltyNeg	the coefficient penalizing negative deviations from the exact solution
 * @param penaltyPos	the coefficient penalizing positive deviations from the exact solution
 */
Constraint*
LinearSpec::AddConstraint(double coeff1, Variable* var1,
	double coeff2, Variable* var2, double coeff3, Variable* var3,
	OperatorType op, double rightSide, double penaltyNeg, double penaltyPos)
{
	SummandList* summands = new(std::nothrow) SummandList(2);
	if (!summands)
		return NULL;
	summands->AddItem(new(std::nothrow) Summand(coeff1, var1));
	summands->AddItem(new(std::nothrow) Summand(coeff2, var2));
	summands->AddItem(new(std::nothrow) Summand(coeff3, var3));
	if (!_CheckSummandList(summands))
		return NULL;
	return _AddConstraint(summands, op, rightSide, penaltyNeg, penaltyPos);
}


/**
 * Adds a new soft linear constraint to the specification with four summands.
 *
 * @param coeff1		the constraint's first coefficient
 * @param var1			the constraint's first variable
 * @param coeff2		the constraint's second coefficient
 * @param var2			the constraint's second variable
 * @param coeff3		the constraint's third coefficient
 * @param var3			the constraint's third variable
 * @param coeff4		the constraint's fourth coefficient
 * @param var4			the constraint's fourth variable
 * @param op			the constraint's operand
 * @param rightSide		the constant value on the constraint's right side
 * @param penaltyNeg	the coefficient penalizing negative deviations from the exact solution
 * @param penaltyPos	the coefficient penalizing positive deviations from the exact solution
 */
Constraint*
LinearSpec::AddConstraint(double coeff1, Variable* var1,
	double coeff2, Variable* var2, double coeff3, Variable* var3,
	double coeff4, Variable* var4, OperatorType op, double rightSide,
	double penaltyNeg, double penaltyPos)
{
	SummandList* summands = new(std::nothrow) SummandList(2);
	if (!summands)
		return NULL;
	summands->AddItem(new(std::nothrow) Summand(coeff1, var1));
	summands->AddItem(new(std::nothrow) Summand(coeff2, var2));
	summands->AddItem(new(std::nothrow) Summand(coeff3, var3));
	summands->AddItem(new(std::nothrow) Summand(coeff4, var4));
	if (!_CheckSummandList(summands))
		return NULL;
	return _AddConstraint(summands, op, rightSide, penaltyNeg, penaltyPos);
}


BSize
LinearSpec::MinSize(Variable* width, Variable* height)
{
	return fSolver->MinSize(width, height);
}


BSize
LinearSpec::MaxSize(Variable* width, Variable* height)
{
	return fSolver->MaxSize(width, height);
}



ResultType
LinearSpec::FindMins(const VariableList* variables)
{
	return fSolver->FindMins(variables);
}


ResultType
LinearSpec::FindMaxs(const VariableList* variables)
{
	return fSolver->FindMaxs(variables);
}


bool
LinearSpec::_CheckSummandList(SummandList* list)
{
	bool ok = true;
	for (int i = 0; i < list->CountItems(); i++) {
		if (list->ItemAt(i) == NULL) {
			ok = false;
			break;
		}
	}
	if (ok)
		return true;

	for (int i = 0; i < list->CountItems(); i++)
		delete list->ItemAt(i);
	delete list;
	return false;
}


Constraint*
LinearSpec::_AddConstraint(SummandList* leftSide, OperatorType op,
	double rightSide, double penaltyNeg, double penaltyPos)
{
	Constraint* constraint = new(std::nothrow) Constraint(this, leftSide,
		op, rightSide, penaltyNeg, penaltyPos);
	if (constraint == NULL)
		return NULL;
	if (!AddConstraint(constraint)) {
		delete constraint;
		return NULL;
	}
	return constraint;
}


#ifdef DEBUG_LINEAR_SPECIFICATIONS
static bigtime_t sAverageSolvingTime = 0;
static int32 sSolvedCount = 0;
#endif


ResultType
LinearSpec::Solve()
{
	bigtime_t startTime = system_time();

	fResult = fSolver->Solve();

	fSolvingTime = system_time() - startTime;

#ifdef DEBUG_LINEAR_SPECIFICATIONS
	sAverageSolvingTime += fSolvingTime;
	sSolvedCount++;
	TRACE("Solving time %i average %i [micro s]\n", (int)fSolvingTime,
		int(sAverageSolvingTime / sSolvedCount));
#endif

	return fResult;
}


/**
 * Writes the specification into a text file.
 * The file will be overwritten if it exists.
 *
 * @param fname	the file name
 */
bool
LinearSpec::Save(const char* fileName)
{
	return fSolver->SaveModel(fileName) == B_OK;
}


/**
 * Gets the constraints generated by calls to Variable::SetRange()
 *
 */
void
LinearSpec::GetRangeConstraints(const Variable* var, const Constraint** _min,
	const Constraint** _max) const
{
	fSolver->GetRangeConstraints(var, _min, _max);
}


/**
 * Gets the constraints.
 *
 * @return the constraints
 */
const ConstraintList&
LinearSpec::Constraints() const
{
	return fConstraints;
}


const VariableList&
LinearSpec::UsedVariables() const
{
	return fUsedVariables;
}


const VariableList&
LinearSpec::AllVariables() const
{
	return fVariables;
}


/**
 * Gets the result type.
 *
 * @return the result type
 */
ResultType
LinearSpec::Result() const
{
	return fResult;
}


/**
 * Gets the solving time.
 *
 * @return the solving time
 */
bigtime_t
LinearSpec::SolvingTime() const
{
	return fSolvingTime;
}


BString
LinearSpec::ToString() const
{
	BString string;
	string << "LinearSpec " << (int32)this << ":\n";
	for (int i = 0; i < fVariables.CountItems(); i++) {
		Variable* variable = fVariables.ItemAt(i);
		string += variable->ToString();
		string << "=" << (float)variable->Value() << " ";
	}
	string << "\n";
	for (int i = 0; i < fConstraints.CountItems(); i++) {
		Constraint* c = fConstraints.ItemAt(i);
		string << i << ": ";
		string += c->ToString();
		string << "\n";
	}
	string << "Result=";
	if (fResult == kError)
		string << "kError";
	else if (fResult == kOptimal)
		string << "kOptimal";
	else if (fResult == kSuboptimal)
		string << "kSuboptimal";
	else if (fResult == kInfeasible)
		string << "kInfeasible";
	else if (fResult == kUnbounded)
		string << "kUnbounded";
	else if (fResult == kDegenerate)
		string << "kDegenerate";
	else if (fResult == kNumFailure)
		string << "kNumFailure";
	else
		string << fResult;
	string << " SolvingTime=" << fSolvingTime << "micro s";
	return string;
}

