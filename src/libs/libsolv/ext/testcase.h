/*
 * Copyright (c) 2012, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

#include "pool.h"
#include "repo.h"
#include "solver.h"

#define TESTCASE_RESULT_TRANSACTION	(1 << 0)
#define TESTCASE_RESULT_PROBLEMS	(1 << 1)
#define TESTCASE_RESULT_ORPHANED	(1 << 2)
#define TESTCASE_RESULT_RECOMMENDED	(1 << 3)
#define TESTCASE_RESULT_UNNEEDED	(1 << 4)

extern Id testcase_str2dep(Pool *pool, char *s);
extern const char *testcase_repoid2str(Pool *pool, Id repoid);
extern const char *testcase_solvid2str(Pool *pool, Id p);
extern Repo *testcase_str2repo(Pool *pool, const char *str);
extern Id testcase_str2solvid(Pool *pool, const char *str);
extern const char *testcase_job2str(Pool *pool, Id how, Id what);
extern Id testcase_str2job(Pool *pool, const char *str, Id *whatp);
extern int testcase_write_testtags(Repo *repo, FILE *fp);
extern int testcase_add_testtags(Repo *repo, FILE *fp, int flags);
extern const char *testcase_getsolverflags(Solver *solv);
extern int testcase_setsolverflags(Solver *solv, const char *str);
extern void testcase_resetsolverflags(Solver *solv);
extern char *testcase_solverresult(Solver *solv, int flags);
extern int testcase_write(Solver *solv, char *dir, int resultflags, const char *testcasename, const char *resultname);
extern Solver *testcase_read(Pool *pool, FILE *fp, char *testcase, Queue *job, char **resultp, int *resultflagsp);
extern char *testcase_resultdiff(char *result1, char *result2);
