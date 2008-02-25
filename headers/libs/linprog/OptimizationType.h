/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#ifndef	OPTIMIZATION_TYPE_H
#define	OPTIMIZATION_TYPE_H


namespace LinearProgramming {

/**
 * The two possibilities for optimizing the objective function.
 */
enum OptimizationType {
	MINIMIZE, MAXIMIZE
};

}	// namespace LinearProgramming

using LinearProgramming::OptimizationType;

#endif
