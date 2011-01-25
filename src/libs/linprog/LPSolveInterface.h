/*
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef LP_SOLVE_INTERFACE_H
#define LP_SOLVE_INTERFACE_H


#include "LinearSpec.h"

#include "lp_lib.h"


namespace LinearProgramming {


class LPSolveInterface : public SolverInterface {
public:
								LPSolveInterface(LinearSpec* linearSpec);
								~LPSolveInterface();

			ResultType			Solve();

			bool				VariableAdded(Variable* variable);
			bool				VariableRemoved(Variable* variable);
			bool				VariableRangeChanged(Variable* variable);

			bool				ConstraintAdded(Constraint* constraint);
			bool				ConstraintRemoved(Constraint* constraint);
			bool				LeftSideChanged(Constraint* constraint);
			bool				RightSideChanged(Constraint* constraint);
			bool				OperatorChanged(Constraint* constraint);

			bool				SaveModel(const char* fileName);

			BSize				MinSize(Variable* width, Variable* height);
			BSize				MaxSize(Variable* width, Variable* height);

			bool				SetOptimization(OptimizationType value);
			OptimizationType	Optimization() const;
			bool				SetObjectiveFunction(int nElements,
									double* coefficients,
									int* variableIndices);
			SummandList*		ObjectiveFunction();
			double				GetObjectiveValue();
			//! Caller takes ownership of the Summand's and the SummandList.
			SummandList*		SwapObjectiveFunction(
									SummandList* objFunction);
			void				SetObjectiveFunction(SummandList* objFunction);
private:
			void				_UpdateObjectiveFunction();

			ResultType			_Presolve(const VariableList& variables);
			void				_RemovePresolved();

			lprec*				fLpPresolved;
			lprec*				fLP;

			OptimizationType	fOptimization;
			SummandList*		fObjFunction;
};


}	// namespace LinearProgramming


#endif // LP_SOLVE_INTERFACE_H
