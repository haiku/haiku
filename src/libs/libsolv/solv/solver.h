/*
 * Copyright (c) 2007, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/*
 * solver.h
 *
 */

#ifndef LIBSOLV_SOLVER_H
#define LIBSOLV_SOLVER_H

#include "pooltypes.h"
#include "pool.h"
#include "repo.h"
#include "queue.h"
#include "bitmap.h"
#include "transaction.h"
#include "rules.h"
#include "problems.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _Solver {
  Pool *pool;				/* back pointer to pool */
  Queue job;				/* copy of the job we're solving */

  int (*solution_callback)(struct _Solver *solv, void *data);
  void *solution_callback_data;

  int pooljobcnt;			/* number of pooljob entries in job queue */

#ifdef LIBSOLV_INTERNAL
  Repo *installed;			/* copy of pool->installed */
  
  /* list of rules, ordered
   * rpm rules first, then features, updates, jobs, learnt
   * see start/end offsets below
   */
  Rule *rules;				/* all rules */
  Id nrules;				/* [Offset] index of the last rule */

  Queue ruleassertions;			/* Queue of all assertion rules */

  /* start/end offset for rule 'areas' */
    
  Id rpmrules_end;                      /* [Offset] rpm rules end */
    
  Id featurerules;			/* feature rules start/end */
  Id featurerules_end;
    
  Id updaterules;			/* policy rules, e.g. keep packages installed or update. All literals > 0 */
  Id updaterules_end;
    
  Id jobrules;				/* user rules */
  Id jobrules_end;

  Id infarchrules;			/* inferior arch rules */
  Id infarchrules_end;

  Id duprules;				/* dist upgrade rules */
  Id duprules_end;
    
  Id bestrules;				/* rules from SOLVER_FORCEBEST */
  Id bestrules_end;
  Id *bestrules_pkg;

  Id choicerules;			/* choice rules (always weak) */
  Id choicerules_end;
  Id *choicerules_ref;

  Id learntrules;			/* learnt rules, (end == nrules) */

  Map noupdate;				/* don't try to update these
                                           installed solvables */
  Map multiversion;			/* ignore obsoletes for these (multiinstall) */

  Map updatemap;			/* bring these installed packages to the newest version */
  int updatemap_all;			/* bring all packages to the newest version */

  Map bestupdatemap;			/* create best rule for those packages */
  int bestupdatemap_all;		/* bring all packages to the newest version */

  Map fixmap;				/* fix these packages */
  int fixmap_all;			/* fix all packages */

  Queue weakruleq;			/* index into 'rules' for weak ones */
  Map weakrulemap;			/* map rule# to '1' for weak rules, 1..learntrules */

  Id *watches;				/* Array of rule offsets
					 * watches has nsolvables*2 entries and is addressed from the middle
					 * middle-solvable : decision to conflict, offset point to linked-list of rules
					 * middle+solvable : decision to install: offset point to linked-list of rules
					 */

  Queue ruletojob;                      /* index into job queue: jobs for which a rule exits */

  /* our decisions: */
  Queue decisionq;                      /* >0:install, <0:remove/conflict */
  Queue decisionq_why;			/* index of rule, Offset into rules */

  Id *decisionmap;			/* map for all available solvables,
					 * = 0: undecided
					 * > 0: level of decision when installed,
					 * < 0: level of decision when conflict */

  int decisioncnt_update;
  int decisioncnt_keep;
  int decisioncnt_resolve;
  int decisioncnt_weak;
  int decisioncnt_orphan;

  /* learnt rule history */
  Queue learnt_why;
  Queue learnt_pool;

  Queue branches;
  int propagate_index;                  /* index into decisionq for non-propagated decisions */

  Queue problems;                       /* list of lists of conflicting rules, < 0 for job rules */
  Queue solutions;			/* refined problem storage space */

  Queue orphaned;			/* orphaned packages (to be removed?) */

  int stats_learned;			/* statistic */
  int stats_unsolvable;			/* statistic */

  Map recommendsmap;			/* recommended packages from decisionmap */
  Map suggestsmap;			/* suggested packages from decisionmap */
  int recommends_index;			/* recommendsmap/suggestsmap is created up to this level */

  Id *obsoletes;			/* obsoletes for each installed solvable */
  Id *obsoletes_data;			/* data area for obsoletes */
  Id *multiversionupdaters;		/* updaters for multiversion packages in updatesystem mode */

  /*-------------------------------------------------------------------------------------------------------------
   * Solver configuration
   *-------------------------------------------------------------------------------------------------------------*/

  int allowdowngrade;			/* allow to downgrade installed solvable */
  int allownamechange;			/* allow to change name of installed solvables */
  int allowarchchange;			/* allow to change architecture of installed solvables */
  int allowvendorchange;		/* allow to change vendor of installed solvables */
  int allowuninstall;			/* allow removal of installed solvables */
  int noupdateprovide;			/* true: update packages needs not to provide old package */
  int dosplitprovides;			/* true: consider legacy split provides */
  int dontinstallrecommended;		/* true: do not install recommended packages */
  int addalreadyrecommended;		/* true: also install recommended packages that were already recommended by the installed packages */
  int dontshowinstalledrecommended;	/* true: do not show recommended packages that are already installed */
  
  int noinfarchcheck;			/* true: do not forbid inferior architectures */
  int keepexplicitobsoletes;		/* true: honor obsoletes during multiinstall */
  int bestobeypolicy;			/* true: stay in policy with the best rules */
  int noautotarget;			/* true: do not assume targeted for up/dup jobs that contain no installed solvable */

    
  Map dupmap;				/* dup these packages*/
  int dupmap_all;			/* dup all packages */
  Map dupinvolvedmap;			/* packages involved in dup process */
  int dup_allowdowngrade;		/* dup mode: allow to downgrade installed solvable */
  int dup_allownamechange;		/* dup mode: allow to change name of installed solvable */
  int dup_allowarchchange;		/* dup mode: allow to change architecture of installed solvables */
  int dup_allowvendorchange;		/* dup mode: allow to change vendor of installed solvables */

  Map droporphanedmap;			/* packages to drop in dup mode */
  int droporphanedmap_all;

  Map cleandepsmap;			/* try to drop these packages as of cleandeps erases */

  Queue *ruleinfoq;			/* tmp space for solver_ruleinfo() */

  Queue *cleandeps_updatepkgs;		/* packages we update in cleandeps mode */
  Queue *cleandeps_mistakes;		/* mistakes we made */

  Queue *update_targets;		/* update to specific packages */

  Queue *installsuppdepq;		/* deps from the install namespace provides hack */

  Queue addedmap_deduceq;		/* deduce addedmap from rpm rules */
#endif	/* LIBSOLV_INTERNAL */
};

typedef struct _Solver Solver;

/*
 * queue commands
 */

#define SOLVER_SOLVABLE			0x01
#define SOLVER_SOLVABLE_NAME		0x02
#define SOLVER_SOLVABLE_PROVIDES	0x03
#define SOLVER_SOLVABLE_ONE_OF		0x04
#define SOLVER_SOLVABLE_REPO		0x05
#define SOLVER_SOLVABLE_ALL		0x06

#define SOLVER_SELECTMASK		0xff

#define SOLVER_NOOP			0x0000
#define SOLVER_INSTALL       		0x0100
#define SOLVER_ERASE         		0x0200
#define SOLVER_UPDATE			0x0300
#define SOLVER_WEAKENDEPS      		0x0400
#define SOLVER_MULTIVERSION		0x0500
#define SOLVER_LOCK			0x0600
#define SOLVER_DISTUPGRADE		0x0700
#define SOLVER_VERIFY			0x0800
#define SOLVER_DROP_ORPHANED		0x0900
#define SOLVER_USERINSTALLED            0x0a00

#define SOLVER_JOBMASK			0xff00

#define SOLVER_WEAK			0x010000
#define SOLVER_ESSENTIAL		0x020000
#define SOLVER_CLEANDEPS                0x040000
/* ORUPDATE makes SOLVER_INSTALL not prune to installed
 * packages, thus updating installed packages */
#define SOLVER_ORUPDATE			0x080000
/* FORCEBEST makes the solver insist on best packages, so
 * you will get problem reported if the best package is
 * not installable. This can be used with INSTALL, UPDATE
 * and DISTUPGRADE */
#define SOLVER_FORCEBEST		0x100000
/* TARGETED is used in up/dup jobs to make the solver
 * treat the specified selection as target packages.
 * It is automatically assumed if the selection does not
 * contain an "installed" package unless the
 * NO_AUTOTARGET solver flag is set */
#define SOLVER_TARGETED			0x200000

#define SOLVER_SETEV			0x01000000
#define SOLVER_SETEVR			0x02000000
#define SOLVER_SETARCH			0x04000000
#define SOLVER_SETVENDOR		0x08000000
#define SOLVER_SETREPO			0x10000000
#define SOLVER_NOAUTOSET		0x20000000
#define SOLVER_SETNAME			0x40000000

#define SOLVER_SETMASK			0x7f000000

/* legacy */
#define SOLVER_NOOBSOLETES 		SOLVER_MULTIVERSION

#define SOLVER_REASON_UNRELATED		0
#define SOLVER_REASON_UNIT_RULE		1
#define SOLVER_REASON_KEEP_INSTALLED	2
#define SOLVER_REASON_RESOLVE_JOB	3
#define SOLVER_REASON_UPDATE_INSTALLED	4
#define SOLVER_REASON_CLEANDEPS_ERASE	5
#define SOLVER_REASON_RESOLVE		6
#define SOLVER_REASON_WEAKDEP		7
#define SOLVER_REASON_RESOLVE_ORPHAN	8

#define SOLVER_REASON_RECOMMENDED	16
#define SOLVER_REASON_SUPPLEMENTED	17


#define SOLVER_FLAG_ALLOW_DOWNGRADE		1
#define SOLVER_FLAG_ALLOW_ARCHCHANGE		2
#define SOLVER_FLAG_ALLOW_VENDORCHANGE		3
#define SOLVER_FLAG_ALLOW_UNINSTALL		4
#define SOLVER_FLAG_NO_UPDATEPROVIDE		5
#define SOLVER_FLAG_SPLITPROVIDES		6
#define SOLVER_FLAG_IGNORE_RECOMMENDED		7
#define SOLVER_FLAG_ADD_ALREADY_RECOMMENDED	8
#define SOLVER_FLAG_NO_INFARCHCHECK		9
#define SOLVER_FLAG_ALLOW_NAMECHANGE		10
#define SOLVER_FLAG_KEEP_EXPLICIT_OBSOLETES	11
#define SOLVER_FLAG_BEST_OBEY_POLICY		12
#define SOLVER_FLAG_NO_AUTOTARGET		13

extern Solver *solver_create(Pool *pool);
extern void solver_free(Solver *solv);
extern int  solver_solve(Solver *solv, Queue *job);
extern Transaction *solver_create_transaction(Solver *solv);
extern int solver_set_flag(Solver *solv, int flag, int value);
extern int solver_get_flag(Solver *solv, int flag);

extern int  solver_get_decisionlevel(Solver *solv, Id p);
extern void solver_get_decisionqueue(Solver *solv, Queue *decisionq);
extern int  solver_get_lastdecisionblocklevel(Solver *solv);
extern void solver_get_decisionblock(Solver *solv, int level, Queue *decisionq);

extern void solver_get_orphaned(Solver *solv, Queue *orphanedq);
extern void solver_get_recommendations(Solver *solv, Queue *recommendationsq, Queue *suggestionsq, int noselected);
extern void solver_get_unneeded(Solver *solv, Queue *unneededq, int filtered);

extern int  solver_describe_decision(Solver *solv, Id p, Id *infop);
extern void solver_describe_weakdep_decision(Solver *solv, Id p, Queue *whyq);


extern void solver_calculate_multiversionmap(Pool *pool, Queue *job, Map *multiversionmap);
extern void solver_calculate_noobsmap(Pool *pool, Queue *job, Map *multiversionmap);	/* obsolete */
extern void solver_create_state_maps(Solver *solv, Map *installedmap, Map *conflictsmap);

/* XXX: why is this not static? */
Id *solver_create_decisions_obsoletesmap(Solver *solv);

void solver_calc_duchanges(Solver *solv, DUChanges *mps, int nmps);
int solver_calc_installsizechange(Solver *solv);
void solver_trivial_installable(Solver *solv, Queue *pkgs, Queue *res);

void pool_job2solvables(Pool *pool, Queue *pkgs, Id how, Id what);
int  pool_isemptyupdatejob(Pool *pool, Id how, Id what);

/* iterate over all literals of a rule */
#define FOR_RULELITERALS(l, pp, r)				\
    for (pp = r->d < 0 ? -r->d - 1 : r->d,			\
         l = r->p; l; l = (pp <= 0 ? (pp-- ? 0 : r->w2) :	\
         pool->whatprovidesdata[pp++]))




/* XXX: this currently doesn't work correctly for SOLVER_SOLVABLE_REPO and
   SOLVER_SOLVABLE_ALL */

/* iterate over all packages selected by a job */
#define FOR_JOB_SELECT(p, pp, select, what)					\
    if (select == SOLVER_SOLVABLE_REPO || select == SOLVER_SOLVABLE_ALL)	\
	p = pp = 0;								\
    else for (pp = (select == SOLVER_SOLVABLE ? 0 :				\
               select == SOLVER_SOLVABLE_ONE_OF ? what : 			\
               pool_whatprovides(pool, what)),					\
         p = (select == SOLVER_SOLVABLE ? what : pool->whatprovidesdata[pp++]); \
					 p ; p = pool->whatprovidesdata[pp++])	\
      if (select == SOLVER_SOLVABLE_NAME && 					\
			pool_match_nevr(pool, pool->solvables + p, what) == 0)	\
	continue;								\
      else

#ifdef __cplusplus
}
#endif

#endif /* LIBSOLV_SOLVER_H */
