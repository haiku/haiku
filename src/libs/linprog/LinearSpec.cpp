/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "LinearSpec.h"


/**
 * Constructor.
 * Creates a new specification for a linear programming problem.
 */
LinearSpec::LinearSpec()
	:
	fLpPresolved(NULL),
	fOptimization(MINIMIZE),
	fObjFunction(new SummandList()),
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
	RemovePresolved();
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
	Variable* variable = new Variable(this);
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
	double d = 0;
	int i = 0;

	if (!fVariables.AddItem(variable))
		return false;
	if (add_columnex(fLP, 0, &d, &i) == 0) {
		fVariables.RemoveItem(variable);
		return false;
	}

	if (!SetRange(variable, -20000, 20000)) {
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
	variable->Invalidate();

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
	Constraint* c = new Constraint(this, summands, op, rightSide,
		INFINITY, INFINITY);
	RemovePresolved();
	return c;
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
	SummandList* summands = new SummandList(1);
	summands->AddItem(new Summand(coeff1, var1));
	Constraint* c = new Constraint(this, summands, op, rightSide,
		INFINITY, INFINITY);
	RemovePresolved();
	return c;
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
	SummandList* summands = new SummandList(2);
	summands->AddItem(new Summand(coeff1, var1));
	summands->AddItem(new Summand(coeff2, var2));
	Constraint* c = new Constraint(this, summands, op, rightSide,
		INFINITY, INFINITY);
	RemovePresolved();
	return c;
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
	SummandList* summands = new SummandList(3);
	summands->AddItem(new Summand(coeff1, var1));
	summands->AddItem(new Summand(coeff2, var2));
	summands->AddItem(new Summand(coeff3, var3));
	Constraint* c = new Constraint(this, summands, op, rightSide,
		INFINITY, INFINITY);
	RemovePresolved();
	return c;
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
	SummandList* summands = new SummandList(3);
	summands->AddItem(new Summand(coeff1, var1));
	summands->AddItem(new Summand(coeff2, var2));
	summands->AddItem(new Summand(coeff3, var3));
	summands->AddItem(new Summand(coeff4, var4));
	Constraint* c = new Constraint(this, summands, op, rightSide,
		INFINITY, INFINITY);
	RemovePresolved();
	return c;
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
	Constraint* c = new Constraint(this, summands, op, rightSide,
			penaltyNeg, penaltyPos);
	RemovePresolved();
	return c;
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
	SummandList* summands = new SummandList(1);
	summands->AddItem(new Summand(coeff1, var1));
	Constraint* c = new Constraint(this, summands, op, rightSide,
		penaltyNeg, penaltyPos);
	RemovePresolved();
	return c;
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
	SummandList* summands = new SummandList(2);
	summands->AddItem(new Summand(coeff1, var1));
	summands->AddItem(new Summand(coeff2, var2));
	Constraint* c = new Constraint(this, summands, op, rightSide,
		penaltyNeg, penaltyPos);
	RemovePresolved();
	return c;
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
	SummandList* summands = new SummandList(2);
	summands->AddItem(new Summand(coeff1, var1));
	summands->AddItem(new Summand(coeff2, var2));
	summands->AddItem(new Summand(coeff3, var3));
	Constraint* c = new Constraint(this, summands, op, rightSide,
		penaltyNeg, penaltyPos);
	RemovePresolved();
	return c;
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
	SummandList* summands = new SummandList(2);
	summands->AddItem(new Summand(coeff1, var1));
	summands->AddItem(new Summand(coeff2, var2));
	summands->AddItem(new Summand(coeff3, var3));
	summands->AddItem(new Summand(coeff4, var4));
	Constraint* c = new Constraint(this, summands, op, rightSide,
		penaltyNeg, penaltyPos);
	RemovePresolved();
	return c;
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
	return new PenaltyFunction(this, var, xs, gs);
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

	RemovePresolved();
}


/**
 * Remove a cached presolved model, if existent.
 * This is automatically done each time after the model has been changed,
 * to avoid an old cached presolved model getting out of sync.
 */
void
LinearSpec::RemovePresolved()
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
LinearSpec::Presolve()
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
		return Presolve();

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

