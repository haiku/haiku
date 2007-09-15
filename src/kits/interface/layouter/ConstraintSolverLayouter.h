/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef	CONSTRAINT_SOLVER_LAYOUTER_H
#define	CONSTRAINT_SOLVER_LAYOUTER_H


#ifdef USE_QOCA_CONSTRAINT_SOLVER
#	include "QocaConstraintSolverLayouter.h"
	typedef QocaConstraintSolverLayouter ConstraintSolverLayouter;
#else
#	include "SimpleLayouter.h"
	typedef SimpleLayouter ConstraintSolverLayouter;
#endif


#endif	//	CONSTRAINT_SOLVER_LAYOUTER_H
