/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */
#ifndef	SUMMAND_H
#define	SUMMAND_H

#include <ObjectList.h>


namespace LinearProgramming {

class LinearSpec;
class Variable;

/**
 * A summand of a linear term.
 */
class Summand {
public:
								Summand(Summand* summand);
								Summand(double coeff, Variable* var);
								~Summand();

			double				Coeff();
			void				SetCoeff(double coeff);
			Variable*			Var();
			void				SetVar(Variable* var);

			int32				VariableIndex();
private:
			double				fCoeff;
			Variable*			fVar;
};

typedef BObjectList<Summand> SummandList;

}	// namespace LinearProgramming

using LinearProgramming::Summand;
using LinearProgramming::SummandList;


#endif	// OBJ_FUNCTION_SUMMAND_H

