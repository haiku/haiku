/* Compute look-ahead criteria for Bison.

   Copyright (C) 1984, 1986, 1989, 2000, 2001, 2002, 2003
   Free Software Foundation, Inc.

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


/* Compute how to make the finite state machine deterministic; find
   which rules need lookahead in each state, and which lookahead
   tokens they accept.  */

#include "system.h"

#include <bitset.h>
#include <bitsetv.h>
#include <quotearg.h>

#include "LR0.h"
#include "complain.h"
#include "derives.h"
#include "getargs.h"
#include "gram.h"
#include "lalr.h"
#include "nullable.h"
#include "reader.h"
#include "relation.h"
#include "symtab.h"

goto_number *goto_map = NULL;
static goto_number ngotos = 0;
state_number *from_state = NULL;
state_number *to_state = NULL;

/* Linked list of goto numbers.  */
typedef struct goto_list
{
  struct goto_list *next;
  goto_number value;
} goto_list;


/* LA is a LR by NTOKENS matrix of bits.  LA[l, i] is 1 if the rule
   LArule[l] is applicable in the appropriate state when the next
   token is symbol i.  If LA[l, i] and LA[l, j] are both 1 for i != j,
   it is a conflict.  */

static bitsetv LA = NULL;
size_t nLA;


/* And for the famous F variable, which name is so descriptive that a
   comment is hardly needed.  <grin>.  */
static bitsetv F = NULL;

static goto_number **includes;
static goto_list **lookback;




static void
set_goto_map (void)
{
  state_number s;
  goto_number *temp_map;

  CALLOC (goto_map, nvars + 1);
  CALLOC (temp_map, nvars + 1);

  ngotos = 0;
  for (s = 0; s < nstates; ++s)
    {
      transitions *sp = states[s]->transitions;
      int i;
      for (i = sp->num - 1; i >= 0 && TRANSITION_IS_GOTO (sp, i); --i)
	{
	  if (ngotos >= GOTO_NUMBER_MAXIMUM)
	    abort ();
	  ngotos++;
	  goto_map[TRANSITION_SYMBOL (sp, i) - ntokens]++;
	}
    }

  {
    int k = 0;
    int i;
    for (i = ntokens; i < nsyms; i++)
      {
	temp_map[i - ntokens] = k;
	k += goto_map[i - ntokens];
      }

    for (i = ntokens; i < nsyms; i++)
      goto_map[i - ntokens] = temp_map[i - ntokens];

    goto_map[nsyms - ntokens] = ngotos;
    temp_map[nsyms - ntokens] = ngotos;
  }

  CALLOC (from_state, ngotos);
  CALLOC (to_state, ngotos);

  for (s = 0; s < nstates; ++s)
    {
      transitions *sp = states[s]->transitions;
      int i;
      for (i = sp->num - 1; i >= 0 && TRANSITION_IS_GOTO (sp, i); --i)
	{
	  int k = temp_map[TRANSITION_SYMBOL (sp, i) - ntokens]++;
	  from_state[k] = s;
	  to_state[k] = sp->states[i]->number;
	}
    }

  XFREE (temp_map);
}



/*----------------------------------------------------------.
| Map a state/symbol pair into its numeric representation.  |
`----------------------------------------------------------*/

static int
map_goto (state_number s0, symbol_number sym)
{
  int high;
  int low;
  int middle;
  state_number s;

  low = goto_map[sym - ntokens];
  high = goto_map[sym - ntokens + 1] - 1;

  for (;;)
    {
      if (high < low)
	abort ();
      middle = (low + high) / 2;
      s = from_state[middle];
      if (s == s0)
	return middle;
      else if (s < s0)
	low = middle + 1;
      else
	high = middle - 1;
    }
}


static void
initialize_F (void)
{
  goto_number **reads = CALLOC (reads, ngotos);
  goto_number *edge = CALLOC (edge, ngotos + 1);
  int nedges = 0;

  int i;

  F = bitsetv_create (ngotos, ntokens, BITSET_FIXED);

  for (i = 0; i < ngotos; i++)
    {
      state_number stateno = to_state[i];
      transitions *sp = states[stateno]->transitions;

      int j;
      FOR_EACH_SHIFT (sp, j)
	bitset_set (F[i], TRANSITION_SYMBOL (sp, j));

      for (; j < sp->num; j++)
	{
	  symbol_number sym = TRANSITION_SYMBOL (sp, j);
	  if (nullable[sym - ntokens])
	    edge[nedges++] = map_goto (stateno, sym);
	}

      if (nedges)
	{
	  CALLOC (reads[i], nedges + 1);
	  memcpy (reads[i], edge, nedges * sizeof (edge[0]));
	  reads[i][nedges] = -1;
	  nedges = 0;
	}
    }

  relation_digraph (reads, ngotos, &F);

  for (i = 0; i < ngotos; i++)
    XFREE (reads[i]);

  XFREE (reads);
  XFREE (edge);
}


static void
add_lookback_edge (state *s, rule *r, int gotono)
{
  int ri = state_reduction_find (s, r);
  goto_list *sp = MALLOC (sp, 1);
  sp->next = lookback[(s->reductions->lookaheads - LA) + ri];
  sp->value = gotono;
  lookback[(s->reductions->lookaheads - LA) + ri] = sp;
}



static void
build_relations (void)
{
  goto_number *edge = CALLOC (edge, ngotos + 1);
  state_number *states1 = CALLOC (states1, ritem_longest_rhs () + 1);
  int i;

  CALLOC (includes, ngotos);

  for (i = 0; i < ngotos; i++)
    {
      int nedges = 0;
      symbol_number symbol1 = states[to_state[i]]->accessing_symbol;
      rule **rulep;

      for (rulep = derives[symbol1 - ntokens]; *rulep; rulep++)
	{
	  bool done;
	  int length = 1;
	  item_number *rp;
	  state *s = states[from_state[i]];
	  states1[0] = s->number;

	  for (rp = (*rulep)->rhs; *rp >= 0; rp++)
	    {
	      s = transitions_to (s->transitions,
				  item_number_as_symbol_number (*rp));
	      states1[length++] = s->number;
	    }

	  if (!s->consistent)
	    add_lookback_edge (s, *rulep, i);

	  length--;
	  done = false;
	  while (!done)
	    {
	      done = true;
	      rp--;
	      /* JF added rp>=ritem &&   I hope to god its right! */
	      if (rp >= ritem && ISVAR (*rp))
		{
		  /* Downcasting from item_number to symbol_number.  */
		  edge[nedges++] = map_goto (states1[--length],
					     item_number_as_symbol_number (*rp));
		  if (nullable[*rp - ntokens])
		    done = false;
		}
	    }
	}

      if (nedges)
	{
	  int j;
	  CALLOC (includes[i], nedges + 1);
	  for (j = 0; j < nedges; j++)
	    includes[i][j] = edge[j];
	  includes[i][nedges] = -1;
	}
    }

  XFREE (edge);
  XFREE (states1);

  relation_transpose (&includes, ngotos);
}



static void
compute_FOLLOWS (void)
{
  int i;

  relation_digraph (includes, ngotos, &F);

  for (i = 0; i < ngotos; i++)
    XFREE (includes[i]);

  XFREE (includes);
}


static void
compute_lookaheads (void)
{
  size_t i;
  goto_list *sp;

  for (i = 0; i < nLA; i++)
    for (sp = lookback[i]; sp; sp = sp->next)
      bitset_or (LA[i], LA[i], F[sp->value]);

  /* Free LOOKBACK. */
  for (i = 0; i < nLA; i++)
    LIST_FREE (goto_list, lookback[i]);

  XFREE (lookback);
  bitsetv_free (F);
}


/*-----------------------------------------------------------.
| Count the number of lookaheads required for S (NLOOKAHEADS |
| member).                                                   |
`-----------------------------------------------------------*/

static int
state_lookaheads_count (state *s)
{
  int k;
  int nlookaheads = 0;
  reductions *rp = s->reductions;
  transitions *sp = s->transitions;

  /* We need a lookahead either to distinguish different
     reductions (i.e., there are two or more), or to distinguish a
     reduction from a shift.  Otherwise, it is straightforward,
     and the state is `consistent'.  */
  if (rp->num > 1
      || (rp->num == 1 && sp->num &&
	  !TRANSITION_IS_DISABLED (sp, 0) && TRANSITION_IS_SHIFT (sp, 0)))
    nlookaheads += rp->num;
  else
    s->consistent = 1;

  for (k = 0; k < sp->num; k++)
    if (!TRANSITION_IS_DISABLED (sp, k) && TRANSITION_IS_ERROR (sp, k))
      {
	s->consistent = 0;
	break;
      }

  return nlookaheads;
}


/*----------------------------------------------.
| Compute LA, NLA, and the lookaheads members.  |
`----------------------------------------------*/

static void
initialize_LA (void)
{
  state_number i;
  bitsetv pLA;

  /* Compute the total number of reductions requiring a lookahead.  */
  nLA = 0;
  for (i = 0; i < nstates; i++)
    nLA += state_lookaheads_count (states[i]);
  /* Avoid having to special case 0.  */
  if (!nLA)
    nLA = 1;

  pLA = LA = bitsetv_create (nLA, ntokens, BITSET_FIXED);
  CALLOC (lookback, nLA);

  /* Initialize the members LOOKAHEADS for each state which reductions
     require lookaheads.  */
  for (i = 0; i < nstates; i++)
    {
      int count = state_lookaheads_count (states[i]);
      if (count)
	{
	  states[i]->reductions->lookaheads = pLA;
	  pLA += count;
	}
    }
}


/*---------------------------------------.
| Output the lookaheads for each state.  |
`---------------------------------------*/

static void
lookaheads_print (FILE *out)
{
  state_number i;
  int j, k;
  fprintf (out, "Lookaheads: BEGIN\n");
  for (i = 0; i < nstates; ++i)
    {
      reductions *reds = states[i]->reductions;
      bitset_iterator iter;
      int nlookaheads = 0;

      if (reds->lookaheads)
	for (k = 0; k < reds->num; ++k)
	  if (reds->lookaheads[k])
	    ++nlookaheads;

      fprintf (out, "State %d: %d lookaheads\n",
	       i, nlookaheads);

      if (reds->lookaheads)
	for (j = 0; j < reds->num; ++j)
	  BITSET_FOR_EACH (iter, reds->lookaheads[j], k, 0)
	  {
	    fprintf (out, "   on %d (%s) -> rule %d\n",
		     k, symbols[k]->tag,
		     reds->rules[j]->number);
	  };
    }
  fprintf (out, "Lookaheads: END\n");
}

void
lalr (void)
{
  initialize_LA ();
  set_goto_map ();
  initialize_F ();
  build_relations ();
  compute_FOLLOWS ();
  compute_lookaheads ();

  if (trace_flag & trace_sets)
    lookaheads_print (stderr);
}


void
lalr_free (void)
{
  state_number s;
  for (s = 0; s < nstates; ++s)
    states[s]->reductions->lookaheads = NULL;
  bitsetv_free (LA);
}
