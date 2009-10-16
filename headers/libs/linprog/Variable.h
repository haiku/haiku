/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#ifndef	VARIABLE_H
#define	VARIABLE_H

#include <File.h>
#include <SupportDefs.h>
#include <List.h>


namespace LinearProgramming {

class Constraint;
class LinearSpec;
class Summand;

/**
 * Contains minimum and maximum values.
 */
class Variable {

public:
	int32				Index();
	LinearSpec*			LS() const;
	double				Value() const;
	void				SetValue(double value);
	double				Min() const;
	void				SetMin(double min);
	double				Max() const;
	void				SetMax(double max);
	void				SetRange(double min, double max);

	const char*			Label();
	void				SetLabel(const char* label);
	
	BString*			ToBString();
	const char*			ToString();

	Constraint*			IsEqual(Variable* var);
	Constraint*			IsSmallerOrEqual(Variable* var);
	Constraint*			IsGreaterOrEqual(Variable* var);

	Constraint*			IsEqual(Variable* var,
							double penaltyNeg, double penaltyPos);
	Constraint*			IsSmallerOrEqual(Variable* var,
							double penaltyNeg, double penaltyPos);
	Constraint*			IsGreaterOrEqual(Variable* var,
							double penaltyNeg, double penaltyPos);

	bool				IsValid();
	void				Invalidate();

	virtual				~Variable();

protected:
						Variable(LinearSpec* ls);

private:
	LinearSpec*			fLS;
	BList*				fUsingSummands;  // All Summands that link to this Variable
	double				fValue;
	double				fMin;
	double				fMax;
	char*				fLabel;

	bool				fIsValid;

public:
	friend class		LinearSpec;
	friend class		Constraint;
	friend class		Summand;

};

}	// namespace LinearProgramming

using LinearProgramming::Variable;

#endif	// VARIABLE_H
