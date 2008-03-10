/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#ifndef	LINEAR_SPEC_H
#define	LINEAR_SPEC_H

#include "Variable.h"
#include "Constraint.h"
#include "Summand.h"
#include "PenaltyFunction.h"
#include "OperatorType.h"
#include "ResultType.h"
#include "OptimizationType.h"

#include "lp_lib.h"

#include <List.h>
#include <OS.h>
#include <SupportDefs.h>
#include <math.h>


namespace LinearProgramming {
	
class Constraint;
class ObjFunctionSummand;
class PenaltyFunction;
class Variable;

/**
 * Specification of a linear programming problem.
 */
class LinearSpec {
	
public:
						LinearSpec();
						~LinearSpec();

	Variable*			AddVariable();

	Constraint*			AddConstraint(BList* summands, 
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

	Constraint*			AddConstraint(BList* summands, 
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
	
	PenaltyFunction*	AddPenaltyFunction(Variable* var, BList* xs, BList* gs);

	BList*				ObjFunction();
	void				SetObjFunction(BList* summands);
	void				UpdateObjFunction();

	ResultType			Presolve();
	void				RemovePresolved();
	ResultType			Solve();
	void				Save(char* fname);

	int32				CountColumns() const;
	OptimizationType	Optimization() const;
	void				SetOptimization(OptimizationType value);
	BList*				Variables() const;
	BList*				Constraints() const;
	ResultType			Result() const;
	double				ObjectiveValue() const;
	double				SolvingTime() const;

protected:
	int32 				fCountColumns;

private:
	lprec*				fLpPresolved;
	OptimizationType	fOptimization;
	lprec*				fLP;
	BList*				fObjFunction;
	BList*				fVariables;
	BList*				fConstraints;
	ResultType			fResult;
	double 				fObjectiveValue;
	double 				fSolvingTime;

public:
	friend class		Constraint;
	friend class		Variable;

};

}	// namespace LinearProgramming

using LinearProgramming::LinearSpec;

#endif	// LINEAR_SPEC_H
