#ifndef _STDBOOL_H_
#define _STDBOOL_H_
/* 
 * Copyright 2005, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
*/

#ifndef __cplusplus
typedef enum { false = 0, true = 1 } _Bool;
#define true 1
#define false 0
#else
typedef bool _Bool;
#define true true
#define false false
#endif
#define bool _Bool

#define __bool_true_false_are_defined 1

#endif	/* _STDBOOL_H_ */
