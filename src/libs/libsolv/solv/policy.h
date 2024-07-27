/*
 * Copyright (c) 2007, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/*
 * Generic policy interface for SAT solver
 * The policy* function can be "overloaded" by defining a callback in the solver struct.
 */

#include "solver.h"

#ifdef __cplusplus
extern "C" {
#endif

#define POLICY_MODE_CHOOSE	0
#define POLICY_MODE_RECOMMEND	1
#define POLICY_MODE_SUGGEST	2


#define POLICY_ILLEGAL_DOWNGRADE	1
#define POLICY_ILLEGAL_ARCHCHANGE	2
#define POLICY_ILLEGAL_VENDORCHANGE	4
#define POLICY_ILLEGAL_NAMECHANGE	8

extern void policy_filter_unwanted(Solver *solv, Queue *plist, int mode);
extern int  policy_illegal_archchange(Solver *solv, Solvable *s1, Solvable *s2);
extern int  policy_illegal_vendorchange(Solver *solv, Solvable *s1, Solvable *s2);
extern int  policy_is_illegal(Solver *solv, Solvable *s1, Solvable *s2, int ignore);
extern void policy_findupdatepackages(Solver *solv, Solvable *s, Queue *qs, int allowall);

extern void policy_create_obsolete_index(Solver *solv);

#ifdef __cplusplus
}
#endif
