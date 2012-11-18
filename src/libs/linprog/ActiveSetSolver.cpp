/*
 * Copyright 2010 - 2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */


#include "ActiveSetSolver.h"

#include <algorithm>
#include <stdio.h>

#include "LayoutOptimizer.h"


// #define DEBUG_ACTIVE_SOLVER

#ifdef DEBUG_ACTIVE_SOLVER
#define TRACE(x...) printf(x)
#else
#define TRACE(x...) /* nothing */
#endif


using namespace LinearProgramming;
using namespace std;


EquationSystem::EquationSystem(int32 rows, int32 columns)
	:
	fRows(rows),
	fColumns(columns)
{
	fMatrix = allocate_matrix(fRows, fColumns);
	fB = new double[fColumns];
	// better init all values to prevent side cases where not all variables
	// needed to solve the problem, coping theses values to the results could
	// cause problems
	for (int i = 0; i < fColumns; i++)
		fB[i] = 0;
	zero_matrix(fMatrix, fRows, fColumns);

	fRowIndices = new int32[fRows];
	fColumnIndices = new int32[fColumns];
	for (int i = 0; i < fRows; i++)
		fRowIndices[i] = i;
	for (int i = 0; i < fColumns; i++)
		fColumnIndices[i] = i;
}


EquationSystem::~EquationSystem()
{
	free_matrix(fMatrix);
	delete[] fB;
	delete[] fRowIndices;
	delete[] fColumnIndices;
}


void
EquationSystem::SetRows(int32 rows)
{
	fRows = rows;
}


int32
EquationSystem::Rows()
{
	return fRows;
}


int32
EquationSystem::Columns()
{
	return fColumns;
}


bool
EquationSystem::InRange(int32 row, int32 column)
{
	if (row < 0 || row >= fRows)
		return false;
	if (column < 0 || column >= fColumns)
		return false;
	return true;
}


double&
EquationSystem::A(int32 row, int32 column)
{
	return fMatrix[fRowIndices[row]][fColumnIndices[column]];
}


double&
EquationSystem::B(int32 row)
{
	return fB[row];
}


void
EquationSystem::Results(double* results, int32 size)
{
	for (int i = 0; i < size; i++)
		results[i] = 0;
	for (int i = 0; i < fColumns; i++) {
		int32 index = fColumnIndices[i];
		if (index < fRows)
			results[index] = fB[i];
	}
}


void
EquationSystem::SwapColumn(int32 i, int32 j)
{
	swap(fColumnIndices[i], fColumnIndices[j]);
}


void
EquationSystem::SwapRow(int32 i, int32 j)
{
	swap(fRowIndices[i], fRowIndices[j]);
	swap(fB[i], fB[j]);
}


bool
EquationSystem::GaussianElimination()
{
	// basic solve
	for (int i = 0; i < fRows; i++) {
		// find none zero element
		int swapRow = -1;
		for (int r = i; r < fRows; r++) {
			double& value = fMatrix[fRowIndices[r]][fColumnIndices[i]];
			if (fuzzy_equals(value, 0))
				continue;
			swapRow = r;
			break;
		}
		if (swapRow == -1) {
			int swapColumn = -1;
			for (int c = i + 1; c < fColumns; c++) {
				double& value = fMatrix[fRowIndices[i]][fColumnIndices[c]];
				if (fuzzy_equals(value, 0))
					continue;
				swapRow = i;
				swapColumn = c;
				break;
			}
			if (swapColumn == -1) {
				TRACE("can't solve column %i\n", i);
				return false;
			}
			SwapColumn(i, swapColumn);
		}
		if (i != swapRow)
			SwapRow(i, swapRow);

		// normalize
		_EliminateColumn(i, i + 1, fRows - 1);
	}
	return true;
}


bool
EquationSystem::GaussJordan()
{
	if (!GaussianElimination())
		return false;

	for (int32 i = fRows - 1; i >= 0; i--)
		_EliminateColumn(i, 0, i - 1);

	return true;
}


void
EquationSystem::GaussJordan(int32 i)
{
	_EliminateColumn(i, 0, fRows - 1);
}


void
EquationSystem::RemoveLinearlyDependentRows()
{
	double** temp = allocate_matrix(fRows, fColumns);
	bool independentRows[fRows];

	// copy to temp
	copy_matrix(fMatrix, temp, fRows, fColumns);
	int nIndependent = compute_dependencies(temp, fRows, fColumns,
		independentRows);
	if (nIndependent == fRows)
		return;

	// remove the rows
	for (int i = 0; i < fRows; i++) {
		if (!independentRows[i]) {
			int lastDepRow = -1;
			for (int d = fRows - 1; d > i; d--) {
				if (independentRows[d]) {
					lastDepRow = d;
					break;
				}
			}
			if (lastDepRow < 0)
				break;
			SwapRow(i, lastDepRow);
			fRows--;
		}
	}
	fRows = nIndependent;

	free_matrix(temp);
}


void
EquationSystem::RemoveUnusedVariables()
{
	for (int c = 0; c < fColumns; c++) {
		bool used = false;
		for (int r = 0; r < fRows; r++) {
			if (!fuzzy_equals(fMatrix[r][fColumnIndices[c]], 0)) {
				used = true;
				break;
			}
		}
		if (used)
			continue;

		//MoveColumnRight(c, fColumns - 1);
		SwapColumn(c, fColumns - 1);
		fColumns--;
		c--;
	}
}


void
EquationSystem::MoveColumnRight(int32 i, int32 target)
{
	int32 index = fColumnIndices[i];
	for (int c = i; c < target; c++)
		fColumnIndices[c] = fColumnIndices[c + 1];
	fColumnIndices[target] = index;
}


void
EquationSystem::Print()
{
	for (int m = 0; m < fRows; m++) {
		for (int n = 0; n < fColumns; n++)
			printf("%.1f ", fMatrix[fRowIndices[m]][fColumnIndices[n]]);
		printf("= %.1f\n", fB[m]);
	}
}


void
EquationSystem::_EliminateColumn(int32 column, int32 startRow, int32 endRow)
{
	double value = fMatrix[fRowIndices[column]][fColumnIndices[column]];
	if (value != 1.) {
		for (int j = column; j < fColumns; j++)
			fMatrix[fRowIndices[column]][fColumnIndices[j]] /= value;
		fB[column] /= value;
	}

	for (int r = startRow; r < endRow + 1; r++) {
		if (r == column)
			continue;
		double q = -fMatrix[fRowIndices[r]][fColumnIndices[column]];
		// don't need to do anything, since matrix is typically sparse this
		// should save some work
		if (fuzzy_equals(q, 0))
			continue;
		for (int c = column; c < fColumns; c++)
			fMatrix[fRowIndices[r]][fColumnIndices[c]]
				+= fMatrix[fRowIndices[column]][fColumnIndices[c]] * q;

		fB[r] += fB[column] * q;
	}
}


namespace {


Constraint*
AddMinConstraint(LinearSpec* spec, Variable* var)
{
	return spec->AddConstraint(1, var, kEQ, 0, 5, 5);
}


Constraint*
AddMaxConstraint(LinearSpec* spec, Variable* var)
{
	static const double kHugeValue = 32000;
	return spec->AddConstraint(1, var, kEQ, kHugeValue, 5, 5);
}

};


ActiveSetSolver::ActiveSetSolver(LinearSpec* linearSpec)
	:
	SolverInterface(linearSpec),

	fVariables(linearSpec->UsedVariables()),
	fConstraints(linearSpec->Constraints())
{

}


ActiveSetSolver::~ActiveSetSolver()
{

}


/* Using algorithm found in:
Solving Inequalities and Proving Farkas's Lemma Made Easy
David Avis and Bohdan Kaluzny
The American Mathematical Monthly
Vol. 111, No. 2 (Feb., 2004), pp. 152-157 */
static bool
solve(EquationSystem& system)
{
	// basic solve
	if (!system.GaussJordan())
		return false;

	bool done = false;
	while (!done) {
		double smallestB = HUGE_VALF;
		int smallestBRow = -1;
		for (int row = 0; row < system.Rows(); row++) {
			if (system.B(row) > 0 || fuzzy_equals(system.B(row), 0))
				continue;

			double bValue = fabs(system.B(row));
			if (bValue < smallestB) {
				smallestB = bValue;
				smallestBRow = row;
			}
		}
		if (smallestBRow == -1) {
			done = true;
			break;
		}

		int negValueCol = -1;
		for (int col = system.Rows(); col < system.Columns(); col++) {
			double value = system.A(smallestBRow, col);
			if (value > 0 || fuzzy_equals(value, 0))
				continue;
			negValueCol = col;
			break;
		}
		if (negValueCol == -1) {
			TRACE("can't solve\n");
			return false;
		}

		system.SwapColumn(smallestBRow, negValueCol);

		// eliminate
		system.GaussJordan(smallestBRow);
	}

	return true;
}


static bool is_soft_inequality(Constraint* constraint)
{
	if (constraint->PenaltyNeg() <= 0. && constraint->PenaltyPos() <= 0.)
		return false;
	if (constraint->Op() != kEQ)
		return true;
	return false;
}


class SoftInequalityAdder {
public:
	SoftInequalityAdder(LinearSpec* linSpec, ConstraintList& allConstraints)
		:
		fLinearSpec(linSpec)
	{
		for (int32 c = 0; c < allConstraints.CountItems(); c++) {
			Constraint* constraint = allConstraints.ItemAt(c);
			if (!is_soft_inequality(constraint))
				continue;
	
			Variable* variable = fLinearSpec->AddVariable();
			variable->SetRange(0, 20000);

			Constraint* helperConst = fLinearSpec->AddConstraint(1, variable,
				kEQ, 0, constraint->PenaltyNeg(), constraint->PenaltyPos());
			fInequalitySoftConstraints.AddItem(helperConst);
			
			double coeff = -1;
			if (constraint->Op() == kGE)
				coeff = 1;
	
			Constraint* modifiedConstraint = new Constraint(constraint);
			allConstraints.AddItem(modifiedConstraint, c);
			allConstraints.RemoveItemAt(c + 1);
			fModifiedIneqConstraints.AddItem(modifiedConstraint);
			modifiedConstraint->LeftSide()->AddItem(
				new Summand(coeff, variable));
		}
		for (int32 c = 0; c < fInequalitySoftConstraints.CountItems(); c++)
			allConstraints.AddItem(fInequalitySoftConstraints.ItemAt(c));
	}

	~SoftInequalityAdder()
	{
		for (int32 c = 0; c < fModifiedIneqConstraints.CountItems(); c++)
			delete fModifiedIneqConstraints.ItemAt(c);
		for (int32 c = 0; c < fInequalitySoftConstraints.CountItems(); c++) {
			Constraint* con = fInequalitySoftConstraints.ItemAt(c);
			fLinearSpec->RemoveVariable(con->LeftSide()->ItemAt(0)->Var());
				// this also deletes the constraint
		}
	}

private:
	LinearSpec*		fLinearSpec;
	ConstraintList	fModifiedIneqConstraints;
	ConstraintList	fInequalitySoftConstraints;
};


ResultType
ActiveSetSolver::Solve()
{
	
	// make a copy of the original constraints and create soft inequality
	// constraints
	ConstraintList allConstraints(fConstraints);
	SoftInequalityAdder adder(fLinearSpec, allConstraints);

	int32 nConstraints = allConstraints.CountItems();
	int32 nVariables = fVariables.CountItems();

	if (nVariables > nConstraints) {
		TRACE("More variables then constraints! vars: %i, constraints: %i\n",
			(int)nVariables, (int)nConstraints);
		return kInfeasible;
	}

	/* First find an initial solution and then optimize it using the active set
	method. */
	EquationSystem system(nConstraints, nVariables + nConstraints);

	int32 slackIndex = nVariables;
	// setup constraint matrix and add slack variables if necessary
	int32 rowIndex = 0;
	for (int32 c = 0; c < nConstraints; c++) {
		Constraint* constraint = allConstraints.ItemAt(c);
		if (is_soft(constraint))
			continue;
		SummandList* leftSide = constraint->LeftSide();
		system.B(rowIndex) = constraint->RightSide();
		for (int32 sIndex = 0; sIndex < leftSide->CountItems(); sIndex++ ) {
			Summand* summand = leftSide->ItemAt(sIndex);
			double coefficient = summand->Coeff();
			int32 columnIndex = summand->VariableIndex();
			system.A(rowIndex, columnIndex) = coefficient;
		}
		if (constraint->Op() == kLE) {
			system.A(rowIndex, slackIndex) = 1;
			slackIndex++;
		} else if (constraint->Op() == kGE) {
			system.A(rowIndex, slackIndex) = -1;
			slackIndex++;
		}
		rowIndex++;
	}

	system.SetRows(rowIndex);

	system.RemoveLinearlyDependentRows();
	system.RemoveUnusedVariables();

	if (!solve(system))
		return kInfeasible;

	double results[nVariables + nConstraints];
	system.Results(results, nVariables + nConstraints);
	TRACE("base system solved\n");

	LayoutOptimizer optimizer(allConstraints, nVariables);
	if (!optimizer.Solve(results))
		return kInfeasible;

	// back to the variables
	for (int32 i = 0; i < nVariables; i++)
		fVariables.ItemAt(i)->SetValue(results[i]);

	for (int32 i = 0; i < nVariables; i++)
		TRACE("var %f\n", results[i]);

	return kOptimal;
}


bool
ActiveSetSolver::VariableAdded(Variable* variable)
{
	if (fVariableGEConstraints.AddItem(NULL) == false)
		return false;
	if (fVariableLEConstraints.AddItem(NULL) == false) {
		// clean up
		int32 count = fVariableGEConstraints.CountItems();
		fVariableGEConstraints.RemoveItemAt(count - 1);
		return false;	
	}
	return true;
}


bool
ActiveSetSolver::VariableRemoved(Variable* variable)
{
	fVariableGEConstraints.RemoveItemAt(variable->GlobalIndex());
	fVariableLEConstraints.RemoveItemAt(variable->GlobalIndex());
	return true;
}


bool
ActiveSetSolver::VariableRangeChanged(Variable* variable)
{
	double min = variable->Min();
	double max = variable->Max();
	int32 variableIndex = variable->GlobalIndex();

	Constraint* constraintGE = fVariableGEConstraints.ItemAt(variableIndex);
	Constraint* constraintLE = fVariableLEConstraints.ItemAt(variableIndex);
	if (constraintGE == NULL && min > -20000) {
		constraintGE = fLinearSpec->AddConstraint(1, variable, kGE, 0);
		if (constraintGE == NULL)
			return false;
		constraintGE->SetLabel("Var Min");
		fVariableGEConstraints.RemoveItemAt(variableIndex);
		fVariableGEConstraints.AddItem(constraintGE, variableIndex);
	}
	if (constraintLE == NULL && max < 20000) {
		constraintLE = fLinearSpec->AddConstraint(1, variable, kLE, 20000);
		if (constraintLE == NULL)
			return false;
		constraintLE->SetLabel("Var Max");
		fVariableLEConstraints.RemoveItemAt(variableIndex);
		fVariableLEConstraints.AddItem(constraintLE, variableIndex);
	}

	if (constraintGE)
		constraintGE->SetRightSide(min);
	if (constraintLE)
		constraintLE->SetRightSide(max);
	return true;
}


bool
ActiveSetSolver::ConstraintAdded(Constraint* constraint)
{
	return true;
}


bool
ActiveSetSolver::ConstraintRemoved(Constraint* constraint)
{
	return true;
}


bool
ActiveSetSolver::LeftSideChanged(Constraint* constraint)
{
	return true;
}


bool
ActiveSetSolver::RightSideChanged(Constraint* constraint)
{
	return true;
}


bool
ActiveSetSolver::OperatorChanged(Constraint* constraint)
{
	return true;
}


bool
ActiveSetSolver::PenaltiesChanged(Constraint* constraint)
{
	return true;
}


bool
ActiveSetSolver::SaveModel(const char* fileName)
{
	return false;
}


ResultType
ActiveSetSolver::FindMaxs(const VariableList* variables)
{
	return _FindWithConstraintsNoSoft(variables, AddMaxConstraint);
}


ResultType
ActiveSetSolver::FindMins(const VariableList* variables)
{
	return _FindWithConstraintsNoSoft(variables, AddMinConstraint);
}


void
ActiveSetSolver::GetRangeConstraints(const Variable* var,
	const Constraint** _min, const Constraint** _max) const
{
	int32 variableIndex = var->GlobalIndex();

	if (_min)
		*_min = fVariableGEConstraints.ItemAt(variableIndex);
	if (_max)
		*_max = fVariableLEConstraints.ItemAt(variableIndex);
}


void
ActiveSetSolver::_RemoveSoftConstraint(ConstraintList& list)
{
	ConstraintList allConstraints = fLinearSpec->Constraints();
	for (int i = 0; i < allConstraints.CountItems(); i++) {
		Constraint* constraint = allConstraints.ItemAt(i);
		// soft eq an ineq constraint?
		if (constraint->PenaltyNeg() <= 0. && constraint->PenaltyPos() <= 0.)
			continue;

		if (RemoveConstraint(constraint, false, false) == true)
			list.AddItem(constraint);
	}
}


void
ActiveSetSolver::_AddSoftConstraint(const ConstraintList& list)
{
	for (int i = 0; i < list.CountItems(); i++) {
		Constraint* constraint = list.ItemAt(i);
		// at least don't leak it
		if (AddConstraint(constraint, false) == false)
			delete constraint;
	}
}


ResultType
ActiveSetSolver::_FindWithConstraintsNoSoft(const VariableList* variables,
	AddConstraintFunc constraintFunc)
{
	ConstraintList softConstraints;
	_RemoveSoftConstraint(softConstraints);

	ConstraintList constraints;
	for (int32 i = 0; i < variables->CountItems(); i++)
		constraints.AddItem(constraintFunc(fLinearSpec, variables->ItemAt(i)));

	ResultType result = Solve();
	for (int32 i = 0; i < constraints.CountItems(); i++)
		fLinearSpec->RemoveConstraint(constraints.ItemAt(i));

	_AddSoftConstraint(softConstraints);

	if (result != kOptimal)
		TRACE("Could not solve the layout specification (%d). ", result);

	return result;
}
