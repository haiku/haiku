/* unwind_prot.c - a simple unwind-protect system for internal variables */

/* I can't stand it anymore!  Please can't we just write the
   whole Unix system in lisp or something? */

/* Copyright (C) 1987-2009 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

/* **************************************************************** */
/*								    */
/*		      Unwind Protection Scheme for Bash		    */
/*								    */
/* **************************************************************** */
#include "config.h"

#include "bashtypes.h"
#include "bashansi.h"

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#if STDC_HEADERS
#  include <stddef.h>
#endif

#ifndef offsetof
#  define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#include "command.h"
#include "general.h"
#include "unwind_prot.h"
#include "quit.h"
#include "sig.h"

/* Structure describing a saved variable and the value to restore it to.  */
typedef struct {
  char *variable;
  int size;
  char desired_setting[1]; /* actual size is `size' */
} SAVED_VAR;

/* If HEAD.CLEANUP is null, then ARG.V contains a tag to throw back to.
   If HEAD.CLEANUP is restore_variable, then SV.V contains the saved
   variable.  Otherwise, call HEAD.CLEANUP (ARG.V) to clean up.  */
typedef union uwp {
  struct uwp_head {
    union uwp *next;
    Function *cleanup;
  } head;
  struct {
    struct uwp_head uwp_head;
    char *v;
  } arg;
  struct {
    struct uwp_head uwp_head;
    SAVED_VAR v;
  } sv;
} UNWIND_ELT;


static void without_interrupts __P((VFunction *, char *, char *));
static void unwind_frame_discard_internal __P((char *, char *));
static void unwind_frame_run_internal __P((char *, char *));
static void add_unwind_protect_internal __P((Function *, char *));
static void remove_unwind_protect_internal __P((char *, char *));
static void run_unwind_protects_internal __P((char *, char *));
static void clear_unwind_protects_internal __P((char *, char *));
static inline void restore_variable __P((SAVED_VAR *));
static void unwind_protect_mem_internal __P((char *, char *));

static UNWIND_ELT *unwind_protect_list = (UNWIND_ELT *)NULL;

#define uwpalloc(elt)	(elt) = (UNWIND_ELT *)xmalloc (sizeof (UNWIND_ELT))
#define uwpfree(elt)	free(elt)

/* Run a function without interrupts.  This relies on the fact that the
   FUNCTION cannot change the value of interrupt_immediately.  (I.e., does
   not call QUIT (). */
static void
without_interrupts (function, arg1, arg2)
     VFunction *function;
     char *arg1, *arg2;
{
  int old_interrupt_immediately;

  old_interrupt_immediately = interrupt_immediately;
  interrupt_immediately = 0;

  (*function)(arg1, arg2);

  interrupt_immediately = old_interrupt_immediately;
}

/* Start the beginning of a region. */
void
begin_unwind_frame (tag)
     char *tag;
{
  add_unwind_protect ((Function *)NULL, tag);
}

/* Discard the unwind protects back to TAG. */
void
discard_unwind_frame (tag)
     char *tag;
{
  if (unwind_protect_list)
    without_interrupts (unwind_frame_discard_internal, tag, (char *)NULL);
}

/* Run the unwind protects back to TAG. */
void
run_unwind_frame (tag)
     char *tag;
{
  if (unwind_protect_list)
    without_interrupts (unwind_frame_run_internal, tag, (char *)NULL);
}

/* Add the function CLEANUP with ARG to the list of unwindable things. */
void
add_unwind_protect (cleanup, arg)
     Function *cleanup;
     char *arg;
{
  without_interrupts (add_unwind_protect_internal, (char *)cleanup, arg);
}

/* Remove the top unwind protect from the list. */
void
remove_unwind_protect ()
{
  if (unwind_protect_list)
    without_interrupts
      (remove_unwind_protect_internal, (char *)NULL, (char *)NULL);
}

/* Run the list of cleanup functions in unwind_protect_list. */
void
run_unwind_protects ()
{
  if (unwind_protect_list)
    without_interrupts
      (run_unwind_protects_internal, (char *)NULL, (char *)NULL);
}

/* Erase the unwind-protect list.  If flags is 1, free the elements. */
void
clear_unwind_protect_list (flags)
     int flags;
{
  char *flag;

  if (unwind_protect_list)
    {
      flag = flags ? "" : (char *)NULL;
      without_interrupts
        (clear_unwind_protects_internal, flag, (char *)NULL);
    }
}

int
have_unwind_protects ()
{
  return (unwind_protect_list != 0);
}

/* **************************************************************** */
/*								    */
/*			The Actual Functions		 	    */
/*								    */
/* **************************************************************** */

static void
add_unwind_protect_internal (cleanup, arg)
     Function *cleanup;
     char *arg;
{
  UNWIND_ELT *elt;

  uwpalloc (elt);
  elt->head.next = unwind_protect_list;
  elt->head.cleanup = cleanup;
  elt->arg.v = arg;
  unwind_protect_list = elt;
}

static void
remove_unwind_protect_internal (ignore1, ignore2)
     char *ignore1, *ignore2;
{
  UNWIND_ELT *elt;

  elt = unwind_protect_list;
  if (elt)
    {
      unwind_protect_list = unwind_protect_list->head.next;
      uwpfree (elt);
    }
}

static void
run_unwind_protects_internal (ignore1, ignore2)
     char *ignore1, *ignore2;
{
  unwind_frame_run_internal ((char *) NULL, (char *) NULL);
}

static void
clear_unwind_protects_internal (flag, ignore)
     char *flag, *ignore;
{
  if (flag)
    {
      while (unwind_protect_list)
	remove_unwind_protect_internal ((char *)NULL, (char *)NULL);
    }
  unwind_protect_list = (UNWIND_ELT *)NULL;
}

static void
unwind_frame_discard_internal (tag, ignore)
     char *tag, *ignore;
{
  UNWIND_ELT *elt;

  while (elt = unwind_protect_list)
    {
      unwind_protect_list = unwind_protect_list->head.next;
      if (elt->head.cleanup == 0 && (STREQ (elt->arg.v, tag)))
	{
	  uwpfree (elt);
	  break;
	}
      else
	uwpfree (elt);
    }
}

/* Restore the value of a variable, based on the contents of SV.
   sv->desired_setting is a block of memory SIZE bytes long holding the
   value itself.  This block of memory is copied back into the variable. */
static inline void
restore_variable (sv)
     SAVED_VAR *sv;
{
  FASTCOPY (sv->desired_setting, sv->variable, sv->size);
}

static void
unwind_frame_run_internal (tag, ignore)
     char *tag, *ignore;
{
  UNWIND_ELT *elt;

  while (elt = unwind_protect_list)
    {
      unwind_protect_list = elt->head.next;

      /* If tag, then compare. */
      if (!elt->head.cleanup)
	{
	  if (tag && STREQ (elt->arg.v, tag))
	    {
	      uwpfree (elt);
	      break;
	    }
	}
      else
	{
	  if (elt->head.cleanup == (Function *) restore_variable)
	    restore_variable (&elt->sv.v);
	  else
	    (*(elt->head.cleanup)) (elt->arg.v);
	}

      uwpfree (elt);
    }
}

static void
unwind_protect_mem_internal (var, psize)
     char *var;
     char *psize;
{
  int size, allocated;
  UNWIND_ELT *elt;

  size = *(int *) psize;
  allocated = size + offsetof (UNWIND_ELT, sv.v.desired_setting[0]);
  elt = (UNWIND_ELT *)xmalloc (allocated);
  elt->head.next = unwind_protect_list;
  elt->head.cleanup = (Function *) restore_variable;
  elt->sv.v.variable = var;
  elt->sv.v.size = size;
  FASTCOPY (var, elt->sv.v.desired_setting, size);
  unwind_protect_list = elt;
}

/* Save the value of a variable so it will be restored when unwind-protects
   are run.  VAR is a pointer to the variable.  SIZE is the size in
   bytes of VAR.  */
void
unwind_protect_mem (var, size)
     char *var;
     int size;
{
  without_interrupts (unwind_protect_mem_internal, var, (char *) &size);
}
