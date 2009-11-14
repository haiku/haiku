/* arrayfunc.c -- High-level array functions used by other parts of the shell. */

/* Copyright (C) 2001-2009 Free Software Foundation, Inc.

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

#include "config.h"

#if defined (ARRAY_VARS)

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif
#include <stdio.h>

#include "bashintl.h"

#include "shell.h"
#include "pathexp.h"

#include "shmbutil.h"

#include "builtins/common.h"

extern char *this_command_name;
extern int last_command_exit_value;
extern int array_needs_making;

static SHELL_VAR *bind_array_var_internal __P((SHELL_VAR *, arrayind_t, char *, char *, int));

static char *quote_assign __P((const char *));
static void quote_array_assignment_chars __P((WORD_LIST *));
static char *array_value_internal __P((char *, int, int, int *));

/* Standard error message to use when encountering an invalid array subscript */
const char * const bash_badsub_errmsg = N_("bad array subscript");

/* **************************************************************** */
/*								    */
/*  Functions to manipulate array variables and perform assignments */
/*								    */
/* **************************************************************** */

/* Convert a shell variable to an array variable.  The original value is
   saved as array[0]. */
SHELL_VAR *
convert_var_to_array (var)
     SHELL_VAR *var;
{
  char *oldval;
  ARRAY *array;

  oldval = value_cell (var);
  array = array_create ();
  if (oldval)
    array_insert (array, 0, oldval);

  FREE (value_cell (var));
  var_setarray (var, array);

  /* these aren't valid anymore */
  var->dynamic_value = (sh_var_value_func_t *)NULL;
  var->assign_func = (sh_var_assign_func_t *)NULL;

  INVALIDATE_EXPORTSTR (var);
  if (exported_p (var))
    array_needs_making++;

  VSETATTR (var, att_array);
  VUNSETATTR (var, att_invisible);

  return var;
}

/* Convert a shell variable to an array variable.  The original value is
   saved as array[0]. */
SHELL_VAR *
convert_var_to_assoc (var)
     SHELL_VAR *var;
{
  char *oldval;
  HASH_TABLE *hash;

  oldval = value_cell (var);
  hash = assoc_create (0);
  if (oldval)
    assoc_insert (hash, savestring ("0"), oldval);

  FREE (value_cell (var));
  var_setassoc (var, hash);

  /* these aren't valid anymore */
  var->dynamic_value = (sh_var_value_func_t *)NULL;
  var->assign_func = (sh_var_assign_func_t *)NULL;

  INVALIDATE_EXPORTSTR (var);
  if (exported_p (var))
    array_needs_making++;

  VSETATTR (var, att_assoc);
  VUNSETATTR (var, att_invisible);

  return var;
}

static SHELL_VAR *
bind_array_var_internal (entry, ind, key, value, flags)
     SHELL_VAR *entry;
     arrayind_t ind;
     char *key;
     char *value;
     int flags;
{
  SHELL_VAR *dentry;
  char *newval;

  /* If we're appending, we need the old value of the array reference, so
     fake out make_variable_value with a dummy SHELL_VAR */
  if (flags & ASS_APPEND)
    {
      dentry = (SHELL_VAR *)xmalloc (sizeof (SHELL_VAR));
      dentry->name = savestring (entry->name);
      if (assoc_p (entry))
	newval = assoc_reference (assoc_cell (entry), key);
      else
	newval = array_reference (array_cell (entry), ind);
      if (newval)
	dentry->value = savestring (newval);
      else
	{
	  dentry->value = (char *)xmalloc (1);
	  dentry->value[0] = '\0';
	}
      dentry->exportstr = 0;
      dentry->attributes = entry->attributes & ~(att_array|att_assoc|att_exported);
      /* Leave the rest of the members uninitialized; the code doesn't look
	 at them. */
      newval = make_variable_value (dentry, value, flags);	 
      dispose_variable (dentry);
    }
  else
    newval = make_variable_value (entry, value, flags);

  if (entry->assign_func)
    (*entry->assign_func) (entry, newval, ind, key);
  else if (assoc_p (entry))
    assoc_insert (assoc_cell (entry), key, newval);
  else
    array_insert (array_cell (entry), ind, newval);
  FREE (newval);

  return (entry);
}

/* Perform an array assignment name[ind]=value.  If NAME already exists and
   is not an array, and IND is 0, perform name=value instead.  If NAME exists
   and is not an array, and IND is not 0, convert it into an array with the
   existing value as name[0].

   If NAME does not exist, just create an array variable, no matter what
   IND's value may be. */
SHELL_VAR *
bind_array_variable (name, ind, value, flags)
     char *name;
     arrayind_t ind;
     char *value;
     int flags;
{
  SHELL_VAR *entry;

  entry = var_lookup (name, shell_variables);

  if (entry == (SHELL_VAR *) 0)
    entry = make_new_array_variable (name);
  else if (readonly_p (entry) || noassign_p (entry))
    {
      if (readonly_p (entry))
	err_readonly (name);
      return (entry);
    }
  else if (array_p (entry) == 0)
    entry = convert_var_to_array (entry);

  /* ENTRY is an array variable, and ARRAY points to the value. */
  return (bind_array_var_internal (entry, ind, 0, value, flags));
}

SHELL_VAR *
bind_array_element (entry, ind, value, flags)
     SHELL_VAR *entry;
     arrayind_t ind;
     char *value;
     int flags;
{
  return (bind_array_var_internal (entry, ind, 0, value, flags));
}
                    
SHELL_VAR *
bind_assoc_variable (entry, name, key, value, flags)
     SHELL_VAR *entry;
     char *name;
     char *key;
     char *value;
     int flags;
{
  SHELL_VAR *dentry;
  char *newval;

  if (readonly_p (entry) || noassign_p (entry))
    {
      if (readonly_p (entry))
	err_readonly (name);
      return (entry);
    }

  return (bind_array_var_internal (entry, 0, key, value, flags));
}

/* Parse NAME, a lhs of an assignment statement of the form v[s], and
   assign VALUE to that array element by calling bind_array_variable(). */
SHELL_VAR *
assign_array_element (name, value, flags)
     char *name, *value;
     int flags;
{
  char *sub, *vname, *akey;
  arrayind_t ind;
  int sublen;
  SHELL_VAR *entry;

  vname = array_variable_name (name, &sub, &sublen);

  if (vname == 0)
    return ((SHELL_VAR *)NULL);

  if ((ALL_ELEMENT_SUB (sub[0]) && sub[1] == ']') || (sublen <= 1))
    {
      free (vname);
      err_badarraysub (name);
      return ((SHELL_VAR *)NULL);
    }

  entry = find_variable (vname);

  if (entry && assoc_p (entry))
    {
      sub[sublen-1] = '\0';
      akey = expand_assignment_string_to_string (sub, 0);	/* [ */
      sub[sublen-1] = ']';
      if (akey == 0 || *akey == 0)
	{
	  free (vname);
	  err_badarraysub (name);
	  return ((SHELL_VAR *)NULL);
	}
      entry = bind_assoc_variable (entry, vname, akey, value, flags);
    }
  else
    {
      ind = array_expand_index (sub, sublen);
      if (ind < 0)
	{
	  free (vname);
	  err_badarraysub (name);
	  return ((SHELL_VAR *)NULL);
	}
      entry = bind_array_variable (vname, ind, value, flags);
    }

  free (vname);
  return (entry);
}

/* Find the array variable corresponding to NAME.  If there is no variable,
   create a new array variable.  If the variable exists but is not an array,
   convert it to an indexed array.  If FLAGS&1 is non-zero, an existing
   variable is checked for the readonly or noassign attribute in preparation
   for assignment (e.g., by the `read' builtin).  If FLAGS&2 is non-zero, we
   create an associative array. */
SHELL_VAR *
find_or_make_array_variable (name, flags)
     char *name;
     int flags;
{
  SHELL_VAR *var;

  var = find_variable (name);

  if (var == 0)
    var = (flags & 2) ? make_new_assoc_variable (name) : make_new_array_variable (name);
  else if ((flags & 1) && (readonly_p (var) || noassign_p (var)))
    {
      if (readonly_p (var))
	err_readonly (name);
      return ((SHELL_VAR *)NULL);
    }
  else if ((flags & 2) && array_p (var))
    {
      report_error (_("%s: cannot convert indexed to associative array"), name);
      return ((SHELL_VAR *)NULL);
    }
  else if (array_p (var) == 0 && assoc_p (var) == 0)
    var = convert_var_to_array (var);

  return (var);
}
  
/* Perform a compound assignment statement for array NAME, where VALUE is
   the text between the parens:  NAME=( VALUE ) */
SHELL_VAR *
assign_array_from_string (name, value, flags)
     char *name, *value;
     int flags;
{
  SHELL_VAR *var;
  int vflags;

  vflags = 1;
  if (flags & ASS_MKASSOC)
    vflags |= 2;

  var = find_or_make_array_variable (name, vflags);
  if (var == 0)
    return ((SHELL_VAR *)NULL);

  return (assign_array_var_from_string (var, value, flags));
}

/* Sequentially assign the indices of indexed array variable VAR from the
   words in LIST. */
SHELL_VAR *
assign_array_var_from_word_list (var, list, flags)
     SHELL_VAR *var;
     WORD_LIST *list;
     int flags;
{
  register arrayind_t i;
  register WORD_LIST *l;
  ARRAY *a;

  a = array_cell (var);
  i = (flags & ASS_APPEND) ? array_max_index (a) + 1 : 0;

  for (l = list; l; l = l->next, i++)
    if (var->assign_func)
      (*var->assign_func) (var, l->word->word, i, 0);
    else
      array_insert (a, i, l->word->word);
  return var;
}

WORD_LIST *
expand_compound_array_assignment (var, value, flags)
     SHELL_VAR *var;
     char *value;
     int flags;
{
  WORD_LIST *list, *nlist;
  char *val;
  int ni;

  /* I don't believe this condition is ever true any more. */
  if (*value == '(')	/*)*/
    {
      ni = 1;
      val = extract_array_assignment_list (value, &ni);
      if (val == 0)
	return (WORD_LIST *)NULL;
    }
  else
    val = value;

  /* Expand the value string into a list of words, performing all the
     shell expansions including pathname generation and word splitting. */
  /* First we split the string on whitespace, using the shell parser
     (ksh93 seems to do this). */
  list = parse_string_to_word_list (val, 1, "array assign");

  /* If we're using [subscript]=value, we need to quote each [ and ] to
     prevent unwanted filename expansion. */
  if (list)
    quote_array_assignment_chars (list);

  /* Now that we've split it, perform the shell expansions on each
     word in the list. */
  nlist = list ? expand_words_no_vars (list) : (WORD_LIST *)NULL;

  dispose_words (list);

  if (val != value)
    free (val);

  return nlist;
}

void
assign_compound_array_list (var, nlist, flags)
     SHELL_VAR *var;
     WORD_LIST *nlist;
     int flags;
{
  ARRAY *a;
  HASH_TABLE *h;
  WORD_LIST *list;
  char *w, *val, *nval;
  int len, iflags;
  arrayind_t ind, last_ind;
  char *akey;

  a = (var && array_p (var)) ? array_cell (var) : (ARRAY *)0;
  h = (var && assoc_p (var)) ? assoc_cell (var) : (HASH_TABLE *)0;

  akey = (char *)0;
  ind = 0;

  /* Now that we are ready to assign values to the array, kill the existing
     value. */
  if ((flags & ASS_APPEND) == 0)
    {
      if (array_p (var) && a)
	array_flush (a);
      else if (assoc_p (var) && h)
	assoc_flush (h);
    }

  last_ind = (a && (flags & ASS_APPEND)) ? array_max_index (a) + 1 : 0;

  for (list = nlist; list; list = list->next)
    {
      iflags = flags;
      w = list->word->word;

      /* We have a word of the form [ind]=value */
      if ((list->word->flags & W_ASSIGNMENT) && w[0] == '[')
	{
	  len = skipsubscript (w, 0);

	  /* XXX - changes for `+=' */
 	  if (w[len] != ']' || (w[len+1] != '=' && (w[len+1] != '+' || w[len+2] != '=')))
	    {
	      if (assoc_p (var))
		{
		  err_badarraysub (w);
		  continue;
		}
	      nval = make_variable_value (var, w, flags);
	      if (var->assign_func)
		(*var->assign_func) (var, nval, last_ind, 0);
	      else
		array_insert (a, last_ind, nval);
	      FREE (nval);
	      last_ind++;
	      continue;
	    }

	  if (len == 1)
	    {
	      err_badarraysub (w);
	      continue;
	    }

	  if (ALL_ELEMENT_SUB (w[1]) && len == 2)
	    {
	      if (assoc_p (var))
		report_error (_("%s: invalid associative array key"), w);
	      else
		report_error (_("%s: cannot assign to non-numeric index"), w);
	      continue;
	    }

	  if (array_p (var))
	    {
	      ind = array_expand_index (w + 1, len);
	      if (ind < 0)
		{
		  err_badarraysub (w);
		  continue;
		}

	      last_ind = ind;
	    }
	  else if (assoc_p (var))
	    {
	      akey = substring (w, 1, len);
	      if (akey == 0 || *akey == 0)
		{
		  err_badarraysub (w);
		  continue;
		}
	    }

	  /* XXX - changes for `+=' -- just accept the syntax.  ksh93 doesn't do this */
	  if (w[len + 1] == '+' && w[len + 2] == '=')
	    {
	      iflags |= ASS_APPEND;
	      val = w + len + 3;
	    }
	  else
	    val = w + len + 2;
	}
      else if (assoc_p (var))
	{
	  report_error (_("%s: %s: must use subscript when assigning associative array"), var->name, w);
	  continue;
	}
      else		/* No [ind]=value, just a stray `=' */
	{
	  ind = last_ind;
	  val = w;
	}

      if (integer_p (var))
	this_command_name = (char *)NULL;	/* no command name for errors */
      bind_array_var_internal (var, ind, akey, val, iflags);
      last_ind++;
    }
}

/* Perform a compound array assignment:  VAR->name=( VALUE ).  The
   VALUE has already had the parentheses stripped. */
SHELL_VAR *
assign_array_var_from_string (var, value, flags)
     SHELL_VAR *var;
     char *value;
     int flags;
{
  WORD_LIST *nlist;

  if (value == 0)
    return var;

  nlist = expand_compound_array_assignment (var, value, flags);
  assign_compound_array_list (var, nlist, flags);

  if (nlist)
    dispose_words (nlist);
  return (var);
}

/* Quote globbing chars and characters in $IFS before the `=' in an assignment
   statement (usually a compound array assignment) to protect them from
   unwanted filename expansion or word splitting. */
static char *
quote_assign (string)
     const char *string;
{
  size_t slen;
  int saw_eq;
  char *temp, *t;
  const char *s, *send;
  DECLARE_MBSTATE;

  slen = strlen (string);
  send = string + slen;

  t = temp = (char *)xmalloc (slen * 2 + 1);
  saw_eq = 0;
  for (s = string; *s; )
    {
      if (*s == '=')
	saw_eq = 1;
      if (saw_eq == 0 && (glob_char_p (s) || isifs (*s)))
	*t++ = '\\';

      COPY_CHAR_P (t, s, send);
    }
  *t = '\0';
  return temp;
}

/* For each word in a compound array assignment, if the word looks like
   [ind]=value, quote globbing chars and characters in $IFS before the `='. */
static void
quote_array_assignment_chars (list)
     WORD_LIST *list;
{
  char *nword;
  WORD_LIST *l;

  for (l = list; l; l = l->next)
    {
      if (l->word == 0 || l->word->word == 0 || l->word->word[0] == '\0')
	continue;	/* should not happen, but just in case... */
      /* Don't bother if it doesn't look like [ind]=value */
      if (l->word->word[0] != '[' || xstrchr (l->word->word, '=') == 0) /* ] */
	continue;
      nword = quote_assign (l->word->word);
      free (l->word->word);
      l->word->word = nword;
    }
}

/* skipsubscript moved to subst.c to use private functions. 2009/02/24. */

/* This function is called with SUB pointing to just after the beginning
   `[' of an array subscript and removes the array element to which SUB
   expands from array VAR.  A subscript of `*' or `@' unsets the array. */
int
unbind_array_element (var, sub)
     SHELL_VAR *var;
     char *sub;
{
  int len;
  arrayind_t ind;
  char *akey;
  ARRAY_ELEMENT *ae;

  len = skipsubscript (sub, 0);
  if (sub[len] != ']' || len == 0)
    {
      builtin_error ("%s[%s: %s", var->name, sub, _(bash_badsub_errmsg));
      return -1;
    }
  sub[len] = '\0';

  if (ALL_ELEMENT_SUB (sub[0]) && sub[1] == 0)
    {
      unbind_variable (var->name);
      return (0);
    }

  if (assoc_p (var))
    {
      akey = expand_assignment_string_to_string (sub, 0);     /* [ */
      if (akey == 0 || *akey == 0)
	{
	  builtin_error ("[%s]: %s", sub, _(bash_badsub_errmsg));
	  return -1;
	}
      assoc_remove (assoc_cell (var), akey);
    }
  else
    {
      ind = array_expand_index (sub, len+1);
      if (ind < 0)
	{
	  builtin_error ("[%s]: %s", sub, _(bash_badsub_errmsg));
	  return -1;
	}
      ae = array_remove (array_cell (var), ind);
      if (ae)
	array_dispose_element (ae);
    }

  return 0;
}

/* Format and output an array assignment in compound form VAR=(VALUES),
   suitable for re-use as input. */
void
print_array_assignment (var, quoted)
     SHELL_VAR *var;
     int quoted;
{
  char *vstr;

  vstr = array_to_assign (array_cell (var), quoted);

  if (vstr == 0)
    printf ("%s=%s\n", var->name, quoted ? "'()'" : "()");
  else
    {
      printf ("%s=%s\n", var->name, vstr);
      free (vstr);
    }
}

/* Format and output an associative array assignment in compound form
   VAR=(VALUES), suitable for re-use as input. */
void
print_assoc_assignment (var, quoted)
     SHELL_VAR *var;
     int quoted;
{
  char *vstr;

  vstr = assoc_to_assign (assoc_cell (var), quoted);

  if (vstr == 0)
    printf ("%s=%s\n", var->name, quoted ? "'()'" : "()");
  else
    {
      printf ("%s=%s\n", var->name, vstr);
      free (vstr);
    }
}

/***********************************************************************/
/*								       */
/* Utility functions to manage arrays and their contents for expansion */
/*								       */
/***********************************************************************/

/* Return 1 if NAME is a properly-formed array reference v[sub]. */
int
valid_array_reference (name)
     char *name;
{
  char *t;
  int r, len;

  t = xstrchr (name, '[');	/* ] */
  if (t)
    {
      *t = '\0';
      r = legal_identifier (name);
      *t = '[';
      if (r == 0)
	return 0;
      /* Check for a properly-terminated non-blank subscript. */
      len = skipsubscript (t, 0);
      if (t[len] != ']' || len == 1)
	return 0;
      for (r = 1; r < len; r++)
	if (whitespace (t[r]) == 0)
	  return 1;
      return 0;
    }
  return 0;
}

/* Expand the array index beginning at S and extending LEN characters. */
arrayind_t
array_expand_index (s, len)
     char *s;
     int len;
{
  char *exp, *t;
  int expok;
  arrayind_t val;

  exp = (char *)xmalloc (len);
  strncpy (exp, s, len - 1);
  exp[len - 1] = '\0';
  t = expand_arith_string (exp, 0);
  this_command_name = (char *)NULL;
  val = evalexp (t, &expok);
  free (t);
  free (exp);
  if (expok == 0)
    {
      last_command_exit_value = EXECUTION_FAILURE;

      top_level_cleanup ();      
      jump_to_top_level (DISCARD);
    }
  return val;
}

/* Return the name of the variable specified by S without any subscript.
   If SUBP is non-null, return a pointer to the start of the subscript
   in *SUBP. If LENP is non-null, the length of the subscript is returned
   in *LENP.  This returns newly-allocated memory. */
char *
array_variable_name (s, subp, lenp)
     char *s, **subp;
     int *lenp;
{
  char *t, *ret;
  int ind, ni;

  t = xstrchr (s, '[');
  if (t == 0)
    {
      if (subp)
      	*subp = t;
      if (lenp)
	*lenp = 0;
      return ((char *)NULL);
    }
  ind = t - s;
  ni = skipsubscript (s, ind);
  if (ni <= ind + 1 || s[ni] != ']')
    {
      err_badarraysub (s);
      if (subp)
      	*subp = t;
      if (lenp)
	*lenp = 0;
      return ((char *)NULL);
    }

  *t = '\0';
  ret = savestring (s);
  *t++ = '[';		/* ] */

  if (subp)
    *subp = t;
  if (lenp)
    *lenp = ni - ind;

  return ret;
}

/* Return the variable specified by S without any subscript.  If SUBP is
   non-null, return a pointer to the start of the subscript in *SUBP.
   If LENP is non-null, the length of the subscript is returned in *LENP. */
SHELL_VAR *
array_variable_part (s, subp, lenp)
     char *s, **subp;
     int *lenp;
{
  char *t;
  SHELL_VAR *var;

  t = array_variable_name (s, subp, lenp);
  if (t == 0)
    return ((SHELL_VAR *)NULL);
  var = find_variable (t);

  free (t);
  return (var == 0 || invisible_p (var)) ? (SHELL_VAR *)0 : var;
}

/* Return a string containing the elements in the array and subscript
   described by S.  If the subscript is * or @, obeys quoting rules akin
   to the expansion of $* and $@ including double quoting.  If RTYPE
   is non-null it gets 1 if the array reference is name[*], 2 if the
   reference is name[@], and 0 otherwise. */
static char *
array_value_internal (s, quoted, allow_all, rtype)
     char *s;
     int quoted, allow_all, *rtype;
{
  int len;
  arrayind_t ind;
  char *akey;
  char *retval, *t, *temp;
  WORD_LIST *l;
  SHELL_VAR *var;

  var = array_variable_part (s, &t, &len);

  /* Expand the index, even if the variable doesn't exist, in case side
     effects are needed, like ${w[i++]} where w is unset. */
#if 0
  if (var == 0)
    return (char *)NULL;
#endif

  if (len == 0)
    return ((char *)NULL);	/* error message already printed */

  /* [ */
  if (ALL_ELEMENT_SUB (t[0]) && t[1] == ']')
    {
      if (rtype)
	*rtype = (t[0] == '*') ? 1 : 2;
      if (allow_all == 0)
	{
	  err_badarraysub (s);
	  return ((char *)NULL);
	}
      else if (var == 0 || value_cell (var) == 0)
	return ((char *)NULL);
      else if (array_p (var) == 0 && assoc_p (var) == 0)
	l = add_string_to_list (value_cell (var), (WORD_LIST *)NULL);
      else if (assoc_p (var))
	{
	  l = assoc_to_word_list (assoc_cell (var));
	  if (l == (WORD_LIST *)NULL)
	    return ((char *)NULL);
	}
      else
	{
	  l = array_to_word_list (array_cell (var));
	  if (l == (WORD_LIST *)NULL)
	    return ((char *) NULL);
	}

      if (t[0] == '*' && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
	{
	  temp = string_list_dollar_star (l);
	  retval = quote_string (temp);
	  free (temp);
	}
      else	/* ${name[@]} or unquoted ${name[*]} */
	retval = string_list_dollar_at (l, quoted);

      dispose_words (l);
    }
  else
    {
      if (rtype)
	*rtype = 0;
      if (var == 0 || array_p (var) || assoc_p (var) == 0)
	{
	  ind = array_expand_index (t, len);
	  if (ind < 0)
	    {
index_error:
	      if (var)
		err_badarraysub (var->name);
	      else
		{
		  t[-1] = '\0';
		  err_badarraysub (s);
		  t[-1] = '[';	/* ] */
		}
	      return ((char *)NULL);
	    }
	}
      else if (assoc_p (var))
	{
	  t[len - 1] = '\0';
	  akey = expand_assignment_string_to_string (t, 0);	/* [ */
	  t[len - 1] = ']';
	  if (akey == 0 || *akey == 0)
	    goto index_error;
	}
     
      if (var == 0)
	return ((char *)NULL);
      if (array_p (var) == 0 && assoc_p (var) == 0)
	return (ind == 0 ? value_cell (var) : (char *)NULL);
      else if (assoc_p (var))
	retval = assoc_reference (assoc_cell (var), akey);
      else
	retval = array_reference (array_cell (var), ind);
    }

  return retval;
}

/* Return a string containing the elements described by the array and
   subscript contained in S, obeying quoting for subscripts * and @. */
char *
array_value (s, quoted, rtype)
     char *s;
     int quoted, *rtype;
{
  return (array_value_internal (s, quoted, 1, rtype));
}

/* Return the value of the array indexing expression S as a single string.
   If ALLOW_ALL is 0, do not allow `@' and `*' subscripts.  This is used
   by other parts of the shell such as the arithmetic expression evaluator
   in expr.c. */
char *
get_array_value (s, allow_all, rtype)
     char *s;
     int allow_all, *rtype;
{
  return (array_value_internal (s, 0, allow_all, rtype));
}

char *
array_keys (s, quoted)
     char *s;
     int quoted;
{
  int len;
  char *retval, *t, *temp;
  WORD_LIST *l;
  SHELL_VAR *var;

  var = array_variable_part (s, &t, &len);

  /* [ */
  if (var == 0 || ALL_ELEMENT_SUB (t[0]) == 0 || t[1] != ']')
    return (char *)NULL;

  if (array_p (var) == 0 && assoc_p (var) == 0)
    l = add_string_to_list ("0", (WORD_LIST *)NULL);
  else if (assoc_p (var))
    l = assoc_keys_to_word_list (assoc_cell (var));
  else
    l = array_keys_to_word_list (array_cell (var));
  if (l == (WORD_LIST *)NULL)
    return ((char *) NULL);

  if (t[0] == '*' && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
    {
      temp = string_list_dollar_star (l);
      retval = quote_string (temp);
      free (temp);
    }
  else	/* ${!name[@]} or unquoted ${!name[*]} */
    retval = string_list_dollar_at (l, quoted);

  dispose_words (l);
  return retval;
}
#endif /* ARRAY_VARS */
