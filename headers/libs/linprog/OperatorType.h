/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#ifndef	OPERATOR_TYPE_H
#define	OPERATOR_TYPE_H


namespace LinearProgramming {

/**
 * Possible operators for linear constraints.
 */
enum OperatorType {
	kEQ,
	kLE,
	kGE
};

}	// namespace LinearProgramming

using LinearProgramming::OperatorType;

#endif
