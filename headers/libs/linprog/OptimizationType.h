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
