/*
 * Copyright 2005, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _STDBOOL_H_
#define _STDBOOL_H_

#ifndef __bool_true_false_are_defined

#ifndef __cplusplus
#	if __GNUC__ < 3
// sizeof(_Bool) == 1 must be true for BeOS compatibility
typedef unsigned char _Bool;
#	endif
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
