/*
 * Copyright 2005, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _STDBOOL_H_
#define _STDBOOL_H_

#ifndef __bool_true_false_are_defined

#ifndef __cplusplus
//typedef enum { false = 0, true = 1 } _Bool;
	// ToDo: this would change the bool size compared to standard BeOS
typedef unsigned char _Bool;
#	define bool _Bool
#	define true 1
#	define false 0
#else
#	define _Bool bool
#	define bool bool
#	define true true
#	define false false
#endif

#define __bool_true_false_are_defined 1

#endif	// __bool_true_false_are_defined

#endif	/* _STDBOOL_H_ */
