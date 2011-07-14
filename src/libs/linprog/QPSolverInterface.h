/*
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef QP_SOLVER_INTERFACE_H
#define QP_SOLVER_INTERFACE_H


#include "LinearSpec.h"

#include <vector>


/*! The purpose of this class is to manage soft inequality constraints. This is
done by adding a slack variable which is minimized. For example:
$$width - s < maxSize$$
$$s = 0, soft constraint$$
*/
class QPSolverInterface : public LinearProgramming::SolverInterface {
public:
								QPSolverInterface(LinearSpec* linearSpec);
	virtual						~QPSolverInterface();

	virtual	bool				ConstraintAdded(Constraint* constraint);
	virtual	bool				ConstraintRemoved(Constraint* constraint);

private:
			struct SoftInEqData {
				Summand*	slack;
				Constraint*	constraint;
				Constraint*	minSlackConstraint;
			};

			typedef std::vector<SoftInEqData> SoftInEqList;

	inline	void				_DestroyData(SoftInEqData& data);
	inline	bool				_IsSoftInequality(Constraint* constraint);

			SoftInEqList		fInEqSlackConstraints;
};


#endif // QP_SOLVER_INTERFACE_H
