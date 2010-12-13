/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "LPSolveInterface.h"


using namespace LinearProgramming;


LPSolveInterface::LPSolveInterface()
	:
	fLpPresolved(NULL),
	fLP(NULL)
{
	fLP = make_lp(0, 0);
	if (fLP == NULL)
		printf("Couldn't construct a new model.");

	// minimize the objective functions, this is the default of lp_solve so we
	// don't have to do it here:
	// set_minim(fLP);

	set_verbose(fLP, 1);
}


LPSolveInterface::~LPSolveInterface()
{
	_RemovePresolved();
	delete_lp(fLP);
}


ResultType
LPSolveInterface::Solve(VariableList& variables)
{
	if (fLpPresolved != NULL)
		return _Presolve(variables);

	ResultType result = (ResultType)solve(fLP);

	if (result == OPTIMAL) {
		int32 size = variables.CountItems();
		double x[size];
		if (!get_variables(fLP, &x[0]))
			printf("Error in get_variables.\n");

		for (int32 i = 0; i < size; i++)
			variables.ItemAt(i)->SetValue(x[i]);
	}
	return result;
}


double
LPSolveInterface::GetObjectiveValue()
{
	if (fLpPresolved)
		return get_objective(fLpPresolved);
	return get_objective(fLP);
}


bool
LPSolveInterface::AddVariable()
{
	double d = 0;
	int i = 0;
	if (!add_columnex(fLP, 0, &d, &i))
		return false;
	_RemovePresolved();
	return true;
}


bool
LPSolveInterface::RemoveVariable(int variable)
{
	if (!del_column(fLP, variable))
		return false;
	_RemovePresolved();
	return true;
}


bool
LPSolveInterface::SetVariableRange(int variable, double min, double max)
{
	if (!set_bounds(fLP, variable, min, max))
		return false;
	_RemovePresolved();
	return true;
}


bool
LPSolveInterface::AddConstraint(int nElements, double* coefficients,
	int* variableIndices, OperatorType op, double rightSide)
{
	if (!add_constraintex(fLP, nElements, coefficients, variableIndices,
		(op == kEQ ? EQ : (op == kGE) ? GE : LE), rightSide)) {
		return false;
	}
	_RemovePresolved();
	return true;
}


bool
LPSolveInterface::RemoveConstraint(int constraint)
{
	if (!del_constraint(fLP, constraint))
		return false;
	_RemovePresolved();
	return true;
}


bool
LPSolveInterface::SetLeftSide(int constraint, int nElements,
	double* coefficients, int* variableIndices)
{
	if (!set_rowex(fLP, constraint, nElements, coefficients, variableIndices))
		return false;
	_RemovePresolved();
	return true;
}


bool
LPSolveInterface::SetRightSide(int constraint, double value)
{
	if (!set_rh(fLP, constraint, value))
		return false;
	_RemovePresolved();
	return true;
}


bool
LPSolveInterface::SetOperator(int constraint, OperatorType op)
{
	if (!set_constr_type(fLP, constraint, op == kEQ) ? EQ : (op == kGE) ? GE
		: LE) {
		return false;
	}
	_RemovePresolved();
	return true;
}


bool
LPSolveInterface::SetObjectiveFunction(int nElements, double* coefficients,
	int* variableIndices)
{
	if (!set_obj_fnex(fLP, nElements, coefficients, variableIndices))
		return false;
	_RemovePresolved();
	return true;
}


bool
LPSolveInterface::SetOptimization(OptimizationType value)
{
	if (value == kMinimize)
		set_minim(fLP);
	else
		set_maxim(fLP);
	return true;
}


bool
LPSolveInterface::SaveModel(const char* fileName)
{
	// TODO: Constness should be fixed in liblpsolve API.
	if (!write_lp(fLP, const_cast<char*>(fileName)))
		return false;
	return true;
}


/**
 * Remove a cached presolved model, if existent.
 * This is automatically done each time after the model has been changed,
 * to avoid an old cached presolved model getting out of sync.
 */
void
LPSolveInterface::_RemovePresolved()
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
LPSolveInterface::_Presolve(VariableList& variables)
{
	if (fLpPresolved == NULL) {
		fLpPresolved = copy_lp(fLP);
		set_presolve(fLpPresolved, PRESOLVE_ROWS | PRESOLVE_COLS
			| PRESOLVE_LINDEP, get_presolveloops(fLpPresolved));
	}

	ResultType result = (ResultType)solve(fLpPresolved);

	if (result == OPTIMAL) {
		int32 size = variables.CountItems();
		for (int32 i = 0; i < size; i++) {
			Variable* current = variables.ItemAt(i);
			current->SetValue(get_var_primalresult(fLpPresolved,
				get_Norig_rows(fLpPresolved) + current->Index()));
		}
	}

	return result;
}
