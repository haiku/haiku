#ifndef _STDBOOL_H_
#define _STDBOOL_H_
/* 
** Distributed under the terms of the OpenBeOS License.
*/

#ifndef __cplusplus
typedef enum { false = 0, true = 1 } _Bool;
#else
typedef bool _Bool;
#endif
#define bool _Bool

#define true 1
#define false 0
#define __bool_true_false_are_defined 1

#endif	/* _STDBOOL_H_ */
