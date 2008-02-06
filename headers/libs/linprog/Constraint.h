#ifndef	CONSTRAINT_H
#define	CONSTRAINT_H

#include "OperatorType.h"

#include <List.h>
#include <String.h>
#include <SupportDefs.h>


namespace LinearProgramming {
	
class LinearSpec;

/**
 * Hard linear constraint, i.e.&nbsp;one that must be satisfied. 
 * May render a specification infeasible.
 */
class Constraint {
	
public:
	int32				Index();
	BList*				Coeffs();
	BList*				Vars();
	virtual void			ChangeLeftSide(BList* coeffs, BList* vars);
	virtual OperatorType	Op();
	virtual void			SetOp(OperatorType value);
	double				RightSide();
	void					SetRightSide(double value);
	BString				ToString();
	virtual				~Constraint();
	
protected:
						Constraint();

private:
						Constraint(LinearSpec* ls, BList* coeffs, BList* vars, 
								OperatorType op, double rightSide);

protected:
	LinearSpec*			fLS;
	BList*				fCoeffs;
	BList*				fVars;
	OperatorType			fOp;
	double				fRightSide;

public:
	friend class			LinearSpec;

};

}	// namespace LinearProgramming

using LinearProgramming::Constraint;

#endif	// CONSTRAINT_H
