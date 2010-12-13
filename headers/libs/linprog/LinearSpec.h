/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler, haiku@clemens-zeidler.de
 * Distributed under the terms of the MIT License.
 */
#ifndef	LINEAR_SPEC_H
#define	LINEAR_SPEC_H

#include <math.h>

#include <List.h>
#include <OS.h>
#include <String.h>
#include <SupportDefs.h>

#include "Constraint.h"
#include "LinearProgrammingTypes.h"
#include "PenaltyFunction.h"
#include "Summand.h"
#include "Variable.h"


namespace LinearProgramming {

class SolverInterface {
public:
	virtual						~SolverInterface() {}

	virtual ResultType			Solve(VariableList& variables) = 0;
	virtual double				GetObjectiveValue() = 0;

	virtual	bool				AddVariable() = 0;
	virtual	bool				RemoveVariable(int variable) = 0;
	virtual	bool				SetVariableRange(int variable, double min,
									double max) = 0;

	virtual	bool				AddConstraint(int nElements,
									double* coefficients, int* variableIndices,
									OperatorType op, double rightSide) = 0;
	virtual	bool				RemoveConstraint(int constraint) = 0;
	virtual	bool				SetLeftSide(int constraint, int nElements,
									double* coefficients,
									int* variableIndices) = 0;
	virtual	bool				SetRightSide(int constraint, double value) = 0;
	virtual	bool				SetOperator(int constraint,
									OperatorType op) = 0;

	virtual bool				SetObjectiveFunction(int nElements,
									double* coefficients,
									int* variableIndices) = 0;
	virtual bool				SetOptimization(OptimizationType value) = 0;

	virtual bool				SaveModel(const char* fileName) = 0;
};


/*!
 * Specification of a linear programming problem.
 */
class LinearSpec {
public:
								LinearSpec();
	virtual						~LinearSpec();

			Variable*			AddVariable();
			bool				AddVariable(Variable* variable);
			bool				RemoveVariable(Variable* variable,
									bool deleteVariable = true);
			int32				IndexOf(const Variable* variable) const;
			bool				UpdateRange(Variable* variable);

			bool				AddConstraint(Constraint* constraint);
			bool				RemoveConstraint(Constraint* constraint,
									bool deleteConstraint = true);

			Constraint*			AddConstraint(SummandList* summands,
									OperatorType op, double rightSide);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									OperatorType op, double rightSide);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									OperatorType op, double rightSide);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									double coeff3, Variable* var3,
									OperatorType op, double rightSide);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									double coeff3, Variable* var3,
									double coeff4, Variable* var4,
									OperatorType op, double rightSide);

			Constraint*			AddConstraint(SummandList* summands,
									OperatorType op, double rightSide,
									double penaltyNeg, double penaltyPos);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									OperatorType op, double rightSide,
									double penaltyNeg, double penaltyPos);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									OperatorType op, double rightSide,
									double penaltyNeg, double penaltyPos);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									double coeff3, Variable* var3,
									OperatorType op, double rightSide,
									double penaltyNeg, double penaltyPos);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									double coeff3, Variable* var3,
									double coeff4, Variable* var4,
									OperatorType op, double rightSide,
									double penaltyNeg, double penaltyPos);

			PenaltyFunction*	AddPenaltyFunction(Variable* var, BList* xs,
									BList* gs);

			SummandList*		ObjectiveFunction();
			//! Caller takes ownership of the Summand's and the SummandList.
			SummandList*		SwapObjectiveFunction(
									SummandList* objFunction);
			void				SetObjectiveFunction(SummandList* objFunction);
			void				UpdateObjectiveFunction();

			ResultType			Solve();
			bool				Save(const char* fileName);

			int32				CountColumns() const;
			OptimizationType	Optimization() const;
			void				SetOptimization(OptimizationType value);

			ResultType			Result() const;
			double				ObjectiveValue() const;
			double				SolvingTime() const;

			operator BString() const;
			void				GetString(BString& string) const;

	const	ConstraintList&		Constraints() const;

protected:
	friend class Constraint;
 			bool				UpdateLeftSide(Constraint* constraint);
			bool				UpdateRightSide(Constraint* constraint);
			bool				UpdateOperator(Constraint* constraint);
private:
			/*! Check if all entries != NULL otherwise delete the list and its
			entries. */
			bool				_CheckSummandList(SummandList* list);
			Constraint*			_AddConstraint(SummandList* leftSide,
									OperatorType op, double rightSide,
									double penaltyNeg, double penaltyPos);

			OptimizationType	fOptimization;

			SummandList*		fObjFunction;
			VariableList		fVariables;
			ConstraintList		fConstraints;
			ResultType			fResult;
			double 				fObjectiveValue;
			double 				fSolvingTime;

			SolverInterface*	fSolver;
};

}	// namespace LinearProgramming

using LinearProgramming::LinearSpec;

#endif	// LINEAR_SPEC_H
