/* tree.c -- helper functions to build and evaluate the expression tree.
   Copyright (C) 1990, 91, 92, 93, 94, 2000, 2001, 2003, 2004, 2005, 2006, 2007 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "defs.h"
#include "../gnulib/lib/xalloc.h"

#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define _(Text) Text
#endif
#ifdef gettext_noop
# define N_(String) gettext_noop (String)
#else
/* See locate.c for explanation as to why not use (String) */
# define N_(String) String
#endif

static struct predicate *scan_rest PARAMS((struct predicate **input,
				       struct predicate *head,
				       short int prev_prec));
static void merge_pred PARAMS((struct predicate *beg_list, struct predicate *end_list, struct predicate **last_p));
static struct predicate *set_new_parent PARAMS((struct predicate *curr, enum predicate_precedence high_prec, struct predicate **prevp));

/* Return a pointer to a tree that represents the
   expression prior to non-unary operator *INPUT.
   Set *INPUT to point at the next input predicate node.

   Only accepts the following:
   
   <primary>
   expression		[operators of higher precedence]
   <uni_op><primary>
   (arbitrary expression)
   <uni_op>(arbitrary expression)
   
   In other words, you can not start out with a bi_op or close_paren.

   If the following operator (if any) is of a higher precedence than
   PREV_PREC, the expression just nabbed is part of a following
   expression, which really is the expression that should be handed to
   our caller, so get_expr recurses. */

struct predicate *
get_expr (struct predicate **input, short int prev_prec)
{
  struct predicate *next = NULL;

  if (*input == NULL)
    error (1, 0, _("invalid expression"));
  
  switch ((*input)->p_type)
    {
    case NO_TYPE:
      error (1, 0, _("invalid expression"));
      break;

    case BI_OP:
      error (1, 0, _("invalid expression; you have used a binary operator with nothing before it."));
      break;

    case CLOSE_PAREN:
      error (1, 0, _("invalid expression; you have too many ')'"));
      break;

    case PRIMARY_TYPE:
      next = *input;
      *input = (*input)->pred_next;
      break;

    case UNI_OP:
      next = *input;
      *input = (*input)->pred_next;
      next->pred_right = get_expr (input, NEGATE_PREC);
      break;

    case OPEN_PAREN:
      *input = (*input)->pred_next;
      next = get_expr (input, NO_PREC);
      if ((*input == NULL)
	  || ((*input)->p_type != CLOSE_PAREN))
	error (1, 0, _("invalid expression; I was expecting to find a ')' somewhere but did not see one."));
      *input = (*input)->pred_next;	/* move over close */
      break;

    default:
      error (1, 0, _("oops -- invalid expression type!"));
      break;
    }

  /* We now have the first expression and are positioned to check
     out the next operator.  If NULL, all done.  Otherwise, if
     PREV_PREC < the current node precedence, we must continue;
     the expression we just nabbed is more tightly bound to the
     following expression than to the previous one. */
  if (*input == NULL)
    return (next);
  if ((int) (*input)->p_prec > (int) prev_prec)
    {
      next = scan_rest (input, next, prev_prec);
      if (next == NULL)
	error (1, 0, _("invalid expression"));
    }
  return (next);
}

/* Scan across the remainder of a predicate input list starting
   at *INPUT, building the rest of the expression tree to return.
   Stop at the first close parenthesis or the end of the input list.
   Assumes that get_expr has been called to nab the first element
   of the expression tree.
   
   *INPUT points to the current input predicate list element.
   It is updated as we move along the list to point to the
   terminating input element.
   HEAD points to the predicate element that was obtained
   by the call to get_expr.
   PREV_PREC is the precedence of the previous predicate element. */

static struct predicate *
scan_rest (struct predicate **input,
	   struct predicate *head,
	   short int prev_prec)
{
  struct predicate *tree;	/* The new tree we are building. */

  if ((*input == NULL) || ((*input)->p_type == CLOSE_PAREN))
    return (NULL);
  tree = head;
  while ((*input != NULL) && ((int) (*input)->p_prec > (int) prev_prec))
    {
      switch ((*input)->p_type)
	{
	case NO_TYPE:
	case PRIMARY_TYPE:
	case UNI_OP:
	case OPEN_PAREN:
	  /* I'm not sure how we get here, so it is not obvious what
	   * sort of mistakes might give rise to this condition.
	   */
	  error (1, 0, _("invalid expression"));
	  break;

	case BI_OP:
	  (*input)->pred_left = tree;
	  tree = *input;
	  *input = (*input)->pred_next;
	  tree->pred_right = get_expr (input, tree->p_prec);
	  break;

	case CLOSE_PAREN:
	  return tree;

	default:
	  error (1, 0,
		 _("oops -- invalid expression type (%d)!"),
		 (int)(*input)->p_type);
	  break;
	}
    }
  return tree;
}

/* Optimize the ordering of the predicates in the tree.  Rearrange
   them to minimize work.  Strategies:
   * Evaluate predicates that don't need inode information first;
     the predicates are divided into 1 or more groups separated by
     predicates (if any) which have "side effects", such as printing.
     The grouping implements the partial ordering on predicates which
     those with side effects impose.

   * Place -name, -iname, -path, -ipath, -regex and -iregex at the front
     of a group, with -name, -iname, -path and -ipath ahead of
     -regex and -iregex.  Predicates which are moved to the front
     of a group by definition do not have side effects.  Both
     -regex and -iregex both use pred_regex.

     This routine "normalizes" the predicate tree by ensuring that
     all expression predicates have AND (or OR or COMMA) parent nodes
     which are linked along the left edge of the expression tree.
     This makes manipulation of subtrees easier.  

     EVAL_TREEP points to the root pointer of the predicate tree
     to be rearranged.  opt_expr may return a new root pointer there.
     Return true if the tree contains side effects, false if not. */

boolean
opt_expr (struct predicate **eval_treep)
{
  /* List of -name and -path predicates to move. */
  struct predicate *name_list = NULL;
  struct predicate *end_name_list = NULL;
  /* List of -regex predicates to move. */
  struct predicate *regex_list = NULL;
  struct predicate *end_regex_list = NULL;
  struct predicate *curr;
  struct predicate **prevp;	/* Address of `curr' node. */
  struct predicate **last_sidep; /* Last predicate with side effects. */
  PRED_FUNC pred_func;
  enum predicate_type p_type;
  boolean has_side_effects = false; /* Return value. */
  enum predicate_precedence prev_prec, /* precedence of last BI_OP in branch */
			    biop_prec; /* topmost BI_OP precedence in branch */


  if (eval_treep == NULL || *eval_treep == NULL)
    return (false);

  /* Set up to normalize tree as a left-linked list of ANDs or ORs.
     Set `curr' to the leftmost node, `prevp' to its address, and
     `pred_func' to the predicate type of its parent. */
  prevp = eval_treep;
  prev_prec = AND_PREC;
  curr = *prevp;
  while (curr->pred_left != NULL)
    {
      prevp = &curr->pred_left;
      prev_prec = curr->p_prec;	/* must be a BI_OP */
      curr = curr->pred_left;
    }

  /* Link in the appropriate BI_OP for the last expression, if needed. */
  if (curr->p_type != BI_OP)
    set_new_parent (curr, prev_prec, prevp);
  
#ifdef DEBUG
  /* Normalized tree. */
  fprintf (stderr, "Normalized Eval Tree:\n");
  print_tree (stderr, *eval_treep, 0);
#endif

  /* Rearrange the predicates. */
  prevp = eval_treep;
  biop_prec = NO_PREC; /* not COMMA_PREC */
  if ((*prevp) && (*prevp)->p_type == BI_OP)
    biop_prec = (*prevp)->p_prec;
  while ((curr = *prevp) != NULL)
    {
      /* If there is a BI_OP of different precedence from the first
	 in the pred_left chain, create a new parent of the
	 original precedence, link the new parent to the left of the
	 previous and link CURR to the right of the new parent. 
	 This preserves the precedence of expressions in the tree
	 in case we rearrange them. */
      if (curr->p_type == BI_OP)
	{
          if (curr->p_prec != biop_prec)
	    curr = set_new_parent(curr, biop_prec, prevp);
	}
	  
      /* See which predicate type we have. */
      p_type = curr->pred_right->p_type;
      pred_func = curr->pred_right->pred_func;

      switch (p_type)
	{
	case NO_TYPE:
	case PRIMARY_TYPE:
	  /* Don't rearrange the arguments of the comma operator, it is
	     not commutative.  */
	  if (biop_prec == COMMA_PREC)
	    break;

	  /* If it's one of our special primaries, move it to the
	     front of the list for that primary. */
	  if (pred_func == pred_name || pred_func == pred_path ||
	      pred_func == pred_iname || pred_func == pred_ipath)
	    {
	      *prevp = curr->pred_left;
	      curr->pred_left = name_list;
	      name_list = curr;

	      if (end_name_list == NULL)
		end_name_list = curr;

	      continue;
	    }

	  if (pred_func == pred_regex)
	    {
	      *prevp = curr->pred_left;
	      curr->pred_left = regex_list;
	      regex_list = curr;

	      if (end_regex_list == NULL)
		end_regex_list = curr;

	      continue;
	    }

	  break;

	case UNI_OP:
	  /* For NOT, check the expression trees below the NOT. */
	  curr->pred_right->side_effects
	    = opt_expr (&curr->pred_right->pred_right);
	  break;

	case BI_OP:
	  /* For nested AND or OR, recurse (AND/OR form layers on the left of
	     the tree), and continue scanning this level of AND or OR. */
	  curr->pred_right->side_effects = opt_expr (&curr->pred_right);
	  break;

	  /* At this point, get_expr and scan_rest have already removed
	     all of the user's parentheses. */

	default:
	  error (1, 0, _("oops -- invalid expression type!"));
	  break;
	}

      if (curr->pred_right->side_effects == true)
	{
	  last_sidep = prevp;

	  /* Incorporate lists and reset list pointers for this group.  */
	  if (name_list != NULL)
	    {
	      merge_pred (name_list, end_name_list, last_sidep);
	      name_list = end_name_list = NULL;
	    }

	  if (regex_list != NULL)
	    {
	      merge_pred (regex_list, end_regex_list, last_sidep);
	      regex_list = end_regex_list = NULL;
	    }

	  has_side_effects = true;
	}

      prevp = &curr->pred_left;
    }

  /* Do final list merges. */
  last_sidep = prevp;
  if (name_list != NULL)
    merge_pred (name_list, end_name_list, last_sidep);
  if (regex_list != NULL)
    merge_pred (regex_list, end_regex_list, last_sidep);

  return (has_side_effects);
}

/* Link in a new parent BI_OP node for CURR, at *PREVP, with precedence
   HIGH_PREC. */

static struct predicate *
set_new_parent (struct predicate *curr, enum predicate_precedence high_prec, struct predicate **prevp)
{
  struct predicate *new_parent;

  new_parent = (struct predicate *) xmalloc (sizeof (struct predicate));
  new_parent->p_type = BI_OP;
  new_parent->p_prec = high_prec;
  new_parent->need_stat = false;
  new_parent->need_type = false;

  switch (high_prec)
    {
    case COMMA_PREC:
      new_parent->pred_func = pred_comma;
      break;
    case OR_PREC:
      new_parent->pred_func = pred_or;
      break;
    case AND_PREC:
      new_parent->pred_func = pred_and;
      break;
    default:
      ;				/* empty */
    }
  
  new_parent->side_effects = false;
  new_parent->no_default_print = false;
  new_parent->args.str = NULL;
  new_parent->pred_next = NULL;

  /* Link in new_parent.
     Pushes rest of left branch down 1 level to new_parent->pred_right. */
  new_parent->pred_left = NULL;
  new_parent->pred_right = curr;
  *prevp = new_parent;

#ifdef	DEBUG
  new_parent->p_name = (char *) find_pred_name (new_parent->pred_func);
#endif /* DEBUG */

  return (new_parent);
}

/* Merge the predicate list that starts at BEG_LIST and ends at END_LIST
   into the tree at LAST_P. */

static void
merge_pred (struct predicate *beg_list, struct predicate *end_list, struct predicate **last_p)
{
  end_list->pred_left = *last_p;
  *last_p = beg_list;
}

/* Find the first node in expression tree TREE that requires
   a stat call and mark the operator above it as needing a stat
   before calling the node.   Since the expression precedences 
   are represented in the tree, some preds that need stat may not
   get executed (because the expression value is determined earlier.)
   So every expression needing stat must be marked as such, not just
   the earliest, to be sure to obtain the stat.  This still guarantees 
   that a stat is made as late as possible.  Return true if the top node 
   in TREE requires a stat, false if not. */

boolean
mark_stat (struct predicate *tree)
{
  /* The tree is executed in-order, so walk this way (apologies to Aerosmith)
     to find the first predicate for which the stat is needed. */
  switch (tree->p_type)
    {
    case NO_TYPE:
    case PRIMARY_TYPE:
      return tree->need_stat;

    case UNI_OP:
      if (mark_stat (tree->pred_right))
	tree->need_stat = true;
      return (false);

    case BI_OP:
      /* ANDs and ORs are linked along ->left ending in NULL. */
      if (tree->pred_left != NULL)
	mark_stat (tree->pred_left);

      if (mark_stat (tree->pred_right))
	tree->need_stat = true;

      return (false);

    default:
      error (1, 0, _("oops -- invalid expression type in mark_stat!"));
      return (false);
    }
}

/* Find the first node in expression tree TREE that we will
   need to know the file type, if any.   Operates in the same 
   was as mark_stat().
*/
boolean
mark_type (struct predicate *tree)
{
  /* The tree is executed in-order, so walk this way (apologies to Aerosmith)
     to find the first predicate for which the type information is needed. */
  switch (tree->p_type)
    {
    case NO_TYPE:
    case PRIMARY_TYPE:
      return tree->need_type;

    case UNI_OP:
      if (mark_type (tree->pred_right))
	tree->need_type = true;
      return false;

    case BI_OP:
      /* ANDs and ORs are linked along ->left ending in NULL. */
      if (tree->pred_left != NULL)
	mark_type (tree->pred_left);

      if (mark_type (tree->pred_right))
	tree->need_type = true;

      return false;

    default:
      error (1, 0, _("oops -- invalid expression type in mark_type!"));
      return (false);
    }
}

