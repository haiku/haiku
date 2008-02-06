#ifndef	SOFT_CONSTRAINT_H
#define	SOFT_CONSTRAINT_H

#include "Constraint.h"

#include <List.h>


namespace LinearProgramming {
	
class LinearSpec;
class ObjFunctionSummand;
class Variable;

/**
 * Soft constraint, i.e.&nbsp;one that does not necessarily have to be satisfied.
 * Use this instead of hard constraints to avoid over-constrained specifications.
 */
class SoftConstraint : public Constraint {

public:
	void					ChangeLeftSide(BList* coeffs, BList* vars);
	OperatorType			Op();
	void					SetOp(OperatorType value);
	double				PenaltyNeg();
	void					SetPenaltyNeg(double value);
	double				PenaltyPos();
	void					SetPenaltyPos(double value);
	//~ string				ToString();
	Variable*				DNeg() const;
	Variable*				DPos() const;
						~SoftConstraint();

protected:
						SoftConstraint(LinearSpec* ls, BList* coeffs, BList* vars, 
								OperatorType op, double rightSide,
								double penaltyNeg, double penaltyPos);

private:
	Variable*				fDNeg;
	Variable*				fDPos;
	ObjFunctionSummand*	fDNegSummand;
	ObjFunctionSummand*	fDPosSummand;

public:
	friend class			LinearSpec;

};

}	// namespace LinearProgramming

using LinearProgramming::SoftConstraint;

#endif	// SOFT_CONSTRAINT_H
