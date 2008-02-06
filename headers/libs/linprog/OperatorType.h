#ifndef	OPERATOR_TYPE_H
#define	OPERATOR_TYPE_H


namespace LinearProgramming {

/**
 * Possible operators for linear constraints.
 */
enum OperatorType {
	EQ, LE, GE
};

}	// namespace LinearProgramming

using LinearProgramming::OperatorType;

#endif
