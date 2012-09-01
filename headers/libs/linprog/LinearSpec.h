/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler, haiku@clemens-zeidler.de
 * Distributed under the terms of the MIT License.
 */
#ifndef	LINEAR_SPEC_H
#define	LINEAR_SPEC_H

#include <ObjectList.h>
#include <OS.h>
#include <Referenceable.h>
#include <Size.h>
#include <String.h>
#include <SupportDefs.h>

#include "Constraint.h"
#include "LinearProgrammingTypes.h"
#include "Summand.h"
#include "Variable.h"


namespace LinearProgramming {


class LinearSpec;


const BSize kMinSize(0, 0);
const BSize kMaxSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);


class SolverInterface {
public:
								SolverInterface(LinearSpec* linSpec);

	virtual						~SolverInterface();

	virtual ResultType			Solve() = 0;

	virtual	bool				VariableAdded(Variable* variable) = 0;
	virtual	bool				VariableRemoved(Variable* variable) = 0;
	virtual	bool				VariableRangeChanged(Variable* variable) = 0;

	virtual	bool				ConstraintAdded(Constraint* constraint) = 0;
	virtual	bool				ConstraintRemoved(Constraint* constraint) = 0;
	virtual	bool				LeftSideChanged(Constraint* constraint) = 0;
	virtual	bool				RightSideChanged(Constraint* constraint) = 0;
	virtual	bool				OperatorChanged(Constraint* constraint) = 0;
	virtual	bool				PenaltiesChanged(Constraint* constraint) = 0;

	virtual bool				SaveModel(const char* fileName) = 0;

	virtual	ResultType			FindMins(const VariableList* variables) = 0;
	virtual	ResultType			FindMaxs(const VariableList* variables) = 0;

	virtual	void				GetRangeConstraints(const Variable* var,
									const Constraint** _min,
									const Constraint** _max) const = 0;

protected:
			bool				AddConstraint(Constraint* constraint,
									bool notifyListener);
			bool				RemoveConstraint(Constraint* constraint,
									bool deleteConstraint, bool notifyListener);
			
			LinearSpec*			fLinearSpec; 
};


class SpecificationListener {
public:
	virtual						~SpecificationListener();

	virtual	void				VariableAdded(Variable* variable);
	virtual	void				VariableRemoved(Variable* variable);
	virtual void				ConstraintAdded(Constraint* constraint);
	virtual void				ConstraintRemoved(Constraint* constraint);
};


/*!
 * Specification of a linear programming problem.
 */
class LinearSpec : public BReferenceable {
public:
								LinearSpec();
	virtual						~LinearSpec();

			/*! Does not takes ownership of the listener. */
			bool				AddListener(SpecificationListener* listener);
			bool				RemoveListener(SpecificationListener* listener);

			Variable*			AddVariable();
			bool				AddVariable(Variable* variable);
			bool				RemoveVariable(Variable* variable,
									bool deleteVariable = true);
			int32				IndexOf(const Variable* variable) const;
			int32				GlobalIndexOf(const Variable* variable) const;
			bool				UpdateRange(Variable* variable);

			bool				AddConstraint(Constraint* constraint);
			bool				RemoveConstraint(Constraint* constraint,
									bool deleteConstraint = true);

			Constraint*			AddConstraint(SummandList* summands,
									OperatorType op, double rightSide,
									double penaltyNeg = -1,
									double penaltyPos = -1);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									OperatorType op, double rightSide,
									double penaltyNeg = -1,
									double penaltyPos = -1);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									OperatorType op, double rightSide,
									double penaltyNeg = -1,
									double penaltyPos = -1);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									double coeff3, Variable* var3,
									OperatorType op, double rightSide,
									double penaltyNeg = -1,
									double penaltyPos = -1);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									double coeff3, Variable* var3,
									double coeff4, Variable* var4,
									OperatorType op, double rightSide,
									double penaltyNeg = -1,
									double penaltyPos = -1);

			ResultType			FindMins(const VariableList* variables);
			ResultType			FindMaxs(const VariableList* variables);

			ResultType			Solve();
			bool				Save(const char* fileName);

			int32				CountColumns() const;

			ResultType			Result() const;
			bigtime_t			SolvingTime() const;

			BString				ToString() const;

			void				GetRangeConstraints(const Variable*,
									const Constraint** _min,
								   	const Constraint** _max) const;
	const	ConstraintList&		Constraints() const;
	const	VariableList&		UsedVariables() const;
	const	VariableList&		AllVariables() const;

protected:
	friend class Constraint;
	friend class SolverInterface;

 			bool				UpdateLeftSide(Constraint* constraint,
 									const SummandList* oldSummands);
			bool				UpdateRightSide(Constraint* constraint);
			bool				UpdateOperator(Constraint* constraint);
			bool				UpdatePenalties(Constraint* constraint);

			bool				AddConstraint(Constraint* constraint,
									bool notifyListener);
			bool				RemoveConstraint(Constraint* constraint,
									bool deleteConstraint, bool notifyListener);
private:
			/*! Check if all entries != NULL otherwise delete the list and its
			entries. */
			bool				_CheckSummandList(SummandList* list);
			Constraint*			_AddConstraint(SummandList* leftSide,
									OperatorType op, double rightSide,
									double penaltyNeg, double penaltyPos);

			void				_AddConstraintRef(Variable* var);
			void				_RemoveConstraintRef(Variable* var);
			
			VariableList		fVariables;
			VariableList		fUsedVariables;
			ConstraintList		fConstraints;
			ResultType			fResult;
			bigtime_t 			fSolvingTime;

			SolverInterface*	fSolver;

			BObjectList<SpecificationListener>	fListeners;
};

}	// namespace LinearProgramming

using LinearProgramming::LinearSpec;

#endif	// LINEAR_SPEC_H
