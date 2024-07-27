/*
 * Copyright (c) 2008, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

#include <stdio.h>
#include <string.h>

#include "pool.h"
#include "util.h"
#include "dirpool.h"

#define DIR_BLOCK 127

/* directories are stored as components,
 * components are simple ids from the string pool
 *   /usr/bin   ->  "", "usr", "bin"
 *   /usr/lib   ->  "", "usr", "lib"
 *   foo/bar    ->  "foo", "bar"
 *   /usr/games ->  "", "usr", "games"
 *
 * all directories are stores in the "dirs" array
 *   dirs[id] > 0 : component string pool id
 *   dirs[id] <= 0 : -(parent directory id)
 *
 * Directories with the same parent are stored as
 * multiple blocks. We need multiple blocks because
 * we cannot insert entries into old blocks, as that
 * would shift the ids of already used directories.
 * Each block starts with (-parent_dirid) and contains
 * component ids of the directory entries.
 * (The (-parent_dirid) entry is not a valid directory
 * id, it's just used internally)
 *
 * There is also the aux "dirtraverse" array, which
 * is created on demand to speed things up a bit.
 * if dirs[id] > 0, dirtravers[id] points to the first
 * entry in the last block with parent id.
 * if dirs[id] <= 0, dirtravers[id] points to the entry
 * in the previous block with the same parent.
 * (Thus it acts as a linked list that starts at the
 * parent dirid and chains all the blocks with that
 * parent.)
 *
 *  id    dirs[id]  dirtraverse[id]
 *   0     0           8       [no parent, block#0]
 *   1    ""           3
 *   2    -1                   [parent 1, /, block #0]
 *   3    "usr"       12
 *   4    -3                   [parent 3, /usr, block #0]
 *   5    "bin"
 *   6    "lib"
 *   7     0           1       [no parent, block#1]
 *   8    "foo"       10
 *   9    -8                   [parent 8, foo, block #0]
 *  10    "bar"
 *  11    -3           5       [parent 3, /usr, block #1]
 *  12    "games"
 *   
 * to find all children of dirid 3 ("/usr"), follow the
 * dirtraverse link to 12 -> "games". Then follow the
 * dirtraverse link of this block to 5 -> "bin", "lib"
 */

void
dirpool_init(Dirpool *dp)
{
  memset(dp, 0, sizeof(*dp));
}

void
dirpool_free(Dirpool *dp)
{
  solv_free(dp->dirs);
  solv_free(dp->dirtraverse);
}

void
dirpool_make_dirtraverse(Dirpool *dp)
{
  Id parent, i, *dirtraverse;
  if (!dp->ndirs)
    return;
  dp->dirs = solv_extend_resize(dp->dirs, dp->ndirs, sizeof(Id), DIR_BLOCK);
  dirtraverse = solv_calloc_block(dp->ndirs, sizeof(Id), DIR_BLOCK);
  for (parent = 0, i = 0; i < dp->ndirs; i++)
    {
      if (dp->dirs[i] > 0)
	continue;
      parent = -dp->dirs[i];
      dirtraverse[i] = dirtraverse[parent];
      dirtraverse[parent] = i + 1;
    }
  dp->dirtraverse = dirtraverse;
}

Id
dirpool_add_dir(Dirpool *dp, Id parent, Id comp, int create)
{
  Id did, d, ds, *dirtraverse;

  if (!dp->ndirs)
    {
      if (!create)
	return 0;
      dp->ndirs = 2;
      dp->dirs = solv_extend_resize(dp->dirs, dp->ndirs, sizeof(Id), DIR_BLOCK);
      dp->dirs[0] = 0;
      dp->dirs[1] = 1;	/* "" */
    }
  if (parent == 0 && comp == 1)
    return 1;
  if (!dp->dirtraverse)
    dirpool_make_dirtraverse(dp);
  /* check all entries with this parent if we
   * already have this component */
  dirtraverse = dp->dirtraverse;
  ds = dirtraverse[parent];
  while (ds)
    {
      /* ds: first component in this block
       * ds-1: parent link */
      for (d = ds--; d < dp->ndirs; d++)
	{
	  if (dp->dirs[d] == comp)
	    return d;
	  if (dp->dirs[d] <= 0)	/* reached end of this block */
	    break;
	}
      if (ds)
        ds = dp->dirtraverse[ds];
    }
  if (!create)
    return 0;
  /* a new one, find last parent */
  for (did = dp->ndirs - 1; did > 0; did--)
    if (dp->dirs[did] <= 0)
      break;
  if (dp->dirs[did] != -parent)
    {
      /* make room for parent entry */
      dp->dirs = solv_extend(dp->dirs, dp->ndirs, 1, sizeof(Id), DIR_BLOCK);
      dp->dirtraverse = solv_extend(dp->dirtraverse, dp->ndirs, 1, sizeof(Id), DIR_BLOCK);
      /* new parent block, link in */
      dp->dirs[dp->ndirs] = -parent;
      dp->dirtraverse[dp->ndirs] = dp->dirtraverse[parent];
      dp->dirtraverse[parent] = ++dp->ndirs;
    }
  /* make room for new entry */
  dp->dirs = solv_extend(dp->dirs, dp->ndirs, 1, sizeof(Id), DIR_BLOCK);
  dp->dirtraverse = solv_extend(dp->dirtraverse, dp->ndirs, 1, sizeof(Id), DIR_BLOCK);
  dp->dirs[dp->ndirs] = comp;
  dp->dirtraverse[dp->ndirs] = 0;
  return dp->ndirs++;
}
