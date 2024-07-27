/*
 * Copyright (c) 2007, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/*
 * Generic policy interface for SAT solver
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "solver.h"
#include "solver_private.h"
#include "evr.h"
#include "policy.h"
#include "poolvendor.h"
#include "poolarch.h"


/*-----------------------------------------------------------------*/

/*
 * prep for prune_best_version
 *   sort by name
 */

static int
prune_to_best_version_sortcmp(const void *ap, const void *bp, void *dp)
{
  Pool *pool = dp;
  int r;
  Id a = *(Id *)ap;
  Id b = *(Id *)bp;
  Solvable *sa, *sb;

  sa = pool->solvables + a;
  sb = pool->solvables + b;
  r = sa->name - sb->name;
  if (r)
    {
      const char *na, *nb;
      /* different names. We use real strcmp here so that the result
       * is not depending on some random solvable order */
      na = pool_id2str(pool, sa->name);
      nb = pool_id2str(pool, sb->name);
      return strcmp(na, nb);
    }
  /* the same name, bring installed solvables to the front */
  if (pool->installed)
    {
      if (sa->repo == pool->installed)
	{
	  if (sb->repo != pool->installed)
	    return -1;
	}
      else if (sb->repo == pool->installed)
	return 1;	
    }
  /* sort by repository sub-prio (installed repo handled above) */
  r = (sb->repo ? sb->repo->subpriority : 0) - (sa->repo ? sa->repo->subpriority : 0);
  if (r)
    return r;
  /* no idea about the order, sort by id */
  return a - b;
}


/*
 * prune to repository with highest priority.
 * does not prune installed solvables.
 */

static void
prune_to_highest_prio(Pool *pool, Queue *plist)
{
  int i, j;
  Solvable *s;
  int bestprio = 0, bestprioset = 0;

  /* prune to highest priority */
  for (i = 0; i < plist->count; i++)  /* find highest prio in queue */
    {
      s = pool->solvables + plist->elements[i];
      if (pool->installed && s->repo == pool->installed)
	continue;
      if (!bestprioset || s->repo->priority > bestprio)
	{
	  bestprio = s->repo->priority;
	  bestprioset = 1;
	}
    }
  if (!bestprioset)
    return;
  for (i = j = 0; i < plist->count; i++) /* remove all with lower prio */
    {
      s = pool->solvables + plist->elements[i];
      if (s->repo->priority == bestprio || (pool->installed && s->repo == pool->installed))
	plist->elements[j++] = plist->elements[i];
    }
  plist->count = j;
}

static void
prune_to_highest_prio_per_name(Pool *pool, Queue *plist)
{
  Queue pq;
  int i, j, k;
  Id name;

  queue_init(&pq);
  solv_sort(plist->elements, plist->count, sizeof(Id), prune_to_best_version_sortcmp, pool);
  queue_push(&pq, plist->elements[0]);
  name = pool->solvables[pq.elements[0]].name;
  for (i = 1, j = 0; i < plist->count; i++)
    {
      if (pool->solvables[plist->elements[i]].name != name)
	{
	  if (pq.count > 2)
	    prune_to_highest_prio(pool, &pq);
	  for (k = 0; k < pq.count; k++)
	    plist->elements[j++] = pq.elements[k];
	  queue_empty(&pq);
	  queue_push(&pq, plist->elements[i]);
	  name = pool->solvables[pq.elements[0]].name;
	}
    }
  if (pq.count > 2)
    prune_to_highest_prio(pool, &pq);
  for (k = 0; k < pq.count; k++)
    plist->elements[j++] = pq.elements[k];
  queue_free(&pq);
  plist->count = j;
}


/*
 * prune to recommended/suggested packages.
 * does not prune installed packages (they are also somewhat recommended).
 */

static void
prune_to_recommended(Solver *solv, Queue *plist)
{
  Pool *pool = solv->pool;
  int i, j, k, ninst;
  Solvable *s;
  Id p, pp, rec, *recp, sug, *sugp;

  ninst = 0;
  if (pool->installed)
    {
      for (i = 0; i < plist->count; i++)
	{
	  p = plist->elements[i];
	  s = pool->solvables + p;
	  if (pool->installed && s->repo == pool->installed)
	    ninst++;
	}
    }
  if (plist->count - ninst < 2)
    return;

  /* update our recommendsmap/suggestsmap */
  if (solv->recommends_index < 0)
    {
      MAPZERO(&solv->recommendsmap);
      MAPZERO(&solv->suggestsmap);
      solv->recommends_index = 0;
    }
  while (solv->recommends_index < solv->decisionq.count)
    {
      p = solv->decisionq.elements[solv->recommends_index++];
      if (p < 0)
	continue;
      s = pool->solvables + p;
      if (s->recommends)
	{
	  recp = s->repo->idarraydata + s->recommends;
          while ((rec = *recp++) != 0)
	    FOR_PROVIDES(p, pp, rec)
	      MAPSET(&solv->recommendsmap, p);
	}
      if (s->suggests)
	{
	  sugp = s->repo->idarraydata + s->suggests;
          while ((sug = *sugp++) != 0)
	    FOR_PROVIDES(p, pp, sug)
	      MAPSET(&solv->suggestsmap, p);
	}
    }

  /* prune to recommended/supplemented */
  ninst = 0;
  for (i = j = 0; i < plist->count; i++)
    {
      p = plist->elements[i];
      s = pool->solvables + p;
      if (pool->installed && s->repo == pool->installed)
	{
	  ninst++;
	  if (j)
	    plist->elements[j++] = p;
	  continue;
	}
      if (!MAPTST(&solv->recommendsmap, p))
	if (!solver_is_supplementing(solv, s))
	  continue;
      if (!j && ninst)
	{
	  for (k = 0; j < ninst; k++)
	    {
	      s = pool->solvables + plist->elements[k];
	      if (pool->installed && s->repo == pool->installed)
	        plist->elements[j++] = plist->elements[k];
	    }
	}
      plist->elements[j++] = p;
    }
  if (j)
    plist->count = j;

  /* anything left to prune? */
  if (plist->count - ninst < 2)
    return;

  /* prune to suggested/enhanced */
  ninst = 0;
  for (i = j = 0; i < plist->count; i++)
    {
      p = plist->elements[i];
      s = pool->solvables + p;
      if (pool->installed && s->repo == pool->installed)
	{
	  ninst++;
	  if (j)
	    plist->elements[j++] = p;
	  continue;
	}
      if (!MAPTST(&solv->suggestsmap, p))
        if (!solver_is_enhancing(solv, s))
	  continue;
      if (!j && ninst)
	{
	  for (k = 0; j < ninst; k++)
	    {
	      s = pool->solvables + plist->elements[k];
	      if (pool->installed && s->repo == pool->installed)
	        plist->elements[j++] = plist->elements[k];
	    }
	}
      plist->elements[j++] = p;
    }
  if (j)
    plist->count = j;
}

static void
prune_to_best_arch(const Pool *pool, Queue *plist)
{
  Id a, bestscore;
  Solvable *s;
  int i, j;

  if (!pool->id2arch || plist->count < 2)
    return;
  bestscore = 0;
  for (i = 0; i < plist->count; i++)
    {
      s = pool->solvables + plist->elements[i];
      a = s->arch;
      a = (a <= pool->lastarch) ? pool->id2arch[a] : 0;
      if (a && a != 1 && (!bestscore || a < bestscore))
	bestscore = a;
    }
  if (!bestscore)
    return;
  for (i = j = 0; i < plist->count; i++)
    {
      s = pool->solvables + plist->elements[i];
      a = s->arch;
      if (a > pool->lastarch)
	continue;
      a = pool->id2arch[a];
      /* a == 1 -> noarch */
      if (a != 1 && ((a ^ bestscore) & 0xffff0000) != 0)
	continue;
      plist->elements[j++] = plist->elements[i];
    }
  if (j)
    plist->count = j;
}


struct trj_data {
  Pool *pool;
  Queue *plist;
  Id *stack;
  Id nstack;
  Id *low;
  Id firstidx;
  Id idx;
};

/* This is Tarjan's SCC algorithm, slightly modified */
static void
trj_visit(struct trj_data *trj, Id node)
{
  Id *low = trj->low;
  Pool *pool = trj->pool;
  Queue *plist = trj->plist;
  Id myidx, stackstart;
  Solvable *s;
  int i;
  Id p, pp, obs, *obsp;

  low[node] = myidx = trj->idx++;
  trj->stack[(stackstart = trj->nstack++)] = node;

  s = pool->solvables + plist->elements[node];
  if (s->obsoletes)
    {
      obsp = s->repo->idarraydata + s->obsoletes;
      while ((obs = *obsp++) != 0)
	{
	  FOR_PROVIDES(p, pp, obs)
	    {
	      Solvable *ps = pool->solvables + p;
	      if (ps->name == s->name)
		continue;
	      if (!pool->obsoleteusesprovides && !pool_match_nevr(pool, ps, obs))
		continue;
	      if (pool->obsoleteusescolors && !pool_colormatch(pool, s, ps))
		continue;
	      /* hmm, expensive. should use hash if plist is big */
	      for (i = 0; i < plist->count; i++)
		{
		  if (node != i && plist->elements[i] == p)
		    {
		      Id l = low[i];
		      if (!l)
			{
			  if (!ps->obsoletes)
			    {
			      /* don't bother */
			      trj->idx++;
			      low[i] = -1;
			      continue;
			    }
			  trj_visit(trj, i);
			  l = low[i];
			}
		      if (l < 0)
			continue;
		      if (l < trj->firstidx)
			{
			  int k;
			  /* this means we have reached an old SCC found earlier.
			   * delete it as we obsolete it */
			  for (k = l; ; k++)
			    {
			      if (low[trj->stack[k]] == l)
				low[trj->stack[k]] = -1;
			      else
				break;
			    }
			}
		      else if (l < low[node])
			low[node] = l;
		    }
		}
	    }
	}
    }
  if (low[node] == myidx)	/* found a SCC? */
    {
      /* we're only interested in SCCs that contain the first node,
       * as all others are "obsoleted" */
      if (myidx != trj->firstidx)
	myidx = -1;
      for (i = stackstart; i < trj->nstack; i++)
	low[trj->stack[i]] = myidx;
      trj->nstack = stackstart;	/* empty stack */
    }
}
  
/*
 * remove entries from plist that are obsoleted by other entries
 * with different name.
 */
static void
prune_obsoleted(Pool *pool, Queue *plist)
{
  Id data_buf[2 * 16], *data;
  struct trj_data trj;
  int i, j;
  Solvable *s;
  
  if (plist->count <= 16)
    {
      memset(data_buf, 0, sizeof(data_buf));
      data = data_buf;
    }
  else
    data = solv_calloc(plist->count, 2 * sizeof(Id));
  trj.pool = pool;
  trj.plist = plist;
  trj.low = data;
  trj.idx = 1;
  trj.stack = data + plist->count - 1;	/* -1 so we can index with idx (which starts with 1) */
  for (i = 0; i < plist->count; i++)
    {
      if (trj.low[i])
	continue;
      s = pool->solvables + plist->elements[i];
      if (s->obsoletes)
	{
	  trj.firstidx = trj.nstack = trj.idx;
          trj_visit(&trj, i);
	}
      else
        {
          Id myidx = trj.idx++;
          trj.low[i] = myidx;
          trj.stack[myidx] = i;
        }
    }
  for (i = j = 0; i < plist->count; i++)
    if (trj.low[i] >= 0)
      plist->elements[j++] = plist->elements[i];
  plist->count = j;
  if (data != data_buf)
    solv_free(data);
}

/* this is prune_obsoleted special-cased for two elements */
static void
prune_obsoleted_2(Pool *pool, Queue *plist)
{
  int i;
  Solvable *s;
  Id p, pp, obs, *obsp;
  Id other;
  int obmap = 0;

  for (i = 0; i < 2; i++)
    {
      s = pool->solvables + plist->elements[i];
      other = plist->elements[1 - i];
      if (s->obsoletes)
	{
	  obsp = s->repo->idarraydata + s->obsoletes;
	  while ((obs = *obsp++) != 0)
	    {
	      FOR_PROVIDES(p, pp, obs)
		{
		  Solvable *ps;
		  if (p != other)
		    continue;
		  ps = pool->solvables + p;
		  if (ps->name == s->name)
		    continue;
		  if (!pool->obsoleteusesprovides && !pool_match_nevr(pool, ps, obs))
		    continue;
		  if (pool->obsoleteusescolors && !pool_colormatch(pool, s, ps))
		    continue;
		  obmap |= 1 << i;
		  break;
		}
	      if (p)
		break;
	    }
	}
    }
  if (obmap == 0 || obmap == 3)
    return;
  if (obmap == 2)
    plist->elements[0] = plist->elements[1];
  plist->count = 1;
}

/*
 * bring those elements to the front of the queue that
 * have a installed solvable with the same name
 */
static void
move_installed_to_front(Pool *pool, Queue *plist)
{
  int i, j;
  Solvable *s;
  Id p, pp;

  for (i = j = 0; i < plist->count; i++)
    {
      s = pool->solvables + plist->elements[i];
      if (s->repo != pool->installed)
        {
          FOR_PROVIDES(p, pp, s->name)
	    {
	      Solvable *ps = pool->solvables + p;
	      if (s->name == ps->name && ps->repo == pool->installed)
		{
		  s = ps;
		  break;
		}
	    }
        }
      if (s->repo == pool->installed)
	{
	  if (i != j)
	    {
	      p = plist->elements[i];
              if (i - j == 1)
		plist->elements[i] = plist->elements[j];
	      else
	        memmove(plist->elements + j + 1, plist->elements + j, (i - j) * sizeof(Id));
	      plist->elements[j] = p;
	    }
	  else if (j + 2 == plist->count)
	    break;	/* no need to check last element if all prev ones are installed */
	  j++;
	}
    }
}

/*
 * prune_to_best_version
 *
 * sort list of packages (given through plist) by name and evr
 * return result through plist
 */
static void
prune_to_best_version(Pool *pool, Queue *plist)
{
  int i, j;
  Solvable *s, *best;

  if (plist->count < 2)		/* no need to prune for a single entry */
    return;
  POOL_DEBUG(SOLV_DEBUG_POLICY, "prune_to_best_version %d\n", plist->count);

  /* sort by name first, prefer installed */
  solv_sort(plist->elements, plist->count, sizeof(Id), prune_to_best_version_sortcmp, pool);

  /* now find best 'per name' */
  best = 0;
  for (i = j = 0; i < plist->count; i++)
    {
      s = pool->solvables + plist->elements[i];

      POOL_DEBUG(SOLV_DEBUG_POLICY, "- %s[%s]\n",
		 pool_solvable2str(pool, s),
		 (pool->installed && s->repo == pool->installed) ? "installed" : "not installed");

      if (!best)		/* if no best yet, the current is best */
        {
          best = s;
          continue;
        }

      /* name switch: finish group, re-init */
      if (best->name != s->name)   /* new name */
        {
          plist->elements[j++] = best - pool->solvables; /* move old best to front */
          best = s;		/* take current as new best */
          continue;
        }

      if (best->evr != s->evr)	/* compare evr */
        {
          if (pool_evrcmp(pool, best->evr, s->evr, EVRCMP_COMPARE) < 0)
            best = s;
        }
    }
  plist->elements[j++] = best - pool->solvables;	/* finish last group */
  plist->count = j;

  /* we reduced the list to one package per name, now look at
   * package obsoletes */
  if (plist->count > 1)
    {
      if (plist->count == 2)
        prune_obsoleted_2(pool, plist);
      else
        prune_obsoleted(pool, plist);
    }
  if (plist->count > 1 && pool->installed)
    move_installed_to_front(pool, plist);
}


/* legacy, do not use anymore!
 * (rates arch higher than version, but thats a policy)
 */

static void
prune_best_arch_name_version(const Solver *solv, Pool *pool, Queue *plist)
{
  if (plist->count > 1)
    prune_to_best_arch(pool, plist);
  if (plist->count > 1)
    prune_to_best_version(pool, plist);
}

/* installed packages involed in a dup operation can only be kept
 * if they are identical to a non-installed one */
static void
prune_installed_dup_packages(Solver *solv, Queue *plist)
{
  Pool *pool = solv->pool;
  int i, j, k;

  for (i = j = 0; i < plist->count; i++)
    {
      Id p = plist->elements[i];
      Solvable *s = pool->solvables + p;
      if (s->repo == pool->installed && (solv->dupmap_all || (solv->dupinvolvedmap.size && MAPTST(&solv->dupinvolvedmap, p))))
	{
	  for (k = 0; k < plist->count; k++)
	    {
	      Solvable *s2 = pool->solvables + plist->elements[k];
	      if (s2->repo != pool->installed && solvable_identical(s, s2))
		break;
	    }
	  if (k == plist->count)
	    continue;	/* no identical package found, ignore installed package */
	}
      plist->elements[j++] = p;
    }
  if (j)
    plist->count = j;
}

/*
 *  POLICY_MODE_CHOOSE:     default, do all pruning steps
 *  POLICY_MODE_RECOMMEND:  leave out prune_to_recommended
 *  POLICY_MODE_SUGGEST:    leave out prune_to_recommended, do prio pruning just per name
 */
void
policy_filter_unwanted(Solver *solv, Queue *plist, int mode)
{
  Pool *pool = solv->pool;
  if (plist->count > 1)
    {
      if (mode != POLICY_MODE_SUGGEST)
        prune_to_highest_prio(pool, plist);
      else
        prune_to_highest_prio_per_name(pool, plist);
      /* installed dup packages need special treatment as prio pruning
       * does not prune installed packages */
      if (plist->count > 1 && pool->installed && (solv->dupmap_all || solv->dupinvolvedmap.size))
	prune_installed_dup_packages(solv, plist);
    }
  if (plist->count > 1 && mode == POLICY_MODE_CHOOSE)
    prune_to_recommended(solv, plist);
  prune_best_arch_name_version(solv, pool, plist);
}


/* check if there is an illegal architecture change if
 * installed solvable s1 is replaced by s2 */
int
policy_illegal_archchange(Solver *solv, Solvable *s1, Solvable *s2)
{
  Pool *pool = solv->pool;
  Id a1 = s1->arch, a2 = s2->arch;

  /* we allow changes to/from noarch */
  if (a1 == a2 || a1 == pool->noarchid || a2 == pool->noarchid)
    return 0;
  if (!pool->id2arch)
    return 0;
  a1 = a1 <= pool->lastarch ? pool->id2arch[a1] : 0;
  a2 = a2 <= pool->lastarch ? pool->id2arch[a2] : 0;
  if (((a1 ^ a2) & 0xffff0000) != 0)
    return 1;
  return 0;
}

/* check if there is an illegal vendor change if
 * installed solvable s1 is replaced by s2 */
int
policy_illegal_vendorchange(Solver *solv, Solvable *s1, Solvable *s2)
{
  Pool *pool = solv->pool;
  Id v1, v2;
  Id vendormask1, vendormask2;

  if (pool->custom_vendorcheck)
     return pool->custom_vendorcheck(pool, s1, s2);

  /* treat a missing vendor as empty string */
  v1 = s1->vendor ? s1->vendor : ID_EMPTY;
  v2 = s2->vendor ? s2->vendor : ID_EMPTY;
  if (v1 == v2)
    return 0;
  vendormask1 = pool_vendor2mask(pool, v1);
  if (!vendormask1)
    return 1;	/* can't match */
  vendormask2 = pool_vendor2mask(pool, v2);
  if ((vendormask1 & vendormask2) != 0)
    return 0;
  return 1;	/* no class matches */
}

/* check if it is illegal to replace installed
 * package "is" with package "s" (which must obsolete "is")
 */
int
policy_is_illegal(Solver *solv, Solvable *is, Solvable *s, int ignore)
{
  Pool *pool = solv->pool;
  int ret = 0;
  int duppkg = solv->dupmap_all ? 1 : 0;
  if (!(ignore & POLICY_ILLEGAL_DOWNGRADE) && !(duppkg ? solv->dup_allowdowngrade : solv->allowdowngrade))
    {
      if (is->name == s->name && pool_evrcmp(pool, is->evr, s->evr, EVRCMP_COMPARE) > 0)
	ret |= POLICY_ILLEGAL_DOWNGRADE;
    }
  if (!(ignore & POLICY_ILLEGAL_ARCHCHANGE) && !(duppkg ? solv->dup_allowarchchange : solv->allowarchchange))
    {
      if (is->arch != s->arch && policy_illegal_archchange(solv, is, s))
	ret |= POLICY_ILLEGAL_ARCHCHANGE;
    }
  if (!(ignore & POLICY_ILLEGAL_VENDORCHANGE) && !(duppkg ? solv->dup_allowvendorchange : solv->allowvendorchange))
    {
      if (is->vendor != s->vendor && policy_illegal_vendorchange(solv, is, s))
	ret |= POLICY_ILLEGAL_VENDORCHANGE;
    }
  if (!(ignore & POLICY_ILLEGAL_NAMECHANGE) && !(duppkg ? solv->dup_allownamechange : solv->allownamechange))
    {
      if (is->name != s->name)
	ret |= POLICY_ILLEGAL_NAMECHANGE;
    }
  return ret;
}

/*-------------------------------------------------------------------
 * 
 * create reverse obsoletes map for installed solvables
 *
 * For each installed solvable find which packages with *different* names
 * obsolete the solvable.
 * This index is used in policy_findupdatepackages() below.
 */
void
policy_create_obsolete_index(Solver *solv)
{
  Pool *pool = solv->pool;
  Solvable *s;
  Repo *installed = solv->installed;
  Id p, pp, obs, *obsp, *obsoletes, *obsoletes_data;
  int i, n, cnt;

  solv->obsoletes = solv_free(solv->obsoletes);
  solv->obsoletes_data = solv_free(solv->obsoletes_data);
  if (!installed || installed->start == installed->end)
    return;
  cnt = installed->end - installed->start;
  solv->obsoletes = obsoletes = solv_calloc(cnt, sizeof(Id));
  for (i = 1; i < pool->nsolvables; i++)
    {
      s = pool->solvables + i;
      if (!s->obsoletes)
	continue;
      if (!pool_installable(pool, s))
	continue;
      obsp = s->repo->idarraydata + s->obsoletes;
      while ((obs = *obsp++) != 0)
	{
	  FOR_PROVIDES(p, pp, obs)
	    {
	      Solvable *ps = pool->solvables + p;;
	      if (ps->repo != installed)
		continue;
	      if (ps->name == s->name)
		continue;
	      if (!pool->obsoleteusesprovides && !pool_match_nevr(pool, ps, obs))
		continue;
	      if (pool->obsoleteusescolors && !pool_colormatch(pool, s, ps))
		continue;
	      obsoletes[p - installed->start]++;
	    }
	}
    }
  n = 0;
  for (i = 0; i < cnt; i++)
    if (obsoletes[i])
      {
        n += obsoletes[i] + 1;
        obsoletes[i] = n;
      }
  solv->obsoletes_data = obsoletes_data = solv_calloc(n + 1, sizeof(Id));
  POOL_DEBUG(SOLV_DEBUG_STATS, "obsoletes data: %d entries\n", n + 1);
  for (i = pool->nsolvables - 1; i > 0; i--)
    {
      s = pool->solvables + i;
      if (!s->obsoletes)
	continue;
      if (!pool_installable(pool, s))
	continue;
      obsp = s->repo->idarraydata + s->obsoletes;
      while ((obs = *obsp++) != 0)
	{
	  FOR_PROVIDES(p, pp, obs)
	    {
	      Solvable *ps = pool->solvables + p;;
	      if (ps->repo != installed)
		continue;
	      if (ps->name == s->name)
		continue;
	      if (!pool->obsoleteusesprovides && !pool_match_nevr(pool, ps, obs))
		continue;
	      if (pool->obsoleteusescolors && !pool_colormatch(pool, s, ps))
		continue;
	      if (obsoletes_data[obsoletes[p - installed->start]] != i)
		obsoletes_data[--obsoletes[p - installed->start]] = i;
	    }
	}
    }
}


/*
 * find update candidates
 * 
 * s: installed solvable to be updated
 * qs: [out] queue to hold Ids of candidates
 * allow_all: 0 = dont allow downgrades, 1 = allow all candidates
 *            2 = dup mode
 * 
 */
void
policy_findupdatepackages(Solver *solv, Solvable *s, Queue *qs, int allow_all)
{
  /* installed packages get a special upgrade allowed rule */
  Pool *pool = solv->pool;
  Id p, pp, n, p2, pp2;
  Id obs, *obsp;
  Solvable *ps;
  int haveprovobs = 0;
  int allowdowngrade = allow_all ? 1 : solv->allowdowngrade;
  int allownamechange = allow_all ? 1 : solv->allownamechange;
  int allowarchchange = allow_all ? 1 : solv->allowarchchange;
  int allowvendorchange = allow_all ? 1 : solv->allowvendorchange;
  if (allow_all == 2)
    {
      allowdowngrade = solv->dup_allowdowngrade;
      allownamechange = solv->dup_allownamechange;
      allowarchchange = solv->dup_allowarchchange;
      allowvendorchange = solv->dup_allowvendorchange;
    }

  queue_empty(qs);

  n = s - pool->solvables;

  /*
   * look for updates for s
   */
  FOR_PROVIDES(p, pp, s->name)	/* every provider of s' name */
    {
      if (p == n)		/* skip itself */
	continue;

      ps = pool->solvables + p;
      if (s->name == ps->name)	/* name match */
	{
	  if (!allowdowngrade && pool_evrcmp(pool, s->evr, ps->evr, EVRCMP_COMPARE) > 0)
	    continue;
	}
      else if (!allownamechange)
	continue;
      else if (!solv->noupdateprovide && ps->obsoletes)   /* provides/obsoletes combination ? */
	{
	  obsp = ps->repo->idarraydata + ps->obsoletes;
	  while ((obs = *obsp++) != 0)	/* for all obsoletes */
	    {
	      FOR_PROVIDES(p2, pp2, obs)   /* and all matching providers of the obsoletes */
		{
		  Solvable *ps2 = pool->solvables + p2;
		  if (!pool->obsoleteusesprovides && !pool_match_nevr(pool, ps2, obs))
		    continue;
		  if (pool->obsoleteusescolors && !pool_colormatch(pool, s, ps2))
		    continue;
		  if (p2 == n)		/* match ! */
		    break;
		}
	      if (p2)			/* match! */
		break;
	    }
	  if (!obs)			/* continue if no match */
	    continue;
	  /* here we have 'p' with a matching provides/obsoletes combination
	   * thus flagging p as a valid update candidate for s
	   */
	  haveprovobs = 1;
	}
      else
        continue;
      if (!allowarchchange && s->arch != ps->arch && policy_illegal_archchange(solv, s, ps))
	continue;
      if (!allowvendorchange && s->vendor != ps->vendor && policy_illegal_vendorchange(solv, s, ps))
	continue;
      queue_push(qs, p);
    }
  if (!allownamechange)
    return;
  /* if we have found some valid candidates and noupdateprovide is not set, we're
     done. otherwise we fallback to all obsoletes */
  if (!solv->noupdateprovide && haveprovobs)
    return;
  if (solv->obsoletes && solv->obsoletes[n - solv->installed->start])
    {
      Id *opp;
      for (opp = solv->obsoletes_data + solv->obsoletes[n - solv->installed->start]; (p = *opp++) != 0;)
	{
	  ps = pool->solvables + p;
	  if (!allowarchchange && s->arch != ps->arch && policy_illegal_archchange(solv, s, ps))
	    continue;
	  if (!allowvendorchange && s->vendor != ps->vendor && policy_illegal_vendorchange(solv, s, ps))
	    continue;
	  queue_push(qs, p);
	}
    }
}

