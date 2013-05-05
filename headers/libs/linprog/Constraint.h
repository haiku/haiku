/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */
#ifndef	CONSTRAINT_H
#define	CONSTRAINT_H

#include <ObjectList.h>
#include <String.h>
#include <SupportDefs.h>

#include "LinearProgrammingTypes.h"
#include "Summand.h"
#include "Variable.h"


namespace LinearProgramming {

class LinearSpec;

/**
 * Hard linear constraint, i.e.&nbsp;one that must be satisfied.
 * May render a specification infeasible.
 */
class Constraint {
public:
								Constraint();
			/*! Creates a soft copy of the constraint which is not connected
			to the solver. Variables are not copied or cloned but the soft copy
			has its own summand list with summands. */
								Constraint(Constraint* constraint);

			int32				Index() const;

			SummandList*		LeftSide();

			bool				SetLeftSide(SummandList* summands,
									bool deleteOldSummands);
			
			bool				SetLeftSide(double coeff1, Variable* var1);
			bool				SetLeftSide(double coeff1, Variable* var1,
									double coeff2, Variable* var2);
			bool				SetLeftSide(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									double coeff3, Variable* var3);
			bool				SetLeftSide(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									double coeff3, Variable* var3,
									double coeff4, Variable* var4);

			OperatorType		Op();
			void				SetOp(OperatorType value);
			double				RightSide() const;
			void				SetRightSide(double value);
			double				PenaltyNeg() const;
			void				SetPenaltyNeg(double value);
			double				PenaltyPos() const;
			void				SetPenaltyPos(double value);

			const char*			Label();
			void				SetLabel(const char* label);

			bool				IsValid();
			void				Invalidate();

			BString				ToString() const;
			void				PrintToStream();

								~Constraint();

protected:
								Constraint(LinearSpec* ls,
									SummandList* summands, OperatorType op,
									double rightSide,
									double penaltyNeg = -1,
									double penaltyPos = -1);

private:
			LinearSpec*			fLS;
			SummandList*		fLeftSide;
			OperatorType		fOp;
			double				fRightSide;

			double				fPenaltyNeg;
			double				fPenaltyPos;
			BString				fLabel;

public:
	friend class		LinearSpec;
};


typedef BObjectList<Constraint> ConstraintList;


}	// namespace LinearProgramming

using LinearProgramming::Constraint;
using LinearProgramming::ConstraintList;

#endif	// CONSTRAINT_H

