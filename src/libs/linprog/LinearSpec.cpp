/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "LinearSpec.h"

#include <new>


/**
 * Constructor.
 * Creates a new specification for a linear programming problem.
 */
LinearSpec::LinearSpec()
	:
	fLpPresolved(NULL),
	fOptimization(MINIMIZE),
	fObjFunction(new(std::nothrow) SummandList()),
	fResult(ERROR),
	fObjectiveValue(NAN),
	fSolvingTime(NAN)
{
	fLP = make_lp(0, 0);
	if (fLP == NULL)
		printf("Couldn't construct a new model.");

	// minimize the objective functions, this is the default of lp_solve so we
	// don't have to do it here:
	// set_minim(fLP);

	set_verbose(fLP, 1);
}


/**
 * Destructor.
 * Removes the specification and deletes all constraints,
 * objective function summands and variables.
 */
LinearSpec::~LinearSpec()
{
	_RemovePresolved();
	for (int32 i = 0; i < fConstraints.CountItems(); i++)
		delete (Constraint*)fConstraints.ItemAt(i);
	for (int32 i = 0; i < fObjFunction->CountItems(); i++)
		delete (Summand*)fObjFunction->ItemAt(i);
	while (fVariables.CountItems() > 0)
		RemoveVariable(fVariables.ItemAt(0));

	delete_lp(fLP);

	delete fObjFunction;
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

	double d = 0;
	int i = 0;

	if (!fVariables.AddItem(variable))
		return false;
	if (add_columnex(fLP, 0, &d, &i) == 0) {
		fVariables.RemoveItem(variable);
		return false;
	}

	if (!SetRange(variable, variable->Min(), variable->Max())) {
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

	if (!del_column(fLP, index))
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
LinearSpec::SetRange(Variable* variable, double min, double max)
{
	return set_bounds(fLP, IndexOf(variable), min, max);
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

	if (penaltyNeg != INFINITY && penaltyNeg != 0. && op != OperatorType(LE)) {
		constraint->fDNegObjSummand
			= new(std::nothrow) Summand(constraint->PenaltyNeg(),
				AddVariable());
		fObjFunction->AddItem(constraint->fDNegObjSummand);
		varIndexes[nCoefficient] = constraint->fDNegObjSummand->Var()->Index();
		coeffs[nCoefficient] = 1.0;
		nCoefficient++;
	}

	if (penaltyPos != INFINITY && penaltyPos != 0. && op != OperatorType(GE)) {
		constraint->fDPosObjSummand
			= new(std::nothrow) Summand(constraint->PenaltyPos(),
				AddVariable());
		fObjFunction->AddItem(constraint->fDPosObjSummand);
		varIndexes[nCoefficient] = constraint->fDPosObjSummand->Var()->Index();
		coeffs[nCoefficient] = -1.0;
		nCoefficient++;
	}

	if (!add_constraintex(fLP, nCoefficient, &coeffs[0], &varIndexes[0],
			(op == OperatorType(EQ) ? EQ : (op == OperatorType(GE)) ? GE
				: LE), rightSide))
		return false;

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

	del_constraint(fLP, constraint->Index());
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

	if (!set_rowex(fLP, constraint->Index(), i, &coeffs[0],
		&varIndexes[0]))
		return false;

	UpdateObjectiveFunction();
	_RemovePresolved();
	return true;
}


bool
LinearSpec::UpdateRightSide(Constraint* constraint)
{
	if (!set_rh(fLP, constraint->Index(), constraint->RightSide()))
		return false;

	_RemovePresolved();
	return true;
}


bool
LinearSpec::UpdateOperator(Constraint* constraint)
{
	OperatorType op = constraint->Op();
	if (!set_constr_type(fLP, constraint->Index(),
		(op == OperatorType(EQ)) ? EQ : (op == OperatorType(GE)) ? GE
			: LE))
		return false;

	_RemovePresolved();
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

	if (!set_obj_fnex(fLP, size, &coeffs[0], &varIndexes[0]))
		printf("Error in set_obj_fnex.\n");

	_RemovePresolved();
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
	_RemovePresolved();
	return constraint;
}


/**
 * Remove a cached presolved model, if existent.
 * This is automatically done each time after the model has been changed,
 * to avoid an old cached presolved model getting out of sync.
 */
void
LinearSpec::_RemovePresolved()
{
	if (fLpPresolved == NULL)
		return;
	delete_lp(fLpPresolved);
	fLpPresolved = NULL;
}


/**
 * Creates and caches a simplified version of the linear programming problem,
 * where redundant rows, columns and constraints are removed,
 * if it has not been created before.
 * Then, the simplified problem is solved.
 *
 * @return the result of the solving attempt
 */
ResultType
LinearSpec::_Presolve()
{
	bigtime_t start, end;
	start = system_time();

	if (fLpPresolved == NULL) {
		fLpPresolved = copy_lp(fLP);
		set_presolve(fLpPresolved, PRESOLVE_ROWS | PRESOLVE_COLS
			| PRESOLVE_LINDEP, get_presolveloops(fLpPresolved));
	}

	fResult = (ResultType)solve(fLpPresolved);
	fObjectiveValue = get_objective(fLpPresolved);

	if (fResult == OPTIMAL) {
		int32 size = fVariables.CountItems();
		for (int32 i = 0; i < size; i++) {
			Variable* current = (Variable*)fVariables.ItemAt(i);
			current->SetValue(get_var_primalresult(fLpPresolved,
					get_Norig_rows(fLpPresolved) + current->Index()));
		}
	}

	end = system_time();
	fSolvingTime = (end - start) / 1000.0;

	return fResult;
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
	if (fLpPresolved != NULL)
		return _Presolve();

	bigtime_t start, end;
	start = system_time();

	fResult = (ResultType)solve(fLP);
	fObjectiveValue = get_objective(fLP);

	if (fResult == OPTIMAL) {
		int32 size = fVariables.CountItems();
		double x[size];
		if (!get_variables(fLP, &x[0]))
			printf("Error in get_variables.\n");

		int32 i = 0;
		while (i < size) {
			((Variable*)fVariables.ItemAt(i))->SetValue(x[i]);
			i++;
		}
	}

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
void
LinearSpec::Save(const char* fileName)
{
	// TODO: Constness should be fixed in liblpsolve API.
	write_lp(fLP, const_cast<char*>(fileName));
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
	if (fOptimization == MINIMIZE)
		set_minim(fLP);
	else
		set_maxim(fLP);
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

