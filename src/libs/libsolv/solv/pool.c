/*
 * Copyright (c) 2007-2009, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/*
 * pool.c
 * 
 * The pool contains information about solvables
 * stored optimized for memory consumption and fast retrieval.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

#include "pool.h"
#include "repo.h"
#include "poolid.h"
#include "poolid_private.h"
#include "poolarch.h"
#include "util.h"
#include "bitmap.h"
#include "evr.h"

#define SOLVABLE_BLOCK	255

#define KNOWNID_INITIALIZE
#include "knownid.h"
#undef KNOWNID_INITIALIZE

/* create pool */
Pool *
pool_create(void)
{
  Pool *pool;
  Solvable *s;

  pool = (Pool *)solv_calloc(1, sizeof(*pool));

  stringpool_init (&pool->ss, initpool_data);

  /* alloc space for RelDep 0 */
  pool->rels = solv_extend_resize(0, 1, sizeof(Reldep), REL_BLOCK);
  pool->nrels = 1;
  memset(pool->rels, 0, sizeof(Reldep));

  /* alloc space for Solvable 0 and system solvable */
  pool->solvables = solv_extend_resize(0, 2, sizeof(Solvable), SOLVABLE_BLOCK);
  pool->nsolvables = 2;
  memset(pool->solvables, 0, 2 * sizeof(Solvable));

  queue_init(&pool->vendormap);

#if defined(DEBIAN)
  pool->disttype = DISTTYPE_DEB;
  pool->noarchid = ARCH_ALL;
#elif defined(ARCHLINUX)
  pool->disttype = DISTTYPE_ARCH;
  pool->noarchid = ARCH_ANY;
#elif defined(HAIKU)
  pool->disttype = DISTTYPE_HAIKU;
  pool->noarchid = ARCH_ANY;
  pool->obsoleteusesprovides = 1;
#else
  pool->disttype = DISTTYPE_RPM;
  pool->noarchid = ARCH_NOARCH;
#endif

  /* initialize the system solvable */
  s = pool->solvables + SYSTEMSOLVABLE;
  s->name = SYSTEM_SYSTEM;
  s->arch = pool->noarchid;
  s->evr = ID_EMPTY;

  pool->debugmask = SOLV_DEBUG_RESULT;	/* FIXME */
#ifdef FEDORA
  pool->obsoleteusescolors = 1;
#endif
#ifdef RPM5
  pool->forbidselfconflicts = 1;
  pool->obsoleteusesprovides = 1;
  pool->implicitobsoleteusesprovides = 1;
  pool->havedistepoch = 1;
#endif
  return pool;
}


/* free all the resources of our pool */
void
pool_free(Pool *pool)
{
  int i;

  pool_freewhatprovides(pool);
  pool_freeidhashes(pool);
  pool_freeallrepos(pool, 1);
  solv_free(pool->id2arch);
  solv_free(pool->id2color);
  solv_free(pool->solvables);
  stringpool_free(&pool->ss);
  solv_free(pool->rels);
  pool_setvendorclasses(pool, 0);
  queue_free(&pool->vendormap);
  queue_free(&pool->pooljobs);
  for (i = 0; i < POOL_TMPSPACEBUF; i++)
    solv_free(pool->tmpspace.buf[i]);
  for (i = 0; i < pool->nlanguages; i++)
    free((char *)pool->languages[i]);
  solv_free(pool->languages);
  solv_free(pool->languagecache);
  solv_free(pool->errstr);
  solv_free(pool->rootdir);
  solv_free(pool);
}

void
pool_freeallrepos(Pool *pool, int reuseids)
{
  int i;

  pool_freewhatprovides(pool);
  for (i = 1; i < pool->nrepos; i++) 
    if (pool->repos[i])
      repo_freedata(pool->repos[i]);
  pool->repos = solv_free(pool->repos);
  pool->nrepos = 0; 
  pool->urepos = 0; 
  /* the first two solvables don't belong to a repo */
  pool_free_solvable_block(pool, 2, pool->nsolvables - 2, reuseids);
}

#ifdef MULTI_SEMANTICS
void
pool_setdisttype(Pool *pool, int disttype)
{
  pool->disttype = disttype;
  if (disttype == DISTTYPE_RPM)
    pool->noarchid = ARCH_NOARCH;
  if (disttype == DISTTYPE_DEB)
    pool->noarchid = ARCH_ALL;
  if (disttype == DISTTYPE_ARCH)
    pool->noarchid = ARCH_ANY;
  if (disttype == DISTTYPE_HAIKU)
    pool->noarchid = ARCH_ANY;
  pool->solvables[SYSTEMSOLVABLE].arch = pool->noarchid;
}
#endif

int
pool_get_flag(Pool *pool, int flag)
{
  switch (flag)
    {
    case POOL_FLAG_PROMOTEEPOCH:
      return pool->promoteepoch;
    case POOL_FLAG_FORBIDSELFCONFLICTS:
      return pool->forbidselfconflicts;
    case POOL_FLAG_OBSOLETEUSESPROVIDES:
      return pool->obsoleteusesprovides;
    case POOL_FLAG_IMPLICITOBSOLETEUSESPROVIDES:
      return pool->implicitobsoleteusesprovides;
    case POOL_FLAG_OBSOLETEUSESCOLORS:
      return pool->obsoleteusescolors;
    case POOL_FLAG_NOINSTALLEDOBSOLETES:
      return pool->noinstalledobsoletes;
    case POOL_FLAG_HAVEDISTEPOCH:
      return pool->havedistepoch;
    default:
      break;
    }
  return -1;
}

int
pool_set_flag(Pool *pool, int flag, int value)
{
  int old = pool_get_flag(pool, flag);
  switch (flag)
    {
    case POOL_FLAG_PROMOTEEPOCH:
      pool->promoteepoch = value;
      break;
    case POOL_FLAG_FORBIDSELFCONFLICTS:
      pool->forbidselfconflicts = value;
      break;
    case POOL_FLAG_OBSOLETEUSESPROVIDES:
      pool->obsoleteusesprovides = value;
      break;
    case POOL_FLAG_IMPLICITOBSOLETEUSESPROVIDES:
      pool->implicitobsoleteusesprovides = value;
      break;
    case POOL_FLAG_OBSOLETEUSESCOLORS:
      pool->obsoleteusescolors = value;
      break;
    case POOL_FLAG_NOINSTALLEDOBSOLETES:
      pool->noinstalledobsoletes = value;
      break;
    case POOL_FLAG_HAVEDISTEPOCH:
      pool->havedistepoch = value;
      break;
    default:
      break;
    }
  return old;
}


Id
pool_add_solvable(Pool *pool)
{
  pool->solvables = solv_extend(pool->solvables, pool->nsolvables, 1, sizeof(Solvable), SOLVABLE_BLOCK);
  memset(pool->solvables + pool->nsolvables, 0, sizeof(Solvable));
  return pool->nsolvables++;
}

Id
pool_add_solvable_block(Pool *pool, int count)
{
  Id nsolvables = pool->nsolvables;
  if (!count)
    return nsolvables;
  pool->solvables = solv_extend(pool->solvables, pool->nsolvables, count, sizeof(Solvable), SOLVABLE_BLOCK);
  memset(pool->solvables + nsolvables, 0, sizeof(Solvable) * count);
  pool->nsolvables += count;
  return nsolvables;
}

void
pool_free_solvable_block(Pool *pool, Id start, int count, int reuseids)
{
  if (!count)
    return;
  if (reuseids && start + count == pool->nsolvables)
    {
      /* might want to shrink solvable array */
      pool->nsolvables = start;
      return;
    }
  memset(pool->solvables + start, 0, sizeof(Solvable) * count);
}


void
pool_set_installed(Pool *pool, Repo *installed)
{
  if (pool->installed == installed)
    return;
  pool->installed = installed;
  pool_freewhatprovides(pool);
}

static int
pool_shrink_whatprovides_sortcmp(const void *ap, const void *bp, void *dp)
{
  int r;
  Pool *pool = dp;
  Id oa, ob, *da, *db;
  oa = pool->whatprovides[*(Id *)ap];
  ob = pool->whatprovides[*(Id *)bp];
  if (oa == ob)
    return *(Id *)ap - *(Id *)bp;
  da = pool->whatprovidesdata + oa;
  db = pool->whatprovidesdata + ob;
  while (*db)
    if ((r = (*da++ - *db++)) != 0)
      return r;
  if (*da)
    return *da;
  return *(Id *)ap - *(Id *)bp;
}

/*
 * pool_shrink_whatprovides  - unify whatprovides data
 *
 * whatprovides_rel must be empty for this to work!
 *
 */
static void
pool_shrink_whatprovides(Pool *pool)
{
  Id i, n, id;
  Id *sorted;
  Id lastid, *last, *dp, *lp;
  Offset o;
  int r;

  if (pool->ss.nstrings < 3)
    return;
  sorted = solv_malloc2(pool->ss.nstrings, sizeof(Id));
  for (i = id = 0; id < pool->ss.nstrings; id++)
    if (pool->whatprovides[id] >= 4)
      sorted[i++] = id;
  n = i;
  solv_sort(sorted, n, sizeof(Id), pool_shrink_whatprovides_sortcmp, pool);
  last = 0;
  lastid = 0;
  for (i = 0; i < n; i++)
    {
      id = sorted[i];
      o = pool->whatprovides[id];
      dp = pool->whatprovidesdata + o;
      if (last)
	{
	  lp = last;
	  while (*dp)	
	    if (*dp++ != *lp++)
	      {
		last = 0;
	        break;
	      }
	  if (last && *lp)
	    last = 0;
	  if (last)
	    {
	      pool->whatprovides[id] = -lastid;
	      continue;
	    }
	}
      last = pool->whatprovidesdata + o;
      lastid = id;
    }
  solv_free(sorted);
  dp = pool->whatprovidesdata + 4;
  for (id = 1; id < pool->ss.nstrings; id++)
    {
      o = pool->whatprovides[id];
      if (!o)
	continue;
      if ((Id)o < 0)
	{
	  i = -(Id)o;
	  if (i >= id)
	    abort();
	  pool->whatprovides[id] = pool->whatprovides[i];
	  continue;
	}
      if (o < 4)
	continue;
      lp = pool->whatprovidesdata + o;
      if (lp < dp)
	abort();
      pool->whatprovides[id] = dp - pool->whatprovidesdata;
      while ((*dp++ = *lp++) != 0)
	;
    }
  o = dp - pool->whatprovidesdata;
  POOL_DEBUG(SOLV_DEBUG_STATS, "shrunk whatprovidesdata from %d to %d\n", pool->whatprovidesdataoff, o);
  if (pool->whatprovidesdataoff == o)
    return;
  r = pool->whatprovidesdataoff - o;
  pool->whatprovidesdataoff = o;
  pool->whatprovidesdata = solv_realloc(pool->whatprovidesdata, (o + pool->whatprovidesdataleft) * sizeof(Id));
  if (r > pool->whatprovidesdataleft)
    r = pool->whatprovidesdataleft;
  memset(pool->whatprovidesdata + o, 0, r * sizeof(Id));
}


/*
 * pool_createwhatprovides()
 * 
 * create hashes over pool of solvables to ease provide lookups
 * 
 */
void
pool_createwhatprovides(Pool *pool)
{
  int i, num, np, extra;
  Offset off;
  Solvable *s;
  Id id;
  Offset *idp, n;
  Offset *whatprovides;
  Id *whatprovidesdata, *d;
  Repo *installed = pool->installed;
  unsigned int now;

  now = solv_timems(0);
  POOL_DEBUG(SOLV_DEBUG_STATS, "number of solvables: %d, memory used: %d K\n", pool->nsolvables, pool->nsolvables * (int)sizeof(Solvable) / 1024);
  POOL_DEBUG(SOLV_DEBUG_STATS, "number of ids: %d + %d\n", pool->ss.nstrings, pool->nrels);
  POOL_DEBUG(SOLV_DEBUG_STATS, "string memory used: %d K array + %d K data,  rel memory used: %d K array\n", pool->ss.nstrings / (1024 / (int)sizeof(Id)), pool->ss.sstrings / 1024, pool->nrels * (int)sizeof(Reldep) / 1024);
  if (pool->ss.stringhashmask || pool->relhashmask)
    POOL_DEBUG(SOLV_DEBUG_STATS, "string hash memory: %d K, rel hash memory : %d K\n", (pool->ss.stringhashmask + 1) / (int)(1024/sizeof(Id)), (pool->relhashmask + 1) / (int)(1024/sizeof(Id)));

  pool_freeidhashes(pool);	/* XXX: should not be here! */
  pool_freewhatprovides(pool);
  num = pool->ss.nstrings;
  pool->whatprovides = whatprovides = solv_calloc_block(num, sizeof(Offset), WHATPROVIDES_BLOCK);
  pool->whatprovides_rel = solv_calloc_block(pool->nrels, sizeof(Offset), WHATPROVIDES_BLOCK);

  /* count providers for each name */
  for (i = pool->nsolvables - 1; i > 0; i--)
    {
      Id *pp;
      s = pool->solvables + i;
      if (!s->provides || !s->repo || s->repo->disabled)
	continue;
      /* we always need the installed solvable in the whatprovides data,
         otherwise obsoletes/conflicts on them won't work */
      if (s->repo != installed && !pool_installable(pool, s))
	continue;
      pp = s->repo->idarraydata + s->provides;
      while ((id = *pp++) != 0)
	{
	  while (ISRELDEP(id))
	    {
	      Reldep *rd = GETRELDEP(pool, id);
	      id = rd->name;
	    }
	  whatprovides[id]++;	       /* inc count of providers */
	}
    }

  off = 4;	/* first entry is undef, second is empty list, third is system solvable  */
  np = 0;			       /* number of names provided */
  for (i = 0, idp = whatprovides; i < num; i++, idp++)
    {
      n = *idp;
      if (!n)			       /* no providers */
	continue;
      off += n;			       /* make space for all providers */
      *idp = off++;		       /* now idp points to terminating zero */
      np++;			       /* inc # of provider 'slots' for stats */
    }

  POOL_DEBUG(SOLV_DEBUG_STATS, "provide ids: %d\n", np);

  /* reserve some space for relation data */
  extra = 2 * pool->nrels;
  if (extra < 256)
    extra = 256;

  POOL_DEBUG(SOLV_DEBUG_STATS, "provide space needed: %d + %d\n", off, extra);

  /* alloc space for all providers + extra */
  whatprovidesdata = solv_calloc(off + extra, sizeof(Id));
  whatprovidesdata[2] = SYSTEMSOLVABLE;

  /* now fill data for all provides */
  for (i = pool->nsolvables - 1; i > 0; i--)
    {
      Id *pp;
      s = pool->solvables + i;
      if (!s->provides || !s->repo || s->repo->disabled)
	continue;
      if (s->repo != installed && !pool_installable(pool, s))
	continue;

      /* for all provides of this solvable */
      pp = s->repo->idarraydata + s->provides;
      while ((id = *pp++) != 0)
	{
	  while (ISRELDEP(id))
	    {
	      Reldep *rd = GETRELDEP(pool, id);
	      id = rd->name;
	    }
	  d = whatprovidesdata + whatprovides[id];   /* offset into whatprovidesdata */
	  if (*d != i)		/* don't add same solvable twice */
	    {
	      d[-1] = i;
	      whatprovides[id]--;
	    }
	}
    }
  pool->whatprovidesdata = whatprovidesdata;
  pool->whatprovidesdataoff = off;
  pool->whatprovidesdataleft = extra;
  pool_shrink_whatprovides(pool);
  POOL_DEBUG(SOLV_DEBUG_STATS, "whatprovides memory used: %d K id array, %d K data\n", (pool->ss.nstrings + pool->nrels + WHATPROVIDES_BLOCK) / (int)(1024/sizeof(Id)), (pool->whatprovidesdataoff + pool->whatprovidesdataleft) / (int)(1024/sizeof(Id)));
  POOL_DEBUG(SOLV_DEBUG_STATS, "createwhatprovides took %d ms\n", solv_timems(now));
}

/*
 * free all of our whatprovides data
 * be careful, everything internalized with pool_queuetowhatprovides is
 * gone, too
 */
void
pool_freewhatprovides(Pool *pool)
{
  pool->whatprovides = solv_free(pool->whatprovides);
  pool->whatprovides_rel = solv_free(pool->whatprovides_rel);
  pool->whatprovidesdata = solv_free(pool->whatprovidesdata);
  pool->whatprovidesdataoff = 0;
  pool->whatprovidesdataleft = 0;
}


/******************************************************************************/

/*
 * pool_queuetowhatprovides  - add queue contents to whatprovidesdata
 * 
 * used for whatprovides, jobs, learnt rules, selections
 * input: q: queue of Ids
 * returns: Offset into whatprovidesdata
 *
 */
Id
pool_queuetowhatprovides(Pool *pool, Queue *q)
{
  Offset off;
  int count = q->count;

  if (count == 0)		       /* queue empty -> 1 */
    return 1;
  if (count == 1 && q->elements[0] == SYSTEMSOLVABLE)
    return 2;

  /* extend whatprovidesdata if needed, +1 for 0-termination */
  if (pool->whatprovidesdataleft < count + 1)
    {
      POOL_DEBUG(SOLV_DEBUG_STATS, "growing provides hash data...\n");
      pool->whatprovidesdata = solv_realloc(pool->whatprovidesdata, (pool->whatprovidesdataoff + count + 4096) * sizeof(Id));
      pool->whatprovidesdataleft = count + 4096;
    }

  /* copy queue to next free slot */
  off = pool->whatprovidesdataoff;
  memcpy(pool->whatprovidesdata + pool->whatprovidesdataoff, q->elements, count * sizeof(Id));

  /* adapt count and 0-terminate */
  pool->whatprovidesdataoff += count;
  pool->whatprovidesdata[pool->whatprovidesdataoff++] = 0;
  pool->whatprovidesdataleft -= count + 1;

  return (Id)off;
}


/*************************************************************************/

#if defined(MULTI_SEMANTICS)
# define EVRCMP_DEPCMP (pool->disttype == DISTTYPE_DEB ? EVRCMP_COMPARE : EVRCMP_MATCH_RELEASE)
#elif defined(DEBIAN)
# define EVRCMP_DEPCMP EVRCMP_COMPARE
#else
# define EVRCMP_DEPCMP EVRCMP_MATCH_RELEASE
#endif

/* check if a package's nevr matches a dependency */

int
pool_match_nevr_rel(Pool *pool, Solvable *s, Id d)
{
  Reldep *rd = GETRELDEP(pool, d);
  Id name = rd->name;
  Id evr = rd->evr;
  int flags = rd->flags;

  if (flags > 7)
    {
      switch (flags)
	{
	case REL_ARCH:
	  if (s->arch != evr)
	    {
	      if (evr != ARCH_SRC || s->arch != ARCH_NOSRC)
	        return 0;
	    }
	  return pool_match_nevr(pool, s, name);
	case REL_OR:
	  if (pool_match_nevr(pool, s, name))
	    return 1;
	  return pool_match_nevr(pool, s, evr);
	case REL_AND:
	case REL_WITH:
	  if (!pool_match_nevr(pool, s, name))
	    return 0;
	  return pool_match_nevr(pool, s, evr);
	default:
	  return 0;
	}
    }
  if (!pool_match_nevr(pool, s, name))
    return 0;
  if (evr == s->evr)
    return (flags & REL_EQ) ? 1 : 0;
  if (!flags)
    return 0;
  if (flags == 7)
    return 1;
  switch (pool_evrcmp(pool, s->evr, evr, EVRCMP_DEPCMP))
    {
    case -2:
      return 1;
    case -1:
      return (flags & REL_LT) ? 1 : 0;
    case 0:
      return (flags & REL_EQ) ? 1 : 0;
    case 1:
      return (flags & REL_GT) ? 1 : 0;
    case 2:
      return (flags & REL_EQ) ? 1 : 0;
    default:
      break;
    }
  return 0;
}

#if defined(HAIKU) || defined(MULTI_SEMANTICS)
/* forward declaration */
static int pool_match_flags_evr_rel_compat(Pool *pool, Reldep *range, int flags, int evr);
#endif

/* match (flags, evr) against provider (pflags, pevr) */
static inline int
pool_match_flags_evr(Pool *pool, int pflags, Id pevr, int flags, int evr)
{
  if (!pflags || !flags || pflags >= 8 || flags >= 8)
    return 0;
  if (flags == 7 || pflags == 7)
    return 1;		/* rel provides every version */
  if ((pflags & flags & (REL_LT | REL_GT)) != 0)
    return 1;		/* both rels show in the same direction */
  if (pevr == evr)
    return (flags & pflags & REL_EQ) ? 1 : 0;
#if defined(HAIKU) || defined(MULTI_SEMANTICS)
  if (ISRELDEP(pevr))
    {
      Reldep *rd = GETRELDEP(pool, pevr);
      if (rd->flags == REL_COMPAT)
	return pool_match_flags_evr_rel_compat(pool, rd, flags, evr);
    }
#endif
  switch (pool_evrcmp(pool, pevr, evr, EVRCMP_DEPCMP))
    {
    case -2:
      return (pflags & REL_EQ) ? 1 : 0;
    case -1:
      return (flags & REL_LT) || (pflags & REL_GT) ? 1 : 0;
    case 0:
      return (flags & pflags & REL_EQ) ? 1 : 0;
    case 1:
      return (flags & REL_GT) || (pflags & REL_LT) ? 1 : 0;
    case 2:
      return (flags & REL_EQ) ? 1 : 0;
    default:
      break;
    }
  return 0;
}

#if defined(HAIKU) || defined(MULTI_SEMANTICS)
static int
pool_match_flags_evr_rel_compat(Pool *pool, Reldep *range, int flags, int evr)
{
  /* range->name is the actual version, range->evr the backwards compatibility
     version. If flags are '>=' or '>', we match the compatibility version
     as well, otherwise only the actual version. */
  if (!(flags & REL_GT) || (flags & REL_LT))
    return pool_match_flags_evr(pool, REL_EQ, range->name, flags, evr);
  return pool_match_flags_evr(pool, REL_LT | REL_EQ, range->name, flags, evr) &&
         pool_match_flags_evr(pool, REL_GT | REL_EQ, range->evr, REL_EQ, evr);
}
#endif

/* match two dependencies (d1 = provider) */

int
pool_match_dep(Pool *pool, Id d1, Id d2)
{
  Reldep *rd1, *rd2;

  if (d1 == d2)
    return 1;
  if (!ISRELDEP(d1))
    {
      if (!ISRELDEP(d2))
	return 0;
      rd2 = GETRELDEP(pool, d2);
      return pool_match_dep(pool, d1, rd2->name);
    }
  rd1 = GETRELDEP(pool, d1);
  if (!ISRELDEP(d2))
    {
      return pool_match_dep(pool, rd1->name, d2);
    }
  rd2 = GETRELDEP(pool, d2);
  /* first match name */
  if (!pool_match_dep(pool, rd1->name, rd2->name))
    return 0;
  /* name matches, check flags and evr */
  return pool_match_flags_evr(pool, rd1->flags, rd1->evr, rd2->flags, rd2->evr);
}

/*
 * addrelproviders
 * 
 * add packages fulfilling the relation to whatprovides array
 * no exact providers, do range match
 * 
 */

Id
pool_addrelproviders(Pool *pool, Id d)
{
  Reldep *rd = GETRELDEP(pool, d);
  Reldep *prd;
  Queue plist;
  Id buf[16];
  Id name = rd->name;
  Id evr = rd->evr;
  int flags = rd->flags;
  Id pid, *pidp;
  Id p, *pp;

  d = GETRELID(d);
  queue_init_buffer(&plist, buf, sizeof(buf)/sizeof(*buf));

  if (flags >= 8)
    {
      /* special relation */
      Id wp = 0;
      Id *pp2, *pp3;

      switch (flags)
	{
	case REL_AND:
	case REL_WITH:
	  wp = pool_whatprovides(pool, name);
	  pp2 = pool_whatprovides_ptr(pool, evr);
	  pp = pool->whatprovidesdata + wp;
	  while ((p = *pp++) != 0)
	    {
	      for (pp3 = pp2; *pp3; pp3++)
		if (*pp3 == p)
		  break;
	      if (*pp3)
		queue_push(&plist, p);	/* found it */
	      else
		wp = 0;
	    }
	  break;
	case REL_OR:
	  wp = pool_whatprovides(pool, name);
	  pp = pool->whatprovidesdata + wp;
	  if (!*pp)
	    wp = pool_whatprovides(pool, evr);
	  else
	    {
	      int cnt;
	      while ((p = *pp++) != 0)
		queue_push(&plist, p);
	      cnt = plist.count;
	      pp = pool_whatprovides_ptr(pool, evr);
	      while ((p = *pp++) != 0)
		queue_pushunique(&plist, p);
	      if (plist.count != cnt)
		wp = 0;
	    }
	  break;
	case REL_NAMESPACE:
	  if (name == NAMESPACE_OTHERPROVIDERS)
	    {
	      wp = pool_whatprovides(pool, evr);
	      break;
	    }
	  if (pool->nscallback)
	    {
	      /* ask callback which packages provide the dependency
	       * 0:  none
	       * 1:  the system (aka SYSTEMSOLVABLE)
	       * >1: set of packages, stored as offset on whatprovidesdata
	       */
	      p = pool->nscallback(pool, pool->nscallbackdata, name, evr);
	      if (p > 1)
		wp = p;
	      if (p == 1)
		queue_push(&plist, SYSTEMSOLVABLE);
	    }
	  break;
	case REL_ARCH:
	  /* small hack: make it possible to match <pkg>.src
	   * we have to iterate over the solvables as src packages do not
	   * provide anything, thus they are not indexed in our
	   * whatprovides hash */
	  if (evr == ARCH_SRC || evr == ARCH_NOSRC)
	    {
	      Solvable *s;
	      for (p = 1, s = pool->solvables + p; p < pool->nsolvables; p++, s++)
		{
		  if (!s->repo)
		    continue;
		  if (s->arch != evr && s->arch != ARCH_NOSRC)
		    continue;
		  if (pool_disabled_solvable(pool, s))
		    continue;
		  if (pool_match_nevr(pool, s, name))
		    queue_push(&plist, p);
		}
	      break;
	    }
	  wp = pool_whatprovides(pool, name);
	  pp = pool->whatprovidesdata + wp;
	  while ((p = *pp++) != 0)
	    {
	      Solvable *s = pool->solvables + p;
	      if (s->arch == evr)
		queue_push(&plist, p);
	      else
		wp = 0;
	    }
	  break;
	case REL_FILECONFLICT:
	  pp = pool_whatprovides_ptr(pool, name);
	  while ((p = *pp++) != 0)
	    {
	      Id origd = MAKERELDEP(d);
	      Solvable *s = pool->solvables + p;
	      if (!s->provides)
		continue;
	      pidp = s->repo->idarraydata + s->provides;
	      while ((pid = *pidp++) != 0)
		if (pid == origd)
		  break;
	      if (pid)
		queue_push(&plist, p);
	    }
	  break;
	default:
	  break;
	}
      if (wp)
	{
	  /* we can reuse an existing entry */
	  queue_free(&plist);
	  pool->whatprovides_rel[d] = wp;
	  return wp;
	}
    }
  else if (flags)
    {
      /* simple version comparison relation */
#if 0
      POOL_DEBUG(SOLV_DEBUG_STATS, "addrelproviders: what provides %s?\n", pool_dep2str(pool, name));
#endif
      pp = pool_whatprovides_ptr(pool, name);
      while (ISRELDEP(name))
	{
          rd = GETRELDEP(pool, name);
	  name = rd->name;
	}
      while ((p = *pp++) != 0)
	{
	  Solvable *s = pool->solvables + p;
	  if (!s->provides)
	    {
	      /* no provides - check nevr */
	      if (pool_match_nevr_rel(pool, s, MAKERELDEP(d)))
	        queue_push(&plist, p);
	      continue;
	    }
	  /* solvable p provides name in some rels */
	  pidp = s->repo->idarraydata + s->provides;
	  while ((pid = *pidp++) != 0)
	    {
	      if (!ISRELDEP(pid))
		{
		  if (pid != name)
		    continue;		/* wrong provides name */
		  if (pool->disttype == DISTTYPE_DEB)
		    continue;		/* unversioned provides can never match versioned deps */
		  break;
		}
	      prd = GETRELDEP(pool, pid);
	      if (prd->name != name)
		continue;		/* wrong provides name */
	      /* right package, both deps are rels. check flags/evr */
	      if (pool_match_flags_evr(pool, prd->flags, prd->evr, flags, evr))
		break;	/* matches */
	    }
	  if (!pid)
	    continue;	/* none of the providers matched */
	  queue_push(&plist, p);
	}
      /* make our system solvable provide all unknown rpmlib() stuff */
      if (plist.count == 0 && !strncmp(pool_id2str(pool, name), "rpmlib(", 7))
	queue_push(&plist, SYSTEMSOLVABLE);
    }
  /* add providers to whatprovides */
#if 0
  POOL_DEBUG(SOLV_DEBUG_STATS, "addrelproviders: adding %d packages to %d\n", plist.count, d);
#endif
  pool->whatprovides_rel[d] = pool_queuetowhatprovides(pool, &plist);
  queue_free(&plist);

  return pool->whatprovides_rel[d];
}

/*************************************************************************/

void
pool_debug(Pool *pool, int type, const char *format, ...)
{
  va_list args;
  char buf[1024];

  if ((type & (SOLV_FATAL|SOLV_ERROR)) == 0)
    {
      if ((pool->debugmask & type) == 0)
	return;
    }
  va_start(args, format);
  if (!pool->debugcallback)
    {
      if ((type & (SOLV_FATAL|SOLV_ERROR)) == 0 && !(pool->debugmask & SOLV_DEBUG_TO_STDERR))
        vprintf(format, args);
      else
        vfprintf(stderr, format, args);
      return;
    }
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);
  pool->debugcallback(pool, pool->debugcallbackdata, type, buf);
}

int
pool_error(Pool *pool, int ret, const char *format, ...)
{
  va_list args;
  int l;
  va_start(args, format);
  if (!pool->errstr)
    {
      pool->errstra = 1024;
      pool->errstr = solv_malloc(pool->errstra);
    }
  if (!*format)
    {
      *pool->errstr = 0;
      l = 0;
    }
  else
    l = vsnprintf(pool->errstr, pool->errstra, format, args);
  va_end(args);
  if (l >= 0 && l + 1 > pool->errstra)
    {
      pool->errstra = l + 256;
      pool->errstr = solv_realloc(pool->errstr, pool->errstra);
      va_start(args, format);
      l = vsnprintf(pool->errstr, pool->errstra, format, args);
      va_end(args);
    }
  if (l < 0)
    strcpy(pool->errstr, "unknown error");
  if (pool->debugmask & SOLV_ERROR)
    pool_debug(pool, SOLV_ERROR, "%s\n", pool->errstr);
  return ret;
}

char *
pool_errstr(Pool *pool)
{
  return pool->errstr ? pool->errstr : "no error";
}

void
pool_setdebuglevel(Pool *pool, int level)
{
  int mask = SOLV_DEBUG_RESULT;
  if (level > 0)
    mask |= SOLV_DEBUG_STATS|SOLV_DEBUG_ANALYZE|SOLV_DEBUG_UNSOLVABLE|SOLV_DEBUG_SOLVER|SOLV_DEBUG_TRANSACTION|SOLV_ERROR;
  if (level > 1)
    mask |= SOLV_DEBUG_JOB|SOLV_DEBUG_SOLUTIONS|SOLV_DEBUG_POLICY;
  if (level > 2)
    mask |= SOLV_DEBUG_PROPAGATE;
  if (level > 3)
    mask |= SOLV_DEBUG_RULE_CREATION;
  mask |= pool->debugmask & SOLV_DEBUG_TO_STDERR;	/* keep bit */
  pool->debugmask = mask;
}

void pool_setdebugcallback(Pool *pool, void (*debugcallback)(struct _Pool *, void *data, int type, const char *str), void *debugcallbackdata)
{
  pool->debugcallback = debugcallback;
  pool->debugcallbackdata = debugcallbackdata;
}

void pool_setdebugmask(Pool *pool, int mask)
{
  pool->debugmask = mask;
}

void pool_setloadcallback(Pool *pool, int (*cb)(struct _Pool *, struct _Repodata *, void *), void *loadcbdata)
{
  pool->loadcallback = cb;
  pool->loadcallbackdata = loadcbdata;
}

/*************************************************************************/

struct searchfiles {
  Id *ids;
  char **dirs;
  char **names;
  int nfiles;
  Map seen;
};

#define SEARCHFILES_BLOCK 127

static void
pool_addfileprovides_dep(Pool *pool, Id *ida, struct searchfiles *sf, struct searchfiles *isf)
{
  Id dep, sid;
  const char *s, *sr;
  struct searchfiles *csf;

  while ((dep = *ida++) != 0)
    {
      csf = sf;
      while (ISRELDEP(dep))
	{
	  Reldep *rd;
	  sid = pool->ss.nstrings + GETRELID(dep);
	  if (MAPTST(&csf->seen, sid))
	    {
	      dep = 0;
	      break;
	    }
	  MAPSET(&csf->seen, sid);
	  rd = GETRELDEP(pool, dep);
	  if (rd->flags < 8)
	    dep = rd->name;
	  else if (rd->flags == REL_NAMESPACE)
	    {
	      if (rd->name == NAMESPACE_INSTALLED || rd->name == NAMESPACE_SPLITPROVIDES)
		{
		  csf = isf;
		  if (!csf || MAPTST(&csf->seen, sid))
		    {
		      dep = 0;
		      break;
		    }
		  MAPSET(&csf->seen, sid);
		}
	      dep = rd->evr;
	    }
	  else if (rd->flags == REL_FILECONFLICT)
	    {
	      dep = 0;
	      break;
	    }
	  else
	    {
	      Id ids[2];
	      ids[0] = rd->name;
	      ids[1] = 0;
	      pool_addfileprovides_dep(pool, ids, csf, isf);
	      dep = rd->evr;
	    }
	}
      if (!dep)
	continue;
      if (MAPTST(&csf->seen, dep))
	continue;
      MAPSET(&csf->seen, dep);
      s = pool_id2str(pool, dep);
      if (*s != '/')
	continue;
      csf->ids = solv_extend(csf->ids, csf->nfiles, 1, sizeof(Id), SEARCHFILES_BLOCK);
      csf->dirs = solv_extend(csf->dirs, csf->nfiles, 1, sizeof(const char *), SEARCHFILES_BLOCK);
      csf->names = solv_extend(csf->names, csf->nfiles, 1, sizeof(const char *), SEARCHFILES_BLOCK);
      csf->ids[csf->nfiles] = dep;
      sr = strrchr(s, '/');
      csf->names[csf->nfiles] = solv_strdup(sr + 1);
      csf->dirs[csf->nfiles] = solv_malloc(sr - s + 1);
      if (sr != s)
        strncpy(csf->dirs[csf->nfiles], s, sr - s);
      csf->dirs[csf->nfiles][sr - s] = 0;
      csf->nfiles++;
    }
}

struct addfileprovides_cbdata {
  int nfiles;
  Id *ids;
  char **dirs;
  char **names;

  Id *dids;

  Map providedids;

  Map useddirs;
};

static int
addfileprovides_cb(void *cbdata, Solvable *s, Repodata *data, Repokey *key, KeyValue *value)
{
  struct addfileprovides_cbdata *cbd = cbdata;
  int i;

  if (!cbd->useddirs.size)
    {
      map_init(&cbd->useddirs, data->dirpool.ndirs + 1);
      for (i = 0; i < cbd->nfiles; i++)
	{
	  Id did;
	  if (MAPTST(&cbd->providedids, cbd->ids[i]))
	    {
	      cbd->dids[i] = 0;
	      continue;
	    }
	  did = repodata_str2dir(data, cbd->dirs[i], 0);
	  cbd->dids[i] = did;
	  if (did)
	    MAPSET(&cbd->useddirs, did);
	}
      repodata_free_dircache(data);
    }
  if (value->id >= data->dirpool.ndirs || !MAPTST(&cbd->useddirs, value->id))
    return 0;
  for (i = 0; i < cbd->nfiles; i++)
    {
      if (cbd->dids[i] != value->id)
	continue;
      if (!strcmp(cbd->names[i], value->str))
	break;
    }
  if (i == cbd->nfiles)
    return 0;
  s->provides = repo_addid_dep(s->repo, s->provides, cbd->ids[i], SOLVABLE_FILEMARKER);
  return 0;
}

static void
pool_addfileprovides_search(Pool *pool, struct addfileprovides_cbdata *cbd, struct searchfiles *sf, Repo *repoonly)
{
  Id p;
  Repodata *data;
  Repo *repo;
  Queue fileprovidesq;
  int i, j, repoid, repodataid;
  int provstart, provend;
  Map donemap;
  int ndone, incomplete;

  if (!pool->urepos)
    return;

  cbd->nfiles = sf->nfiles;
  cbd->ids = sf->ids;
  cbd->dirs = sf->dirs;
  cbd->names = sf->names;
  cbd->dids = solv_realloc2(cbd->dids, sf->nfiles, sizeof(Id));
  map_init(&cbd->providedids, pool->ss.nstrings);

  repoid = 1;
  repo = repoonly ? repoonly : pool->repos[repoid];
  map_init(&donemap, pool->nsolvables);
  queue_init(&fileprovidesq);
  provstart = provend = 0;
  for (;;)
    {
      if (!repo || repo->disabled)
	{
	  if (repoonly || ++repoid == pool->nrepos)
	    break;
	  repo = pool->repos[repoid];
	  continue;
	}
      ndone = 0;
      FOR_REPODATAS(repo, repodataid, data)
	{
	  if (ndone >= repo->nsolvables)
	    break;

	  if (repodata_lookup_idarray(data, SOLVID_META, REPOSITORY_ADDEDFILEPROVIDES, &fileprovidesq))
	    {
	      map_empty(&cbd->providedids);
	      for (i = 0; i < fileprovidesq.count; i++)
		MAPSET(&cbd->providedids, fileprovidesq.elements[i]);
	      provstart = data->start;
	      provend = data->end;
	      for (i = 0; i < cbd->nfiles; i++)
		if (!MAPTST(&cbd->providedids, cbd->ids[i]))
		  break;
	      if (i == cbd->nfiles)
		{
		  /* great! no need to search files */
		  for (p = data->start; p < data->end; p++)
		    if (pool->solvables[p].repo == repo)
		      {
			if (MAPTST(&donemap, p))
			  continue;
			MAPSET(&donemap, p);
			ndone++;
		      }
		  continue;
		}
	    }

	  if (!repodata_has_keyname(data, SOLVABLE_FILELIST))
	    continue;

	  if (data->start < provstart || data->end > provend)
	    {
	      map_empty(&cbd->providedids);
	      provstart = provend = 0;
	    }

	  /* check if the data is incomplete */
	  incomplete = 0;
	  if (data->state == REPODATA_AVAILABLE)
	    {
	      for (j = 1; j < data->nkeys; j++)
		if (data->keys[j].name != REPOSITORY_SOLVABLES && data->keys[j].name != SOLVABLE_FILELIST)
		  break;
	      if (j < data->nkeys)
		{
#if 0
		  for (i = 0; i < cbd->nfiles; i++)
		    if (!MAPTST(&cbd->providedids, cbd->ids[i]) && !repodata_filelistfilter_matches(data, pool_id2str(pool, cbd->ids[i])))
		      printf("need complete filelist because of %s\n", pool_id2str(pool, cbd->ids[i]));
#endif
		  for (i = 0; i < cbd->nfiles; i++)
		    if (!MAPTST(&cbd->providedids, cbd->ids[i]) && !repodata_filelistfilter_matches(data, pool_id2str(pool, cbd->ids[i])))
		      break;
		  if (i < cbd->nfiles)
		    incomplete = 1;
		}
	    }

	  /* do the search */
	  map_init(&cbd->useddirs, 0);
	  for (p = data->start; p < data->end; p++)
	    if (pool->solvables[p].repo == repo)
	      {
		if (MAPTST(&donemap, p))
		  continue;
	        repodata_search(data, p, SOLVABLE_FILELIST, 0, addfileprovides_cb, cbd);
		if (!incomplete)
		  {
		    MAPSET(&donemap, p);
		    ndone++;
		  }
	      }
	  map_free(&cbd->useddirs);
	}

      if (repoonly || ++repoid == pool->nrepos)
	break;
      repo = pool->repos[repoid];
    }
  map_free(&donemap);
  queue_free(&fileprovidesq);
  map_free(&cbd->providedids);
}

void
pool_addfileprovides_queue(Pool *pool, Queue *idq, Queue *idqinst)
{
  Solvable *s;
  Repo *installed, *repo;
  struct searchfiles sf, isf, *isfp;
  struct addfileprovides_cbdata cbd;
  int i;
  unsigned int now;

  installed = pool->installed;
  now = solv_timems(0);
  memset(&sf, 0, sizeof(sf));
  map_init(&sf.seen, pool->ss.nstrings + pool->nrels);
  memset(&isf, 0, sizeof(isf));
  map_init(&isf.seen, pool->ss.nstrings + pool->nrels);

  if (idq)
    queue_empty(idq);
  if (idqinst)
    queue_empty(idqinst);
  isfp = installed ? &isf : 0;
  for (i = 1, s = pool->solvables + i; i < pool->nsolvables; i++, s++)
    {
      repo = s->repo;
      if (!repo)
	continue;
      if (s->obsoletes)
        pool_addfileprovides_dep(pool, repo->idarraydata + s->obsoletes, &sf, isfp);
      if (s->conflicts)
        pool_addfileprovides_dep(pool, repo->idarraydata + s->conflicts, &sf, isfp);
      if (s->requires)
        pool_addfileprovides_dep(pool, repo->idarraydata + s->requires, &sf, isfp);
      if (s->recommends)
        pool_addfileprovides_dep(pool, repo->idarraydata + s->recommends, &sf, isfp);
      if (s->suggests)
        pool_addfileprovides_dep(pool, repo->idarraydata + s->suggests, &sf, isfp);
      if (s->supplements)
        pool_addfileprovides_dep(pool, repo->idarraydata + s->supplements, &sf, isfp);
      if (s->enhances)
        pool_addfileprovides_dep(pool, repo->idarraydata + s->enhances, &sf, isfp);
    }
  map_free(&sf.seen);
  map_free(&isf.seen);
  POOL_DEBUG(SOLV_DEBUG_STATS, "found %d file dependencies, %d installed file dependencies\n", sf.nfiles, isf.nfiles);
  cbd.dids = 0;
  if (sf.nfiles)
    {
#if 0
      for (i = 0; i < sf.nfiles; i++)
	POOL_DEBUG(SOLV_DEBUG_STATS, "looking up %s in filelist\n", pool_id2str(pool, sf.ids[i]));
#endif
      pool_addfileprovides_search(pool, &cbd, &sf, 0);
      if (idq)
        for (i = 0; i < sf.nfiles; i++)
	  queue_push(idq, sf.ids[i]);
      if (idqinst)
        for (i = 0; i < sf.nfiles; i++)
	  queue_push(idqinst, sf.ids[i]);
      solv_free(sf.ids);
      for (i = 0; i < sf.nfiles; i++)
	{
	  solv_free(sf.dirs[i]);
	  solv_free(sf.names[i]);
	}
      solv_free(sf.dirs);
      solv_free(sf.names);
    }
  if (isf.nfiles)
    {
#if 0
      for (i = 0; i < isf.nfiles; i++)
	POOL_DEBUG(SOLV_DEBUG_STATS, "looking up %s in installed filelist\n", pool_id2str(pool, isf.ids[i]));
#endif
      if (installed)
        pool_addfileprovides_search(pool, &cbd, &isf, installed);
      if (installed && idqinst)
        for (i = 0; i < isf.nfiles; i++)
	  queue_pushunique(idqinst, isf.ids[i]);
      solv_free(isf.ids);
      for (i = 0; i < isf.nfiles; i++)
	{
	  solv_free(isf.dirs[i]);
	  solv_free(isf.names[i]);
	}
      solv_free(isf.dirs);
      solv_free(isf.names);
    }
  solv_free(cbd.dids);
  pool_freewhatprovides(pool);	/* as we have added provides */
  POOL_DEBUG(SOLV_DEBUG_STATS, "addfileprovides took %d ms\n", solv_timems(now));
}

void
pool_addfileprovides(Pool *pool)
{
  pool_addfileprovides_queue(pool, 0, 0);
}

void
pool_search(Pool *pool, Id p, Id key, const char *match, int flags, int (*callback)(void *cbdata, Solvable *s, struct _Repodata *data, struct _Repokey *key, struct _KeyValue *kv), void *cbdata)
{
  if (p)
    {
      if (pool->solvables[p].repo)
        repo_search(pool->solvables[p].repo, p, key, match, flags, callback, cbdata);
      return;
    }
  /* FIXME: obey callback return value! */
  for (p = 1; p < pool->nsolvables; p++)
    if (pool->solvables[p].repo)
      repo_search(pool->solvables[p].repo, p, key, match, flags, callback, cbdata);
}

void
pool_clear_pos(Pool *pool)
{
  memset(&pool->pos, 0, sizeof(pool->pos));
}


void
pool_set_languages(Pool *pool, const char **languages, int nlanguages)
{
  int i;

  pool->languagecache = solv_free(pool->languagecache);
  pool->languagecacheother = 0;
  if (pool->nlanguages)
    {
      for (i = 0; i < pool->nlanguages; i++)
	free((char *)pool->languages[i]);
      free(pool->languages);
    }
  pool->nlanguages = nlanguages;
  if (!nlanguages)
    return;
  pool->languages = solv_calloc(nlanguages, sizeof(const char **));
  for (i = 0; i < pool->nlanguages; i++)
    pool->languages[i] = solv_strdup(languages[i]);
}

Id
pool_id2langid(Pool *pool, Id id, const char *lang, int create)
{
  const char *n;
  char buf[256], *p;
  int l;

  if (!lang || !*lang)
    return id;
  n = pool_id2str(pool, id);
  l = strlen(n) + strlen(lang) + 2;
  if (l > sizeof(buf))
    p = solv_malloc(strlen(n) + strlen(lang) + 2);
  else
    p = buf;
  sprintf(p, "%s:%s", n, lang);
  id = pool_str2id(pool, p, create);
  if (p != buf)
    free(p);
  return id;
}

char *
pool_alloctmpspace(Pool *pool, int len)
{
  int n = pool->tmpspace.n;
  if (!len)
    return 0;
  if (len > pool->tmpspace.len[n])
    {
      pool->tmpspace.buf[n] = solv_realloc(pool->tmpspace.buf[n], len + 32);
      pool->tmpspace.len[n] = len + 32;
    }
  pool->tmpspace.n = (n + 1) % POOL_TMPSPACEBUF;
  return pool->tmpspace.buf[n];
}

static char *
pool_alloctmpspace_free(Pool *pool, const char *space, int len)
{
  if (space)
    {
      int n, oldn;
      n = oldn = pool->tmpspace.n;
      for (;;)
	{
	  if (!n--)
	    n = POOL_TMPSPACEBUF - 1;
	  if (n == oldn)
	    break;
	  if (pool->tmpspace.buf[n] != space)
	    continue;
	  if (len > pool->tmpspace.len[n])
	    {
	      pool->tmpspace.buf[n] = solv_realloc(pool->tmpspace.buf[n], len + 32);
	      pool->tmpspace.len[n] = len + 32;
	    }
          return pool->tmpspace.buf[n];
	}
    }
  return 0;
}

void
pool_freetmpspace(Pool *pool, const char *space)
{
  int n = pool->tmpspace.n;
  if (!space)
    return;
  n = (n + (POOL_TMPSPACEBUF - 1)) % POOL_TMPSPACEBUF;
  if (pool->tmpspace.buf[n] == space)
    pool->tmpspace.n = n;
}

char *
pool_tmpjoin(Pool *pool, const char *str1, const char *str2, const char *str3)
{
  int l1, l2, l3;
  char *s, *str;
  l1 = str1 ? strlen(str1) : 0;
  l2 = str2 ? strlen(str2) : 0;
  l3 = str3 ? strlen(str3) : 0;
  s = str = pool_alloctmpspace(pool, l1 + l2 + l3 + 1);
  if (l1)
    {
      strcpy(s, str1);
      s += l1;
    }
  if (l2)
    {
      strcpy(s, str2);
      s += l2;
    }
  if (l3)
    {
      strcpy(s, str3);
      s += l3;
    }
  *s = 0;
  return str;
}

char *
pool_tmpappend(Pool *pool, const char *str1, const char *str2, const char *str3)
{
  int l1, l2, l3;
  char *s, *str;

  l1 = str1 ? strlen(str1) : 0;
  l2 = str2 ? strlen(str2) : 0;
  l3 = str3 ? strlen(str3) : 0;
  str = pool_alloctmpspace_free(pool, str1, l1 + l2 + l3 + 1);
  if (str)
    str1 = str;
  else
    str = pool_alloctmpspace(pool, l1 + l2 + l3 + 1);
  s = str;
  if (l1)
    {
      if (s != str1)
        strcpy(s, str1);
      s += l1;
    }
  if (l2)
    {
      strcpy(s, str2);
      s += l2;
    }
  if (l3)
    {
      strcpy(s, str3);
      s += l3;
    }
  *s = 0;
  return str;
}

const char *
pool_bin2hex(Pool *pool, const unsigned char *buf, int len)
{
  char *s;
  if (!len)
    return "";
  s = pool_alloctmpspace(pool, 2 * len + 1);
  solv_bin2hex(buf, len, s);
  return s;
}

/*******************************************************************/

struct mptree {
  Id sibling;
  Id child;
  const char *comp;
  int compl;
  Id mountpoint;
};

struct ducbdata {
  DUChanges *mps;
  struct mptree *mptree;
  int addsub;
  int hasdu;

  Id *dirmap;
  int nmap;
  Repodata *olddata;
};


static int
solver_fill_DU_cb(void *cbdata, Solvable *s, Repodata *data, Repokey *key, KeyValue *value)
{
  struct ducbdata *cbd = cbdata;
  Id mp;

  if (data != cbd->olddata)
    {
      Id dn, mp, comp, *dirmap, *dirs;
      int i, compl;
      const char *compstr;
      struct mptree *mptree;

      /* create map from dir to mptree */
      cbd->dirmap = solv_free(cbd->dirmap);
      cbd->nmap = 0;
      dirmap = solv_calloc(data->dirpool.ndirs, sizeof(Id));
      mptree = cbd->mptree;
      mp = 0;
      for (dn = 2, dirs = data->dirpool.dirs + dn; dn < data->dirpool.ndirs; dn++)
	{
	  comp = *dirs++;
	  if (comp <= 0)
	    {
	      mp = dirmap[-comp];
	      continue;
	    }
	  if (mp < 0)
	    {
	      /* unconnected */
	      dirmap[dn] = mp;
	      continue;
	    }
	  if (!mptree[mp].child)
	    {
	      dirmap[dn] = -mp;
	      continue;
	    }
	  if (data->localpool)
	    compstr = stringpool_id2str(&data->spool, comp);
	  else
	    compstr = pool_id2str(data->repo->pool, comp);
	  compl = strlen(compstr);
	  for (i = mptree[mp].child; i; i = mptree[i].sibling)
	    if (mptree[i].compl == compl && !strncmp(mptree[i].comp, compstr, compl))
	      break;
	  dirmap[dn] = i ? i : -mp;
	}
      /* change dirmap to point to mountpoint instead of mptree */
      for (dn = 0; dn < data->dirpool.ndirs; dn++)
	{
	  mp = dirmap[dn];
	  dirmap[dn] = mptree[mp > 0 ? mp : -mp].mountpoint;
	}
      cbd->dirmap = dirmap;
      cbd->nmap = data->dirpool.ndirs;
      cbd->olddata = data;
    }
  cbd->hasdu = 1;
  if (value->id < 0 || value->id >= cbd->nmap)
    return 0;
  mp = cbd->dirmap[value->id];
  if (mp < 0)
    return 0;
  if (cbd->addsub > 0)
    {
      cbd->mps[mp].kbytes += value->num;
      cbd->mps[mp].files += value->num2;
    }
  else
    {
      cbd->mps[mp].kbytes -= value->num;
      cbd->mps[mp].files -= value->num2;
    }
  return 0;
}

static void
propagate_mountpoints(struct mptree *mptree, int pos, Id mountpoint)
{
  int i;
  if (mptree[pos].mountpoint == -1)
    mptree[pos].mountpoint = mountpoint;
  else
    mountpoint = mptree[pos].mountpoint;
  for (i = mptree[pos].child; i; i = mptree[i].sibling)
    propagate_mountpoints(mptree, i, mountpoint);
}

#define MPTREE_BLOCK 15

void
pool_calc_duchanges(Pool *pool, Map *installedmap, DUChanges *mps, int nmps)
{
  char *p;
  const char *path, *compstr;
  struct mptree *mptree;
  int i, nmptree;
  int pos, compl;
  int mp;
  struct ducbdata cbd;
  Solvable *s;
  Id sp;
  Map ignoredu;
  Repo *oldinstalled = pool->installed;

  memset(&ignoredu, 0, sizeof(ignoredu));
  cbd.mps = mps;
  cbd.addsub = 0;
  cbd.dirmap = 0;
  cbd.nmap = 0;
  cbd.olddata = 0;

  mptree = solv_extend_resize(0, 1, sizeof(struct mptree), MPTREE_BLOCK);

  /* our root node */
  mptree[0].sibling = 0;
  mptree[0].child = 0;
  mptree[0].comp = 0;
  mptree[0].compl = 0;
  mptree[0].mountpoint = -1;
  nmptree = 1;
  
  /* create component tree */
  for (mp = 0; mp < nmps; mp++)
    {
      mps[mp].kbytes = 0;
      mps[mp].files = 0;
      pos = 0;
      path = mps[mp].path;
      while(*path == '/')
	path++;
      while (*path)
	{
	  if ((p = strchr(path, '/')) == 0)
	    {
	      compstr = path;
	      compl = strlen(compstr);
	      path += compl;
	    }
	  else
	    {
	      compstr = path;
	      compl = p - path;
	      path = p + 1;
	      while(*path == '/')
		path++;
	    }
          for (i = mptree[pos].child; i; i = mptree[i].sibling)
	    if (mptree[i].compl == compl && !strncmp(mptree[i].comp, compstr, compl))
	      break;
	  if (!i)
	    {
	      /* create new node */
	      mptree = solv_extend(mptree, nmptree, 1, sizeof(struct mptree), MPTREE_BLOCK);
	      i = nmptree++;
	      mptree[i].sibling = mptree[pos].child;
	      mptree[i].child = 0;
	      mptree[i].comp = compstr;
	      mptree[i].compl = compl;
	      mptree[i].mountpoint = -1;
	      mptree[pos].child = i;
	    }
	  pos = i;
	}
      mptree[pos].mountpoint = mp;
    }

  propagate_mountpoints(mptree, 0, mptree[0].mountpoint);

#if 0
  for (i = 0; i < nmptree; i++)
    {
      printf("#%d sibling: %d\n", i, mptree[i].sibling);
      printf("#%d child: %d\n", i, mptree[i].child);
      printf("#%d comp: %s\n", i, mptree[i].comp);
      printf("#%d compl: %d\n", i, mptree[i].compl);
      printf("#%d mountpont: %d\n", i, mptree[i].mountpoint);
    }
#endif

  cbd.mptree = mptree;
  cbd.addsub = 1;
  for (sp = 1, s = pool->solvables + sp; sp < pool->nsolvables; sp++, s++)
    {
      if (!s->repo || (oldinstalled && s->repo == oldinstalled))
	continue;
      if (!MAPTST(installedmap, sp))
	continue;
      cbd.hasdu = 0;
      repo_search(s->repo, sp, SOLVABLE_DISKUSAGE, 0, 0, solver_fill_DU_cb, &cbd);
      if (!cbd.hasdu && oldinstalled)
	{
	  Id op, opp;
	  /* no du data available, ignore data of all installed solvables we obsolete */
	  if (!ignoredu.map)
	    map_init(&ignoredu, oldinstalled->end - oldinstalled->start);
	  if (s->obsoletes)
	    {
	      Id obs, *obsp = s->repo->idarraydata + s->obsoletes;
	      while ((obs = *obsp++) != 0)
		FOR_PROVIDES(op, opp, obs)
		  if (op >= oldinstalled->start && op < oldinstalled->end)
		    MAPSET(&ignoredu, op - oldinstalled->start);
	    }
	  FOR_PROVIDES(op, opp, s->name)
	    if (pool->solvables[op].name == s->name)
	      if (op >= oldinstalled->start && op < oldinstalled->end)
		MAPSET(&ignoredu, op - oldinstalled->start);
	}
    }
  cbd.addsub = -1;
  if (oldinstalled)
    {
      /* assumes we allways have du data for installed solvables */
      FOR_REPO_SOLVABLES(oldinstalled, sp, s)
	{
	  if (MAPTST(installedmap, sp))
	    continue;
	  if (ignoredu.map && MAPTST(&ignoredu, sp - oldinstalled->start))
	    continue;
	  repo_search(oldinstalled, sp, SOLVABLE_DISKUSAGE, 0, 0, solver_fill_DU_cb, &cbd);
	}
    }
  if (ignoredu.map)
    map_free(&ignoredu);
  solv_free(cbd.dirmap);
  solv_free(mptree);
}

int
pool_calc_installsizechange(Pool *pool, Map *installedmap)
{
  Id sp;
  Solvable *s;
  int change = 0;
  Repo *oldinstalled = pool->installed;

  for (sp = 1, s = pool->solvables + sp; sp < pool->nsolvables; sp++, s++)
    {
      if (!s->repo || (oldinstalled && s->repo == oldinstalled))
	continue;
      if (!MAPTST(installedmap, sp))
	continue;
      change += solvable_lookup_sizek(s, SOLVABLE_INSTALLSIZE, 0);
    }
  if (oldinstalled)
    {
      FOR_REPO_SOLVABLES(oldinstalled, sp, s)
	{
	  if (MAPTST(installedmap, sp))
	    continue;
	  change -= solvable_lookup_sizek(s, SOLVABLE_INSTALLSIZE, 0);
	}
    }
  return change;
}

/* map:
 *  1: installed
 *  2: conflicts with installed
 *  8: interesting (only true if installed)
 * 16: undecided
 */
 
static inline Id dep2name(Pool *pool, Id dep)
{
  while (ISRELDEP(dep))
    {
      Reldep *rd = rd = GETRELDEP(pool, dep);
      dep = rd->name;
    }
  return dep;
}

static int providedbyinstalled_multiversion(Pool *pool, unsigned char *map, Id n, Id con) 
{
  Id p, pp;
  Solvable *sn = pool->solvables + n; 

  FOR_PROVIDES(p, pp, sn->name)
    {    
      Solvable *s = pool->solvables + p; 
      if (s->name != sn->name || s->arch != sn->arch)
        continue;
      if ((map[p] & 9) != 9)
        continue;
      if (pool_match_nevr(pool, pool->solvables + p, con))
	continue;
      return 1;		/* found installed package that doesn't conflict */
    }
  return 0;
}

static inline int providedbyinstalled(Pool *pool, unsigned char *map, Id dep, int ispatch, Map *multiversionmap)
{
  Id p, pp;
  int r = 0;
  FOR_PROVIDES(p, pp, dep)
    {
      if (p == SYSTEMSOLVABLE)
        return 1;	/* always boring, as never constraining */
      if (ispatch && !pool_match_nevr(pool, pool->solvables + p, dep))
	continue;
      if (ispatch && multiversionmap && multiversionmap->size && MAPTST(multiversionmap, p) && ISRELDEP(dep))
	if (providedbyinstalled_multiversion(pool, map, p, dep))
	  continue;
      if ((map[p] & 9) == 9)
	return 9;
      r |= map[p] & 17;
    }
  return r;
}

/*
 * pool_trivial_installable - calculate if a set of solvables is
 * trivial installable without any other installs/deinstalls of
 * packages not belonging to the set.
 *
 * the state is returned in the result queue:
 * 1:  solvable is installable without any other package changes
 * 0:  solvable is not installable
 * -1: solvable is installable, but doesn't constrain any installed packages
 */

void
pool_trivial_installable_multiversionmap(Pool *pool, Map *installedmap, Queue *pkgs, Queue *res, Map *multiversionmap)
{
  int i, r, m, did;
  Id p, *dp, con, *conp, req, *reqp;
  unsigned char *map;
  Solvable *s;

  map = solv_calloc(pool->nsolvables, 1);
  for (p = 1; p < pool->nsolvables; p++)
    {
      if (!MAPTST(installedmap, p))
	continue;
      map[p] |= 9;
      s = pool->solvables + p;
      if (!s->conflicts)
	continue;
      conp = s->repo->idarraydata + s->conflicts;
      while ((con = *conp++) != 0)
	{
	  dp = pool_whatprovides_ptr(pool, con);
	  for (; *dp; dp++)
	    map[p] |= 2;	/* XXX: self conflict ? */
	}
    }
  for (i = 0; i < pkgs->count; i++)
    map[pkgs->elements[i]] = 16;

  for (i = 0, did = 0; did < pkgs->count; i++, did++)
    {
      if (i == pkgs->count)
	i = 0;
      p = pkgs->elements[i];
      if ((map[p] & 16) == 0)
	continue;
      if ((map[p] & 2) != 0)
	{
	  map[p] = 2;
	  continue;
	}
      s = pool->solvables + p;
      m = 1;
      if (s->requires)
	{
	  reqp = s->repo->idarraydata + s->requires;
	  while ((req = *reqp++) != 0)
	    {
	      if (req == SOLVABLE_PREREQMARKER)
		continue;
	      r = providedbyinstalled(pool, map, req, 0, 0);
	      if (!r)
		{
		  /* decided and miss */
		  map[p] = 2;
		  did = 0;
		  break;
		}
	      if (r == 16)
		break;	/* undecided */
	      m |= r;	/* 1 | 9 | 17 */
	    }
	  if (req)
	    continue;
	  if ((m & 9) == 9)
	    m = 9;
	}
      if (s->conflicts)
	{
	  int ispatch = 0;	/* see solver.c patch handling */

	  if (!strncmp("patch:", pool_id2str(pool, s->name), 6))
	    ispatch = 1;
	  conp = s->repo->idarraydata + s->conflicts;
	  while ((con = *conp++) != 0)
	    {
	      if ((providedbyinstalled(pool, map, con, ispatch, multiversionmap) & 1) != 0)
		{
		  map[p] = 2;
		  did = 0;
		  break;
		}
	      if ((m == 1 || m == 17) && ISRELDEP(con))
		{
		  con = dep2name(pool, con);
		  if ((providedbyinstalled(pool, map, con, ispatch, multiversionmap) & 1) != 0)
		    m = 9;
		}
	    }
	  if (con)
	    continue;	/* found a conflict */
	}
#if 0
      if (s->repo && s->repo != oldinstalled)
	{
	  Id p2, obs, *obsp, *pp;
	  Solvable *s2;
	  if (s->obsoletes)
	    {
	      obsp = s->repo->idarraydata + s->obsoletes;
	      while ((obs = *obsp++) != 0)
		{
		  if ((providedbyinstalled(pool, map, obs, 0, 0) & 1) != 0)
		    {
		      map[p] = 2;
		      break;
		    }
		}
	      if (obs)
		continue;
	    }
	  FOR_PROVIDES(p2, pp, s->name)
	    {
	      s2 = pool->solvables + p2;
	      if (s2->name == s->name && (map[p2] & 1) != 0)
		{
		  map[p] = 2;
		  break;
		}
	    }
	  if (p2)
	    continue;
	}
#endif
      if (m != map[p])
	{
	  map[p] = m;
	  did = 0;
	}
    }
  queue_free(res);
  queue_init_clone(res, pkgs);
  for (i = 0; i < pkgs->count; i++)
    {
      m = map[pkgs->elements[i]];
      if ((m & 9) == 9)
	r = 1;
      else if (m & 1)
	r = -1;
      else
	r = 0;
      res->elements[i] = r;
    }
  free(map);
}

void
pool_trivial_installable(Pool *pool, Map *installedmap, Queue *pkgs, Queue *res)
{
  pool_trivial_installable_multiversionmap(pool, installedmap, pkgs, res, 0);
}

const char *
pool_lookup_str(Pool *pool, Id entry, Id keyname)
{
  if (entry == SOLVID_POS && pool->pos.repo)
    return repodata_lookup_str(pool->pos.repo->repodata + pool->pos.repodataid, SOLVID_POS, keyname);
  if (entry <= 0)
    return 0;
  return solvable_lookup_str(pool->solvables + entry, keyname);
}

Id
pool_lookup_id(Pool *pool, Id entry, Id keyname)
{
  if (entry == SOLVID_POS && pool->pos.repo)
    {
      Repodata *data = pool->pos.repo->repodata + pool->pos.repodataid;
      Id id = repodata_lookup_id(data, SOLVID_POS, keyname);
      return data->localpool ? repodata_globalize_id(data, id, 1) : id;
    }
  if (entry <= 0)
    return 0;
  return solvable_lookup_id(pool->solvables + entry, keyname);
}

unsigned long long
pool_lookup_num(Pool *pool, Id entry, Id keyname, unsigned long long notfound)
{
  if (entry == SOLVID_POS && pool->pos.repo)
    {
      unsigned long long value;
      if (repodata_lookup_num(pool->pos.repo->repodata + pool->pos.repodataid, SOLVID_POS, keyname, &value))
	return value;
      return notfound;
    }
  if (entry <= 0)
    return notfound;
  return solvable_lookup_num(pool->solvables + entry, keyname, notfound);
}

int
pool_lookup_void(Pool *pool, Id entry, Id keyname)
{
  if (entry == SOLVID_POS && pool->pos.repo)
    return repodata_lookup_void(pool->pos.repo->repodata + pool->pos.repodataid, SOLVID_POS, keyname);
  if (entry <= 0)
    return 0;
  return solvable_lookup_void(pool->solvables + entry, keyname);
}

const unsigned char *
pool_lookup_bin_checksum(Pool *pool, Id entry, Id keyname, Id *typep)
{
  if (entry == SOLVID_POS && pool->pos.repo)
    return repodata_lookup_bin_checksum(pool->pos.repo->repodata + pool->pos.repodataid, SOLVID_POS, keyname, typep);
  if (entry <= 0)
    return 0;
  return solvable_lookup_bin_checksum(pool->solvables + entry, keyname, typep);
}

const char *
pool_lookup_checksum(Pool *pool, Id entry, Id keyname, Id *typep)
{
  if (entry == SOLVID_POS && pool->pos.repo)
    {
      const unsigned char *chk = repodata_lookup_bin_checksum(pool->pos.repo->repodata + pool->pos.repodataid, SOLVID_POS, keyname, typep);
      return chk ? repodata_chk2str(pool->pos.repo->repodata + pool->pos.repodataid, *typep, chk) : 0;
    }
  if (entry <= 0)
    return 0;
  return solvable_lookup_checksum(pool->solvables + entry, keyname, typep);
}

const char *
pool_lookup_deltalocation(Pool *pool, Id entry, unsigned int *medianrp)
{
  const char *loc;
  if (medianrp)
    *medianrp = 0;
  if (entry != SOLVID_POS)
    return 0;
  loc = pool_lookup_str(pool, entry, DELTA_LOCATION_DIR);
  loc = pool_tmpjoin(pool, loc, loc ? "/" : 0, pool_lookup_str(pool, entry, DELTA_LOCATION_NAME));
  loc = pool_tmpappend(pool, loc, "-", pool_lookup_str(pool, entry, DELTA_LOCATION_EVR));
  loc = pool_tmpappend(pool, loc, ".", pool_lookup_str(pool, entry, DELTA_LOCATION_SUFFIX));
  return loc;
}

static void
add_new_provider(Pool *pool, Id id, Id p)
{
  Queue q;
  Id *pp;

  while (ISRELDEP(id))
    {
      Reldep *rd = GETRELDEP(pool, id);
      id = rd->name;
    }

  queue_init(&q);
  for (pp = pool->whatprovidesdata + pool->whatprovides[id]; *pp; pp++)
    {
      if (*pp == p)
	{
	  queue_free(&q);
	  return;
	}
      if (*pp > p)
	{
	  queue_push(&q, p);
	  p = 0;
	}
      queue_push(&q, *pp);
    }
  if (p)
    queue_push(&q, p);
  pool->whatprovides[id] = pool_queuetowhatprovides(pool, &q);
  queue_free(&q);
}

void
pool_add_fileconflicts_deps(Pool *pool, Queue *conflicts)
{
  int hadhashes = pool->relhashtbl ? 1 : 0;
  Solvable *s;
  Id fn, p, q, md5;
  Id id;
  int i;

  if (!conflicts->count)
    return;
  for (i = 0; i < conflicts->count; i += 6)
    {
      fn = conflicts->elements[i];
      p = conflicts->elements[i + 1];
      md5 = conflicts->elements[i + 2];
      q = conflicts->elements[i + 4];
      id = pool_rel2id(pool, fn, md5, REL_FILECONFLICT, 1);
      s = pool->solvables + p;
      if (!s->repo)
	continue;
      s->provides = repo_addid_dep(s->repo, s->provides, id, SOLVABLE_FILEMARKER);
      if (pool->whatprovides)
	add_new_provider(pool, fn, p);
      if (pool->whatprovides_rel)
	pool->whatprovides_rel[GETRELID(id)] = 0;	/* clear cache */
      s = pool->solvables + q;
      if (!s->repo)
	continue;
      s->conflicts = repo_addid_dep(s->repo, s->conflicts, id, 0);
    }
  if (!hadhashes)
    pool_freeidhashes(pool);
}

char *
pool_prepend_rootdir(Pool *pool, const char *path)
{
  if (!path)
    return 0;
  if (!pool->rootdir)
    return solv_strdup(path);
  return solv_dupjoin(pool->rootdir, "/", *path == '/' ? path + 1 : path);
}

const char *
pool_prepend_rootdir_tmp(Pool *pool, const char *path)
{
  if (!path)
    return 0;
  if (!pool->rootdir)
    return path;
  return pool_tmpjoin(pool, pool->rootdir, "/", *path == '/' ? path + 1 : path);
}

void
pool_set_rootdir(Pool *pool, const char *rootdir)
{
  solv_free(pool->rootdir);
  pool->rootdir = solv_strdup(rootdir);
}

const char *
pool_get_rootdir(Pool *pool)
{
  return pool->rootdir;
}

/* only used in libzypp */
void
pool_set_custom_vendorcheck(Pool *pool, int (*vendorcheck)(Pool *, Solvable *, Solvable *))
{
  pool->custom_vendorcheck = vendorcheck;
}

/* EOF */
