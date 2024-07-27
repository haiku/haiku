/*
 * Copyright (c) 2007, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/*
 * repo.c
 *
 * Manage metadata coming from one repository
 *
 */

#define _GNU_SOURCE
#include <string.h>
#include <fnmatch.h>

#include <stdio.h>
#include <stdlib.h>



#include "repo.h"
#include "pool.h"
#include "poolid_private.h"
#include "util.h"
#include "chksum.h"

#define IDARRAY_BLOCK     4095


/*
 * create empty repo
 * and add to pool
 */

Repo *
repo_create(Pool *pool, const char *name)
{
  Repo *repo;

  pool_freewhatprovides(pool);
  repo = (Repo *)solv_calloc(1, sizeof(*repo));
  if (!pool->nrepos)
    {
      pool->nrepos = 1;	/* start with repoid 1 */
      pool->repos = (Repo **)solv_calloc(2, sizeof(Repo *));
    }
  else
    pool->repos = (Repo **)solv_realloc2(pool->repos, pool->nrepos + 1, sizeof(Repo *));
  pool->repos[pool->nrepos] = repo;
  pool->urepos++;
  repo->repoid = pool->nrepos++;
  repo->name = name ? solv_strdup(name) : 0;
  repo->pool = pool;
  repo->start = pool->nsolvables;
  repo->end = pool->nsolvables;
  repo->nsolvables = 0;
  return repo;
}

void
repo_freedata(Repo *repo)
{
  int i;
  for (i = 1; i < repo->nrepodata; i++)
    repodata_freedata(repo->repodata + i);
  solv_free(repo->repodata);
  solv_free(repo->idarraydata);
  solv_free(repo->rpmdbid);
  solv_free(repo->lastidhash);
  solv_free((char *)repo->name);
  solv_free(repo);
}

/* delete all solvables and repodata blocks from this repo */

void
repo_empty(Repo *repo, int reuseids)
{
  Pool *pool = repo->pool;
  Solvable *s;
  int i;

  pool_freewhatprovides(pool);
  if (reuseids && repo->end == pool->nsolvables)
    {
      /* it's ok to reuse the ids. As this is the last repo, we can
         just shrink the solvable array */
      for (i = repo->end - 1, s = pool->solvables + i; i >= repo->start; i--, s--)
	if (s->repo != repo)
	  break;
      pool_free_solvable_block(pool, i + 1, repo->end - (i + 1), reuseids);
    }
  /* zero out (i.e. free) solvables belonging to this repo */
  for (i = repo->start, s = pool->solvables + i; i < repo->end; i++, s++)
    if (s->repo == repo)
      memset(s, 0, sizeof(*s));
  repo->nsolvables = 0;

  /* free all data belonging to this repo */
  repo->idarraydata = solv_free(repo->idarraydata);
  repo->idarraysize = 0;
  repo->lastoff = 0;
  repo->rpmdbid = solv_free(repo->rpmdbid);
  for (i = 1; i < repo->nrepodata; i++)
    repodata_freedata(repo->repodata + i);
  solv_free(repo->repodata);
  repo->repodata = 0;
  repo->nrepodata = 0;
}

/*
 * remove repo from pool, delete solvables
 *
 */

void
repo_free(Repo *repo, int reuseids)
{
  Pool *pool = repo->pool;
  int i;

  if (repo == pool->installed)
    pool->installed = 0;
  repo_empty(repo, reuseids);
  for (i = 1; i < pool->nrepos; i++)	/* find repo in pool */
    if (pool->repos[i] == repo)
      break;
  if (i == pool->nrepos)	       /* repo not in pool, return */
    return;
  if (i == pool->nrepos - 1 && reuseids)
    pool->nrepos--;
  else
    pool->repos[i] = 0;
  pool->urepos--;
  repo_freedata(repo);
}

Id
repo_add_solvable(Repo *repo)
{
  Id p = pool_add_solvable(repo->pool);
  if (!repo->start || repo->start == repo->end)
    repo->start = repo->end = p;
  /* warning: sidedata must be extended before adapting start/end */
  if (repo->rpmdbid)
    repo->rpmdbid = (Id *)repo_sidedata_extend(repo, repo->rpmdbid, sizeof(Id), p, 1); 
  if (p < repo->start)
    repo->start = p;
  if (p + 1 > repo->end)
    repo->end = p + 1;
  repo->nsolvables++;
  repo->pool->solvables[p].repo = repo;
  return p;
}

Id
repo_add_solvable_block(Repo *repo, int count)
{
  Id p;
  Solvable *s; 
  if (!count)
    return 0;
  p = pool_add_solvable_block(repo->pool, count);
  if (!repo->start || repo->start == repo->end)
    repo->start = repo->end = p;
  /* warning: sidedata must be extended before adapting start/end */
  if (repo->rpmdbid)
    repo->rpmdbid = (Id *)repo_sidedata_extend(repo, repo->rpmdbid, sizeof(Id), p, count);
  if (p < repo->start)
    repo->start = p;
  if (p + count > repo->end)
    repo->end = p + count;
  repo->nsolvables += count;
  for (s = repo->pool->solvables + p; count--; s++)
    s->repo = repo;
  return p;
}

void
repo_free_solvable(Repo *repo, Id p, int reuseids)
{
  repo_free_solvable_block(repo, p, 1, reuseids);
}

void
repo_free_solvable_block(Repo *repo, Id start, int count, int reuseids)
{
  Solvable *s;
  Repodata *data;
  int i;
  if (start + count == repo->end)
    repo->end -= count;
  repo->nsolvables -= count;
  for (s = repo->pool->solvables + start, i = count; i--; s++)
    s->repo = 0;
  pool_free_solvable_block(repo->pool, start, count, reuseids);
  FOR_REPODATAS(repo, i, data)
    {
      int dstart, dend;
      if (data->end > repo->end)
        repodata_shrink(data, repo->end);
      dstart = data->start > start ? data->start : start;
      dend = data->end < start + count ? data->end : start + count;
      if (dstart < dend)
	{
	  if (data->attrs)
	    {
	      int j;
	      for (j = dstart; j < dend; j++)	
		data->attrs[j - data->start] = solv_free(data->attrs[j - data->start]);
	    }
	  if (data->incoreoffset)
	    memset(data->incoreoffset + (dstart - data->start), 0, (dend - dstart) * sizeof(Id));
	}
    }
}


/* repository sidedata is solvable data allocated on demand.
 * It is used for data that is normally not present
 * in the solvable like the rpmdbid.
 * The solvable allocation funcions need to make sure that
 * the sidedata gets extended if new solvables get added.
 */

#define REPO_SIDEDATA_BLOCK 63

void *
repo_sidedata_create(Repo *repo, size_t size)
{
  return solv_calloc_block(repo->end - repo->start, size, REPO_SIDEDATA_BLOCK);
}

void *
repo_sidedata_extend(Repo *repo, void *b, size_t size, Id p, int count)
{
  int n = repo->end - repo->start;
  if (p < repo->start)
    {
      int d = repo->start - p;
      b = solv_extend(b, n, d, size, REPO_SIDEDATA_BLOCK);
      memmove((char *)b + d * size, b, n * size);
      memset(b, 0, d * size);
      n += d;
    }
  if (p + count > repo->end)
    {
      int d = p + count - repo->end;
      b = solv_extend(b, n, d, size, REPO_SIDEDATA_BLOCK);
      memset((char *)b + n * size, 0, d * size);
    }
  return b;
}

/*
 * add Id to idarraydata used to store dependencies
 * olddeps: old array offset to extend
 * returns new array offset
 */

Offset
repo_addid(Repo *repo, Offset olddeps, Id id)
{
  Id *idarray;
  int idarraysize;
  int i;

  idarray = repo->idarraydata;
  idarraysize = repo->idarraysize;

  if (!idarray)			       /* alloc idarray if not done yet */
    {
      idarraysize = 1;
      idarray = solv_extend_resize(0, 1, sizeof(Id), IDARRAY_BLOCK);
      idarray[0] = 0;
      repo->lastoff = 0;
    }

  if (!olddeps)				/* no deps yet */
    {
      olddeps = idarraysize;
      idarray = solv_extend(idarray, idarraysize, 1, sizeof(Id), IDARRAY_BLOCK);
    }
  else if (olddeps == repo->lastoff)	/* extend at end */
    idarraysize--;
  else					/* can't extend, copy old */
    {
      i = olddeps;
      olddeps = idarraysize;
      for (; idarray[i]; i++)
        {
	  idarray = solv_extend(idarray, idarraysize, 1, sizeof(Id), IDARRAY_BLOCK);
          idarray[idarraysize++] = idarray[i];
        }
      idarray = solv_extend(idarray, idarraysize, 1, sizeof(Id), IDARRAY_BLOCK);
    }

  idarray[idarraysize++] = id;		/* insert Id into array */
  idarray = solv_extend(idarray, idarraysize, 1, sizeof(Id), IDARRAY_BLOCK);
  idarray[idarraysize++] = 0;		/* ensure NULL termination */

  repo->idarraydata = idarray;
  repo->idarraysize = idarraysize;
  repo->lastoff = olddeps;

  return olddeps;
}

#define REPO_ADDID_DEP_HASHTHRES	64
#define REPO_ADDID_DEP_HASHMIN		128

/* 
 * Optimization for packages with an excessive amount of provides/requires:
 * if the number of deps exceed a threshold, we build a hash of the already
 * seen ids.
 */
static Offset
repo_addid_dep_hash(Repo *repo, Offset olddeps, Id id, Id marker, int size)
{
  Id oid, *oidp;
  int before;
  Hashval h, hh;
  Id hid;

  before = 0;
  if (marker)
    {
      if (marker < 0)
	{
	  marker = -marker;
	  before = 1;
	}
      if (marker == id)
	marker = 0;
    }

  /* maintain hash and lastmarkerpos */
  if (repo->lastidhash_idarraysize != repo->idarraysize || size * 2 > repo->lastidhash_mask || repo->lastmarker != marker)
    {
      repo->lastmarkerpos = 0;
      if (size * 2 > repo->lastidhash_mask)
	{
	  repo->lastidhash_mask = mkmask(size < REPO_ADDID_DEP_HASHMIN ? REPO_ADDID_DEP_HASHMIN : size);
	  repo->lastidhash = solv_realloc2(repo->lastidhash, repo->lastidhash_mask + 1, sizeof(Id));
	}
      memset(repo->lastidhash, 0, (repo->lastidhash_mask + 1) * sizeof(Id));
      for (oidp = repo->idarraydata + olddeps; (oid = *oidp) != 0; oidp++)
	{
	  h = oid & repo->lastidhash_mask;
	  hh = HASHCHAIN_START;
	  while (repo->lastidhash[h] != 0)
	    h = HASHCHAIN_NEXT(h, hh, repo->lastidhash_mask);
	  repo->lastidhash[h] = oid;
	  if (marker && oid == marker)
	    repo->lastmarkerpos = oidp - repo->idarraydata;
	}
      repo->lastmarker = marker;
      repo->lastidhash_idarraysize = repo->idarraysize;
    }

  /* check the hash! */
  h = id & repo->lastidhash_mask;
  hh = HASHCHAIN_START;
  while ((hid = repo->lastidhash[h]) != 0 && hid != id)
    h = HASHCHAIN_NEXT(h, hh, repo->lastidhash_mask);
  /* put new element in hash */
  if (!hid)
    repo->lastidhash[h] = id;
  else if (marker == SOLVABLE_FILEMARKER && (!before || !repo->lastmarkerpos))
    return olddeps;
  if (marker && !before && !repo->lastmarkerpos)
    {
      /* we have to add the marker first */
      repo->lastmarkerpos = repo->idarraysize - 1;
      olddeps = repo_addid(repo, olddeps, marker);
      /* now put marker in hash */
      h = marker & repo->lastidhash_mask;
      hh = HASHCHAIN_START;
      while (repo->lastidhash[h] != 0)
	h = HASHCHAIN_NEXT(h, hh, repo->lastidhash_mask);
      repo->lastidhash[h] = marker;
      repo->lastidhash_idarraysize = repo->idarraysize;
    }
  if (!hid)
    {
      /* new entry, insert in correct position */
      if (marker && before && repo->lastmarkerpos)
	{
	  /* need to add it before the marker */
	  olddeps = repo_addid(repo, olddeps, id);	/* dummy to make room */
	  memmove(repo->idarraydata + repo->lastmarkerpos + 1, repo->idarraydata + repo->lastmarkerpos, (repo->idarraysize - repo->lastmarkerpos - 2) * sizeof(Id));
	  repo->idarraydata[repo->lastmarkerpos++] = id;
	}
      else
	{
	  /* just append it to the end */
	  olddeps = repo_addid(repo, olddeps, id);
	}
      repo->lastidhash_idarraysize = repo->idarraysize;
      return olddeps;
    }
  /* we already have it in the hash */
  if (!marker)
    return olddeps;
  if (marker == SOLVABLE_FILEMARKER)
    {
      /* check if it is in the wrong half */
      /* (we already made sure that "before" and "lastmarkerpos" are set, see above) */
      for (oidp = repo->idarraydata + repo->lastmarkerpos + 1; (oid = *oidp) != 0; oidp++)
	if (oid == id)
	  break;
      if (!oid)
	return olddeps;
      /* yes, wrong half. copy it over */
      memmove(repo->idarraydata + repo->lastmarkerpos + 1, repo->idarraydata + repo->lastmarkerpos, (oidp - (repo->idarraydata + repo->lastmarkerpos)) * sizeof(Id));
      repo->idarraydata[repo->lastmarkerpos++] = id;
      return olddeps;
    }
  if (before)
    return olddeps;
  /* check if it is in the correct half */
  for (oidp = repo->idarraydata + repo->lastmarkerpos + 1; (oid = *oidp) != 0; oidp++)
    if (oid == id)
      return olddeps;
  /* nope, copy it over */
  for (oidp = repo->idarraydata + olddeps; (oid = *oidp) != 0; oidp++)
    if (oid == id)
      break;
  if (!oid)
    return olddeps;	/* should not happen */
  memmove(oidp, oidp + 1, (repo->idarraydata + repo->idarraysize - oidp - 2) * sizeof(Id));
  repo->idarraydata[repo->idarraysize - 2] = id;
  repo->lastmarkerpos--;	/* marker has been moved */
  return olddeps;
}

/*
 * add dependency (as Id) to repo, also unifies dependencies
 * olddeps = offset into idarraydata
 * marker= 0 for normal dep
 * marker > 0 add dep after marker
 * marker < 0 add dep before -marker
 * returns new start of dependency array
 */
Offset
repo_addid_dep(Repo *repo, Offset olddeps, Id id, Id marker)
{
  Id oid, *oidp, *markerp;
  int before;

  if (!olddeps)
    {
      if (marker > 0)
	olddeps = repo_addid(repo, olddeps, marker);
      return repo_addid(repo, olddeps, id);
    }

  /* check if we should use the hash optimization */
  if (olddeps == repo->lastoff)
    {
      int size = repo->idarraysize - 1 - repo->lastoff;
      if (size >= REPO_ADDID_DEP_HASHTHRES)
        return repo_addid_dep_hash(repo, olddeps, id, marker, size);
    }

  before = 0;
  if (marker)
    {
      if (marker < 0)
	{
	  marker = -marker;
	  before = 1;
	}
      if (marker == id)
	marker = 0;
    }

  if (!marker)
    {
      for (oidp = repo->idarraydata + olddeps; (oid = *oidp) != 0; oidp++)
        if (oid == id)
	  return olddeps;
      return repo_addid(repo, olddeps, id);
    }

  markerp = 0;
  for (oidp = repo->idarraydata + olddeps; (oid = *oidp) != 0; oidp++)
    {
      if (oid == marker)
	markerp = oidp;
      else if (oid == id)
	break;
    }

  if (oid)
    {
      if (marker == SOLVABLE_FILEMARKER)
	{
	  if (!markerp || !before)
            return olddeps;
          /* we found it, but in the second half */
          memmove(markerp + 1, markerp, (oidp - markerp) * sizeof(Id));
          *markerp = id;
          return olddeps;
	}
      if (markerp || before)
        return olddeps;
      /* we found it, but in the first half */
      markerp = oidp++;
      for (; (oid = *oidp) != 0; oidp++)
        if (oid == marker)
          break;
      if (!oid)
        {
	  /* no marker in array yet */
          oidp--;
          if (markerp < oidp)
            memmove(markerp, markerp + 1, (oidp - markerp) * sizeof(Id));
          *oidp = marker;
          return repo_addid(repo, olddeps, id);
        }
      while (oidp[1])
        oidp++;
      memmove(markerp, markerp + 1, (oidp - markerp) * sizeof(Id));
      *oidp = id;
      return olddeps;
    }
  /* id not yet in array */
  if (!before && !markerp)
    olddeps = repo_addid(repo, olddeps, marker);
  else if (before && markerp)
    {
      *markerp++ = id;
      id = *--oidp;
      if (markerp < oidp)
        memmove(markerp + 1, markerp, (oidp - markerp) * sizeof(Id));
      *markerp = marker;
    }
  return repo_addid(repo, olddeps, id);
}


/*
 * reserve Ids
 * make space for 'num' more dependencies
 * returns new start of dependency array
 *
 * reserved ids will always begin at offset idarraysize
 */

Offset
repo_reserve_ids(Repo *repo, Offset olddeps, int num)
{
  num++;	/* room for trailing ID_NULL */

  if (!repo->idarraysize)	       /* ensure buffer space */
    {
      repo->idarraysize = 1;
      repo->idarraydata = solv_extend_resize(0, 1 + num, sizeof(Id), IDARRAY_BLOCK);
      repo->idarraydata[0] = 0;
      repo->lastoff = 1;
      return 1;
    }

  if (olddeps && olddeps != repo->lastoff)   /* if not appending */
    {
      /* can't insert into idarray, this would invalidate all 'larger' offsets
       * so create new space at end and move existing deps there.
       * Leaving 'hole' at old position.
       */

      Id *idstart, *idend;
      int count;

      for (idstart = idend = repo->idarraydata + olddeps; *idend++; )   /* find end */
	;
      count = idend - idstart - 1 + num;	       /* new size */

      repo->idarraydata = solv_extend(repo->idarraydata, repo->idarraysize, count, sizeof(Id), IDARRAY_BLOCK);
      /* move old deps to end */
      olddeps = repo->lastoff = repo->idarraysize;
      memcpy(repo->idarraydata + olddeps, idstart, count - num);
      repo->idarraysize = olddeps + count - num;

      return olddeps;
    }

  if (olddeps)			       /* appending */
    repo->idarraysize--;

  /* make room*/
  repo->idarraydata = solv_extend(repo->idarraydata, repo->idarraysize, num, sizeof(Id), IDARRAY_BLOCK);

  /* appending or new */
  repo->lastoff = olddeps ? olddeps : repo->idarraysize;

  return repo->lastoff;
}


/***********************************************************************/

/*
 * some SUSE specific fixups, should go into a separate file
 */

Offset
repo_fix_supplements(Repo *repo, Offset provides, Offset supplements, Offset freshens)
{
  Pool *pool = repo->pool;
  Id id, idp, idl;
  char buf[1024], *p, *dep;
  int i, l;

  if (provides)
    {
      for (i = provides; repo->idarraydata[i]; i++)
	{
	  id = repo->idarraydata[i];
	  if (ISRELDEP(id))
	    continue;
	  dep = (char *)pool_id2str(pool, id);
	  if (!strncmp(dep, "locale(", 7) && strlen(dep) < sizeof(buf) - 2)
	    {
	      idp = 0;
	      strcpy(buf + 2, dep);
	      dep = buf + 2 + 7;
	      if ((p = strchr(dep, ':')) != 0 && p != dep)
		{
		  *p++ = 0;
		  idp = pool_str2id(pool, dep, 1);
		  dep = p;
		}
	      id = 0;
	      while ((p = strchr(dep, ';')) != 0)
		{
		  if (p == dep)
		    {
		      dep = p + 1;
		      continue;
		    }
		  *p++ = 0;
		  idl = pool_str2id(pool, dep, 1);
		  idl = pool_rel2id(pool, NAMESPACE_LANGUAGE, idl, REL_NAMESPACE, 1);
		  if (id)
		    id = pool_rel2id(pool, id, idl, REL_OR, 1);
		  else
		    id = idl;
		  dep = p;
		}
	      if (dep[0] && dep[1])
		{
	 	  for (p = dep; *p && *p != ')'; p++)
		    ;
		  *p = 0;
		  idl = pool_str2id(pool, dep, 1);
		  idl = pool_rel2id(pool, NAMESPACE_LANGUAGE, idl, REL_NAMESPACE, 1);
		  if (id)
		    id = pool_rel2id(pool, id, idl, REL_OR, 1);
		  else
		    id = idl;
		}
	      if (idp)
		id = pool_rel2id(pool, idp, id, REL_AND, 1);
	      if (id)
		supplements = repo_addid_dep(repo, supplements, id, 0);
	    }
	  else if ((p = strchr(dep, ':')) != 0 && p != dep && p[1] == '/' && strlen(dep) < sizeof(buf))
	    {
	      strcpy(buf, dep);
	      p = buf + (p - dep);
	      *p++ = 0;
	      idp = pool_str2id(pool, buf, 1);
	      /* strip trailing slashes */
	      l = strlen(p);
	      while (l > 1 && p[l - 1] == '/')
		p[--l] = 0;
	      id = pool_str2id(pool, p, 1);
	      id = pool_rel2id(pool, idp, id, REL_WITH, 1);
	      id = pool_rel2id(pool, NAMESPACE_SPLITPROVIDES, id, REL_NAMESPACE, 1);
	      supplements = repo_addid_dep(repo, supplements, id, 0);
	    }
	}
    }
  if (supplements)
    {
      for (i = supplements; repo->idarraydata[i]; i++)
	{
	  id = repo->idarraydata[i];
	  if (ISRELDEP(id))
	    continue;
	  dep = (char *)pool_id2str(pool, id);
	  if (!strncmp(dep, "system:modalias(", 16))
	    dep += 7;
	  if (!strncmp(dep, "modalias(", 9) && dep[9] && dep[10] && strlen(dep) < sizeof(buf))
	    {
	      strcpy(buf, dep);
	      p = strchr(buf + 9, ':');
	      if (p && p != buf + 9 && strchr(p + 1, ':'))
		{
		  *p++ = 0;
		  idp = pool_str2id(pool, buf + 9, 1);
		  p[strlen(p) - 1] = 0;
		  id = pool_str2id(pool, p, 1);
		  id = pool_rel2id(pool, NAMESPACE_MODALIAS, id, REL_NAMESPACE, 1);
		  id = pool_rel2id(pool, idp, id, REL_AND, 1);
		}
	      else
		{
		  p = buf + 9;
		  p[strlen(p) - 1] = 0;
		  id = pool_str2id(pool, p, 1);
		  id = pool_rel2id(pool, NAMESPACE_MODALIAS, id, REL_NAMESPACE, 1);
		}
	      if (id)
		repo->idarraydata[i] = id;
	    }
	  else if (!strncmp(dep, "packageand(", 11) && strlen(dep) < sizeof(buf))
	    {
	      strcpy(buf, dep);
	      id = 0;
	      dep = buf + 11;
	      while ((p = strchr(dep, ':')) != 0)
		{
		  if (p == dep)
		    {
		      dep = p + 1;
		      continue;
		    }
		  /* argh, allow pattern: prefix. sigh */
		  if (p - dep == 7 && !strncmp(dep, "pattern", 7))
		    {
		      p = strchr(p + 1, ':');
		      if (!p)
			break;
		    }
		  *p++ = 0;
		  idp = pool_str2id(pool, dep, 1);
		  if (id)
		    id = pool_rel2id(pool, id, idp, REL_AND, 1);
		  else
		    id = idp;
		  dep = p;
		}
	      if (dep[0] && dep[1])
		{
		  dep[strlen(dep) - 1] = 0;
		  idp = pool_str2id(pool, dep, 1);
		  if (id)
		    id = pool_rel2id(pool, id, idp, REL_AND, 1);
		  else
		    id = idp;
		}
	      if (id)
		repo->idarraydata[i] = id;
	    }
	  else if (!strncmp(dep, "filesystem(", 11) && strlen(dep) < sizeof(buf))
	    {
	      strcpy(buf, dep + 11);
	      if ((p = strrchr(buf, ')')) != 0)
		*p = 0;
	      id = pool_str2id(pool, buf, 1);
	      id = pool_rel2id(pool, NAMESPACE_FILESYSTEM, id, REL_NAMESPACE, 1);
	      repo->idarraydata[i] = id;
	    }
	}
    }
  if (freshens && repo->idarraydata[freshens])
    {
      Id idsupp = 0, idfresh = 0;
      if (!supplements || !repo->idarraydata[supplements])
	return freshens;
      for (i = supplements; repo->idarraydata[i]; i++)
        {
	  if (!idsupp)
	    idsupp = repo->idarraydata[i];
	  else
	    idsupp = pool_rel2id(pool, idsupp, repo->idarraydata[i], REL_OR, 1);
        }
      for (i = freshens; repo->idarraydata[i]; i++)
        {
	  if (!idfresh)
	    idfresh = repo->idarraydata[i];
	  else
	    idfresh = pool_rel2id(pool, idfresh, repo->idarraydata[i], REL_OR, 1);
        }
      if (!idsupp)
        idsupp = idfresh;
      else
	idsupp = pool_rel2id(pool, idsupp, idfresh, REL_AND, 1);
      supplements = repo_addid_dep(repo, 0, idsupp, 0);
    }
  return supplements;
}

Offset
repo_fix_conflicts(Repo *repo, Offset conflicts)
{
  char buf[1024], *p, *dep;
  Pool *pool = repo->pool;
  Id id;
  int i;

  if (!conflicts)
    return conflicts;
  for (i = conflicts; repo->idarraydata[i]; i++)
    {
      id = repo->idarraydata[i];
      if (ISRELDEP(id))
	continue;
      dep = (char *)pool_id2str(pool, id);
      if (!strncmp(dep, "otherproviders(", 15) && strlen(dep) < sizeof(buf) - 2)
	{
	  strcpy(buf, dep + 15);
	  if ((p = strchr(buf, ')')) != 0)
	    *p = 0;
	  id = pool_str2id(pool, buf, 1);
	  id = pool_rel2id(pool, NAMESPACE_OTHERPROVIDERS, id, REL_NAMESPACE, 1);
	  repo->idarraydata[i] = id;
	}
    }
  return conflicts;
}

/***********************************************************************/

struct matchdata
{
  Pool *pool;
  int flags;
  Datamatcher matcher;
  int stop;
  int (*callback)(void *cbdata, Solvable *s, Repodata *data, Repokey *key, KeyValue *kv);
  void *callback_data;
};

int
repo_matchvalue(void *cbdata, Solvable *s, Repodata *data, Repokey *key, KeyValue *kv)
{
  struct matchdata *md = cbdata;

  if (md->matcher.match)
    {
      if (key->name == SOLVABLE_FILELIST && key->type == REPOKEY_TYPE_DIRSTRARRAY && (md->matcher.flags & SEARCH_FILES) != 0)
	if (!datamatcher_checkbasename(&md->matcher, kv->str))
	  return 0;
      if (!repodata_stringify(md->pool, data, key, kv, md->flags))
	return 0;
      if (!datamatcher_match(&md->matcher, kv->str))
	return 0;
    }
  md->stop = md->callback(md->callback_data, s, data, key, kv);
  return md->stop;
}


/* list of all keys we store in the solvable */
/* also used in the dataiterator code in repodata.c */
Repokey repo_solvablekeys[RPM_RPMDBID - SOLVABLE_NAME + 1] = {
  { SOLVABLE_NAME,        REPOKEY_TYPE_ID, 0, KEY_STORAGE_SOLVABLE },
  { SOLVABLE_ARCH,        REPOKEY_TYPE_ID, 0, KEY_STORAGE_SOLVABLE },
  { SOLVABLE_EVR,         REPOKEY_TYPE_ID, 0, KEY_STORAGE_SOLVABLE },
  { SOLVABLE_VENDOR,      REPOKEY_TYPE_ID, 0, KEY_STORAGE_SOLVABLE },
  { SOLVABLE_PROVIDES,    REPOKEY_TYPE_IDARRAY, 0, KEY_STORAGE_SOLVABLE },
  { SOLVABLE_OBSOLETES,   REPOKEY_TYPE_IDARRAY, 0, KEY_STORAGE_SOLVABLE },
  { SOLVABLE_CONFLICTS,   REPOKEY_TYPE_IDARRAY, 0, KEY_STORAGE_SOLVABLE },
  { SOLVABLE_REQUIRES,    REPOKEY_TYPE_IDARRAY, 0, KEY_STORAGE_SOLVABLE },
  { SOLVABLE_RECOMMENDS,  REPOKEY_TYPE_IDARRAY, 0, KEY_STORAGE_SOLVABLE },
  { SOLVABLE_SUGGESTS,    REPOKEY_TYPE_IDARRAY, 0, KEY_STORAGE_SOLVABLE },
  { SOLVABLE_SUPPLEMENTS, REPOKEY_TYPE_IDARRAY, 0, KEY_STORAGE_SOLVABLE },
  { SOLVABLE_ENHANCES,    REPOKEY_TYPE_IDARRAY, 0, KEY_STORAGE_SOLVABLE },
  { RPM_RPMDBID,          REPOKEY_TYPE_NUM, 0, KEY_STORAGE_SOLVABLE },
};

static void
domatch_idarray(Solvable *s, Id keyname, struct matchdata *md, Id *ida)
{
  KeyValue kv;
  kv.entry = 0;
  kv.parent = 0;
  for (; *ida && !md->stop; ida++)
    {
      kv.id = *ida;
      kv.eof = ida[1] ? 0 : 1;
      repo_matchvalue(md, s, 0, repo_solvablekeys + (keyname - SOLVABLE_NAME), &kv);
      kv.entry++;
    }
}

static void
repo_search_md(Repo *repo, Id p, Id keyname, struct matchdata *md)
{
  KeyValue kv;
  Pool *pool = repo->pool;
  Repodata *data;
  int i, j, flags;
  Solvable *s;

  kv.parent = 0;
  md->stop = 0;
  if (!p)
    {
      for (p = repo->start, s = repo->pool->solvables + p; p < repo->end; p++, s++)
	{
	  if (s->repo == repo)
            repo_search_md(repo, p, keyname, md);
	  if (md->stop > SEARCH_NEXT_SOLVABLE)
	    break;
	}
      return;
    }
  else if (p < 0)
    /* The callback only supports solvables, so we can't iterate over the
       extra things.  */
    return;
  flags = md->flags;
  if (!(flags & SEARCH_NO_STORAGE_SOLVABLE))
    {
      s = pool->solvables + p;
      switch(keyname)
	{
	  case 0:
	  case SOLVABLE_NAME:
	    if (s->name)
	      {
		kv.id = s->name;
		repo_matchvalue(md, s, 0, repo_solvablekeys + 0, &kv);
	      }
	    if (keyname || md->stop > SEARCH_NEXT_KEY)
	      return;
	  case SOLVABLE_ARCH:
	    if (s->arch)
	      {
		kv.id = s->arch;
		repo_matchvalue(md, s, 0, repo_solvablekeys + 1, &kv);
	      }
	    if (keyname || md->stop > SEARCH_NEXT_KEY)
	      return;
	  case SOLVABLE_EVR:
	    if (s->evr)
	      {
		kv.id = s->evr;
		repo_matchvalue(md, s, 0, repo_solvablekeys + 2, &kv);
	      }
	    if (keyname || md->stop > SEARCH_NEXT_KEY)
	      return;
	  case SOLVABLE_VENDOR:
	    if (s->vendor)
	      {
		kv.id = s->vendor;
		repo_matchvalue(md, s, 0, repo_solvablekeys + 3, &kv);
	      }
	    if (keyname || md->stop > SEARCH_NEXT_KEY)
	      return;
	  case SOLVABLE_PROVIDES:
	    if (s->provides)
	      domatch_idarray(s, SOLVABLE_PROVIDES, md, repo->idarraydata + s->provides);
	    if (keyname || md->stop > SEARCH_NEXT_KEY)
	      return;
	  case SOLVABLE_OBSOLETES:
	    if (s->obsoletes)
	      domatch_idarray(s, SOLVABLE_OBSOLETES, md, repo->idarraydata + s->obsoletes);
	    if (keyname || md->stop > SEARCH_NEXT_KEY)
	      return;
	  case SOLVABLE_CONFLICTS:
	    if (s->conflicts)
	      domatch_idarray(s, SOLVABLE_CONFLICTS, md, repo->idarraydata + s->conflicts);
	    if (keyname || md->stop > SEARCH_NEXT_KEY)
	      return;
	  case SOLVABLE_REQUIRES:
	    if (s->requires)
	      domatch_idarray(s, SOLVABLE_REQUIRES, md, repo->idarraydata + s->requires);
	    if (keyname || md->stop > SEARCH_NEXT_KEY)
	      return;
	  case SOLVABLE_RECOMMENDS:
	    if (s->recommends)
	      domatch_idarray(s, SOLVABLE_RECOMMENDS, md, repo->idarraydata + s->recommends);
	    if (keyname || md->stop > SEARCH_NEXT_KEY)
	      return;
	  case SOLVABLE_SUPPLEMENTS:
	    if (s->supplements)
	      domatch_idarray(s, SOLVABLE_SUPPLEMENTS, md, repo->idarraydata + s->supplements);
	    if (keyname || md->stop > SEARCH_NEXT_KEY)
	      return;
	  case SOLVABLE_SUGGESTS:
	    if (s->suggests)
	      domatch_idarray(s, SOLVABLE_SUGGESTS, md, repo->idarraydata + s->suggests);
	    if (keyname || md->stop > SEARCH_NEXT_KEY)
	      return;
	  case SOLVABLE_ENHANCES:
	    if (s->enhances)
	      domatch_idarray(s, SOLVABLE_ENHANCES, md, repo->idarraydata + s->enhances);
	    if (keyname || md->stop > SEARCH_NEXT_KEY)
	      return;
	  case RPM_RPMDBID:
	    if (repo->rpmdbid)
	      {
		kv.num = repo->rpmdbid[p - repo->start];
		kv.num2 = 0;
		repo_matchvalue(md, s, 0, repo_solvablekeys + (RPM_RPMDBID - SOLVABLE_NAME), &kv);
	      }
	    if (keyname || md->stop > SEARCH_NEXT_KEY)
	      return;
	    break;
	  default:
	    break;
	}
    }

  FOR_REPODATAS(repo, i, data)
    {
      if (p < data->start || p >= data->end)
	continue;
      if (keyname && !repodata_precheck_keyname(data, keyname))
	continue;
      if (keyname == SOLVABLE_FILELIST && !(md->flags & SEARCH_COMPLETE_FILELIST))
	{
	  /* do not search filelist extensions */
	  if (data->state != REPODATA_AVAILABLE)
	    continue;
	  for (j = 1; j < data->nkeys; j++)
	    if (data->keys[j].name != REPOSITORY_SOLVABLES && data->keys[j].name != SOLVABLE_FILELIST)
	      break;
	  if (j == data->nkeys)
	    continue;
	}
      if (data->state == REPODATA_STUB)
	{
	  if (keyname)
	    {
	      for (j = 1; j < data->nkeys; j++)
		if (keyname == data->keys[j].name)
		  break;
	      if (j == data->nkeys)
		continue;
	    }
	  /* load it */
	  if (data->loadcallback)
	    data->loadcallback(data);
	  else
            data->state = REPODATA_ERROR;
	}
      if (data->state == REPODATA_ERROR)
	continue;
      repodata_search(data, p, keyname, md->flags, repo_matchvalue, md);
      if (md->stop > SEARCH_NEXT_KEY)
	break;
    }
}

void
repo_search(Repo *repo, Id p, Id keyname, const char *match, int flags, int (*callback)(void *cbdata, Solvable *s, Repodata *data, Repokey *key, KeyValue *kv), void *cbdata)
{
  struct matchdata md;

  if (repo->disabled && !(flags & SEARCH_DISABLED_REPOS))
    return;
  memset(&md, 0, sizeof(md));
  md.pool = repo->pool;
  md.flags = flags;
  md.callback = callback;
  md.callback_data = cbdata;
  if (match)
    datamatcher_init(&md.matcher, match, flags);
  repo_search_md(repo, p, keyname, &md);
  if (match)
    datamatcher_free(&md.matcher);
}

const char *
repo_lookup_str(Repo *repo, Id entry, Id keyname)
{
  Pool *pool = repo->pool;
  Repodata *data;
  int i;
  const char *str;

  if (entry >= 0)
    {
      switch (keyname)
	{
	case SOLVABLE_NAME:
	  return pool_id2str(pool, pool->solvables[entry].name);
	case SOLVABLE_ARCH:
	  return pool_id2str(pool, pool->solvables[entry].arch);
	case SOLVABLE_EVR:
	  return pool_id2str(pool, pool->solvables[entry].evr);
	case SOLVABLE_VENDOR:
	  return pool_id2str(pool, pool->solvables[entry].vendor);
	}
    }
  FOR_REPODATAS(repo, i, data)
    {
      if (entry != SOLVID_META && (entry < data->start || entry >= data->end))
	continue;
      if (!repodata_precheck_keyname(data, keyname))
	continue;
      str = repodata_lookup_str(data, entry, keyname);
      if (str)
	return str;
      if (repodata_lookup_type(data, entry, keyname))
	return 0;
    }
  return 0;
}


unsigned long long
repo_lookup_num(Repo *repo, Id entry, Id keyname, unsigned long long notfound)
{
  Repodata *data;
  int i;
  unsigned long long value;

  if (entry >= 0)
    {
      if (keyname == RPM_RPMDBID)
	{
	  if (repo->rpmdbid && entry >= repo->start && entry < repo->end)
	    return repo->rpmdbid[entry - repo->start];
	  return notfound;
	}
    }
  FOR_REPODATAS(repo, i, data)
    {
      if (entry != SOLVID_META && (entry < data->start || entry >= data->end))
	continue;
      if (!repodata_precheck_keyname(data, keyname))
	continue;
      if (repodata_lookup_num(data, entry, keyname, &value))
	return value;
      if (repodata_lookup_type(data, entry, keyname))
	return notfound;
    }
  return notfound;
}

Id
repo_lookup_id(Repo *repo, Id entry, Id keyname)
{
  Repodata *data;
  int i;
  Id id;

  if (entry >= 0)
    {
      switch (keyname)
	{
	case SOLVABLE_NAME:
	  return repo->pool->solvables[entry].name;
	case SOLVABLE_ARCH:
	  return repo->pool->solvables[entry].arch;
	case SOLVABLE_EVR:
	  return repo->pool->solvables[entry].evr;
	case SOLVABLE_VENDOR:
	  return repo->pool->solvables[entry].vendor;
	}
    }
  FOR_REPODATAS(repo, i, data)
    {
      if (entry != SOLVID_META && (entry < data->start || entry >= data->end))
	continue;
      if (!repodata_precheck_keyname(data, keyname))
	continue;
      id = repodata_lookup_id(data, entry, keyname);
      if (id)
	return data->localpool ? repodata_globalize_id(data, id, 1) : id;
      if (repodata_lookup_type(data, entry, keyname))
	return 0;
    }
  return 0;
}

static int
lookup_idarray_solvable(Repo *repo, Offset off, Queue *q)
{
  Id *p;

  queue_empty(q);
  if (off)
    for (p = repo->idarraydata + off; *p; p++)
      queue_push(q, *p);
  return 1;
}

int
repo_lookup_idarray(Repo *repo, Id entry, Id keyname, Queue *q)
{
  Repodata *data;
  int i;
  if (entry >= 0)
    {
      switch (keyname)
        {
	case SOLVABLE_PROVIDES:
	  return lookup_idarray_solvable(repo, repo->pool->solvables[entry].provides, q);
	case SOLVABLE_OBSOLETES:
	  return lookup_idarray_solvable(repo, repo->pool->solvables[entry].obsoletes, q);
	case SOLVABLE_CONFLICTS:
	  return lookup_idarray_solvable(repo, repo->pool->solvables[entry].conflicts, q);
	case SOLVABLE_REQUIRES:
	  return lookup_idarray_solvable(repo, repo->pool->solvables[entry].requires, q);
	case SOLVABLE_RECOMMENDS:
	  return lookup_idarray_solvable(repo, repo->pool->solvables[entry].recommends, q);
	case SOLVABLE_SUGGESTS:
	  return lookup_idarray_solvable(repo, repo->pool->solvables[entry].suggests, q);
	case SOLVABLE_SUPPLEMENTS:
	  return lookup_idarray_solvable(repo, repo->pool->solvables[entry].supplements, q);
	case SOLVABLE_ENHANCES:
	  return lookup_idarray_solvable(repo, repo->pool->solvables[entry].enhances, q);
        }
    }
  FOR_REPODATAS(repo, i, data)
    {
      if (entry != SOLVID_META && (entry < data->start || entry >= data->end))
	continue;
      if (!repodata_precheck_keyname(data, keyname))
	continue;
      if (repodata_lookup_idarray(data, entry, keyname, q))
	{
	  if (data->localpool)
	    {
	      for (i = 0; i < q->count; i++)
		q->elements[i] = repodata_globalize_id(data, q->elements[i], 1);
	    }
	  return 1;
	}
      if (repodata_lookup_type(data, entry, keyname))
	break;
    }
  queue_empty(q);
  return 0;
}

int
repo_lookup_deparray(Repo *repo, Id entry, Id keyname, Queue *q, Id marker)
{
  int r = repo_lookup_idarray(repo, entry, keyname, q);
  if (r && marker && q->count)
    {
      int i;
      if (marker < 0)
	{
	  marker = -marker;
	  for (i = 0; i < q->count; i++)
	    if (q->elements[i] == marker)
	      {
		queue_truncate(q, i);
		return r;
	      }
	}
      else
	{
	  for (i = 0; i < q->count; i++)
	    if (q->elements[i] == marker)
	      {
		queue_deleten(q, 0, i + 1);
		return r;
	      }
	  queue_empty(q);
	}
    }
  return r;
}

const unsigned char *
repo_lookup_bin_checksum(Repo *repo, Id entry, Id keyname, Id *typep)
{
  Repodata *data;
  int i;
  const unsigned char *chk;

  FOR_REPODATAS(repo, i, data)
    {
      if (entry != SOLVID_META && (entry < data->start || entry >= data->end))
	continue;
      if (!repodata_precheck_keyname(data, keyname))
	continue;
      chk = repodata_lookup_bin_checksum(data, entry, keyname, typep);
      if (chk)
	return chk;
      if (repodata_lookup_type(data, entry, keyname))
	return 0;
    }
  *typep = 0;
  return 0;
}

const char *
repo_lookup_checksum(Repo *repo, Id entry, Id keyname, Id *typep)
{
  const unsigned char *chk = repo_lookup_bin_checksum(repo, entry, keyname, typep);
  return chk ? pool_bin2hex(repo->pool, chk, solv_chksum_len(*typep)) : 0;
}

int
repo_lookup_void(Repo *repo, Id entry, Id keyname)
{
  Repodata *data;
  int i;
  Id type;

  FOR_REPODATAS(repo, i, data)
    {
      if (entry != SOLVID_META && (entry < data->start || entry >= data->end))
	continue;
      if (!repodata_precheck_keyname(data, keyname))
	continue;
      type = repodata_lookup_type(data, entry, keyname);
      if (type)
	return type == REPOKEY_TYPE_VOID;
    }
  return 0;
}

Id
repo_lookup_type(Repo *repo, Id entry, Id keyname)
{
  Repodata *data;
  int i;
  Id type;

  FOR_REPODATAS(repo, i, data)
    {
      if (entry != SOLVID_META && (entry < data->start || entry >= data->end))
	continue;
      if (!repodata_precheck_keyname(data, keyname))
	continue;
      type = repodata_lookup_type(data, entry, keyname);
      if (type)
	return type == REPOKEY_TYPE_DELETED ? 0 : type;
    }
  return 0;
}

/***********************************************************************/

Repodata *
repo_add_repodata(Repo *repo, int flags)
{
  Repodata *data;
  int i;
  if ((flags & REPO_USE_LOADING) != 0)
    {
      for (i = repo->nrepodata - 1; i > 0; i--)
	if (repo->repodata[i].state == REPODATA_LOADING)
	  {
	    Repodata *data = repo->repodata + i;
	    /* re-init */
	    /* hack: we mis-use REPO_REUSE_REPODATA here */
	    if (!(flags & REPO_REUSE_REPODATA))
	      repodata_empty(data, (flags & REPO_LOCALPOOL) ? 1 : 0);
	    return data;
	  }
      return 0;	/* must not create a new repodata! */
    }
  if ((flags & REPO_REUSE_REPODATA) != 0)
    {
      for (i = repo->nrepodata - 1; i > 0; i--)
	if (repo->repodata[i].state != REPODATA_STUB)
	  return repo->repodata + i;
    }
  if (!repo->nrepodata)
    {    
      repo->nrepodata = 2;      /* start with id 1 */
      repo->repodata = solv_calloc(repo->nrepodata, sizeof(*data));
    }    
  else 
    {    
      repo->nrepodata++;
      repo->repodata = solv_realloc2(repo->repodata, repo->nrepodata, sizeof(*data));
    }    
  data = repo->repodata + repo->nrepodata - 1; 
  repodata_initdata(data, repo, (flags & REPO_LOCALPOOL) ? 1 : 0);
  return data;
}

Repodata *
repo_id2repodata(Repo *repo, Id id)
{
  return id ? repo->repodata + id : 0;
}

Repodata *
repo_last_repodata(Repo *repo)
{
  int i;
  for (i = repo->nrepodata - 1; i > 0; i--)
    if (repo->repodata[i].state != REPODATA_STUB)
      return repo->repodata + i;
  return repo_add_repodata(repo, 0);
}

void
repo_set_id(Repo *repo, Id p, Id keyname, Id id)
{
  Repodata *data;
  if (p >= 0)
    {
      switch (keyname)
	{
	case SOLVABLE_NAME:
	  repo->pool->solvables[p].name = id;
	  return;
	case SOLVABLE_ARCH:
	  repo->pool->solvables[p].arch = id;
	  return;
	case SOLVABLE_EVR:
	  repo->pool->solvables[p].evr = id;
	  return;
	case SOLVABLE_VENDOR:
	  repo->pool->solvables[p].vendor = id;
	  return;
	}
    }
  data = repo_last_repodata(repo);
  if (data->localpool)
    id = repodata_localize_id(data, id, 1);
  repodata_set_id(data, p, keyname, id);
}

void
repo_set_num(Repo *repo, Id p, Id keyname, unsigned long long num)
{
  Repodata *data;
  if (p >= 0)
    {
      if (keyname == RPM_RPMDBID)
	{
	  if (!repo->rpmdbid)
	    repo->rpmdbid = repo_sidedata_create(repo, sizeof(Id));
	  repo->rpmdbid[p - repo->start] = num;
	  return;
	}
    }
  data = repo_last_repodata(repo);
  repodata_set_num(data, p, keyname, num);
}

void
repo_set_str(Repo *repo, Id p, Id keyname, const char *str)
{
  Repodata *data;
  if (p >= 0)
    {
      switch (keyname)
	{
	case SOLVABLE_NAME:
	case SOLVABLE_ARCH:
	case SOLVABLE_EVR:
	case SOLVABLE_VENDOR:
	  repo_set_id(repo, p, keyname, pool_str2id(repo->pool, str, 1));
	  return;
	}
    }
  data = repo_last_repodata(repo);
  repodata_set_str(data, p, keyname, str);
}

void
repo_set_poolstr(Repo *repo, Id p, Id keyname, const char *str)
{
  Repodata *data;
  if (p >= 0)
    {
      switch (keyname)
	{
	case SOLVABLE_NAME:
	case SOLVABLE_ARCH:
	case SOLVABLE_EVR:
	case SOLVABLE_VENDOR:
	  repo_set_id(repo, p, keyname, pool_str2id(repo->pool, str, 1));
	  return;
	}
    }
  data = repo_last_repodata(repo);
  repodata_set_poolstr(data, p, keyname, str);
}

void
repo_add_poolstr_array(Repo *repo, Id p, Id keyname, const char *str)
{
  Repodata *data = repo_last_repodata(repo);
  repodata_add_poolstr_array(data, p, keyname, str);
}

void
repo_add_deparray(Repo *repo, Id p, Id keyname, Id dep, Id marker)
{
  Repodata *data;
  if (p >= 0)
    {
      Solvable *s = repo->pool->solvables + p;
      switch (keyname)
	{
	case SOLVABLE_PROVIDES:
	  s->provides = repo_addid_dep(repo, s->provides, dep, marker);
	  return;
	case SOLVABLE_OBSOLETES:
	  s->obsoletes = repo_addid_dep(repo, s->obsoletes, dep, marker);
	  return;
	case SOLVABLE_CONFLICTS:
	  s->conflicts = repo_addid_dep(repo, s->conflicts, dep, marker);
	  return;
	case SOLVABLE_REQUIRES:
	  s->requires = repo_addid_dep(repo, s->requires, dep, marker);
	  return;
	case SOLVABLE_RECOMMENDS:
	  s->recommends = repo_addid_dep(repo, s->recommends, dep, marker);
	  return;
	case SOLVABLE_SUGGESTS:
	  s->suggests = repo_addid_dep(repo, s->suggests, dep, marker);
	  return;
	case SOLVABLE_SUPPLEMENTS:
	  s->supplements = repo_addid_dep(repo, s->supplements, dep, marker);
	  return;
	case SOLVABLE_ENHANCES:
	  s->enhances = repo_addid_dep(repo, s->enhances, dep, marker);
	  return;
	}
    }
  data = repo_last_repodata(repo);
  repodata_add_idarray(data, p, keyname, dep);
}

void
repo_add_idarray(Repo *repo, Id p, Id keyname, Id id)
{
  repo_add_deparray(repo, p, keyname, id, 0);
}

static Offset
repo_set_idarray_solvable(Repo *repo, Queue *q)
{
  Offset o = 0;
  int i;
  for (i = 0; i < q->count; i++)
    repo_addid_dep(repo, o, q->elements[i], 0);
  return o;
}

void
repo_set_deparray(Repo *repo, Id p, Id keyname, Queue *q, Id marker)
{
  Repodata *data;
  if (marker)
    {
      /* complex case, splice old and new arrays */
      int i;
      Queue q2;
      queue_init(&q2);
      repo_lookup_deparray(repo, p, keyname, &q2, -marker);
      if (marker > 0)
	{
	  if (q->count)
	    {
	      queue_push(&q2, marker);
	      for (i = 0; i < q->count; i++)
		queue_push(&q2, q->elements[i]);
	    }
	}
      else
	{
	  if (q2.count)
	    queue_insert(&q2, 0, -marker);
	  queue_insertn(&q2, 0, q->count, q->elements);
	}
      repo_set_deparray(repo, p, keyname, &q2, 0);
      queue_free(&q2);
      return;
    }
  if (p >= 0)
    {
      Solvable *s = repo->pool->solvables + p;
      switch (keyname)
	{
	case SOLVABLE_PROVIDES:
	  s->provides = repo_set_idarray_solvable(repo, q);
	  return;
	case SOLVABLE_OBSOLETES:
	  s->obsoletes = repo_set_idarray_solvable(repo, q);
	  return;
	case SOLVABLE_CONFLICTS:
	  s->conflicts = repo_set_idarray_solvable(repo, q);
	  return;
	case SOLVABLE_REQUIRES:
	  s->requires = repo_set_idarray_solvable(repo, q);
	  return;
	case SOLVABLE_RECOMMENDS:
	  s->recommends = repo_set_idarray_solvable(repo, q);
	  return;
	case SOLVABLE_SUGGESTS:
	  s->suggests = repo_set_idarray_solvable(repo, q);
	  return;
	case SOLVABLE_SUPPLEMENTS:
	  s->supplements = repo_set_idarray_solvable(repo, q);
	  return;
	case SOLVABLE_ENHANCES:
	  s->enhances = repo_set_idarray_solvable(repo, q);
	  return;
	}
    }
  data = repo_last_repodata(repo);
  repodata_set_idarray(data, p, keyname, q);
}

void
repo_set_idarray(Repo *repo, Id p, Id keyname, Queue *q)
{
  repo_set_deparray(repo, p, keyname, q, 0);
}

void
repo_unset(Repo *repo, Id p, Id keyname)
{
  Repodata *data;
  if (p >= 0)
    {
      Solvable *s = repo->pool->solvables + p;
      switch (keyname)
	{
	case SOLVABLE_NAME:
	  s->name = 0;
	  return;
	case SOLVABLE_ARCH:
	  s->arch = 0;
	  return;
	case SOLVABLE_EVR:
	  s->evr = 0;
	  return;
	case SOLVABLE_VENDOR:
	  s->vendor = 0;
	  return;
        case RPM_RPMDBID:
	  if (repo->rpmdbid)
	    repo->rpmdbid[p - repo->start] = 0;
	  return;
	case SOLVABLE_PROVIDES:
	  s->provides = 0;
	  return;
	case SOLVABLE_OBSOLETES:
	  s->obsoletes = 0;
	  return;
	case SOLVABLE_CONFLICTS:
	  s->conflicts = 0;
	  return;
	case SOLVABLE_REQUIRES:
	  s->requires = 0;
	  return;
	case SOLVABLE_RECOMMENDS:
	  s->recommends = 0;
	  return;
	case SOLVABLE_SUGGESTS:
	  s->suggests = 0;
	  return;
	case SOLVABLE_SUPPLEMENTS:
	  s->supplements = 0;
	case SOLVABLE_ENHANCES:
	  s->enhances = 0;
	  return;
	default:
	  break;
	}
    }
  data = repo_last_repodata(repo);
  repodata_unset(data, p, keyname);
}

void
repo_internalize(Repo *repo)
{
  int i;
  Repodata *data;

  FOR_REPODATAS(repo, i, data)
    if (data->attrs || data->xattrs)
      repodata_internalize(data);
}

void
repo_disable_paging(Repo *repo)
{
  int i;
  Repodata *data;

  FOR_REPODATAS(repo, i, data)
    repodata_disable_paging(data);
}

/*
vim:cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-0.5s:tw=78:cindent:sw=4:
*/
