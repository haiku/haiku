/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#ifndef	SUMMAND_H
#define	SUMMAND_H


namespace LinearProgramming {

class LinearSpec;
class Variable;

/**
 * A summand of a linear term.
 */
class Summand {

public:
	double			Coeff();
	void			SetCoeff(double coeff);
	Variable*		Var();
	void			SetVar(Variable* var);
					Summand(double coeff, Variable* var);
					~Summand();

private:
	double			fCoeff;
	Variable*		fVar;

};

}	// namespace LinearProgramming

using LinearProgramming::Summand;

#endif	// OBJ_FUNCTION_SUMMAND_H
