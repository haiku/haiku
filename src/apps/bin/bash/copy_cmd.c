/* copy_command.c -- copy a COMMAND structure.  This is needed
   primarily for making function definitions, but I'm not sure
   that anyone else will need it.  */

/* Copyright (C) 1987,1991 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   Bash is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111 USA. */

#include "config.h"

#include "bashtypes.h"

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include <stdio.h>

#include "shell.h"

static PATTERN_LIST *copy_case_clause __P((PATTERN_LIST *));
static PATTERN_LIST *copy_case_clauses __P((PATTERN_LIST *));
static FOR_COM *copy_for_command __P((FOR_COM *));
#if defined (ARITH_FOR_COMMAND)
static ARITH_FOR_COM *copy_arith_for_command __P((ARITH_FOR_COM *));
#endif
static GROUP_COM *copy_group_command __P((GROUP_COM *));
static SUBSHELL_COM *copy_subshell_command __P((SUBSHELL_COM *));
static CASE_COM *copy_case_command __P((CASE_COM *));
static WHILE_COM *copy_while_command __P((WHILE_COM *));
static IF_COM *copy_if_command __P((IF_COM *));
#if defined (DPAREN_ARITHMETIC)
static ARITH_COM *copy_arith_command __P((ARITH_COM *));
#endif
#if defined (COND_COMMAND)
static COND_COM *copy_cond_command __P((COND_COM *));
#endif
static SIMPLE_COM *copy_simple_command __P((SIMPLE_COM *));
static FUNCTION_DEF *copy_function_def __P((FUNCTION_DEF *));

WORD_DESC *
copy_word (w)
     WORD_DESC *w;
{
  WORD_DESC *new_word;

  new_word = make_bare_word (w->word);
  new_word->flags = w->flags;
  return (new_word);
}

/* Copy the chain of words in LIST.  Return a pointer to
   the new chain. */
WORD_LIST *
copy_word_list (list)
     WORD_LIST *list;
{
  WORD_LIST *new_list;

  for (new_list = (WORD_LIST *)NULL; list; list = list->next)
    new_list = make_word_list (copy_word (list->word), new_list);

  return (REVERSE_LIST (new_list, WORD_LIST *));
}

static PATTERN_LIST *
copy_case_clause (clause)
     PATTERN_LIST *clause;
{
  PATTERN_LIST *new_clause;

  new_clause = (PATTERN_LIST *)xmalloc (sizeof (PATTERN_LIST));
  new_clause->patterns = copy_word_list (clause->patterns);
  new_clause->action = copy_command (clause->action);
  return (new_clause);
}

static PATTERN_LIST *
copy_case_clauses (clauses)
     PATTERN_LIST *clauses;
{
  PATTERN_LIST *new_list, *new_clause;

  for (new_list = (PATTERN_LIST *)NULL; clauses; clauses = clauses->next)
    {
      new_clause = copy_case_clause (clauses);
      new_clause->next = new_list;
      new_list = new_clause;
    }
  return (REVERSE_LIST (new_list, PATTERN_LIST *));
}

/* Copy a single redirect. */
REDIRECT *
copy_redirect (redirect)
     REDIRECT *redirect;
{
  REDIRECT *new_redirect;

  new_redirect = (REDIRECT *)xmalloc (sizeof (REDIRECT));
  FASTCOPY ((char *)redirect, (char *)new_redirect, (sizeof (REDIRECT)));
  switch (redirect->instruction)
    {
    case r_reading_until:
    case r_deblank_reading_until:
      new_redirect->here_doc_eof = savestring (redirect->here_doc_eof);
      /*FALLTHROUGH*/
    case r_reading_string:
    case r_appending_to:
    case r_output_direction:
    case r_input_direction:
    case r_inputa_direction:
    case r_err_and_out:
    case r_input_output:
    case r_output_force:
    case r_duplicating_input_word:
    case r_duplicating_output_word:
    case r_move_input_word:
    case r_move_output_word:
      new_redirect->redirectee.filename = copy_word (redirect->redirectee.filename);
      break;
    case r_duplicating_input:
    case r_duplicating_output:
    case r_move_input:
    case r_move_output:
    case r_close_this:
      break;
    }
  return (new_redirect);
}

REDIRECT *
copy_redirects (list)
     REDIRECT *list;
{
  REDIRECT *new_list, *temp;

  for (new_list = (REDIRECT *)NULL; list; list = list->next)
    {
      temp = copy_redirect (list);
      temp->next = new_list;
      new_list = temp;
    }
  return (REVERSE_LIST (new_list, REDIRECT *));
}

static FOR_COM *
copy_for_command (com)
     FOR_COM *com;
{
  FOR_COM *new_for;

  new_for = (FOR_COM *)xmalloc (sizeof (FOR_COM));
  new_for->flags = com->flags;
  new_for->name = copy_word (com->name);
  new_for->map_list = copy_word_list (com->map_list);
  new_for->action = copy_command (com->action);
  return (new_for);
}

#if defined (ARITH_FOR_COMMAND)
static ARITH_FOR_COM *
copy_arith_for_command (com)
     ARITH_FOR_COM *com;
{
  ARITH_FOR_COM *new_arith_for;

  new_arith_for = (ARITH_FOR_COM *)xmalloc (sizeof (ARITH_FOR_COM));
  new_arith_for->flags = com->flags;
  new_arith_for->line = com->line;
  new_arith_for->init = copy_word_list (com->init);
  new_arith_for->test = copy_word_list (com->test);
  new_arith_for->step = copy_word_list (com->step);
  new_arith_for->action = copy_command (com->action);
  return (new_arith_for);
}
#endif /* ARITH_FOR_COMMAND */

static GROUP_COM *
copy_group_command (com)
     GROUP_COM *com;
{
  GROUP_COM *new_group;

  new_group = (GROUP_COM *)xmalloc (sizeof (GROUP_COM));
  new_group->command = copy_command (com->command);
  return (new_group);
}

static SUBSHELL_COM *
copy_subshell_command (com)
     SUBSHELL_COM *com;
{
  SUBSHELL_COM *new_subshell;

  new_subshell = (SUBSHELL_COM *)xmalloc (sizeof (SUBSHELL_COM));
  new_subshell->command = copy_command (com->command);
  new_subshell->flags = com->flags;
  return (new_subshell);
}

static CASE_COM *
copy_case_command (com)
     CASE_COM *com;
{
  CASE_COM *new_case;

  new_case = (CASE_COM *)xmalloc (sizeof (CASE_COM));
  new_case->flags = com->flags;
  new_case->word = copy_word (com->word);
  new_case->clauses = copy_case_clauses (com->clauses);
  return (new_case);
}

static WHILE_COM *
copy_while_command (com)
     WHILE_COM *com;
{
  WHILE_COM *new_while;

  new_while = (WHILE_COM *)xmalloc (sizeof (WHILE_COM));
  new_while->flags = com->flags;
  new_while->test = copy_command (com->test);
  new_while->action = copy_command (com->action);
  return (new_while);
}

static IF_COM *
copy_if_command (com)
     IF_COM *com;
{
  IF_COM *new_if;

  new_if = (IF_COM *)xmalloc (sizeof (IF_COM));
  new_if->flags = com->flags;
  new_if->test = copy_command (com->test);
  new_if->true_case = copy_command (com->true_case);
  new_if->false_case = com->false_case ? copy_command (com->false_case) : com->false_case;
  return (new_if);
}

#if defined (DPAREN_ARITHMETIC)
static ARITH_COM *
copy_arith_command (com)
     ARITH_COM *com;
{
  ARITH_COM *new_arith;

  new_arith = (ARITH_COM *)xmalloc (sizeof (ARITH_COM));
  new_arith->flags = com->flags;
  new_arith->exp = copy_word_list (com->exp);
  new_arith->line = com->line;

  return (new_arith);
}
#endif

#if defined (COND_COMMAND)
static COND_COM *
copy_cond_command (com)
     COND_COM *com;
{
  COND_COM *new_cond;

  new_cond = (COND_COM *)xmalloc (sizeof (COND_COM));
  new_cond->flags = com->flags;
  new_cond->line = com->line;
  new_cond->type = com->type;
  new_cond->op = com->op ? copy_word (com->op) : com->op;
  new_cond->left = com->left ? copy_cond_command (com->left) : (COND_COM *)NULL;
  new_cond->right = com->right ? copy_cond_command (com->right) : (COND_COM *)NULL;

  return (new_cond);
}
#endif

static SIMPLE_COM *
copy_simple_command (com)
     SIMPLE_COM *com;
{
  SIMPLE_COM *new_simple;

  new_simple = (SIMPLE_COM *)xmalloc (sizeof (SIMPLE_COM));
  new_simple->flags = com->flags;
  new_simple->words = copy_word_list (com->words);
  new_simple->redirects = com->redirects ? copy_redirects (com->redirects) : (REDIRECT *)NULL;
  new_simple->line = com->line;
  return (new_simple);
}

static FUNCTION_DEF *
copy_function_def (com)
     FUNCTION_DEF *com;
{
  FUNCTION_DEF *new_def;

  new_def = (FUNCTION_DEF *)xmalloc (sizeof (FUNCTION_DEF));
  new_def->name = copy_word (com->name);
  new_def->command = copy_command (com->command);
  new_def->flags = com->flags;
  new_def->line = com->line;
  return (new_def);
}

/* Copy the command structure in COMMAND.  Return a pointer to the
   copy.  Don't you forget to dispose_command () on this pointer
   later! */
COMMAND *
copy_command (command)
     COMMAND *command;
{
  COMMAND *new_command;

  if (command == NULL)
    return (command);

  new_command = (COMMAND *)xmalloc (sizeof (COMMAND));
  FASTCOPY ((char *)command, (char *)new_command, sizeof (COMMAND));
  new_command->flags = command->flags;
  new_command->line = command->line;

  if (command->redirects)
    new_command->redirects = copy_redirects (command->redirects);

  switch (command->type)
    {
      case cm_for:
	new_command->value.For = copy_for_command (command->value.For);
	break;

#if defined (ARITH_FOR_COMMAND)
      case cm_arith_for:
	new_command->value.ArithFor = copy_arith_for_command (command->value.ArithFor);
	break;
#endif

#if defined (SELECT_COMMAND)
      case cm_select:
	new_command->value.Select =
	  (SELECT_COM *)copy_for_command ((FOR_COM *)command->value.Select);
	break;
#endif

      case cm_group:
	new_command->value.Group = copy_group_command (command->value.Group);
	break;

      case cm_subshell:
	new_command->value.Subshell = copy_subshell_command (command->value.Subshell);
	break;

      case cm_case:
	new_command->value.Case = copy_case_command (command->value.Case);
	break;

      case cm_until:
      case cm_while:
	new_command->value.While = copy_while_command (command->value.While);
	break;

      case cm_if:
	new_command->value.If = copy_if_command (command->value.If);
	break;

#if defined (DPAREN_ARITHMETIC)
      case cm_arith:
	new_command->value.Arith = copy_arith_command (command->value.Arith);
	break;
#endif

#if defined (COND_COMMAND)
      case cm_cond:
	new_command->value.Cond = copy_cond_command (command->value.Cond);
	break;
#endif

      case cm_simple:
	new_command->value.Simple = copy_simple_command (command->value.Simple);
	break;

      case cm_connection:
	{
	  CONNECTION *new_connection;

	  new_connection = (CONNECTION *)xmalloc (sizeof (CONNECTION));
	  new_connection->connector = command->value.Connection->connector;
	  new_connection->first = copy_command (command->value.Connection->first);
	  new_connection->second = copy_command (command->value.Connection->second);
	  new_command->value.Connection = new_connection;
	  break;
	}

      case cm_function_def:
	new_command->value.Function_def = copy_function_def (command->value.Function_def);
	break;
    }
  return (new_command);
}
