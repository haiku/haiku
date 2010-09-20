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

#include "OperatorType.h"
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
			void				SetLeftSide(SummandList* summands);
			void				UpdateLeftSide();
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

			void				WriteXML(BFile* file);

			Variable*			DNeg() const;
			Variable*			DPos() const;

			bool				IsValid();
			void				Invalidate();

								operator BString() const;
			void				GetString(BString& string) const;

								~Constraint();

protected:
								Constraint(LinearSpec* ls,
									SummandList* summands, OperatorType op,
									double rightSide, double penaltyNeg,
									double penaltyPos);

private:
			LinearSpec*			fLS;
			SummandList*		fLeftSide;
			OperatorType		fOp;
			double				fRightSide;
			Summand*			fDNegObjSummand;
			Summand*			fDPosObjSummand;
			BString				fLabel;

			bool				fIsValid;

public:
	friend class		LinearSpec;

};


typedef BObjectList<Constraint> ConstraintList;


}	// namespace LinearProgramming

using LinearProgramming::Constraint;
using LinearProgramming::ConstraintList;

#endif	// CONSTRAINT_H

