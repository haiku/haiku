/* Binary relations.
   Copyright (C) 2002, 2004 Free Software Foundation, Inc.

   This file is part of Bison, the GNU Compiler Compiler.

   Bison is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   Bison is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bison; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "system.h"

#include <bitsetv.h>

#include "getargs.h"
#include "relation.h"

void
relation_print (relation r, size_t size, FILE *out)
{
  unsigned int i;
  unsigned int j;

  for (i = 0; i < size; ++i)
    {
      fprintf (out, "%3d: ", i);
      if (r[i])
	for (j = 0; r[i][j] != -1; ++j)
	  fprintf (out, "%3d ", r[i][j]);
      fputc ('\n', out);
    }
  fputc ('\n', out);
}


/*---------------------------------------------------------------.
| digraph & traverse.                                            |
|                                                                |
| The following variables are used as common storage between the |
| two.                                                           |
`---------------------------------------------------------------*/

static relation R;
static relation_nodes INDEX;
static relation_nodes VERTICES;
static int top;
static int infinity;
static bitsetv F;

static void
traverse (int i)
{
  int j;
  int height;

  VERTICES[++top] = i;
  INDEX[i] = height = top;

  if (R[i])
    for (j = 0; R[i][j] >= 0; ++j)
      {
	if (INDEX[R[i][j]] == 0)
	  traverse (R[i][j]);

	if (INDEX[i] > INDEX[R[i][j]])
	  INDEX[i] = INDEX[R[i][j]];

	bitset_or (F[i], F[i], F[R[i][j]]);
      }

  if (INDEX[i] == height)
    for (;;)
      {
	j = VERTICES[top--];
	INDEX[j] = infinity;

	if (i == j)
	  break;

	bitset_copy (F[j], F[i]);
      }
}


void
relation_digraph (relation r, size_t size, bitsetv *function)
{
  unsigned int i;

  infinity = size + 2;
  CALLOC (INDEX, size + 1);
  CALLOC (VERTICES, size + 1);
  top = 0;

  R = r;
  F = *function;

  for (i = 0; i < size; i++)
    INDEX[i] = 0;

  for (i = 0; i < size; i++)
    if (INDEX[i] == 0 && R[i])
      traverse (i);

  XFREE (INDEX);
  XFREE (VERTICES);

  *function = F;
}


/*-------------------------------------------.
| Destructively transpose R_ARG, of size N.  |
`-------------------------------------------*/

void
relation_transpose (relation *R_arg, int n)
{
  /* The result. */
  relation new_R = CALLOC (new_R, n);
  /* END_R[I] -- next entry of NEW_R[I]. */
  relation end_R = CALLOC (end_R, n);
  /* NEDGES[I] -- total size of NEW_R[I]. */
  int *nedges = CALLOC (nedges, n);
  int i, j;

  if (trace_flag & trace_sets)
    {
      fputs ("relation_transpose: input\n", stderr);
      relation_print (*R_arg, n, stderr);
    }

  /* Count. */
  for (i = 0; i < n; i++)
    if ((*R_arg)[i])
      for (j = 0; (*R_arg)[i][j] >= 0; ++j)
	++nedges[(*R_arg)[i][j]];

  /* Allocate. */
  for (i = 0; i < n; i++)
    if (nedges[i] > 0)
      {
	relation_node *sp = CALLOC (sp, nedges[i] + 1);
	sp[nedges[i]] = -1;
	new_R[i] = sp;
	end_R[i] = sp;
      }

  /* Store. */
  for (i = 0; i < n; i++)
    if ((*R_arg)[i])
      for (j = 0; (*R_arg)[i][j] >= 0; ++j)
	{
	  *end_R[(*R_arg)[i][j]] = i;
	  ++end_R[(*R_arg)[i][j]];
	}

  free (nedges);
  free (end_R);

  /* Free the input: it is replaced with the result. */
  for (i = 0; i < n; i++)
    XFREE ((*R_arg)[i]);
  free (*R_arg);

  if (trace_flag & trace_sets)
    {
      fputs ("relation_transpose: output\n", stderr);
      relation_print (new_R, n, stderr);
    }

  *R_arg = new_R;
}
