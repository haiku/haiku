/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#include "LinearSpec.h"


/**
 * Constructor.
 * Creates a new specification for a linear programming problem.
 */
LinearSpec::LinearSpec()
{
	fLP = make_lp(0, 0);
	if (fLP == NULL)
		printf("Couldn't construct a new model.");
	set_verbose(fLP, 1);
	
	fObjFunction = new BList();
	fVariables = new BList();
	fConstraints = new BList();
	fCountColumns = 0;
	fLpPresolved = NULL;
	fOptimization = MINIMIZE;
	fResult = ERROR;
	fObjectiveValue = NAN;
	fSolvingTime = NAN;
}


/**
 * Destructor.
 * Removes the specification and deletes all constraints, 
 * objective function summands and variables.
 */
LinearSpec::~LinearSpec()
{
	RemovePresolved();
	int i;
	for (i=0; i<fConstraints->CountItems(); i++)
		delete (Constraint*)fConstraints->ItemAt(i);
	for (i=0; i<fObjFunction->CountItems(); i++)
		delete (Summand*)fObjFunction->ItemAt(i);
	for (i=0; i<fVariables->CountItems(); i++)
		delete (Variable*)fVariables->ItemAt(i);
	delete_lp(fLP);
}


/**
 * Adds a new variable to the specification.
 * 
 * @return the new variable
 */
Variable*
LinearSpec::AddVariable()
{
	return new Variable(this);
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
LinearSpec::AddConstraint(BList* summands, OperatorType op, double rightSide)
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
	BList* summands = new BList(1);
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
	BList* summands = new BList(2);
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
	BList* summands = new BList(3);
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
	BList* summands = new BList(3);
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
LinearSpec::AddConstraint(BList* summands, OperatorType op, 
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
	BList* summands = new BList(1);
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
	BList* summands = new BList(2);
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
	BList* summands = new BList(2);
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
	BList* summands = new BList(2);
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
 * @return BList containing the objective function's summands
 */
BList*
LinearSpec::ObjFunction()
{
	return fObjFunction;
}


/**
 * Sets a new objective function.
 * The old objective function summands are NOT deleted.
 * 
 * @param summands	BList containing the objective function's summands
 */
void
LinearSpec::SetObjFunction(BList* summands)
{
	fObjFunction = summands;
	UpdateObjFunction();
}


/**
 * Updates the internal representation of the objective function.
 * Must be called whenever the summands of the objective function are changed.
 */
void
LinearSpec::UpdateObjFunction()
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
		printf("Error in set_obj_fnex.");
	
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
		set_presolve(fLpPresolved, PRESOLVE_ROWS | PRESOLVE_COLS | PRESOLVE_LINDEP,
				get_presolveloops(fLpPresolved));
	}
	
	fResult = (ResultType)solve(fLpPresolved);
	fObjectiveValue = get_objective(fLpPresolved);
	
	if (fResult == OPTIMAL) {
		int32 size = fVariables->CountItems();
		for (int32 i = 0; i < size; i++) {
			Variable* current = (Variable*)fVariables->ItemAt(i);
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
		int32 size = fVariables->CountItems();
		double x[size];
		if (!get_variables(fLP, &x[0]))
			printf("Error in get_variables.");
		
		int32 i = 0;
		while (i < size) {
			((Variable*)fVariables->ItemAt(i))->SetValue(x[i]);
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
LinearSpec::Save(char* fname)
{
	write_lp(fLP, fname);
}


/**
 * Gets the number of columns.
 * 
 * @return the number of columns
 */
int32
LinearSpec::CountColumns() const
{
	return fCountColumns;
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
 * Gets the the variables.
 * 
 * @return the variables
 */
BList*
LinearSpec::Variables() const
{
	return fVariables;
}


/**
 * Gets the constraints.
 * 
 * @return the constraints
 */
BList*
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

