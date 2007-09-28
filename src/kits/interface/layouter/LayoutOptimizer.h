/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LAYOUT_OPTIMIZER_H
#define LAYOUT_OPTIMIZER_H

#include <List.h>
#include <math.h>

static const double kEqualsEpsilon = 0.000001;


namespace BPrivate {
namespace Layout {

class LayoutOptimizer {
public:
								LayoutOptimizer(int32 variableCount);
								~LayoutOptimizer();

			status_t			InitCheck() const;

			LayoutOptimizer*	Clone() const;

			bool				AddConstraint(int32 left, int32 right,
									double value, bool equality);
			bool				AddConstraintsFrom(
									const LayoutOptimizer* solver);
			void				RemoveAllConstraints();

			bool				Solve(const double* desired, double size,
									double* values);

private:
			bool				_Solve(const double* desired, double* values);
			bool				_SolveSubProblem(const double* d, int am,
									double* p);
			void				_SetResult(const double* x, double* values);


			struct Constraint;

			int32				fVariableCount;
			BList				fConstraints;
			double*				fVariables;
			double**			fTemp1;
			double**			fTemp2;
			double**			fZtrans;
			double**			fQ;
			double**			fActiveMatrix;
			double**			fActiveMatrixTemp;
};

}	// namespace Layout
}	// namespace BPrivate

using BPrivate::Layout::LayoutOptimizer;


inline bool
fuzzy_equals(double a, double b)
{
	return fabs(a - b) < kEqualsEpsilon;
}


#endif	// LAYOUT_OPTIMIZER_H
