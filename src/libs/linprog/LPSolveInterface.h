/*
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef LP_SOLVE_INTERFACE_H
#define LP_SOLVE_INTERFACE_H


#include "LinearSpec.h"

#include "lp_lib.h"


class LPSolveInterface : public LinearProgramming::SolverInterface {
public:
								LPSolveInterface();
								~LPSolveInterface();

			ResultType			Solve(VariableList& variables);
			double				GetObjectiveValue();

			bool				AddVariable();
			bool				RemoveVariable(int variable);
			bool				SetVariableRange(int variable, double min,
									double max);

			bool				AddConstraint(int nElements,
									double* coefficients, int* variableIndices,
									OperatorType op, double rightSide);
			bool				RemoveConstraint(int constraint);
			bool				SetLeftSide(int constraint, int nElements,
									double* coefficients, int* variableIndices);
			bool				SetRightSide(int constraint, double value);
			bool				SetOperator(int constraint,
									OperatorType op);

			bool				SetObjectiveFunction(int nElements,
									double* coefficients,
									int* variableIndices);
			bool				SetOptimization(OptimizationType value);

			bool				SaveModel(const char* fileName);

private:
			ResultType			_Presolve(VariableList& variables);
			void				_RemovePresolved();

			lprec*				fLpPresolved;
			lprec*				fLP;
};


#endif // LP_SOLVE_INTERFACE_H
