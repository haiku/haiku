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


//#define DEBUG_ACTIVE_SOLVER

#ifdef DEBUG_ACTIVE_SOLVER
#include <stdio.h>
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
EquationSystem::GaussJordan()
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
				printf("can't solve column %i\n", i);
				return false;
			}
			SwapColumn(i, swapColumn);
		}
		if (i != swapRow)
			SwapRow(i, swapRow);

		// normalize
		GaussJordan(i);
	}
	return true;
}


void
EquationSystem::GaussJordan(int32 i)
{
	double value = fMatrix[fRowIndices[i]][fColumnIndices[i]];
	for (int j = 0; j < fColumns; j++)
		fMatrix[fRowIndices[i]][fColumnIndices[j]] /= value;
	fB[i] /= value;

	for (int r = 0; r < fRows; r++) {
		if (r == i)
			continue;
		double q = -fMatrix[fRowIndices[r]][fColumnIndices[i]];
		// don't need to do nothing, since matrix is typically sparse this
		// should save some work
		if (fuzzy_equals(q, 0))
			continue;
		for (int c = 0; c < fColumns; c++)
			fMatrix[fRowIndices[r]][fColumnIndices[c]]
				+= fMatrix[fRowIndices[i]][fColumnIndices[c]] * q;

		fB[r] += fB[i] * q;
	}
}


void
EquationSystem::RemoveLinearlyDependentRows()
{
	double oldB[fRows];
	for (int r = 0; r < fRows; r++)
		oldB[r] = fB[r];

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


ActiveSetSolver::ActiveSetSolver(LinearSpec* linearSpec)
	:
	QPSolverInterface(linearSpec),

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
bool
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
			printf("can't solve\n");
			return false;
		}

		system.SwapColumn(smallestBRow, negValueCol);

		// eliminate
		system.GaussJordan(smallestBRow);
	}

	return true;
}


ResultType
ActiveSetSolver::Solve()
{
	int32 nConstraints = fConstraints.CountItems();
	int32 nVariables = fVariables.CountItems();

	if (nVariables > nConstraints) {
		printf("More variables then constraints! vars: %i, constraints: %i\n",
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
		Constraint* constraint = fConstraints.ItemAt(c);
		if (constraint->IsSoft())
			continue;
		SummandList* leftSide = constraint->LeftSide();
		system.B(rowIndex) = constraint->RightSide();
		for (int32 sIndex = 0; sIndex < leftSide->CountItems(); sIndex++ ) {
			Summand* summand = leftSide->ItemAt(sIndex);
			double coefficient = summand->Coeff();
			system.A(rowIndex, summand->VariableIndex()) = coefficient;
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
	printf("base system solved\n");

	LayoutOptimizer optimizer(fConstraints, nVariables);
	optimizer.Solve(results);

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
	// TODO: error checks
	fVariableGEConstraints.AddItem(NULL);
	fVariableLEConstraints.AddItem(NULL);
	return true;
}


bool
ActiveSetSolver::VariableRemoved(Variable* variable)
{
	fVariableGEConstraints.RemoveItemAt(variable->Index());
	fVariableLEConstraints.RemoveItemAt(variable->Index());
	return true;
}


bool
ActiveSetSolver::VariableRangeChanged(Variable* variable)
{
	double min = variable->Min();
	double max = variable->Max();
	int32 variableIndex = variable->Index();

	Constraint* constraintGE = fVariableGEConstraints.ItemAt(variableIndex);
	Constraint* constraintLE = fVariableLEConstraints.ItemAt(variableIndex);
	if (constraintGE == NULL && min > -20000) {
		constraintGE = fLinearSpec->AddConstraint(1, variable, kGE, 0);
		if (constraintGE == NULL)
			return false;
		fVariableGEConstraints.RemoveItemAt(variableIndex);
		fVariableGEConstraints.AddItem(constraintGE, variableIndex);
	}
	if (constraintLE == NULL && max < 20000) {
		constraintLE = fLinearSpec->AddConstraint(1, variable, kLE, 20000);
		if (constraintLE == NULL)
			return false;
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
	return QPSolverInterface::ConstraintAdded(constraint);
}


bool
ActiveSetSolver::ConstraintRemoved(Constraint* constraint)
{
	return QPSolverInterface::ConstraintRemoved(constraint);
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
ActiveSetSolver::SaveModel(const char* fileName)
{
	return false;
}


BSize
ActiveSetSolver::MinSize(Variable* width, Variable* height)
{
	ConstraintList softConstraints;
	_RemoveSoftConstraint(softConstraints);

	Constraint* heightConstraint = fLinearSpec->AddConstraint(1, height,
		kEQ, 0, 5, 5);
	Constraint* widthConstraint = fLinearSpec->AddConstraint(1, width,
		kEQ, 0, 5, 5);
	ResultType result = Solve();
	fLinearSpec->RemoveConstraint(heightConstraint);
	fLinearSpec->RemoveConstraint(widthConstraint);

	_AddSoftConstraint(softConstraints);

	if (result == kUnbounded)
		return kMinSize;
	if (result != kOptimal)
		printf("Could not solve the layout specification (%d). ", result);

	return BSize(width->Value(), height->Value());
}


BSize
ActiveSetSolver::MaxSize(Variable* width, Variable* height)
{
	ConstraintList softConstraints;
	_RemoveSoftConstraint(softConstraints);

	const double kHugeValue = 32000;
	Constraint* heightConstraint = fLinearSpec->AddConstraint(1, height,
		kEQ, kHugeValue, 5, 5);
	Constraint* widthConstraint = fLinearSpec->AddConstraint(1, width,
		kEQ, kHugeValue, 5, 5);
	ResultType result = Solve();
	fLinearSpec->RemoveConstraint(heightConstraint);
	fLinearSpec->RemoveConstraint(widthConstraint);

	_AddSoftConstraint(softConstraints);

	if (result == kUnbounded)
		return kMaxSize;
	if (result != kOptimal)
		printf("Could not solve the layout specification (%d). ", result);

	return BSize(width->Value(), height->Value());
}


void
ActiveSetSolver::_RemoveSoftConstraint(ConstraintList& list)
{
	ConstraintList allConstraints = fLinearSpec->Constraints();
	for (int i = 0; i < allConstraints.CountItems(); i++) {
		Constraint* constraint = allConstraints.ItemAt(i);
		if (!constraint->IsSoft())
			continue;

		if (fLinearSpec->RemoveConstraint(constraint, false) == true)
			list.AddItem(constraint);
	}
}


void
ActiveSetSolver::_AddSoftConstraint(const ConstraintList& list)
{
	for (int i = 0; i < list.CountItems(); i++) {
		Constraint* constraint = list.ItemAt(i);
		// at least don't leak it
		if (fLinearSpec->AddConstraint(constraint) == false)
			delete constraint;
	}
}

