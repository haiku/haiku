/*
 * Copyright 2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef	CONSTRAINT_SOLVER_LAYOUTER_H
#define	CONSTRAINT_SOLVER_LAYOUTER_H

#include "Layouter.h"

#include <List.h>

class QcLinInEqSolver;
class QcConstraint;
class QcConstraint;

namespace BPrivate {
namespace Layout {

class ConstraintSolverLayouter : public Layouter {
public:
								ConstraintSolverLayouter(int32 elementCount,
									int32 spacing);
	virtual						~ConstraintSolverLayouter();

	virtual	void				AddConstraints(int32 element, int32 length,
									float min, float max, float preferred);
	virtual	void				SetWeight(int32 element, float weight);

	virtual	float				MinSize();
	virtual	float				MaxSize();
	virtual	float				PreferredSize();

	virtual	LayoutInfo*			CreateLayoutInfo();
	
	virtual	void				Layout(LayoutInfo* layoutInfo, float size);

	virtual	Layouter*			CloneLayouter();

private:
			class Set;

			class Variable;
			class Constraint;
			class MinConstraint;
			class MaxConstraint;
			class MinMaxEntry;
			class MinMaxMatrix;
			class MyLayoutInfo;

			friend class Constraint;

			enum {
				MIN_CONSTRAINT = 0,
				MAX_CONSTRAINT = 1,
			};


			void				_ValidateMinMax();

			void				_AddMinConstraint(int32 first, int32 last,
									int32 value);
			void				_AddMaxConstraint(int32 first, int32 last,
									int32 value);

			int32				_ComputeMin();
			int32				_ComputeMax();
			bool				_ComputeSolution(int32 size);

			bool				_FindIntegerSolution(int32 size);
			bool				_FindIntegerSolution(const Set& nonIntVariables,
									const BList& minConstraints,
									const BList& maxConstraints,
									int32 sizeDiff);

			Constraint*			_CreateConstraint(int32 type, int32 first,
									int32 last, int32 value);
			void				_SetConstraint(int32 type,
									Constraint** constraints,
									int32 element, int32 value, bool restrict);
			void				_SetConstraint(int32 type,
									BList& constraints,
									int32 first, int32 last, int32 value,
									bool restrict);
			int					_IndexOfConstraint(BList& constraints,
									int32 first, int32 last);
			bool				_AddSetSizeConstraint(int32 size);

			void				_AddConstraints();
			void				_AddConstraints(const BList& constraints);
			void				_RemoveSetSizeConstraint();
			void				_RemoveConstraints();
			void				_RemoveConstraint(Constraint* constraint);
			void				_GetInvolvedConstraints(
									const BList& constraints,
									const BList& variables,
									BList& filteredConstraints);
			void				_GetViolatedConstraints(
									const BList& constraints,
									BList& violatedConstraints);
			void				_FilterVariablesByMaxConstraints(
									const BList& variables,
									const BList& constraints,
									Set& filteredVars);

	static	int					_CompareVariables(const void* a, const void* b);
	static	int					_CompareConstraintsByLength(const void* a,
									const void* b);
	static	int					_CompareConstraintsBySatisfactionDistance(
									const void* a, const void* b);

private:
			int32				fElementCount;
			float*				fWeights;
	
			Constraint**		fBasicMinConstraints;
			Constraint**		fBasicMaxConstraints;

			BList				fComplexMinConstraints;
			BList				fComplexMaxConstraints;

			Variable*			fVariables;
			Variable*			fSizeVariable;

			QcLinInEqSolver*	fSolver;
			QcConstraint*		fSizeSumConstraint;
			QcConstraint*		fSetSizeConstraint;

			bool				fConstraintsAdded;
			bool				fSetSizeConstraintAdded;

			int32				fSpacing;
			int32				fMin;
			int32				fMax;
			bool				fMinMaxValid;

			MyLayoutInfo*		fLayoutInfo;
};

}	// namespace Layout
}	// namespace BPrivate

using BPrivate::Layout::ConstraintSolverLayouter;

#endif	// CONSTRAINT_SOLVER_LAYOUTER_H
