/*
 * Copyright (c) 2008, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/*
 * solvable.c
 *
 * set/retrieve data from solvables
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

#include "pool.h"
#include "repo.h"
#include "util.h"
#include "policy.h"
#include "poolvendor.h"
#include "chksum.h"

const char *
pool_solvable2str(Pool *pool, Solvable *s)
{
  const char *n, *e, *a;
  int nl, el, al;
  char *p;
  n = pool_id2str(pool, s->name);
  e = pool_id2str(pool, s->evr);
  /* XXX: may want to skip the epoch here */
  a = pool_id2str(pool, s->arch);
  nl = strlen(n);
  el = strlen(e);
  al = strlen(a);
  if (pool->havedistepoch)
    {
      /* strip the distepoch from the evr */
      const char *de = strrchr(e, '-');
      if (de && (de = strchr(de, ':')) != 0)
	el = de - e;
    }
  p = pool_alloctmpspace(pool, nl + el + al + 3);
  strcpy(p, n);
  if (el)
    {
      p[nl++] = '-';
      strncpy(p + nl, e, el);
    }
  if (al)
    {
      p[nl + el] = pool->disttype == DISTTYPE_HAIKU ? '-' : '.';
      strcpy(p + nl + el + 1, a);
    }
  return p;
}

Id
solvable_lookup_type(Solvable *s, Id keyname)
{
  if (!s->repo)
    return 0;
  return repo_lookup_type(s->repo, s - s->repo->pool->solvables, keyname);
}

Id
solvable_lookup_id(Solvable *s, Id keyname)
{
  if (!s->repo)
    return 0;
  return repo_lookup_id(s->repo, s - s->repo->pool->solvables, keyname);
}

int
solvable_lookup_idarray(Solvable *s, Id keyname, Queue *q)
{
  if (!s->repo)
    {
      queue_empty(q);
      return 0;
    }
  return repo_lookup_idarray(s->repo, s - s->repo->pool->solvables, keyname, q);
}

int
solvable_lookup_deparray(Solvable *s, Id keyname, Queue *q, Id marker)
{
  if (!s->repo)
    {
      queue_empty(q);
      return 0;
    }
  return repo_lookup_deparray(s->repo, s - s->repo->pool->solvables, keyname, q, marker);
}

const char *
solvable_lookup_str(Solvable *s, Id keyname)
{
  if (!s->repo)
    return 0;
  return repo_lookup_str(s->repo, s - s->repo->pool->solvables, keyname);
}

static const char *
solvable_lookup_str_base(Solvable *s, Id keyname, Id basekeyname, int usebase)
{
  Pool *pool;
  const char *str, *basestr;
  Id p, pp;
  Solvable *s2;
  int pass;

  if (!s->repo)
    return 0;
  pool = s->repo->pool;
  str = solvable_lookup_str(s, keyname);
  if (str || keyname == basekeyname)
    return str;
  basestr = solvable_lookup_str(s, basekeyname);
  if (!basestr)
    return 0;
  /* search for a solvable with same name and same base that has the
   * translation */
  if (!pool->whatprovides)
    return usebase ? basestr : 0;
  /* we do this in two passes, first same vendor, then all other vendors */
  for (pass = 0; pass < 2; pass++)
    {
      FOR_PROVIDES(p, pp, s->name)
	{
	  s2 = pool->solvables + p;
	  if (s2->name != s->name)
	    continue;
	  if ((s->vendor == s2->vendor) != (pass == 0))
	    continue;
	  str = solvable_lookup_str(s2, basekeyname);
	  if (!str || strcmp(str, basestr))
	    continue;
	  str = solvable_lookup_str(s2, keyname);
	  if (str)
	    return str;
	}
    }
  return usebase ? basestr : 0;
}

const char *
solvable_lookup_str_poollang(Solvable *s, Id keyname)
{
  Pool *pool;
  int i, cols;
  const char *str;
  Id *row;

  if (!s->repo)
    return 0;
  pool = s->repo->pool;
  if (!pool->nlanguages)
    return solvable_lookup_str(s, keyname);
  cols = pool->nlanguages + 1;
  if (!pool->languagecache)
    {
      pool->languagecache = solv_calloc(cols * ID_NUM_INTERNAL, sizeof(Id));
      pool->languagecacheother = 0;
    }
  if (keyname >= ID_NUM_INTERNAL)
    {
      row = pool->languagecache + ID_NUM_INTERNAL * cols;
      for (i = 0; i < pool->languagecacheother; i++, row += cols)
	if (*row == keyname)
	  break;
      if (i >= pool->languagecacheother)
	{
	  pool->languagecache = solv_realloc2(pool->languagecache, pool->languagecacheother + 1, cols * sizeof(Id));
	  row = pool->languagecache + cols * (ID_NUM_INTERNAL + pool->languagecacheother++);
	  *row = keyname;
	}
    }
  else
    row = pool->languagecache + keyname * cols;
  row++;	/* skip keyname */
  for (i = 0; i < pool->nlanguages; i++, row++)
    {
      if (!*row)
        *row = pool_id2langid(pool, keyname, pool->languages[i], 1);
      str = solvable_lookup_str_base(s, *row, keyname, 0);
      if (str)
	return str;
    }
  return solvable_lookup_str(s, keyname);
}

const char *
solvable_lookup_str_lang(Solvable *s, Id keyname, const char *lang, int usebase)
{
  if (s->repo)
    {
      Id id = pool_id2langid(s->repo->pool, keyname, lang, 0);
      if (id)
        return solvable_lookup_str_base(s, id, keyname, usebase);
      if (!usebase)
	return 0;
    }
  return solvable_lookup_str(s, keyname);
}

unsigned long long
solvable_lookup_num(Solvable *s, Id keyname, unsigned long long notfound)
{
  if (!s->repo)
    return notfound;
  return repo_lookup_num(s->repo, s - s->repo->pool->solvables, keyname, notfound);
}

unsigned int
solvable_lookup_sizek(Solvable *s, Id keyname, unsigned int notfound)
{
  unsigned long long size;
  if (!s->repo)
    return notfound;
  size = solvable_lookup_num(s, keyname, (unsigned long long)notfound << 10);
  return (unsigned int)((size + 1023) >> 10);
}

int
solvable_lookup_void(Solvable *s, Id keyname)
{
  if (!s->repo)
    return 0;
  return repo_lookup_void(s->repo, s - s->repo->pool->solvables, keyname);
}

int
solvable_lookup_bool(Solvable *s, Id keyname)
{
  if (!s->repo)
    return 0;
  /* historic nonsense: there are two ways of storing a bool, as num == 1 or void. test both. */
  if (repo_lookup_type(s->repo, s - s->repo->pool->solvables, keyname) == REPOKEY_TYPE_VOID)
    return 1;
  return repo_lookup_num(s->repo, s - s->repo->pool->solvables, keyname, 0) == 1;
}

const unsigned char *
solvable_lookup_bin_checksum(Solvable *s, Id keyname, Id *typep)
{
  Repo *repo = s->repo;

  if (!repo)
    {
      *typep = 0;
      return 0;
    }
  return repo_lookup_bin_checksum(repo, s - repo->pool->solvables, keyname, typep);
}

const char *
solvable_lookup_checksum(Solvable *s, Id keyname, Id *typep)
{
  const unsigned char *chk = solvable_lookup_bin_checksum(s, keyname, typep);
  return chk ? pool_bin2hex(s->repo->pool, chk, solv_chksum_len(*typep)) : 0;
}

static inline const char *
evrid2vrstr(Pool *pool, Id evrid)
{
  const char *p, *evr = pool_id2str(pool, evrid);
  if (!evr)
    return evr;
  for (p = evr; *p >= '0' && *p <= '9'; p++)
    ;
  return p != evr && *p == ':' && p[1] ? p + 1 : evr;
}

const char *
solvable_lookup_location(Solvable *s, unsigned int *medianrp)
{
  Pool *pool;
  int l = 0;
  char *loc;
  const char *mediadir, *mediafile;

  if (medianrp)
    *medianrp = 0;
  if (!s->repo)
    return 0;
  pool = s->repo->pool;
  if (medianrp)
    *medianrp = solvable_lookup_num(s, SOLVABLE_MEDIANR, 0);
  if (solvable_lookup_void(s, SOLVABLE_MEDIADIR))
    mediadir = pool_id2str(pool, s->arch);
  else
    mediadir = solvable_lookup_str(s, SOLVABLE_MEDIADIR);
  if (mediadir)
    l = strlen(mediadir) + 1;
  if (solvable_lookup_void(s, SOLVABLE_MEDIAFILE))
    {
      const char *name, *evr, *arch;
      name = pool_id2str(pool, s->name);
      evr = evrid2vrstr(pool, s->evr);
      arch = pool_id2str(pool, s->arch);
      /* name-vr.arch.rpm */
      loc = pool_alloctmpspace(pool, l + strlen(name) + strlen(evr) + strlen(arch) + 7);
      if (mediadir)
	sprintf(loc, "%s/%s-%s.%s.rpm", mediadir, name, evr, arch);
      else
	sprintf(loc, "%s-%s.%s.rpm", name, evr, arch);
    }
  else
    {
      mediafile = solvable_lookup_str(s, SOLVABLE_MEDIAFILE);
      if (!mediafile)
	return 0;
      loc = pool_alloctmpspace(pool, l + strlen(mediafile) + 1);
      if (mediadir)
	sprintf(loc, "%s/%s", mediadir, mediafile);
      else
	strcpy(loc, mediafile);
    }
  return loc;
}

const char *
solvable_get_location(Solvable *s, unsigned int *medianrp)
{
  const char *loc = solvable_lookup_location(s, medianrp);
  if (medianrp && *medianrp == 0)
    *medianrp = 1;	/* compat, to be removed */
  return loc;
}

const char *
solvable_lookup_sourcepkg(Solvable *s)
{
  Pool *pool;
  const char *evr, *name;
  Id archid;

  if (!s->repo)
    return 0;
  pool = s->repo->pool;
  if (solvable_lookup_void(s, SOLVABLE_SOURCENAME))
    name = pool_id2str(pool, s->name);
  else
    name = solvable_lookup_str(s, SOLVABLE_SOURCENAME);
  if (!name)
    return 0;
  archid = solvable_lookup_id(s, SOLVABLE_SOURCEARCH);
  if (solvable_lookup_void(s, SOLVABLE_SOURCEEVR))
    evr = evrid2vrstr(pool, s->evr);
  else
    evr = solvable_lookup_str(s, SOLVABLE_SOURCEEVR);
  if (archid == ARCH_SRC || archid == ARCH_NOSRC)
    {
      char *str;
      str = pool_tmpjoin(pool, name, evr ? "-" : 0, evr);
      str = pool_tmpappend(pool, str, ".", pool_id2str(pool, archid));
      return pool_tmpappend(pool, str, ".rpm", 0);
    }
  else 
    return name;	/* FIXME */
}


/*****************************************************************************/

static inline Id dep2name(Pool *pool, Id dep)
{
  while (ISRELDEP(dep))
    {
      Reldep *rd = rd = GETRELDEP(pool, dep);
      dep = rd->name;
    }
  return dep;
}

static int providedbyinstalled_multiversion(Pool *pool, Map *installed, Id n, Id con) 
{
  Id p, pp;
  Solvable *sn = pool->solvables + n; 

  FOR_PROVIDES(p, pp, sn->name)
    {    
      Solvable *s = pool->solvables + p; 
      if (s->name != sn->name || s->arch != sn->arch)
        continue;
      if (!MAPTST(installed, p))
        continue;
      if (pool_match_nevr(pool, pool->solvables + p, con))
        continue;
      return 1;         /* found installed package that doesn't conflict */
    }    
  return 0;
}

static inline int providedbyinstalled(Pool *pool, Map *installed, Id dep, int ispatch, Map *multiversionmap)
{
  Id p, pp;
  FOR_PROVIDES(p, pp, dep)
    {
      if (p == SYSTEMSOLVABLE)
	return -1;
      if (ispatch && !pool_match_nevr(pool, pool->solvables + p, dep))
	continue;
      if (ispatch && multiversionmap && multiversionmap->size && MAPTST(multiversionmap, p) && ISRELDEP(dep))
	if (providedbyinstalled_multiversion(pool, installed, p, dep))
	  continue;
      if (MAPTST(installed, p))
	return 1;
    }
  return 0;
}

/*
 * solvable_trivial_installable_map - anwers is a solvable is installable
 * without any other installs/deinstalls.
 * The packages considered to be installed are provided via the
 * installedmap bitmap. A additional "conflictsmap" bitmap providing
 * information about the conflicts of the installed packages can be
 * used for extra speed up. Provide a NULL pointer if you do not
 * have this information.
 * Both maps can be created with pool_create_state_maps() or
 * solver_create_state_maps().
 *
 * returns:
 * 1:  solvable is installable without any other package changes
 * 0:  solvable is not installable
 * -1: solvable is installable, but doesn't constrain any installed packages
 */
int
solvable_trivial_installable_map(Solvable *s, Map *installedmap, Map *conflictsmap, Map *multiversionmap)
{
  Pool *pool = s->repo->pool;
  Solvable *s2;
  Id p, *dp;
  Id *reqp, req;
  Id *conp, con;
  int r, interesting = 0;

  if (conflictsmap && MAPTST(conflictsmap, s - pool->solvables))
    return 0;
  if (s->requires)
    {
      reqp = s->repo->idarraydata + s->requires;
      while ((req = *reqp++) != 0)
	{
	  if (req == SOLVABLE_PREREQMARKER)
	    continue;
          r = providedbyinstalled(pool, installedmap, req, 0, 0);
	  if (!r)
	    return 0;
	  if (r > 0)
	    interesting = 1;
	}
    }
  if (s->conflicts)
    {
      int ispatch = 0;

      if (!strncmp("patch:", pool_id2str(pool, s->name), 6))
	ispatch = 1;
      conp = s->repo->idarraydata + s->conflicts;
      while ((con = *conp++) != 0)
	{
	  if (providedbyinstalled(pool, installedmap, con, ispatch, multiversionmap))
	    {
	      if (ispatch && solvable_is_irrelevant_patch(s, installedmap))
		return -1;
	      return 0;
	    }
	  if (!interesting && ISRELDEP(con))
	    {
              con = dep2name(pool, con);
	      if (providedbyinstalled(pool, installedmap, con, ispatch, multiversionmap))
		interesting = 1;
	    }
	}
      if (ispatch && interesting && solvable_is_irrelevant_patch(s, installedmap))
	interesting = 0;
    }
#if 0
  if (s->repo)
    {
      Id *obsp, obs;
      Repo *installed = 0;
      if (s->obsoletes && s->repo != installed)
	{
	  obsp = s->repo->idarraydata + s->obsoletes;
	  while ((obs = *obsp++) != 0)
	    {
	      if (providedbyinstalled(pool, installedmap, obs, 0, 0))
		return 0;
	    }
	}
      if (s->repo != installed)
	{
	  Id pp;
	  FOR_PROVIDES(p, pp, s->name)
	    {
	      s2 = pool->solvables + p;
	      if (s2->repo == installed && s2->name == s->name)
		return 0;
	    }
	}
    }
#endif
  if (!conflictsmap)
    {
      int i;

      p = s - pool->solvables;
      for (i = 1; i < pool->nsolvables; i++)
	{
	  if (!MAPTST(installedmap, i))
	    continue;
	  s2 = pool->solvables + i;
	  if (!s2->conflicts)
	    continue;
	  conp = s2->repo->idarraydata + s2->conflicts;
	  while ((con = *conp++) != 0)
	    {
	      dp = pool_whatprovides_ptr(pool, con);
	      for (; *dp; dp++)
		if (*dp == p)
		  return 0;
	    }
	}
     }
  return interesting ? 1 : -1;
}

/*
 * different interface for solvable_trivial_installable_map, where
 * the information about the installed packages is provided
 * by a queue.
 */
int
solvable_trivial_installable_queue(Solvable *s, Queue *installed, Map *multiversionmap)
{
  Pool *pool = s->repo->pool;
  int i;
  Id p;
  Map installedmap;
  int r;

  map_init(&installedmap, pool->nsolvables);
  for (i = 0; i < installed->count; i++)
    {
      p = installed->elements[i];
      if (p > 0)		/* makes it work with decisionq */
	MAPSET(&installedmap, p);
    }
  r = solvable_trivial_installable_map(s, &installedmap, 0, multiversionmap);
  map_free(&installedmap);
  return r;
}

/*
 * different interface for solvable_trivial_installable_map, where
 * the information about the installed packages is provided
 * by a repo containing the installed solvables.
 */
int
solvable_trivial_installable_repo(Solvable *s, Repo *installed, Map *multiversionmap)
{
  Pool *pool = s->repo->pool;
  Id p;
  Solvable *s2;
  Map installedmap;
  int r;

  map_init(&installedmap, pool->nsolvables);
  FOR_REPO_SOLVABLES(installed, p, s2)
    MAPSET(&installedmap, p);
  r = solvable_trivial_installable_map(s, &installedmap, 0, multiversionmap);
  map_free(&installedmap);
  return r;
}

/* FIXME: this mirrors policy_illegal_vendorchange */
static int
pool_illegal_vendorchange(Pool *pool, Solvable *s1, Solvable *s2)
{
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
    return 1;   /* can't match */
  vendormask2 = pool_vendor2mask(pool, v2);
  if ((vendormask1 & vendormask2) != 0)
    return 0;
  return 1;     /* no class matches */
}

/* check if this patch is relevant according to the vendor. To bad that patches
 * don't have a vendor, so we need to do some careful repo testing. */
int
solvable_is_irrelevant_patch(Solvable *s, Map *installedmap)
{
  Pool *pool = s->repo->pool;
  Id con, *conp;
  int hadpatchpackage = 0;

  if (!s->conflicts)
    return 0;
  conp = s->repo->idarraydata + s->conflicts;
  while ((con = *conp++) != 0)
    {
      Reldep *rd;
      Id p, pp, p2, pp2;
      if (!ISRELDEP(con))
	continue;
      rd = GETRELDEP(pool, con);
      if (rd->flags != REL_LT)
	continue;
      FOR_PROVIDES(p, pp, con)
	{
	  Solvable *si;
	  if (!MAPTST(installedmap, p))
	    continue;
	  si = pool->solvables + p;
	  if (!pool_match_nevr(pool, si, con))
	    continue;
	  FOR_PROVIDES(p2, pp2, rd->name)
	    {
	      Solvable *s2 = pool->solvables + p2;
	      if (!pool_match_nevr(pool, s2, rd->name))
		continue;
	      if (pool_match_nevr(pool, s2, con))
		continue;	/* does not fulfill patch */
	      if (s2->repo == s->repo)
		{
		  hadpatchpackage = 1;
		  /* ok, we have a package from the patch repo that solves the conflict. check vendor */
		  if (si->vendor == s2->vendor)
		    return 0;
		  if (!pool_illegal_vendorchange(pool, si, s2))
		    return 0;
		  /* vendor change was illegal, ignore conflict */
		}
	    }
	}
    }
  /* if we didn't find a patchpackage don't claim that the patch is irrelevant */
  if (!hadpatchpackage)
    return 0;
  return 1;
}

/*****************************************************************************/

/*
 * Create maps containing the state of each solvable. Input is a "installed" queue,
 * it contains all solvable ids that are considered to be installed.
 * 
 * The created maps can be used for solvable_trivial_installable_map(),
 * pool_calc_duchanges(), pool_calc_installsizechange().
 *
 */
void
pool_create_state_maps(Pool *pool, Queue *installed, Map *installedmap, Map *conflictsmap)
{
  int i;
  Solvable *s;
  Id p, *dp;
  Id *conp, con;

  map_init(installedmap, pool->nsolvables);
  if (conflictsmap)
    map_init(conflictsmap, pool->nsolvables);
  for (i = 0; i < installed->count; i++)
    {
      p = installed->elements[i];
      if (p <= 0)	/* makes it work with decisionq */
	continue;
      MAPSET(installedmap, p);
      if (!conflictsmap)
	continue;
      s = pool->solvables + p;
      if (!s->conflicts)
	continue;
      conp = s->repo->idarraydata + s->conflicts;
      while ((con = *conp++) != 0)
	{
	  dp = pool_whatprovides_ptr(pool, con);
	  for (; *dp; dp++)
	    MAPSET(conflictsmap, *dp);
	}
    }
}

/* Tests if two solvables have identical content. Currently
 * both solvables need to come from the same pool
 */
int
solvable_identical(Solvable *s1, Solvable *s2)
{
  unsigned int bt1, bt2;
  Id rq1, rq2;
  Id *reqp;

  if (s1->name != s2->name)
    return 0;
  if (s1->arch != s2->arch)
    return 0;
  if (s1->evr != s2->evr)
    return 0;
  /* map missing vendor to empty string */
  if ((s1->vendor ? s1->vendor : 1) != (s2->vendor ? s2->vendor : 1))
    return 0;

  /* looking good, try some fancier stuff */
  /* might also look up the package checksum here */
  bt1 = solvable_lookup_num(s1, SOLVABLE_BUILDTIME, 0);
  bt2 = solvable_lookup_num(s2, SOLVABLE_BUILDTIME, 0);
  if (bt1 && bt2)
    {
      if (bt1 != bt2)
        return 0;
    }
  else
    {
      /* look at requires in a last attempt to find recompiled packages */
      rq1 = rq2 = 0;
      if (s1->requires)
	for (reqp = s1->repo->idarraydata + s1->requires; *reqp; reqp++)
	  rq1 ^= *reqp;
      if (s2->requires)
	for (reqp = s2->repo->idarraydata + s2->requires; *reqp; reqp++)
	  rq2 ^= *reqp;
      if (rq1 != rq2)
	 return 0;
    }
  return 1;
}

/* return the self provide dependency of a solvable */
Id
solvable_selfprovidedep(Solvable *s)
{
  Pool *pool;
  Reldep *rd;
  Id prov, *provp;

  if (!s->repo)
    return s->name;
  pool = s->repo->pool;
  if (s->provides)
    {
      provp = s->repo->idarraydata + s->provides;
      while ((prov = *provp++) != 0)
	{
	  if (!ISRELDEP(prov))
	    continue;
	  rd = GETRELDEP(pool, prov);
	  if (rd->name == s->name && rd->evr == s->evr && rd->flags == REL_EQ)
	    return prov;
	}
    }
  return pool_rel2id(pool, s->name, s->evr, REL_EQ, 1);
}

/* setter functions, simply call the repo variants */
void
solvable_set_id(Solvable *s, Id keyname, Id id)
{
  repo_set_id(s->repo, s - s->repo->pool->solvables, keyname, id);
}

void
solvable_set_num(Solvable *s, Id keyname, unsigned long long num)
{
  repo_set_num(s->repo, s - s->repo->pool->solvables, keyname, num);
}

void
solvable_set_str(Solvable *s, Id keyname, const char *str)
{
  repo_set_str(s->repo, s - s->repo->pool->solvables, keyname, str);
}

void
solvable_set_poolstr(Solvable *s, Id keyname, const char *str)
{
  repo_set_poolstr(s->repo, s - s->repo->pool->solvables, keyname, str);
}

void
solvable_add_poolstr_array(Solvable *s, Id keyname, const char *str)
{
  repo_add_poolstr_array(s->repo, s - s->repo->pool->solvables, keyname, str);
}

void
solvable_add_idarray(Solvable *s, Id keyname, Id id)
{
  repo_add_idarray(s->repo, s - s->repo->pool->solvables, keyname, id);
}

void
solvable_add_deparray(Solvable *s, Id keyname, Id dep, Id marker)
{
  repo_add_deparray(s->repo, s - s->repo->pool->solvables, keyname, dep, marker);
}

void
solvable_set_idarray(Solvable *s, Id keyname, Queue *q)
{
  repo_set_idarray(s->repo, s - s->repo->pool->solvables, keyname, q);
}

void
solvable_set_deparray(Solvable *s, Id keyname, Queue *q, Id marker)
{
  repo_set_deparray(s->repo, s - s->repo->pool->solvables, keyname, q, marker);
}

void
solvable_unset(Solvable *s, Id keyname)
{
  repo_unset(s->repo, s - s->repo->pool->solvables, keyname);
}
