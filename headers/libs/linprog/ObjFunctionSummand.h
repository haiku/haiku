#ifndef	OBJ_FUNCTION_SUMMAND_H
#define	OBJ_FUNCTION_SUMMAND_H


namespace LinearProgramming {

class LinearSpec;
class Variable;

/**
 * A summand of the objective function.
 */
class ObjFunctionSummand {

public:
	double				Coeff();
	void					SetCoeff(double coeff);
	Variable*				Var();
	void					SetVar(Variable* var);
						~ObjFunctionSummand();

protected:
						ObjFunctionSummand(LinearSpec* ls, double coeff, Variable* var);

private:
	LinearSpec*			fLS;
	double				fCoeff;
	Variable*				fVar;

public:
	friend class			LinearSpec;

};

}	// namespace LinearProgramming

using LinearProgramming::ObjFunctionSummand;

#endif	// OBJ_FUNCTION_SUMMAND_H
