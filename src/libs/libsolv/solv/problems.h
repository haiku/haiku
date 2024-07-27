/*
 * Copyright (c) 2007-2009, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/*
 * problems.h
 *
 */

#ifndef LIBSOLV_PROBLEMS_H
#define LIBSOLV_PROBLEMS_H

#ifdef __cplusplus
extern "C" {
#endif


struct _Solver;

#define SOLVER_SOLUTION_JOB             (0)
#define SOLVER_SOLUTION_DISTUPGRADE     (-1)
#define SOLVER_SOLUTION_INFARCH         (-2)
#define SOLVER_SOLUTION_BEST            (-3)
#define SOLVER_SOLUTION_POOLJOB         (-4)

void solver_disableproblem(struct _Solver *solv, Id v);
void solver_enableproblem(struct _Solver *solv, Id v);
int solver_prepare_solutions(struct _Solver *solv);

unsigned int solver_problem_count(struct _Solver *solv);
Id solver_next_problem(struct _Solver *solv, Id problem);
unsigned int solver_solution_count(struct _Solver *solv, Id problem);
Id solver_next_solution(struct _Solver *solv, Id problem, Id solution);
unsigned int solver_solutionelement_count(struct _Solver *solv, Id problem, Id solution);
Id solver_solutionelement_internalid(struct _Solver *solv, Id problem, Id solution);
Id solver_solutionelement_extrajobflags(struct _Solver *solv, Id problem, Id solution);
Id solver_next_solutionelement(struct _Solver *solv, Id problem, Id solution, Id element, Id *p, Id *rp);

void solver_take_solutionelement(struct _Solver *solv, Id p, Id rp, Id extrajobflags, Queue *job);
void solver_take_solution(struct _Solver *solv, Id problem, Id solution, Queue *job);

Id solver_findproblemrule(struct _Solver *solv, Id problem);
void solver_findallproblemrules(struct _Solver *solv, Id problem, Queue *rules);

#ifdef __cplusplus
}
#endif

#endif
