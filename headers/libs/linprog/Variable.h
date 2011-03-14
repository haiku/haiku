/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */
#ifndef	VARIABLE_H
#define	VARIABLE_H


#include <ObjectList.h>
#include <String.h>

namespace LinearProgramming {

class Constraint;
class LinearSpec;
class Summand;

/**
 * Contains minimum and maximum values.
 */
class Variable {
public:
			int32				Index() const;
			int32				GlobalIndex() const;
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

			BString				ToString() const;

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
			//! Dangerous the variable don't belong to the LinearSpec anymore,
			//! delete it yourself!
			void				Invalidate();

	virtual						~Variable();

protected:
								Variable(LinearSpec* ls);

			//! returns the ref count
			int32				AddReference();
			int32				RemoveReference();
private:
			LinearSpec*			fLS;

			double				fValue;
			double				fMin;
			double				fMax;
			BString				fLabel;

			bool				fIsValid;

			int32				fReferenceCount;
public:
	friend class		LinearSpec;
};


typedef BObjectList<Variable> VariableList;


}	// namespace LinearProgramming

using LinearProgramming::Variable;
using LinearProgramming::VariableList;

#endif	// VARIABLE_H
