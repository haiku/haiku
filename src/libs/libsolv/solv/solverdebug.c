/*
 * Copyright (c) 2008, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/*
 * solverdebug.c
 *
 * debug functions for the SAT solver
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "solver.h"
#include "solver_private.h"
#include "solverdebug.h"
#include "bitmap.h"
#include "pool.h"
#include "poolarch.h"
#include "util.h"
#include "evr.h"
#include "policy.h"


/*
 * create obsoletesmap from solver decisions
 *
 * for solvables in installed repo:
 *   0 - not obsoleted
 *   p - one of the packages that obsolete us
 * for all others:
 *   n - number of packages this package obsoletes
 *
 */

/* OBSOLETE: use transaction code instead! */

Id *
solver_create_decisions_obsoletesmap(Solver *solv)
{
  Pool *pool = solv->pool;
  Repo *installed = solv->installed;
  Id p, *obsoletesmap = NULL;
  int i;
  Solvable *s;

  obsoletesmap = (Id *)solv_calloc(pool->nsolvables, sizeof(Id));
  if (installed)
    {
      for (i = 0; i < solv->decisionq.count; i++)
	{
	  Id pp, n;
	  int multi;

	  n = solv->decisionq.elements[i];
	  if (n < 0)
	    continue;
	  if (n == SYSTEMSOLVABLE)
	    continue;
	  s = pool->solvables + n;
	  if (s->repo == installed)		/* obsoletes don't count for already installed packages */
	    continue;
	  multi = solv->multiversion.size && MAPTST(&solv->multiversion, n);
	  FOR_PROVIDES(p, pp, s->name)
	    {
	      Solvable *ps = pool->solvables + p;
	      if (multi && (s->name != ps->name || s->evr != ps->evr || s->arch != ps->arch))
		continue;
	      if (!pool->implicitobsoleteusesprovides && s->name != ps->name)
		continue;
	      if (pool->obsoleteusescolors && !pool_colormatch(pool, s, ps))
		continue;
	      if (ps->repo == installed && !obsoletesmap[p])
		{
		  obsoletesmap[p] = n;
		  obsoletesmap[n]++;
		}
	    }
	}
      for (i = 0; i < solv->decisionq.count; i++)
	{
	  Id obs, *obsp;
	  Id pp, n;

	  n = solv->decisionq.elements[i];
	  if (n < 0)
	    continue;
	  if (n == SYSTEMSOLVABLE)
	    continue;
	  s = pool->solvables + n;
	  if (s->repo == installed)		/* obsoletes don't count for already installed packages */
	    continue;
	  if (!s->obsoletes)
	    continue;
	  if (solv->multiversion.size && MAPTST(&solv->multiversion, n))
	    continue;
	  obsp = s->repo->idarraydata + s->obsoletes;
	  while ((obs = *obsp++) != 0)
	    {
	      FOR_PROVIDES(p, pp, obs)
		{
		  Solvable *ps = pool->solvables + p;
		  if (!pool->obsoleteusesprovides && !pool_match_nevr(pool, ps, obs))
		    continue;
		  if (pool->obsoleteusescolors && !pool_colormatch(pool, s, ps))
		    continue;
		  if (pool->solvables[p].repo == installed && !obsoletesmap[p])
		    {
		      obsoletesmap[p] = n;
		      obsoletesmap[n]++;
		    }
		}
	    }
	}
    }
  return obsoletesmap;
}

void
solver_printruleelement(Solver *solv, int type, Rule *r, Id v)
{
  Pool *pool = solv->pool;
  Solvable *s;
  if (v < 0)
    {
      s = pool->solvables + -v;
      POOL_DEBUG(type, "    !%s [%d]", pool_solvable2str(pool, s), -v);
    }
  else
    {
      s = pool->solvables + v;
      POOL_DEBUG(type, "    %s [%d]", pool_solvable2str(pool, s), v);
    }
  if (pool->installed && s->repo == pool->installed)
    POOL_DEBUG(type, "I");
  if (r)
    {
      if (r->w1 == v)
	POOL_DEBUG(type, " (w1)");
      if (r->w2 == v)
	POOL_DEBUG(type, " (w2)");
    }
  if (solv->decisionmap[s - pool->solvables] > 0)
    POOL_DEBUG(type, " Install.level%d", solv->decisionmap[s - pool->solvables]);
  if (solv->decisionmap[s - pool->solvables] < 0)
    POOL_DEBUG(type, " Conflict.level%d", -solv->decisionmap[s - pool->solvables]);
  POOL_DEBUG(type, "\n");
}


/*
 * print rule
 */

void
solver_printrule(Solver *solv, int type, Rule *r)
{
  Pool *pool = solv->pool;
  int i;
  Id d, v;

  if (r >= solv->rules && r < solv->rules + solv->nrules)   /* r is a solver rule */
    POOL_DEBUG(type, "Rule #%d:", (int)(r - solv->rules));
  else
    POOL_DEBUG(type, "Rule:");		       /* r is any rule */
  if (r && r->d < 0)
    POOL_DEBUG(type, " (disabled)");
  POOL_DEBUG(type, "\n");
  d = r->d < 0 ? -r->d - 1 : r->d;
  for (i = 0; ; i++)
    {
      if (i == 0)
	  /* print direct literal */
	v = r->p;
      else if (!d)
	{
	  if (i == 2)
	    break;
	  /* binary rule --> print w2 as second literal */
	  v = r->w2;
	}
      else
	  /* every other which is in d */
	v = solv->pool->whatprovidesdata[d + i - 1];
      if (v == ID_NULL)
	break;
      solver_printruleelement(solv, type, r, v);
    }
  POOL_DEBUG(type, "    next rules: %d %d\n", r->n1, r->n2);
}

void
solver_printruleclass(Solver *solv, int type, Rule *r)
{
  Pool *pool = solv->pool;
  Id p = r - solv->rules;
  assert(p >= 0);
  if (p < solv->learntrules)
    if (MAPTST(&solv->weakrulemap, p))
      POOL_DEBUG(type, "WEAK ");
  if (solv->learntrules && p >= solv->learntrules)
    POOL_DEBUG(type, "LEARNT ");
  else if (p >= solv->bestrules && p < solv->bestrules_end)
    POOL_DEBUG(type, "BEST ");
  else if (p >= solv->choicerules && p < solv->choicerules_end)
    POOL_DEBUG(type, "CHOICE ");
  else if (p >= solv->infarchrules && p < solv->infarchrules_end)
    POOL_DEBUG(type, "INFARCH ");
  else if (p >= solv->duprules && p < solv->duprules_end)
    POOL_DEBUG(type, "DUP ");
  else if (p >= solv->jobrules && p < solv->jobrules_end)
    POOL_DEBUG(type, "JOB ");
  else if (p >= solv->updaterules && p < solv->updaterules_end)
    POOL_DEBUG(type, "UPDATE ");
  else if (p >= solv->featurerules && p < solv->featurerules_end)
    POOL_DEBUG(type, "FEATURE ");
  solver_printrule(solv, type, r);
}

void
solver_printproblem(Solver *solv, Id v)
{
  Pool *pool = solv->pool;
  int i;
  Rule *r;
  Id *jp;

  if (v > 0)
    solver_printruleclass(solv, SOLV_DEBUG_SOLUTIONS, solv->rules + v);
  else
    {
      v = -(v + 1);
      POOL_DEBUG(SOLV_DEBUG_SOLUTIONS, "JOB %d\n", v);
      jp = solv->ruletojob.elements;
      for (i = solv->jobrules, r = solv->rules + i; i < solv->jobrules_end; i++, r++, jp++)
	if (*jp == v)
	  {
	    POOL_DEBUG(SOLV_DEBUG_SOLUTIONS, "- ");
	    solver_printrule(solv, SOLV_DEBUG_SOLUTIONS, r);
	  }
      POOL_DEBUG(SOLV_DEBUG_SOLUTIONS, "ENDJOB\n");
    }
}

void
solver_printwatches(Solver *solv, int type)
{
  Pool *pool = solv->pool;
  int counter;

  POOL_DEBUG(type, "Watches: \n");
  for (counter = -(pool->nsolvables - 1); counter < pool->nsolvables; counter++)
    POOL_DEBUG(type, "    solvable [%d] -- rule [%d]\n", counter, solv->watches[counter + pool->nsolvables]);
}

void
solver_printdecisionq(Solver *solv, int type)
{
  Pool *pool = solv->pool;
  int i;
  Id p, why;

  POOL_DEBUG(type, "Decisions:\n");
  for (i = 0; i < solv->decisionq.count; i++)
    {
      p = solv->decisionq.elements[i];
      if (p > 0)
        POOL_DEBUG(type, "%d %d install  %s, ", i, solv->decisionmap[p], pool_solvid2str(pool, p));
      else
        POOL_DEBUG(type, "%d %d conflict %s, ", i, -solv->decisionmap[-p], pool_solvid2str(pool, -p));
      why = solv->decisionq_why.elements[i];
      if (why > 0)
	{
	  POOL_DEBUG(type, "forced by ");
	  solver_printruleclass(solv, type, solv->rules + why);
	}
      else if (why < 0)
	{
	  POOL_DEBUG(type, "chosen from ");
	  solver_printruleclass(solv, type, solv->rules - why);
	}
      else
        POOL_DEBUG(type, "picked for some unknown reason.\n");
    }
}

/*
 * printdecisions
 */

void
solver_printdecisions(Solver *solv)
{
  Pool *pool = solv->pool;
  Repo *installed = solv->installed;
  Transaction *trans = solver_create_transaction(solv);
  Id p, type;
  int i, j;
  Solvable *s;
  Queue iq;
  Queue recommendations;
  Queue suggestions;
  Queue orphaned;

  POOL_DEBUG(SOLV_DEBUG_RESULT, "\n");
  POOL_DEBUG(SOLV_DEBUG_RESULT, "transaction:\n");

  queue_init(&iq);
  for (i = 0; i < trans->steps.count; i++)
    {
      p = trans->steps.elements[i];
      s = pool->solvables + p;
      type = transaction_type(trans, p, SOLVER_TRANSACTION_SHOW_ACTIVE|SOLVER_TRANSACTION_SHOW_ALL|SOLVER_TRANSACTION_SHOW_OBSOLETES|SOLVER_TRANSACTION_SHOW_MULTIINSTALL);
      switch(type)
        {
	case SOLVER_TRANSACTION_MULTIINSTALL:
          POOL_DEBUG(SOLV_DEBUG_RESULT, "  multi install %s", pool_solvable2str(pool, s));
	  break;
	case SOLVER_TRANSACTION_MULTIREINSTALL:
          POOL_DEBUG(SOLV_DEBUG_RESULT, "  multi reinstall %s", pool_solvable2str(pool, s));
	  break;
	case SOLVER_TRANSACTION_INSTALL:
          POOL_DEBUG(SOLV_DEBUG_RESULT, "  install   %s", pool_solvable2str(pool, s));
	  break;
	case SOLVER_TRANSACTION_REINSTALL:
          POOL_DEBUG(SOLV_DEBUG_RESULT, "  reinstall %s", pool_solvable2str(pool, s));
	  break;
	case SOLVER_TRANSACTION_DOWNGRADE:
          POOL_DEBUG(SOLV_DEBUG_RESULT, "  downgrade %s", pool_solvable2str(pool, s));
	  break;
	case SOLVER_TRANSACTION_CHANGE:
          POOL_DEBUG(SOLV_DEBUG_RESULT, "  change    %s", pool_solvable2str(pool, s));
	  break;
	case SOLVER_TRANSACTION_UPGRADE:
	case SOLVER_TRANSACTION_OBSOLETES:
          POOL_DEBUG(SOLV_DEBUG_RESULT, "  upgrade   %s", pool_solvable2str(pool, s));
	  break;
	case SOLVER_TRANSACTION_ERASE:
          POOL_DEBUG(SOLV_DEBUG_RESULT, "  erase     %s", pool_solvable2str(pool, s));
	  break;
	default:
	  break;
        }
      switch(type)
        {
	case SOLVER_TRANSACTION_INSTALL:
	case SOLVER_TRANSACTION_ERASE:
	case SOLVER_TRANSACTION_MULTIINSTALL:
	case SOLVER_TRANSACTION_MULTIREINSTALL:
	  POOL_DEBUG(SOLV_DEBUG_RESULT, "\n");
	  break;
	case SOLVER_TRANSACTION_REINSTALL:
	case SOLVER_TRANSACTION_DOWNGRADE:
	case SOLVER_TRANSACTION_CHANGE:
	case SOLVER_TRANSACTION_UPGRADE:
	case SOLVER_TRANSACTION_OBSOLETES:
	  transaction_all_obs_pkgs(trans, p, &iq);
	  if (iq.count)
	    {
	      POOL_DEBUG(SOLV_DEBUG_RESULT, "  (obsoletes");
	      for (j = 0; j < iq.count; j++)
		POOL_DEBUG(SOLV_DEBUG_RESULT, " %s", pool_solvid2str(pool, iq.elements[j]));
	      POOL_DEBUG(SOLV_DEBUG_RESULT, ")");
	    }
	  POOL_DEBUG(SOLV_DEBUG_RESULT, "\n");
	  break;
	default:
	  break;
	}
    }
  queue_free(&iq);

  POOL_DEBUG(SOLV_DEBUG_RESULT, "\n");

  queue_init(&recommendations);
  queue_init(&suggestions);
  queue_init(&orphaned);
  solver_get_recommendations(solv, &recommendations, &suggestions, 0);
  if (recommendations.count)
    {
      POOL_DEBUG(SOLV_DEBUG_RESULT, "recommended packages:\n");
      for (i = 0; i < recommendations.count; i++)
	{
	  s = pool->solvables + recommendations.elements[i];
          if (solv->decisionmap[recommendations.elements[i]] > 0)
	    {
	      if (installed && s->repo == installed)
	        POOL_DEBUG(SOLV_DEBUG_RESULT, "  %s (installed)\n", pool_solvable2str(pool, s));
	      else
	        POOL_DEBUG(SOLV_DEBUG_RESULT, "  %s (selected)\n", pool_solvable2str(pool, s));
	    }
          else
	    POOL_DEBUG(SOLV_DEBUG_RESULT, "  %s\n", pool_solvable2str(pool, s));
	}
      POOL_DEBUG(SOLV_DEBUG_RESULT, "\n");
    }

  if (suggestions.count)
    {
      POOL_DEBUG(SOLV_DEBUG_RESULT, "suggested packages:\n");
      for (i = 0; i < suggestions.count; i++)
	{
	  s = pool->solvables + suggestions.elements[i];
          if (solv->decisionmap[suggestions.elements[i]] > 0)
	    {
	      if (installed && s->repo == installed)
	        POOL_DEBUG(SOLV_DEBUG_RESULT, "  %s (installed)\n", pool_solvable2str(pool, s));
	      else
	        POOL_DEBUG(SOLV_DEBUG_RESULT, "  %s (selected)\n", pool_solvable2str(pool, s));
	    }
	  else
	    POOL_DEBUG(SOLV_DEBUG_RESULT, "  %s\n", pool_solvable2str(pool, s));
	}
      POOL_DEBUG(SOLV_DEBUG_RESULT, "\n");
    }
  if (orphaned.count)
    {
      POOL_DEBUG(SOLV_DEBUG_RESULT, "orphaned packages:\n");
      for (i = 0; i < orphaned.count; i++)
	{
	  s = pool->solvables + orphaned.elements[i];
          if (solv->decisionmap[solv->orphaned.elements[i]] > 0)
	    POOL_DEBUG(SOLV_DEBUG_RESULT, "  %s (kept)\n", pool_solvable2str(pool, s));
	  else
	    POOL_DEBUG(SOLV_DEBUG_RESULT, "  %s (erased)\n", pool_solvable2str(pool, s));
	}
      POOL_DEBUG(SOLV_DEBUG_RESULT, "\n");
    }
  queue_free(&recommendations);
  queue_free(&suggestions);
  queue_free(&orphaned);
  transaction_free(trans);
}

static inline
const char *id2strnone(Pool *pool, Id id)
{
  return !id || id == 1 ? "(none)" : pool_id2str(pool, id);
}

void
transaction_print(Transaction *trans)
{
  Pool *pool = trans->pool;
  Queue classes, pkgs;
  int i, j, mode, l, linel;
  char line[76];
  const char *n;

  queue_init(&classes);
  queue_init(&pkgs);
  mode = 0;
  transaction_classify(trans, mode, &classes);
  for (i = 0; i < classes.count; i += 4)
    {
      Id class = classes.elements[i];
      Id cnt = classes.elements[i + 1];
      switch(class)
	{
	case SOLVER_TRANSACTION_ERASE:
	  POOL_DEBUG(SOLV_DEBUG_RESULT, "%d erased packages:\n", cnt);
	  break;
	case SOLVER_TRANSACTION_INSTALL:
	  POOL_DEBUG(SOLV_DEBUG_RESULT, "%d installed packages:\n", cnt);
	  break;
	case SOLVER_TRANSACTION_REINSTALLED:
	  POOL_DEBUG(SOLV_DEBUG_RESULT, "%d reinstalled packages:\n", cnt);
	  break;
	case SOLVER_TRANSACTION_DOWNGRADED:
	  POOL_DEBUG(SOLV_DEBUG_RESULT, "%d downgraded packages:\n", cnt);
	  break;
	case SOLVER_TRANSACTION_CHANGED:
	  POOL_DEBUG(SOLV_DEBUG_RESULT, "%d changed packages:\n", cnt);
	  break;
	case SOLVER_TRANSACTION_UPGRADED:
	  POOL_DEBUG(SOLV_DEBUG_RESULT, "%d upgraded packages:\n", cnt);
	  break;
	case SOLVER_TRANSACTION_VENDORCHANGE:
	  POOL_DEBUG(SOLV_DEBUG_RESULT, "%d vendor changes from '%s' to '%s':\n", cnt, id2strnone(pool, classes.elements[i + 2]), id2strnone(pool, classes.elements[i + 3]));
	  break;
	case SOLVER_TRANSACTION_ARCHCHANGE:
	  POOL_DEBUG(SOLV_DEBUG_RESULT, "%d arch changes from %s to %s:\n", cnt, pool_id2str(pool, classes.elements[i + 2]), pool_id2str(pool, classes.elements[i + 3]));
	  break;
	default:
	  class = SOLVER_TRANSACTION_IGNORE;
	  break;
	}
      if (class == SOLVER_TRANSACTION_IGNORE)
	continue;
      transaction_classify_pkgs(trans, mode, class, classes.elements[i + 2], classes.elements[i + 3], &pkgs);
      *line = 0;
      linel = 0;
      for (j = 0; j < pkgs.count; j++)
	{
	  Id p = pkgs.elements[j];
	  Solvable *s = pool->solvables + p;
	  Solvable *s2;

	  switch(class)
	    {
	    case SOLVER_TRANSACTION_DOWNGRADED:
	    case SOLVER_TRANSACTION_UPGRADED:
	      s2 = pool->solvables + transaction_obs_pkg(trans, p);
	      POOL_DEBUG(SOLV_DEBUG_RESULT, "  - %s -> %s\n", pool_solvable2str(pool, s), pool_solvable2str(pool, s2));
	      break;
	    case SOLVER_TRANSACTION_VENDORCHANGE:
	    case SOLVER_TRANSACTION_ARCHCHANGE:
	      n = pool_id2str(pool, s->name);
	      l = strlen(n);
	      if (l + linel > sizeof(line) - 3)
		{
		  if (*line)
		    POOL_DEBUG(SOLV_DEBUG_RESULT, "    %s\n", line);
		  *line = 0;
		  linel = 0;
		}
	      if (l + linel > sizeof(line) - 3)
	        POOL_DEBUG(SOLV_DEBUG_RESULT, "    %s\n", n);
	      else
		{
		  if (*line)
		    {
		      strcpy(line + linel, ", ");
		      linel += 2;
		    }
		  strcpy(line + linel, n);
		  linel += l;
		}
	      break;
	    default:
	      POOL_DEBUG(SOLV_DEBUG_RESULT, "  - %s\n", pool_solvable2str(pool, s));
	      break;
	    }
	}
      if (*line)
	POOL_DEBUG(SOLV_DEBUG_RESULT, "    %s\n", line);
      POOL_DEBUG(SOLV_DEBUG_RESULT, "\n");
    }
  queue_free(&classes);
  queue_free(&pkgs);
}

void
solver_printproblemruleinfo(Solver *solv, Id probr)
{
  Pool *pool = solv->pool;
  Id dep, source, target;

  switch (solver_ruleinfo(solv, probr, &source, &target, &dep))
    {
    case SOLVER_RULE_DISTUPGRADE:
      POOL_DEBUG(SOLV_DEBUG_RESULT, "%s does not belong to a distupgrade repository\n", pool_solvid2str(pool, source));
      return;
    case SOLVER_RULE_INFARCH:
      POOL_DEBUG(SOLV_DEBUG_RESULT, "%s has inferior architecture\n", pool_solvid2str(pool, source));
      return;
    case SOLVER_RULE_UPDATE:
      POOL_DEBUG(SOLV_DEBUG_RESULT, "problem with installed package %s\n", pool_solvid2str(pool, source));
      return;
    case SOLVER_RULE_JOB:
      POOL_DEBUG(SOLV_DEBUG_RESULT, "conflicting requests\n");
      return;
    case SOLVER_RULE_JOB_NOTHING_PROVIDES_DEP:
      POOL_DEBUG(SOLV_DEBUG_RESULT, "nothing provides requested %s\n", pool_dep2str(pool, dep));
      return;
    case SOLVER_RULE_JOB_PROVIDED_BY_SYSTEM:
      POOL_DEBUG(SOLV_DEBUG_RESULT, "%s is provided by the system\n", pool_dep2str(pool, dep));
      return;
    case SOLVER_RULE_RPM:
      POOL_DEBUG(SOLV_DEBUG_RESULT, "some dependency problem\n");
      return;
    case SOLVER_RULE_RPM_NOT_INSTALLABLE:
      POOL_DEBUG(SOLV_DEBUG_RESULT, "package %s is not installable\n", pool_solvid2str(pool, source));
      return;
    case SOLVER_RULE_RPM_NOTHING_PROVIDES_DEP:
      POOL_DEBUG(SOLV_DEBUG_RESULT, "nothing provides %s needed by %s\n", pool_dep2str(pool, dep), pool_solvid2str(pool, source));
      return;
    case SOLVER_RULE_RPM_SAME_NAME:
      POOL_DEBUG(SOLV_DEBUG_RESULT, "cannot install both %s and %s\n", pool_solvid2str(pool, source), pool_solvid2str(pool, target));
      return;
    case SOLVER_RULE_RPM_PACKAGE_CONFLICT:
      POOL_DEBUG(SOLV_DEBUG_RESULT, "package %s conflicts with %s provided by %s\n", pool_solvid2str(pool, source), pool_dep2str(pool, dep), pool_solvid2str(pool, target));
      return;
    case SOLVER_RULE_RPM_PACKAGE_OBSOLETES:
      POOL_DEBUG(SOLV_DEBUG_RESULT, "package %s obsoletes %s provided by %s\n", pool_solvid2str(pool, source), pool_dep2str(pool, dep), pool_solvid2str(pool, target));
      return;
    case SOLVER_RULE_RPM_INSTALLEDPKG_OBSOLETES:
      POOL_DEBUG(SOLV_DEBUG_RESULT, "installed package %s obsoletes %s provided by %s\n", pool_solvid2str(pool, source), pool_dep2str(pool, dep), pool_solvid2str(pool, target));
      return;
    case SOLVER_RULE_RPM_IMPLICIT_OBSOLETES:
      POOL_DEBUG(SOLV_DEBUG_RESULT, "package %s implicitly obsoletes %s provided by %s\n", pool_solvid2str(pool, source), pool_dep2str(pool, dep), pool_solvid2str(pool, target));
      return;
    case SOLVER_RULE_RPM_PACKAGE_REQUIRES:
      POOL_DEBUG(SOLV_DEBUG_RESULT, "package %s requires %s, but none of the providers can be installed\n", pool_solvid2str(pool, source), pool_dep2str(pool, dep));
      return;
    case SOLVER_RULE_RPM_SELF_CONFLICT:
      POOL_DEBUG(SOLV_DEBUG_RESULT, "package %s conflicts with %s provided by itself\n", pool_solvid2str(pool, source), pool_dep2str(pool, dep));
      return;
    case SOLVER_RULE_UNKNOWN:
    case SOLVER_RULE_FEATURE:
    case SOLVER_RULE_LEARNT:
    case SOLVER_RULE_CHOICE:
    case SOLVER_RULE_BEST:
      POOL_DEBUG(SOLV_DEBUG_RESULT, "bad rule type\n");
      return;
    }
}

void
solver_printprobleminfo(Solver *solv, Id problem)
{
  solver_printproblemruleinfo(solv, solver_findproblemrule(solv, problem));
}

void
solver_printcompleteprobleminfo(Solver *solv, Id problem)
{
  Queue q;
  Id probr;
  int i, nobad = 0;

  queue_init(&q);
  solver_findallproblemrules(solv, problem, &q);
  for (i = 0; i < q.count; i++)
    {
      probr = q.elements[i];
      if (!(probr >= solv->updaterules && probr < solv->updaterules_end) && !(probr >= solv->jobrules && probr < solv->jobrules_end))
	{
	  nobad = 1;
	  break;
	}
    }
  for (i = 0; i < q.count; i++)
    {
      probr = q.elements[i];
      if (nobad && ((probr >= solv->updaterules && probr < solv->updaterules_end) || (probr >= solv->jobrules && probr < solv->jobrules_end)))
	continue;
      solver_printproblemruleinfo(solv, probr);
    }
  queue_free(&q);
}

void
solver_printsolution(Solver *solv, Id problem, Id solution)
{
  Pool *pool = solv->pool;
  Id p, rp, element, how, what, select;
  Solvable *s, *sd;

  element = 0;
  while ((element = solver_next_solutionelement(solv, problem, solution, element, &p, &rp)) != 0)
    {
      if (p == SOLVER_SOLUTION_JOB || p == SOLVER_SOLUTION_POOLJOB)
	{
	  /* job, rp is index into job queue */
	  if (p == SOLVER_SOLUTION_JOB)
	    rp += solv->pooljobcnt;
	  how = solv->job.elements[rp - 1];
	  what = solv->job.elements[rp];
	  select = how & SOLVER_SELECTMASK;
	  switch (how & SOLVER_JOBMASK)
	    {
	    case SOLVER_INSTALL:
	      if (select == SOLVER_SOLVABLE && solv->installed && pool->solvables[what].repo == solv->installed)
		POOL_DEBUG(SOLV_DEBUG_RESULT, "  - do not keep %s installed\n", pool_solvid2str(pool, what));
	      else if (select == SOLVER_SOLVABLE_PROVIDES)
		POOL_DEBUG(SOLV_DEBUG_RESULT, "  - do not install a solvable %s\n", solver_select2str(pool, select, what));
	      else
		POOL_DEBUG(SOLV_DEBUG_RESULT, "  - do not install %s\n", solver_select2str(pool, select, what));
	      break;
	    case SOLVER_ERASE:
	      if (select == SOLVER_SOLVABLE && !(solv->installed && pool->solvables[what].repo == solv->installed))
		POOL_DEBUG(SOLV_DEBUG_RESULT, "  - do not forbid installation of %s\n", pool_solvid2str(pool, what));
	      else if (select == SOLVER_SOLVABLE_PROVIDES)
		POOL_DEBUG(SOLV_DEBUG_RESULT, "  - do not deinstall all solvables %s\n", solver_select2str(pool, select, what));
	      else
		POOL_DEBUG(SOLV_DEBUG_RESULT, "  - do not deinstall %s\n", solver_select2str(pool, select, what));
	      break;
	    case SOLVER_UPDATE:
	      POOL_DEBUG(SOLV_DEBUG_RESULT, "  - do not install most recent version of %s\n", solver_select2str(pool, select, what));
	      break;
	    case SOLVER_LOCK:
	      POOL_DEBUG(SOLV_DEBUG_RESULT, "  - do not lock %s\n", solver_select2str(pool, select, what));
	      break;
	    default:
	      POOL_DEBUG(SOLV_DEBUG_RESULT, "  - do something different\n");
	      break;
	    }
	}
      else if (p == SOLVER_SOLUTION_INFARCH)
	{
	  s = pool->solvables + rp;
	  if (solv->installed && s->repo == solv->installed)
	    POOL_DEBUG(SOLV_DEBUG_RESULT, "  - keep %s despite the inferior architecture\n", pool_solvable2str(pool, s));
	  else
	    POOL_DEBUG(SOLV_DEBUG_RESULT, "  - install %s despite the inferior architecture\n", pool_solvable2str(pool, s));
	}
      else if (p == SOLVER_SOLUTION_DISTUPGRADE)
	{
	  s = pool->solvables + rp;
	  if (solv->installed && s->repo == solv->installed)
	    POOL_DEBUG(SOLV_DEBUG_RESULT, "  - keep obsolete %s\n", pool_solvable2str(pool, s));
	  else
	    POOL_DEBUG(SOLV_DEBUG_RESULT, "  - install %s from excluded repository\n", pool_solvable2str(pool, s));
	}
      else if (p == SOLVER_SOLUTION_BEST)
	{
	  s = pool->solvables + rp;
	  if (solv->installed && s->repo == solv->installed)
	    POOL_DEBUG(SOLV_DEBUG_RESULT, "  - keep old %s\n", pool_solvable2str(pool, s));
	  else
	    POOL_DEBUG(SOLV_DEBUG_RESULT, "  - install %s despite the old version\n", pool_solvable2str(pool, s));
	}
      else
	{
	  /* policy, replace p with rp */
	  s = pool->solvables + p;
	  sd = rp ? pool->solvables + rp : 0;
	  if (sd)
	    {
	      int illegal = policy_is_illegal(solv, s, sd, 0);
	      if ((illegal & POLICY_ILLEGAL_DOWNGRADE) != 0)
		POOL_DEBUG(SOLV_DEBUG_RESULT, "  - allow downgrade of %s to %s\n", pool_solvable2str(pool, s), pool_solvable2str(pool, sd));
	      if ((illegal & POLICY_ILLEGAL_NAMECHANGE) != 0)
		POOL_DEBUG(SOLV_DEBUG_RESULT, "  - allow name change of %s to %s\n", pool_solvable2str(pool, s), pool_solvable2str(pool, sd));
	      if ((illegal & POLICY_ILLEGAL_ARCHCHANGE) != 0)
		POOL_DEBUG(SOLV_DEBUG_RESULT, "  - allow architecture change of %s to %s\n", pool_solvable2str(pool, s), pool_solvable2str(pool, sd));
	      if ((illegal & POLICY_ILLEGAL_VENDORCHANGE) != 0)
		{
		  if (sd->vendor)
		    POOL_DEBUG(SOLV_DEBUG_RESULT, "  - allow vendor change from '%s' (%s) to '%s' (%s)\n", pool_id2str(pool, s->vendor), pool_solvable2str(pool, s), pool_id2str(pool, sd->vendor), pool_solvable2str(pool, sd));
		  else
		    POOL_DEBUG(SOLV_DEBUG_RESULT, "  - allow vendor change from '%s' (%s) to no vendor (%s)\n", pool_id2str(pool, s->vendor), pool_solvable2str(pool, s), pool_solvable2str(pool, sd));
		}
	      if (!illegal)
		POOL_DEBUG(SOLV_DEBUG_RESULT, "  - allow replacement of %s with %s\n", pool_solvable2str(pool, s), pool_solvable2str(pool, sd));
	    }
	  else
	    {
	      POOL_DEBUG(SOLV_DEBUG_RESULT, "  - allow deinstallation of %s\n", pool_solvable2str(pool, s));
	    }
	}
    }
}

void
solver_printallsolutions(Solver *solv)
{
  Pool *pool = solv->pool;
  int pcnt;
  Id problem, solution;

  POOL_DEBUG(SOLV_DEBUG_RESULT, "Encountered problems! Here are the solutions:\n\n");
  pcnt = 0;
  problem = 0;
  while ((problem = solver_next_problem(solv, problem)) != 0)
    {
      pcnt++;
      POOL_DEBUG(SOLV_DEBUG_RESULT, "Problem %d:\n", pcnt);
      POOL_DEBUG(SOLV_DEBUG_RESULT, "====================================\n");
#if 1
      solver_printprobleminfo(solv, problem);
#else
      solver_printcompleteprobleminfo(solv, problem);
#endif
      POOL_DEBUG(SOLV_DEBUG_RESULT, "\n");
      solution = 0;
      while ((solution = solver_next_solution(solv, problem, solution)) != 0)
        {
	  solver_printsolution(solv, problem, solution);
          POOL_DEBUG(SOLV_DEBUG_RESULT, "\n");
        }
    }
}

void
solver_printtrivial(Solver *solv)
{
  Pool *pool = solv->pool;
  Queue in, out;
  Id p;
  const char *n; 
  Solvable *s; 
  int i;

  queue_init(&in);
  for (p = 1, s = pool->solvables + p; p < solv->pool->nsolvables; p++, s++)
    {   
      n = pool_id2str(pool, s->name);
      if (strncmp(n, "patch:", 6) != 0 && strncmp(n, "pattern:", 8) != 0)
        continue;
      queue_push(&in, p); 
    }   
  if (!in.count)
    {
      queue_free(&in);
      return;
    }
  queue_init(&out);
  solver_trivial_installable(solv, &in, &out);
  POOL_DEBUG(SOLV_DEBUG_RESULT, "trivial installable status:\n");
  for (i = 0; i < in.count; i++)
    POOL_DEBUG(SOLV_DEBUG_RESULT, "  %s: %d\n", pool_solvid2str(pool, in.elements[i]), out.elements[i]);
  POOL_DEBUG(SOLV_DEBUG_RESULT, "\n");
  queue_free(&in);
  queue_free(&out);
}

const char *
solver_select2str(Pool *pool, Id select, Id what)
{
  const char *s;
  char *b;
  select &= SOLVER_SELECTMASK;
  if (select == SOLVER_SOLVABLE)
    return pool_solvid2str(pool, what);
  if (select == SOLVER_SOLVABLE_NAME)
    return pool_dep2str(pool, what);
  if (select == SOLVER_SOLVABLE_PROVIDES)
    {
      s = pool_dep2str(pool, what);
      b = pool_alloctmpspace(pool, 11 + strlen(s));
      sprintf(b, "providing %s", s);
      return b;
    }
  if (select == SOLVER_SOLVABLE_ONE_OF)
    {
      Id p;
      b = 0;
      while ((p = pool->whatprovidesdata[what++]) != 0)
	{
	  s = pool_solvid2str(pool, p);
	  if (b)
	    b = pool_tmpappend(pool, b, ", ", s);
	  else
	    b = pool_tmpjoin(pool, s, 0, 0);
	  pool_freetmpspace(pool, s);
	}
      return b ? b : "nothing";
    }
  if (select == SOLVER_SOLVABLE_REPO)
    {
      b = pool_alloctmpspace(pool, 20);
      sprintf(b, "repo #%d", what);
      return b;
    }
  if (select == SOLVER_SOLVABLE_ALL)
    return "all packages";
  return "unknown job select";
}

const char *
pool_job2str(Pool *pool, Id how, Id what, Id flagmask)
{
  Id select = how & SOLVER_SELECTMASK;
  const char *strstart = 0, *strend = 0;
  char *s;
  int o;

  switch (how & SOLVER_JOBMASK)
    {
    case SOLVER_NOOP:
      return "do nothing";
    case SOLVER_INSTALL:
      if (select == SOLVER_SOLVABLE && pool->installed && pool->solvables[what].repo == pool->installed)
	strstart = "keep ", strend = "installed";
      else if (select == SOLVER_SOLVABLE || select == SOLVER_SOLVABLE_NAME)
	strstart = "install ";
      else if (select == SOLVER_SOLVABLE_PROVIDES)
	strstart = "install a package ";
      else
	strstart = "install one of ";
      break;
    case SOLVER_ERASE:
      if (select == SOLVER_SOLVABLE && !(pool->installed && pool->solvables[what].repo == pool->installed))
	strstart = "keep ", strend = "unstalled";
      else if (select == SOLVER_SOLVABLE_PROVIDES)
	strstart = "deinstall all packages ";
      else
	strstart = "deinstall ";
      break;
    case SOLVER_UPDATE:
      strstart = "update ";
      break;
    case SOLVER_WEAKENDEPS:
      strstart = "weaken deps of ";
      break;
    case SOLVER_MULTIVERSION:
      strstart = "multi version ";
      break;
    case SOLVER_LOCK:
      strstart = "update ";
      break;
    case SOLVER_DISTUPGRADE:
      strstart = "dist upgrade ";
      break;
    case SOLVER_VERIFY:
      strstart = "verify ";
      break;
    case SOLVER_DROP_ORPHANED:
      strstart = "deinstall ", strend = "if orphaned";
      break;
    case SOLVER_USERINSTALLED:
      strstart = "regard ", strend = "as userinstalled";
      break;
    default:
      strstart = "unknown job ";
      break;
    }
  s = pool_tmpjoin(pool, strstart, solver_select2str(pool, select, what), strend);
  how &= flagmask;
  if ((how & ~(SOLVER_SELECTMASK|SOLVER_JOBMASK)) == 0)
    return s;
  o = strlen(s);
  s = pool_tmpappend(pool, s, " ", 0);
  if (how & SOLVER_WEAK)
    s = pool_tmpappend(pool, s, ",weak", 0);
  if (how & SOLVER_ESSENTIAL)
    s = pool_tmpappend(pool, s, ",essential", 0);
  if (how & SOLVER_CLEANDEPS)
    s = pool_tmpappend(pool, s, ",cleandeps", 0);
  if (how & SOLVER_SETEV)
    s = pool_tmpappend(pool, s, ",setev", 0);
  if (how & SOLVER_SETEVR)
    s = pool_tmpappend(pool, s, ",setevr", 0);
  if (how & SOLVER_SETARCH)
    s = pool_tmpappend(pool, s, ",setarch", 0);
  if (how & SOLVER_SETVENDOR)
    s = pool_tmpappend(pool, s, ",setvendor", 0);
  if (how & SOLVER_SETREPO)
    s = pool_tmpappend(pool, s, ",setrepo", 0);
  if (how & SOLVER_NOAUTOSET)
    s = pool_tmpappend(pool, s, ",noautoset", 0);
  if (s[o + 1] != ',')
    s = pool_tmpappend(pool, s, ",?", 0);
  s[o + 1] = '[';
  return pool_tmpappend(pool, s, "]", 0);
}

const char *
pool_selection2str(Pool *pool, Queue *selection, Id flagmask)
{
  char *s;
  const char *s2;
  int i;
  s = pool_tmpjoin(pool, 0, 0, 0);
  for (i = 0; i < selection->count; i += 2)
    {
      Id how = selection->elements[i];
      if (*s)
	s = pool_tmpappend(pool, s, " + ", 0);
      s2 = solver_select2str(pool, how & SOLVER_SELECTMASK, selection->elements[i + 1]);
      s = pool_tmpappend(pool, s, s2, 0);
      pool_freetmpspace(pool, s2);
      how &= flagmask & SOLVER_SETMASK;
      if (how)
	{
	  int o = strlen(s);
	  s = pool_tmpappend(pool, s, " ", 0);
	  if (how & SOLVER_SETEV)
	    s = pool_tmpappend(pool, s, ",setev", 0);
	  if (how & SOLVER_SETEVR)
	    s = pool_tmpappend(pool, s, ",setevr", 0);
	  if (how & SOLVER_SETARCH)
	    s = pool_tmpappend(pool, s, ",setarch", 0);
	  if (how & SOLVER_SETVENDOR)
	    s = pool_tmpappend(pool, s, ",setvendor", 0);
	  if (how & SOLVER_SETREPO)
	    s = pool_tmpappend(pool, s, ",setrepo", 0);
	  if (how & SOLVER_NOAUTOSET)
	    s = pool_tmpappend(pool, s, ",noautoset", 0);
	  if (s[o + 1] != ',')
	    s = pool_tmpappend(pool, s, ",?", 0);
	  s[o + 1] = '[';
	  s = pool_tmpappend(pool, s, "]", 0);
	}
    }
  return s;
}

const char *
solver_problemruleinfo2str(Solver *solv, SolverRuleinfo type, Id source, Id target, Id dep)
{
  Pool *pool = solv->pool;
  char *s;
  switch (type)
    {
    case SOLVER_RULE_DISTUPGRADE:
      return pool_tmpjoin(pool, pool_solvid2str(pool, source), " does not belong to a distupgrade repository", 0);
    case SOLVER_RULE_INFARCH:
      return pool_tmpjoin(pool, pool_solvid2str(pool, source), " has inferior architecture", 0);
    case SOLVER_RULE_UPDATE:
      return pool_tmpjoin(pool, "problem with installed package ", pool_solvid2str(pool, source), 0);
    case SOLVER_RULE_JOB:
      return "conflicting requests";
    case SOLVER_RULE_JOB_NOTHING_PROVIDES_DEP:
      return pool_tmpjoin(pool, "nothing provides requested ", pool_dep2str(pool, dep), 0);
    case SOLVER_RULE_JOB_PROVIDED_BY_SYSTEM:
      return pool_tmpjoin(pool, pool_dep2str(pool, dep), " is provided by the system", 0);
    case SOLVER_RULE_RPM:
      return "some dependency problem";
    case SOLVER_RULE_RPM_NOT_INSTALLABLE:
      return pool_tmpjoin(pool, "package ", pool_solvid2str(pool, source), " is not installable");
    case SOLVER_RULE_RPM_NOTHING_PROVIDES_DEP:
      s = pool_tmpjoin(pool, "nothing provides ", pool_dep2str(pool, dep), 0);
      return pool_tmpappend(pool, s, " needed by ", pool_solvid2str(pool, source));
    case SOLVER_RULE_RPM_SAME_NAME:
      s = pool_tmpjoin(pool, "cannot install both ", pool_solvid2str(pool, source), 0);
      return pool_tmpappend(pool, s, " and ", pool_solvid2str(pool, target));
    case SOLVER_RULE_RPM_PACKAGE_CONFLICT:
      s = pool_tmpjoin(pool, "package ", pool_solvid2str(pool, source), 0);
      s = pool_tmpappend(pool, s, " conflicts with ", pool_dep2str(pool, dep));
      return pool_tmpappend(pool, s, " provided by ", pool_solvid2str(pool, target));
    case SOLVER_RULE_RPM_PACKAGE_OBSOLETES:
      s = pool_tmpjoin(pool, "package ", pool_solvid2str(pool, source), 0);
      s = pool_tmpappend(pool, s, " obsoletes ", pool_dep2str(pool, dep));
      return pool_tmpappend(pool, s, " provided by ", pool_solvid2str(pool, target));
    case SOLVER_RULE_RPM_INSTALLEDPKG_OBSOLETES:
      s = pool_tmpjoin(pool, "installed package ", pool_solvid2str(pool, source), 0);
      s = pool_tmpappend(pool, s, " obsoletes ", pool_dep2str(pool, dep));
      return pool_tmpappend(pool, s, " provided by ", pool_solvid2str(pool, target));
    case SOLVER_RULE_RPM_IMPLICIT_OBSOLETES:
      s = pool_tmpjoin(pool, "package ", pool_solvid2str(pool, source), 0);
      s = pool_tmpappend(pool, s, " implicitly obsoletes ", pool_dep2str(pool, dep));
      return pool_tmpappend(pool, s, " provided by ", pool_solvid2str(pool, target));
    case SOLVER_RULE_RPM_PACKAGE_REQUIRES:
      s = pool_tmpjoin(pool, "package ", pool_solvid2str(pool, source), " requires ");
      return pool_tmpappend(pool, s, pool_dep2str(pool, dep), ", but none of the providers can be installed");
    case SOLVER_RULE_RPM_SELF_CONFLICT:
      s = pool_tmpjoin(pool, "package ", pool_solvid2str(pool, source), " conflicts with ");
      return pool_tmpappend(pool, s, pool_dep2str(pool, dep), " provided by itself");
    default:
      return "bad problem rule type";
    }
}

const char *
solver_solutionelement2str(Solver *solv, Id p, Id rp)
{
  Pool *pool = solv->pool;
  if (p == SOLVER_SOLUTION_JOB || p == SOLVER_SOLUTION_POOLJOB)
    {
      Id how, what;
      if (p == SOLVER_SOLUTION_JOB)
	rp += solv->pooljobcnt;
      how = solv->job.elements[rp - 1];
      what = solv->job.elements[rp];
      return pool_tmpjoin(pool, "do not ask to ", pool_job2str(pool, how, what, 0), 0);
    }
  else if (p == SOLVER_SOLUTION_INFARCH)
    {
      Solvable *s = pool->solvables + rp;
      if (solv->installed && s->repo == solv->installed)
        return pool_tmpjoin(pool, "keep ", pool_solvable2str(pool, s), " despite the inferior architecture");
      else
        return pool_tmpjoin(pool, "install ", pool_solvable2str(pool, s), " despite the inferior architecture");
    }
  else if (p == SOLVER_SOLUTION_DISTUPGRADE)
    {
      Solvable *s = pool->solvables + rp;
      if (solv->installed && s->repo == solv->installed)
        return pool_tmpjoin(pool, "keep obsolete ", pool_solvable2str(pool, s), 0);
      else
        return pool_tmpjoin(pool, "install ", pool_solvable2str(pool, s), " from excluded repository");
    }
  else if (p == SOLVER_SOLUTION_BEST)
    {
      Solvable *s = pool->solvables + rp;
      if (solv->installed && s->repo == solv->installed)
        return pool_tmpjoin(pool, "keep old ", pool_solvable2str(pool, s), 0);
      else
        return pool_tmpjoin(pool, "install ", pool_solvable2str(pool, s), " despite the old version");
    }
  else if (p > 0 && rp == 0)
    return pool_tmpjoin(pool, "allow deinstallation of ", pool_solvid2str(pool, p), 0);
  else if (p > 0 && rp > 0)
    {
      const char *sp = pool_solvid2str(pool, p);
      const char *srp = pool_solvid2str(pool, rp);
      const char *str = pool_tmpjoin(pool, "allow replacement of ", sp, 0);
      return pool_tmpappend(pool, str, " with ", srp);
    }
  else
    return "bad solution element";
}

const char *
policy_illegal2str(Solver *solv, int illegal, Solvable *s, Solvable *rs)
{
  Pool *pool = solv->pool;
  const char *str;
  if (illegal == POLICY_ILLEGAL_DOWNGRADE)
    {
      str = pool_tmpjoin(pool, "downgrade of ", pool_solvable2str(pool, s), 0);
      return pool_tmpappend(pool, str, " to ", pool_solvable2str(pool, rs));
    }
  if (illegal == POLICY_ILLEGAL_NAMECHANGE)
    {
      str = pool_tmpjoin(pool, "name change of ", pool_solvable2str(pool, s), 0);
      return pool_tmpappend(pool, str, " to ", pool_solvable2str(pool, rs));
    }
  if (illegal == POLICY_ILLEGAL_ARCHCHANGE)
    {
      str = pool_tmpjoin(pool, "architecture change of ", pool_solvable2str(pool, s), 0);
      return pool_tmpappend(pool, str, " to ", pool_solvable2str(pool, rs));
    }
  if (illegal == POLICY_ILLEGAL_VENDORCHANGE)
    {
      str = pool_tmpjoin(pool, "vendor change from '", pool_id2str(pool, s->vendor), "' (");
      if (rs->vendor)
	{
          str = pool_tmpappend(pool, str, pool_solvable2str(pool, s), ") to '");
          str = pool_tmpappend(pool, str, pool_id2str(pool, rs->vendor), "' (");
	}
      else
        str = pool_tmpappend(pool, str, pool_solvable2str(pool, s), ") to no vendor (");
      return pool_tmpappend(pool, str, pool_solvable2str(pool, rs), ")");
    }
  return "unknown illegal change";
}
