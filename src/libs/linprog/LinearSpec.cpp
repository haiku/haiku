/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "LinearSpec.h"

#include <new>
#include <stdio.h>

#include "LPSolveInterface.h"


/**
 * Constructor.
 * Creates a new specification for a linear programming problem.
 */
LinearSpec::LinearSpec()
	:
	fOptimization(MINIMIZE),
	fObjFunction(new(std::nothrow) SummandList()),
	fResult(ERROR),
	fObjectiveValue(NAN),
	fSolvingTime(NAN)
{
	fSolver = new LPSolveInterface;
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
	for (int32 i = 0; i < fObjFunction->CountItems(); i++)
		delete (Summand*)fObjFunction->ItemAt(i);
	while (fVariables.CountItems() > 0)
		RemoveVariable(fVariables.ItemAt(0));

	delete fObjFunction;
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
	if (!fSolver->AddVariable()) {
		fVariables.RemoveItem(variable);
		return false;
	}

	if (!UpdateRange(variable)) {
		RemoveVariable(variable, false);
		return false;
	}

	variable->fIsValid = true;
	return true;
}


bool
LinearSpec::RemoveVariable(Variable* variable, bool deleteVariable)
{
	int32 index = IndexOf(variable);
	if (index < 0)
		return false;

	if (!fSolver->RemoveVariable(index))
		return false;
	fVariables.RemoveItemAt(index - 1);
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
		markedForInvalidation.ItemAt(i)->Invalidate();


	if (deleteVariable)
		delete variable;
	return true;
}


int32
LinearSpec::IndexOf(const Variable* variable) const
{
	int32 i = fVariables.IndexOf(variable);
	if (i == -1) {
		printf("Variable 0x%p not part of fLS->Variables().\n", variable);
		return -1;
	}
	return i + 1;
}


bool
LinearSpec::UpdateRange(Variable* variable)
{
	if (!fSolver->SetVariableRange(IndexOf(variable), variable->Min(),
		variable->Max()))
		return false;
	return true;
}


bool
LinearSpec::AddConstraint(Constraint* constraint)
{
	SummandList* summands = constraint->LeftSide();
	OperatorType op = constraint->Op();
	double rightSide = constraint->RightSide();
	double penaltyNeg = constraint->PenaltyNeg();
	double penaltyPos = constraint->PenaltyPos();

	double coeffs[summands->CountItems() + 2];
	int varIndexes[summands->CountItems() + 2];
	int32 nCoefficient = 0;
	for (; nCoefficient < summands->CountItems(); nCoefficient++) {
		Summand* s = summands->ItemAt(nCoefficient);
		coeffs[nCoefficient] = s->Coeff();
		varIndexes[nCoefficient] = s->Var()->Index();
	}

	if (penaltyNeg != INFINITY && penaltyNeg != 0. && op != LE) {
		constraint->fDNegObjSummand
			= new(std::nothrow) Summand(constraint->PenaltyNeg(),
				AddVariable());
		fObjFunction->AddItem(constraint->fDNegObjSummand);
		varIndexes[nCoefficient] = constraint->fDNegObjSummand->Var()->Index();
		coeffs[nCoefficient] = 1.0;
		nCoefficient++;
	}

	if (penaltyPos != INFINITY && penaltyPos != 0. && op != GE) {
		constraint->fDPosObjSummand
			= new(std::nothrow) Summand(constraint->PenaltyPos(),
				AddVariable());
		fObjFunction->AddItem(constraint->fDPosObjSummand);
		varIndexes[nCoefficient] = constraint->fDPosObjSummand->Var()->Index();
		coeffs[nCoefficient] = -1.0;
		nCoefficient++;
	}

	if (!fSolver->AddConstraint(nCoefficient, &coeffs[0], &varIndexes[0], op,
		rightSide)) {
		return false;
	}

	UpdateObjectiveFunction();
	fConstraints.AddItem(constraint);

	return true;
}


bool
LinearSpec::RemoveConstraint(Constraint* constraint, bool deleteConstraint)
{
	if (constraint->fDNegObjSummand) {
		fObjFunction->RemoveItem(constraint->fDNegObjSummand);
		delete constraint->fDNegObjSummand->Var();
		delete constraint->fDNegObjSummand;
		constraint->fDNegObjSummand = NULL;
	}
	if (constraint->fDPosObjSummand) {
		fObjFunction->RemoveItem(constraint->fDPosObjSummand);
		delete constraint->fDPosObjSummand->Var();
		delete constraint->fDPosObjSummand;
		constraint->fDPosObjSummand = NULL;
	}

	fSolver->RemoveConstraint(constraint->Index());
	fConstraints.RemoveItem(constraint);
	constraint->fIsValid = false;

	if (deleteConstraint)
		delete constraint;
	return true;
}


bool
LinearSpec::UpdateLeftSide(Constraint* constraint)
{
	if (!constraint->IsValid())
		return false;

	SummandList* leftSide = constraint->LeftSide();
	OperatorType op = constraint->Op();

	double coeffs[leftSide->CountItems() + 2];
	int varIndexes[leftSide->CountItems() + 2];
	int32 i;
	for (i = 0; i < leftSide->CountItems(); i++) {
		Summand* s = leftSide->ItemAt(i);
		coeffs[i] = s->Coeff();
		varIndexes[i] = s->Var()->Index();
	}

	if (constraint->fDNegObjSummand != NULL && op != OperatorType(LE)) {
		varIndexes[i] = constraint->fDNegObjSummand->Var()->Index();
		coeffs[i] = 1.0;
		i++;
	}

	if (constraint->fDPosObjSummand != NULL && op != OperatorType(GE)) {
		varIndexes[i] = constraint->fDPosObjSummand->Var()->Index();
		coeffs[i] = -1.0;
		i++;
	}

	if (!fSolver->SetLeftSide(constraint->Index(), i, &coeffs[0],
		&varIndexes[0]))
		return false;

	UpdateObjectiveFunction();
	return true;
}


bool
LinearSpec::UpdateRightSide(Constraint* constraint)
{
	if (!fSolver->SetRightSide(constraint->Index(), constraint->RightSide()))
		return false;
	return true;
}


bool
LinearSpec::UpdateOperator(Constraint* constraint)
{
	if (!fSolver->SetOperator(constraint->Index(), constraint->Op()))
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
	return AddConstraint(summands, op, rightSide, INFINITY, INFINITY);
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
	return AddConstraint(coeff1, var1, op, rightSide, INFINITY, INFINITY);
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
	return AddConstraint(coeff1, var1, coeff2, var2, op, rightSide, INFINITY,
		INFINITY);
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
		rightSide, INFINITY, INFINITY);
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
		op, rightSide, INFINITY, INFINITY);
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


/**
 * Adds a new penalty function to the specification.
 *
 * @param var	the penalty function's variable
 * @param xs	the penalty function's sampling points
 * @param gs	the penalty function's gradients
 * @return the new penalty function
 */
PenaltyFunction*
LinearSpec::AddPenaltyFunction(Variable* var, BList* xs, BList* gs)
{
	return new(std::nothrow) PenaltyFunction(this, var, xs, gs);
}


/**
 * Gets the objective function.
 *
 * @return SummandList containing the objective function's summands
 */
SummandList*
LinearSpec::ObjectiveFunction()
{
	return fObjFunction;
}


SummandList*
LinearSpec::SwapObjectiveFunction(SummandList* objFunction)
{
	SummandList* list = fObjFunction;
	fObjFunction = objFunction;
	UpdateObjectiveFunction();
	return list;
}


/**
 * Sets a new objective function.
 *
 * @param summands	SummandList containing the objective function's summands
 */
void
LinearSpec::SetObjectiveFunction(SummandList* objFunction)
{
	for (int32 i = 0; i < fObjFunction->CountItems(); i++)
		delete (Summand*)fObjFunction->ItemAt(i);
	delete fObjFunction;

	fObjFunction = objFunction;
	UpdateObjectiveFunction();
}


/**
 * Updates the internal representation of the objective function.
 * Must be called whenever the summands of the objective function are changed.
 */
void
LinearSpec::UpdateObjectiveFunction()
{
	int32 size = fObjFunction->CountItems();
	double coeffs[size];
	int varIndexes[size];
	Summand* current;
	for (int32 i = 0; i < size; i++) {
		current = (Summand*)fObjFunction->ItemAt(i);
		coeffs[i] = current->Coeff();
		varIndexes[i] = current->Var()->Index();
	}

	if (!fSolver->SetObjectiveFunction(size, &coeffs[0], &varIndexes[0]))
		printf("Error in set_obj_fnex.\n");
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


/**
 * Tries to solve the linear programming problem.
 * If a cached simplified version of the problem exists, it is used instead.
 *
 * @return the result of the solving attempt
 */
ResultType
LinearSpec::Solve()
{
	bigtime_t start, end;
	start = system_time();

	fResult = fSolver->Solve(fVariables);
	fObjectiveValue = fSolver->GetObjectiveValue();

	end = system_time();
	fSolvingTime = (end - start) / 1000.0;

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
 * Gets the current optimization.
 * The default is minimization.
 *
 * @return the current optimization
 */
OptimizationType
LinearSpec::Optimization() const
{
	return fOptimization;
}


/**
 * Sets whether the solver should minimize or maximize the objective function.
 * The default is minimization.
 *
 * @param optimization	the optimization type
 */
void
LinearSpec::SetOptimization(OptimizationType value)
{
	fOptimization = value;
	fSolver->SetOptimization(value);
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
 * Gets the objective value.
 *
 * @return the objective value
 */
double
LinearSpec::ObjectiveValue() const
{
	return fObjectiveValue;
}


/**
 * Gets the solving time.
 *
 * @return the solving time
 */
double
LinearSpec::SolvingTime() const
{
	return fSolvingTime;
}


LinearSpec::operator BString() const
{
	BString string;
	GetString(string);
	return string;
}


void
LinearSpec::GetString(BString& string) const
{
	string << "LinearSpec " << (int32)this << ":\n";
	for (int i = 0; i < fVariables.CountItems(); i++) {
		Variable* variable = static_cast<Variable*>(fVariables.ItemAt(i));
		variable->GetString(string);
		string << "=" << (float)variable->Value() << " ";
	}
	string << "\n";
	for (int i = 0; i < fConstraints.CountItems(); i++) {
		Constraint* c = static_cast<Constraint*>(fConstraints.ItemAt(i));
		string << i << ": ";
		c->GetString(string);
		string << "\n";
	}
	string << "Result=";
	if (fResult==-1)
		string << "ERROR";
	else if (fResult==0)
		string << "OPTIMAL";
	else if (fResult==1)
		string << "SUBOPTIMAL";
	else if (fResult==2)
		string << "INFEASIBLE";
	else if (fResult==3)
		string << "UNBOUNDED";
	else if (fResult==4)
		string << "DEGENERATE";
	else if (fResult==5)
		string << "NUMFAILURE";
	else
		string << fResult;
	string << " SolvingTime=" << (float)fSolvingTime << "ms";
}

