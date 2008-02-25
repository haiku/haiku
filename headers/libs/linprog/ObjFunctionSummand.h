/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#ifndef	OBJ_FUNCTION_SUMMAND_H
#define	OBJ_FUNCTION_SUMMAND_H

#include "Summand.h"


namespace LinearProgramming {

class LinearSpec;
class Variable;

/**
 * A summand of the objective function.
 */
class ObjFunctionSummand : public Summand {

public:
	void		SetCoeff(double coeff);
	void		SetVar(Variable* var);
				~ObjFunctionSummand();

protected:
						ObjFunctionSummand(LinearSpec* ls, double coeff, Variable* var);

private:
	LinearSpec*			fLS;

public:
	friend class		LinearSpec;

};

}	// namespace LinearProgramming

using LinearProgramming::ObjFunctionSummand;

#endif	// OBJ_FUNCTION_SUMMAND_H
