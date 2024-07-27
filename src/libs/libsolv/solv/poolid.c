/*
 * Copyright (c) 2007, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/*
 * poolid.c
 *
 * Id management
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "pool.h"
#include "poolid.h"
#include "poolid_private.h"
#include "util.h"


/* intern string into pool, return id */

Id
pool_str2id(Pool *pool, const char *str, int create)
{
  int oldnstrings = pool->ss.nstrings;
  Id id = stringpool_str2id(&pool->ss, str, create);
  if (create && pool->whatprovides && oldnstrings != pool->ss.nstrings && (id & WHATPROVIDES_BLOCK) == 0)
    {
      /* grow whatprovides array */
      pool->whatprovides = solv_realloc(pool->whatprovides, (id + (WHATPROVIDES_BLOCK + 1)) * sizeof(Offset));
      memset(pool->whatprovides + id, 0, (WHATPROVIDES_BLOCK + 1) * sizeof(Offset));
    }
  return id;
}

Id
pool_strn2id(Pool *pool, const char *str, unsigned int len, int create)
{
  int oldnstrings = pool->ss.nstrings;
  Id id = stringpool_strn2id(&pool->ss, str, len, create);
  if (create && pool->whatprovides && oldnstrings != pool->ss.nstrings && (id & WHATPROVIDES_BLOCK) == 0)
    {
      /* grow whatprovides array */
      pool->whatprovides = solv_realloc(pool->whatprovides, (id + (WHATPROVIDES_BLOCK + 1)) * sizeof(Offset));
      memset(pool->whatprovides + id, 0, (WHATPROVIDES_BLOCK + 1) * sizeof(Offset));
    }
  return id;
}

Id
pool_rel2id(Pool *pool, Id name, Id evr, int flags, int create)
{
  Hashval h, hh, hashmask;
  int i;
  Id id;
  Hashtable hashtbl;
  Reldep *ran;

  hashmask = pool->relhashmask;
  hashtbl = pool->relhashtbl;
  ran = pool->rels;
  
  /* extend hashtable if needed */
  if (pool->nrels * 2 > hashmask)
    {
      solv_free(pool->relhashtbl);
      pool->relhashmask = hashmask = mkmask(pool->nrels + REL_BLOCK);
      pool->relhashtbl = hashtbl = solv_calloc(hashmask + 1, sizeof(Id));
      /* rehash all rels into new hashtable */
      for (i = 1; i < pool->nrels; i++)
	{
	  h = relhash(ran[i].name, ran[i].evr, ran[i].flags) & hashmask;
	  hh = HASHCHAIN_START;
	  while (hashtbl[h])
	    h = HASHCHAIN_NEXT(h, hh, hashmask);
	  hashtbl[h] = i;
	}
    }
  
  /* compute hash and check for match */
  h = relhash(name, evr, flags) & hashmask;
  hh = HASHCHAIN_START;
  while ((id = hashtbl[h]) != 0)
    {
      if (ran[id].name == name && ran[id].evr == evr && ran[id].flags == flags)
	break;
      h = HASHCHAIN_NEXT(h, hh, hashmask);
    }
  if (id)
    return MAKERELDEP(id);

  if (!create)
    return ID_NULL;

  id = pool->nrels++;
  /* extend rel space if needed */
  pool->rels = solv_extend(pool->rels, id, 1, sizeof(Reldep), REL_BLOCK);
  hashtbl[h] = id;
  ran = pool->rels + id;
  ran->name = name;
  ran->evr = evr;
  ran->flags = flags;

  /* extend whatprovides_rel if needed */
  if (pool->whatprovides_rel && (id & WHATPROVIDES_BLOCK) == 0)
    {
      pool->whatprovides_rel = solv_realloc2(pool->whatprovides_rel, id + (WHATPROVIDES_BLOCK + 1), sizeof(Offset));
      memset(pool->whatprovides_rel + id, 0, (WHATPROVIDES_BLOCK + 1) * sizeof(Offset));
    }
  return MAKERELDEP(id);
}


/* Id -> String
 * for rels (returns name only) and strings
 */ 
const char *
pool_id2str(const Pool *pool, Id id)
{
  if (ISRELDEP(id))
    {
      Reldep *rd = GETRELDEP(pool, id);
      if (ISRELDEP(rd->name))
	return "REL";
      return pool->ss.stringspace + pool->ss.strings[rd->name];
    }
  return pool->ss.stringspace + pool->ss.strings[id];
}

static const char *rels[] = {
  " ! ",
  " > ",
  " = ",
  " >= ",
  " < ",
  " <> ",
  " <= ",
  " <=> "
};


/* get operator for RelId */
const char *
pool_id2rel(const Pool *pool, Id id)
{
  Reldep *rd;
  if (!ISRELDEP(id))
    return "";
  rd = GETRELDEP(pool, id);
  switch (rd->flags)
    {
    case 0: case 2: case 3:
    case 6: case 7:
#if !defined(DEBIAN) && !defined(MULTI_SEMANTICS)
    case 1: case 4:
#endif
#if !defined(HAIKU) && !defined(MULTI_SEMANTICS)
    case 5:
#endif
      return rels[rd->flags];
#if defined(DEBIAN) || defined(MULTI_SEMANTICS)
    case 1:
      return pool->disttype == DISTTYPE_DEB ? " >> " : rels[rd->flags];
    case 4:
      return pool->disttype == DISTTYPE_DEB ? " << " : rels[rd->flags];
#endif
#if defined(HAIKU) || defined(MULTI_SEMANTICS)
    case 5:
      return pool->disttype == DISTTYPE_HAIKU ? " != " : rels[rd->flags];
#endif
    case REL_AND:
      return " & ";
    case REL_OR:
      return " | ";
    case REL_WITH:
      return " + ";
    case REL_NAMESPACE:
      return " NAMESPACE ";	/* actually not used in dep2str */
    case REL_ARCH:
      return ".";
    case REL_FILECONFLICT:
      return " FILECONFLICT ";
    case REL_COND:
      return " IF ";
    case REL_COMPAT:
      return " compat >= ";
    default:
      break;
    }
  return " ??? ";
}


/* get e:v.r for Id */
const char *
pool_id2evr(const Pool *pool, Id id)
{
  Reldep *rd;
  if (!ISRELDEP(id))
    return "";
  rd = GETRELDEP(pool, id);
  if (ISRELDEP(rd->evr))
    return "(REL)";
  return pool->ss.stringspace + pool->ss.strings[rd->evr];
}

static int
dep2strlen(const Pool *pool, Id id)
{
  int l = 0;

  while (ISRELDEP(id))
    {
      Reldep *rd = GETRELDEP(pool, id);
      /* add 2 for parens */
      l += 2 + dep2strlen(pool, rd->name) + strlen(pool_id2rel(pool, id));
      id = rd->evr;
    }
  return l + strlen(pool->ss.stringspace + pool->ss.strings[id]);
}

static void
dep2strcpy(const Pool *pool, char *p, Id id, int oldrel)
{
  while (ISRELDEP(id))
    {
      Reldep *rd = GETRELDEP(pool, id);
      if (oldrel == REL_AND || oldrel == REL_OR || oldrel == REL_WITH)
	if (rd->flags == REL_AND || rd->flags == REL_OR || rd->flags == REL_WITH)
	  if (oldrel != rd->flags)
	    {
	      *p++ = '(';
	      dep2strcpy(pool, p, rd->name, rd->flags);
	      p += strlen(p);
	      strcpy(p, pool_id2rel(pool, id));
	      p += strlen(p);
	      dep2strcpy(pool, p, rd->evr, rd->flags);
	      strcat(p, ")");
	      return;
	    }
      dep2strcpy(pool, p, rd->name, rd->flags);
      p += strlen(p);
      if (rd->flags == REL_NAMESPACE)
	{
	  *p++ = '(';
	  dep2strcpy(pool, p, rd->evr, rd->flags);
	  strcat(p, ")");
	  return;
	}
      if (rd->flags == REL_FILECONFLICT)
	{
	  *p = 0;
	  return;
	}
      strcpy(p, pool_id2rel(pool, id));
      p += strlen(p);
      id = rd->evr;
      oldrel = rd->flags;
    }
  strcpy(p, pool->ss.stringspace + pool->ss.strings[id]);
}

const char *
pool_dep2str(Pool *pool, Id id)
{
  char *p;
  if (!ISRELDEP(id))
    return pool->ss.stringspace + pool->ss.strings[id];
  p = pool_alloctmpspace(pool, dep2strlen(pool, id) + 1);
  dep2strcpy(pool, p, id, 0);
  return p;
}

void
pool_shrink_strings(Pool *pool)
{
  stringpool_shrink(&pool->ss);
}

void
pool_shrink_rels(Pool *pool)
{
  pool->rels = solv_extend_resize(pool->rels, pool->nrels, sizeof(Reldep), REL_BLOCK);
}

/* free all hash tables */
void
pool_freeidhashes(Pool *pool)
{
  stringpool_freehash(&pool->ss);
  pool->relhashtbl = solv_free(pool->relhashtbl);
  pool->relhashmask = 0;
}

/* EOF */
