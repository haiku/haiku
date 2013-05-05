/*
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef ACTICE_SET_SOLVER_H
#define ACTICE_SET_SOLVER_H


#include "LinearSpec.h"


class EquationSystem {
public:
								EquationSystem(int32 rows, int32 columns);
								~EquationSystem();

			void				SetRows(int32 rows);
			int32				Rows();
			int32				Columns();

			//! Debug function: check bounds
			bool				InRange(int32 row, int32 column);
	inline	double&				A(int32 row, int32 column);
	inline	double&				B(int32 row);
			/*! Copy the solved variables into results, keeping the original
			variable order. */
	inline	void				Results(double* results, int32 size);

	inline	void				SwapColumn(int32 i, int32 j);
	inline	void				SwapRow(int32 i, int32 j);

			bool				GaussianElimination();
			bool				GaussJordan();
			/*! Gauss Jordan elimination just for one column, the diagonal
			element must be none zero. */
			void				GaussJordan(int32 column);

			void				RemoveLinearlyDependentRows();
			void				RemoveUnusedVariables();

			void				MoveColumnRight(int32 i, int32 target);

			void				Print();
private:
	inline	void				_EliminateColumn(int32 column, int32 startRow,
									int32 endRow);

			int32*				fRowIndices;
			int32*				fColumnIndices;
			double**			fMatrix;
			double*				fB;
			int32				fRows;
			int32				fColumns;
};


class ActiveSetSolver : public LinearProgramming::SolverInterface {
public:
								ActiveSetSolver(LinearSpec* linearSpec);
								~ActiveSetSolver();

			ResultType			Solve();

			bool				VariableAdded(Variable* variable);
			bool				VariableRemoved(Variable* variable);
			bool				VariableRangeChanged(Variable* variable);

			bool				ConstraintAdded(Constraint* constraint);
			bool				ConstraintRemoved(Constraint* constraint);
			bool				LeftSideChanged(Constraint* constraint);
			bool				RightSideChanged(Constraint* constraint);
			bool				OperatorChanged(Constraint* constraint);
			bool				PenaltiesChanged(Constraint* constraint);

			bool				SaveModel(const char* fileName);

			ResultType			FindMaxs(const VariableList* variables);
			ResultType			FindMins(const VariableList* variables);

			void				GetRangeConstraints(const Variable* var,
									const Constraint** _min,
									const Constraint** _max) const;

private:
			void				_RemoveSoftConstraint(ConstraintList& list);
			void				_AddSoftConstraint(const ConstraintList& list);

	typedef Constraint* (*AddConstraintFunc)(LinearSpec* spec, Variable* var);

			ResultType			_FindWithConstraintsNoSoft(
									const VariableList* variables,
									AddConstraintFunc constraintFunc);

	const	VariableList&		fVariables;
	const	ConstraintList&		fConstraints;

			ConstraintList		fVariableGEConstraints;
			ConstraintList		fVariableLEConstraints;
};


#endif // ACTICE_SET_SOLVER_H
