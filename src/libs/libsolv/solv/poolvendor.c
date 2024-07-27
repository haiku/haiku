/*
 * Copyright (c) 2007, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* we need FNM_CASEFOLD */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <fnmatch.h>

#include "pool.h"
#include "poolid.h"
#include "poolvendor.h"
#include "util.h"

/*
 *  const char *vendorsclasses[] = {
 *    "!openSUSE Build Service*",
 *    "SUSE*",
 *    "openSUSE*",
 *    "SGI*",
 *    "Novell*",
 *    "Silicon Graphics*",
 *    "Jpackage Project*",
 *    "ATI Technologies Inc.*",
 *    "Nvidia*",
 *    0,
 *    0,
 *  };
 */

/* allows for 32 different vendor classes */

Id pool_vendor2mask(Pool *pool, Id vendor)
{
  const char *vstr;
  int i;
  Id mask, m;
  const char **v, *vs;

  if (vendor == 0 || !pool->vendorclasses)
    return 0;
  for (i = 0; i < pool->vendormap.count; i += 2)
    if (pool->vendormap.elements[i] == vendor)
      return pool->vendormap.elements[i + 1];
  vstr = pool_id2str(pool, vendor);
  m = 1;
  mask = 0;
  for (v = pool->vendorclasses; ; v++)
    {
      vs = *v;
      if (vs == 0)	/* end of block? */
	{
	  v++;
	  if (*v == 0)
	    break;
	  if (m == (1 << 31))
	    break;	/* sorry, out of bits */
	  m <<= 1;	/* next vendor equivalence class */
	}
      if (fnmatch(*vs == '!' ? vs + 1 : vs, vstr, FNM_CASEFOLD) == 0)
	{
	  if (*vs != '!')
	    mask |= m;
	  while (v[1])	/* forward to next block */
	    v++;
	}
    }
  queue_push(&pool->vendormap, vendor);
  queue_push(&pool->vendormap, mask);
  return mask;
}

void
pool_setvendorclasses(Pool *pool, const char **vendorclasses)
{
  int i;
  const char **v;

  if (pool->vendorclasses)
    {
      for (v = pool->vendorclasses; v[0] || v[1]; v++)
	solv_free((void *)*v);
      pool->vendorclasses = solv_free(pool->vendorclasses);
    }
  if (!vendorclasses || !vendorclasses[0])
    return;
  for (v = vendorclasses; v[0] || v[1]; v++)
    ;
  pool->vendorclasses = solv_calloc(v - vendorclasses + 2, sizeof(const char *));
  for (v = vendorclasses, i = 0; v[0] || v[1]; v++, i++)
    pool->vendorclasses[i] = *v ? solv_strdup(*v) : 0;
  pool->vendorclasses[i++] = 0;
  pool->vendorclasses[i] = 0;
  queue_empty(&pool->vendormap);
}

void
pool_addvendorclass(Pool *pool, const char **vendorclass)
{
  int i, j;

  if (!vendorclass || !vendorclass[0])
    return;
  for (j = 1; vendorclass[j]; j++)
    ;
  i = 0;
  if (pool->vendorclasses)
    {
      for (i = 0; pool->vendorclasses[i] || pool->vendorclasses[i + 1]; i++)
	;
      if (i)
        i++;
    }
  pool->vendorclasses = solv_realloc2(pool->vendorclasses, i + j + 2, sizeof(const char *));
  for (j = 0; vendorclass[j]; j++)
    pool->vendorclasses[i++] = solv_strdup(vendorclass[j]);
  pool->vendorclasses[i++] = 0;
  pool->vendorclasses[i] = 0;
  queue_empty(&pool->vendormap);
}
