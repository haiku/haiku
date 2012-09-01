/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LAYOUT_OPTIMIZER_H
#define LAYOUT_OPTIMIZER_H

#include <List.h>
#include <math.h>

#include "LinearSpec.h"


static const double kEqualsEpsilon = 0.000001;


double** allocate_matrix(int m, int n);
void free_matrix(double** matrix);
void copy_matrix(const double* const* A, double** B, int m, int n);
void zero_matrix(double** A, int m, int n);
int compute_dependencies(double** a, int m, int n, bool* independent);


bool is_soft(Constraint* constraint);


class LayoutOptimizer {
public:
								LayoutOptimizer(const ConstraintList& list,
									int32 variableCount);
								~LayoutOptimizer();

			bool				SetConstraints(const ConstraintList& list,
									int32 variableCount);

			status_t			InitCheck() const;

			bool				Solve(double* initialSolution);

private:
			double				_ActualValue(Constraint* constraint,
									double* values) const;
			double				_RightSide(Constraint* constraint);

			void				_MakeEmpty();
			void				_Init(int32 variableCount, int32 nConstraints);

			bool				_Solve(double* values);
			bool				_SolveSubProblem(const double* d, int am,
									double* p);
			void				_SetResult(const double* x, double* values);


			int32				fVariableCount;
			ConstraintList*		fConstraints;

			double**			fTemp1;
			double**			fTemp2;
			double**			fZtrans;
			double**			fQ;
			double**			fActiveMatrix;
			double**			fActiveMatrixTemp;
			double**			fSoftConstraints;
			double**			fG;
			double*				fDesired;
};


inline bool
fuzzy_equals(double a, double b)
{
	return fabs(a - b) < kEqualsEpsilon;
}


#endif	// LAYOUT_OPTIMIZER_H
