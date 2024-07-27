/*
 * Copyright (c) 2007, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

#include <string.h>
#include "util.h"
#include "strpool.h"

#define STRING_BLOCK      2047
#define STRINGSPACE_BLOCK 65535

void
stringpool_init(Stringpool *ss, const char *strs[])
{
  unsigned totalsize = 0;
  unsigned count;

  memset(ss, 0, sizeof(*ss));
  /* count number and total size of predefined strings */
  for (count = 0; strs[count]; count++)
    totalsize += strlen(strs[count]) + 1;

  /* alloc appropriate space */
  ss->stringspace = solv_extend_resize(0, totalsize, 1, STRINGSPACE_BLOCK);
  ss->strings = solv_extend_resize(0, count, sizeof(Offset), STRING_BLOCK);

  /* now copy predefined strings into allocated space */
  ss->sstrings = 0;
  for (count = 0; strs[count]; count++)
    {
      strcpy(ss->stringspace + ss->sstrings, strs[count]);
      ss->strings[count] = ss->sstrings;
      ss->sstrings += strlen(strs[count]) + 1;
    }
  ss->nstrings = count;
}

void
stringpool_free(Stringpool *ss)
{
  solv_free(ss->strings);
  solv_free(ss->stringspace);
  solv_free(ss->stringhashtbl);
}

void
stringpool_freehash(Stringpool *ss)
{
  ss->stringhashtbl = solv_free(ss->stringhashtbl);
  ss->stringhashmask = 0;
}

void
stringpool_init_empty(Stringpool *ss)
{
  const char *emptystrs[] = {
    "<NULL>",
    "",
    0,
  };
  stringpool_init(ss, emptystrs);
}

void
stringpool_clone(Stringpool *ss, Stringpool *from)
{
  memset(ss, 0, sizeof(*ss));
  ss->strings = solv_extend_resize(0, from->nstrings, sizeof(Offset), STRING_BLOCK);
  memcpy(ss->strings, from->strings, from->nstrings * sizeof(Offset));
  ss->stringspace = solv_extend_resize(0, from->sstrings, 1, STRINGSPACE_BLOCK);
  memcpy(ss->stringspace, from->stringspace, from->sstrings);
  ss->nstrings = from->nstrings;
  ss->sstrings = from->sstrings;
}

Id
stringpool_strn2id(Stringpool *ss, const char *str, unsigned int len, int create)
{
  Hashval h, hh, hashmask, oldhashmask;
  int i;
  Id id;
  Hashtable hashtbl;

  if (!str)
    return STRID_NULL;
  if (!len)
    return STRID_EMPTY;

  hashmask = oldhashmask = ss->stringhashmask;
  hashtbl = ss->stringhashtbl;

  /* expand hashtable if needed */
  if (ss->nstrings * 2 > hashmask)
    {
      solv_free(hashtbl);

      /* realloc hash table */
      ss->stringhashmask = hashmask = mkmask(ss->nstrings + STRING_BLOCK);
      ss->stringhashtbl = hashtbl = (Hashtable)solv_calloc(hashmask + 1, sizeof(Id));

      /* rehash all strings into new hashtable */
      for (i = 1; i < ss->nstrings; i++)
	{
	  h = strhash(ss->stringspace + ss->strings[i]) & hashmask;
	  hh = HASHCHAIN_START;
	  while (hashtbl[h] != 0)
	    h = HASHCHAIN_NEXT(h, hh, hashmask);
	  hashtbl[h] = i;
	}
    }

  /* compute hash and check for match */
  h = strnhash(str, len) & hashmask;
  hh = HASHCHAIN_START;
  while ((id = hashtbl[h]) != 0)
    {
      if(!memcmp(ss->stringspace + ss->strings[id], str, len)
         && ss->stringspace[ss->strings[id] + len] == 0)
	break;
      h = HASHCHAIN_NEXT(h, hh, hashmask);
    }
  if (id || !create)    /* exit here if string found */
    return id;

  /* this should be a test for a flag that tells us if the 
   * correct blocking is used, but adding a flag would break
   * the ABI. So we use the existance of the hash area as
   * indication instead */
  if (!oldhashmask)
    {
      ss->stringspace = solv_extend_resize(ss->stringspace, ss->sstrings + len + 1, 1, STRINGSPACE_BLOCK);
      ss->strings = solv_extend_resize(ss->strings, ss->nstrings + 1, sizeof(Offset), STRING_BLOCK);
    }

  /* generate next id and save in table */
  id = ss->nstrings++;
  hashtbl[h] = id;

  ss->strings = solv_extend(ss->strings, id, 1, sizeof(Offset), STRING_BLOCK);
  ss->strings[id] = ss->sstrings;	/* we will append to the end */

  /* append string to stringspace */
  ss->stringspace = solv_extend(ss->stringspace, ss->sstrings, len + 1, 1, STRINGSPACE_BLOCK);
  memcpy(ss->stringspace + ss->sstrings, str, len);
  ss->stringspace[ss->sstrings + len] = 0;
  ss->sstrings += len + 1;
  return id;
}

Id
stringpool_str2id(Stringpool *ss, const char *str, int create)
{
  if (!str)
    return STRID_NULL;
  if (!*str)
    return STRID_EMPTY;
  return stringpool_strn2id(ss, str, (unsigned int)strlen(str), create);
}

void
stringpool_shrink(Stringpool *ss)
{
  ss->stringspace = solv_extend_resize(ss->stringspace, ss->sstrings, 1, STRINGSPACE_BLOCK);
  ss->strings = solv_extend_resize(ss->strings, ss->nstrings, sizeof(Offset), STRING_BLOCK);
}
