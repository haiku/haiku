/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */

#ifndef	LINEAR_PROGRAMMING_TYPES_H
#define	LINEAR_PROGRAMMING_TYPES_H


namespace LinearProgramming {


/**
 * The possible results of a solving attempt.
 */
enum ResultType {
	kNoMemory = -2,
	kError = -1,
	kOptimal = 0,
	kSuboptimal = 1,
	kInfeasible = 2,
	kUnbounded = 3,
	kDegenerate = 4,
	kNumFailure = 5
};


/**
 * Possible operators for linear constraints.
 */
enum OperatorType {
	kEQ,
	kLE,
	kGE
};



/**
 * The two possibilities for optimizing the objective function.
 */
enum OptimizationType {
	kMinimize,
	kMaximize
};

}	// namespace LinearProgramming


using LinearProgramming::ResultType;
using LinearProgramming::OperatorType;
using LinearProgramming::OptimizationType;


#endif // LINEAR_PROGRAMMING_TYPES_H
