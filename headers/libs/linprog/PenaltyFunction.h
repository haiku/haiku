#ifndef	PENALTY_FUNCTION_H
#define	PENALTY_FUNCTION_H

#include <List.h>


namespace LinearProgramming {

class LinearSpec;
class Variable;

/**
 * Penalty function.
 */
class PenaltyFunction {

protected:
						PenaltyFunction(LinearSpec* ls, Variable* var, BList* xs, BList* gs);	

public:
						~PenaltyFunction();
	const Variable*		Var() const;
	const BList*			Xs() const;
	const BList*			Gs() const;

private:
	LinearSpec*			fLS;
	Variable*				fVar;
	BList*				fXs;		// double
	BList*				fGs;		// double
	BList*				fConstraints;
	BList*				fObjFunctionSummands;

public:
	friend class			LinearSpec;

};

}	// namespace LinearProgramming

using LinearProgramming::PenaltyFunction;

#endif	// PENALTY_FUNCTION_H
