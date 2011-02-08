/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "LPSolveInterface.h"

#include <new>


using namespace LinearProgramming;


LPSolveInterface::LPSolveInterface(LinearSpec* linearSpec)
	:
	SolverInterface(linearSpec),

	fLpPresolved(NULL),
	fLP(NULL),
	fOptimization(kMinimize),
	fObjFunction(new(std::nothrow) SummandList())
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

	for (int32 i = 0; i < fObjFunction->CountItems(); i++)
		delete (Summand*)fObjFunction->ItemAt(i);
	delete fObjFunction;
}


ResultType
LPSolveInterface::Solve()
{
	const VariableList& variables = fLinearSpec->AllVariables();
	if (fLpPresolved != NULL)
		return _Presolve(variables);

	// Try to solve the layout until the result is kOptimal or kInfeasible,
	// maximally 15 tries sometimes the solving algorithm encounters numerical
	// problems (NUMFAILURE), and repeating the solving often helps to overcome
	// them.
	ResultType result = kInfeasible;
	for (int32 tries = 0; tries < 15; tries++) {
		result = (ResultType)solve(fLP);

		if (result == OPTIMAL) {
			int32 size = variables.CountItems();
			double x[size];
			if (!get_variables(fLP, &x[0]))
				printf("Error in get_variables.\n");

			for (int32 i = 0; i < size; i++)
				variables.ItemAt(i)->SetValue(x[i]);

			break;
		} else if (result == kInfeasible)
			break;
	}

	return result;
}


bool
LPSolveInterface::VariableAdded(Variable* variable)
{
	double d = 0;
	int i = 0;
	if (!add_columnex(fLP, 0, &d, &i))
		return false;
	_RemovePresolved();
	return true;
}


bool
LPSolveInterface::VariableRemoved(Variable* variable)
{
	if (!del_column(fLP, variable->GlobalIndex() + 1))
		return false;
	_RemovePresolved();
	return true;
}


bool
LPSolveInterface::VariableRangeChanged(Variable* variable)
{
	double min = variable->Min();
	double max = variable->Max();
	if (!set_bounds(fLP, variable->GlobalIndex() + 1, min, max))
		return false;
	_RemovePresolved();
	return true;
}


bool
LPSolveInterface::ConstraintAdded(Constraint* constraint)
{
	OperatorType op = constraint->Op();
	SummandList* summands = constraint->LeftSide();

	double coeffs[summands->CountItems() + 2];
	int variableIndices[summands->CountItems() + 2];
	int32 nCoefficient = 0;
	for (; nCoefficient < summands->CountItems(); nCoefficient++) {
		Summand* s = summands->ItemAt(nCoefficient);
		coeffs[nCoefficient] = s->Coeff();
		variableIndices[nCoefficient] = s->Var()->GlobalIndex() + 1;
	}

	double penaltyNeg = constraint->PenaltyNeg();
	if (penaltyNeg > 0. && op != kLE) {
		constraint->fDNegObjSummand = new(std::nothrow) Summand(
			constraint->PenaltyNeg(), fLinearSpec->AddVariable());
		fObjFunction->AddItem(constraint->fDNegObjSummand);
		variableIndices[nCoefficient]
			= constraint->fDNegObjSummand->Var()->GlobalIndex() + 1;
		coeffs[nCoefficient] = 1.0;
		nCoefficient++;
	}

	double penaltyPos = constraint->PenaltyPos();
	if (penaltyPos > 0. && op != kGE) {
		constraint->fDPosObjSummand = new(std::nothrow) Summand(
			constraint->PenaltyPos(), fLinearSpec->AddVariable());
		fObjFunction->AddItem(constraint->fDPosObjSummand);
		variableIndices[nCoefficient]
			= constraint->fDPosObjSummand->Var()->GlobalIndex() + 1;
		coeffs[nCoefficient] = -1.0;
		nCoefficient++;
	}

	double rightSide = constraint->RightSide();
	if (!add_constraintex(fLP, nCoefficient, coeffs, variableIndices,
		(op == kEQ ? EQ : (op == kGE) ? GE : LE), rightSide)) {
		return false;
	}

	_UpdateObjectiveFunction();
	_RemovePresolved();
	return true;
}


bool
LPSolveInterface::ConstraintRemoved(Constraint* constraint)
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

	if (!del_constraint(fLP, constraint->Index() + 1))
		return false;
	_UpdateObjectiveFunction();
	_RemovePresolved();
	return true;
}


bool
LPSolveInterface::LeftSideChanged(Constraint* constraint)
{
	if (!constraint->IsValid())
		return false;

	int32 index = constraint->Index() + 1;
	if (index <= 0)
		return false;

	SummandList* leftSide = constraint->LeftSide();
	OperatorType op = constraint->Op();

	double coeffs[leftSide->CountItems() + 2];
	int variableIndices[leftSide->CountItems() + 2];
	int32 i;
	for (i = 0; i < leftSide->CountItems(); i++) {
		Summand* s = leftSide->ItemAt(i);
		coeffs[i] = s->Coeff();
		variableIndices[i] = s->Var()->GlobalIndex() + 1;
	}

	double penaltyNeg = constraint->PenaltyNeg();
	if (penaltyNeg > 0. && op != kLE) {
		if (!constraint->fDNegObjSummand) {
			constraint->fDNegObjSummand = new(std::nothrow) Summand(
				constraint->PenaltyNeg(), fLinearSpec->AddVariable());
			fObjFunction->AddItem(constraint->fDNegObjSummand);
		}
		variableIndices[i] = constraint->fDNegObjSummand->Var()->GlobalIndex() + 1;
		coeffs[i] = 1.0;
		i++;
	} else {
		fObjFunction->RemoveItem(constraint->fDNegObjSummand);
		delete constraint->fDNegObjSummand;
		constraint->fDNegObjSummand = NULL;
	}

	double penaltyPos = constraint->PenaltyPos();
	if (penaltyPos > 0. && op != kGE) {
		if (constraint->fDPosObjSummand == NULL) {
			constraint->fDPosObjSummand = new(std::nothrow) Summand(penaltyPos,
				fLinearSpec->AddVariable());
			fObjFunction->AddItem(constraint->fDPosObjSummand);
		}
		variableIndices[i] = constraint->fDPosObjSummand->Var()->GlobalIndex() + 1;
		coeffs[i] = -1.0;
		i++;
	} else {
		fObjFunction->RemoveItem(constraint->fDPosObjSummand);
		delete constraint->fDPosObjSummand;
		constraint->fDPosObjSummand = NULL;
	}

	if (!set_rowex(fLP, index, i, coeffs, variableIndices))
		return false;
	_UpdateObjectiveFunction();
	_RemovePresolved();
	return true;
}


bool
LPSolveInterface::RightSideChanged(Constraint* constraint)
{
	if (!set_rh(fLP, constraint->Index() + 1, constraint->RightSide()))
		return false;
	_RemovePresolved();
	return true;
}


bool
LPSolveInterface::OperatorChanged(Constraint* constraint)
{
	int32 index = constraint->Index() + 1;
	if (index <= 0)
		return false;

	OperatorType op = constraint->Op();
	if (!set_constr_type(fLP, index, op == kEQ) ? EQ : (op == kGE) ? GE : LE) {
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
	fOptimization = value;
	if (fOptimization == kMinimize)
		set_minim(fLP);
	else
		set_maxim(fLP);
	return true;
}


/**
 * Gets the current optimization.
 * The default is minimization.
 *
 * @return the current optimization
 */
OptimizationType
LPSolveInterface::Optimization() const
{
	return fOptimization;
}


bool
LPSolveInterface::SaveModel(const char* fileName)
{
	// TODO: Constness should be fixed in liblpsolve API.
	if (!write_lp(fLP, const_cast<char*>(fileName)))
		return false;
	return true;
}


BSize
LPSolveInterface::MinSize(Variable* width, Variable* height)
{
	SummandList* newObjFunction = new(std::nothrow) SummandList(2);
	newObjFunction->AddItem(new(std::nothrow) Summand(1.0, width));
	newObjFunction->AddItem(new(std::nothrow) Summand(1.0, height));
	SummandList* oldObjFunction = SwapObjectiveFunction(newObjFunction);

	ResultType result = Solve();

	SetObjectiveFunction(oldObjFunction);

	if (result == kUnbounded)
		return kMinSize;
	if (result != kOptimal)
		printf("Could not solve the layout specification (%d). ", result);

	return BSize(width->Value(), height->Value());
}


BSize
LPSolveInterface::MaxSize(Variable* width, Variable* height)
{
	SummandList* newObjFunction = new(std::nothrow) SummandList(2);
	newObjFunction->AddItem(new(std::nothrow) Summand(-1.0, width));
	newObjFunction->AddItem(new(std::nothrow) Summand(-1.0, height));
	SummandList* oldObjFunction = SwapObjectiveFunction(
		newObjFunction);

	ResultType result = Solve();

	SetObjectiveFunction(oldObjFunction);

	if (result == kUnbounded)
		return kMinSize;
	if (result != kOptimal)
		printf("Could not solve the layout specification (%d). ", result);

	return BSize(width->Value(), height->Value());
}


SummandList*
LPSolveInterface::SwapObjectiveFunction(SummandList* objFunction)
{
	SummandList* list = fObjFunction;
	fObjFunction = objFunction;
	_UpdateObjectiveFunction();
	return list;
}


/**
 * Sets a new objective function.
 *
 * @param summands	SummandList containing the objective function's summands
 */
void
LPSolveInterface::SetObjectiveFunction(SummandList* objFunction)
{
	for (int32 i = 0; i < fObjFunction->CountItems(); i++)
		delete (Summand*)fObjFunction->ItemAt(i);
	delete fObjFunction;

	fObjFunction = objFunction;
	_UpdateObjectiveFunction();
}


/**
 * Gets the objective function.
 *
 * @return SummandList containing the objective function's summands
 */
SummandList*
LPSolveInterface::ObjectiveFunction()
{
	return fObjFunction;
}


double
LPSolveInterface::GetObjectiveValue()
{
	if (fLpPresolved)
		return get_objective(fLpPresolved);
	return get_objective(fLP);
}



/**
 * Updates the internal representation of the objective function.
 * Must be called whenever the summands of the objective function are changed.
 */
void
LPSolveInterface::_UpdateObjectiveFunction()
{
	int32 size = fObjFunction->CountItems();
	double coeffs[size];
	int varIndexes[size];
	Summand* current;
	for (int32 i = 0; i < size; i++) {
		current = (Summand*)fObjFunction->ItemAt(i);
		coeffs[i] = current->Coeff();
		varIndexes[i] = current->Var()->GlobalIndex() + 1;
	}

	if (!SetObjectiveFunction(size, &coeffs[0], &varIndexes[0]))
		printf("Error in set_obj_fnex.\n");
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
LPSolveInterface::_Presolve(const VariableList& variables)
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
				get_Norig_rows(fLpPresolved) + current->GlobalIndex() + 1));
		}
	}

	return result;
}
