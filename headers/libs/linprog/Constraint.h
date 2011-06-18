/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */
#ifndef	CONSTRAINT_H
#define	CONSTRAINT_H

#include <math.h>

#include <File.h>
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
			int32				Index() const;

			SummandList*		LeftSide();
			/*! This just overwrites the current list. The caller has to take
			care about the old left side, i.e. delete it. */
			void				SetLeftSide(SummandList* summands);
			
			void				SetLeftSide(double coeff1, Variable* var1);
			void				SetLeftSide(double coeff1, Variable* var1,
									double coeff2, Variable* var2);
			void				SetLeftSide(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									double coeff3, Variable* var3);
			void				SetLeftSide(double coeff1, Variable* var1,
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

			Variable*			DNeg() const;
			Variable*			DPos() const;

			bool				IsSoft() const;

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
			Summand*			fDNegObjSummand;
			Summand*			fDPosObjSummand;
			BString				fLabel;

			bool				fIsValid;

public:
	friend class		LinearSpec;
	friend class		LPSolveInterface;

};


typedef BObjectList<Constraint> ConstraintList;


}	// namespace LinearProgramming

using LinearProgramming::Constraint;
using LinearProgramming::ConstraintList;

#endif	// CONSTRAINT_H

