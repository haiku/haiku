/* Internals of variables for GNU Make.
Copyright (C) 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1996, 1997,
2002 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include "make.h"
#include "dep.h"
#include "filedef.h"
#include "job.h"
#include "commands.h"
#include "variable.h"
#include "rule.h"
#ifdef WINDOWS32
#include "pathstuff.h"
#endif
#include "hash.h"

/* Hash table of all global variable definitions.  */

static unsigned long
variable_hash_1 (keyv)
    const void *keyv;
{
  struct variable const *key = (struct variable const *) keyv;
  return_STRING_N_HASH_1 (key->name, key->length);
}

static unsigned long
variable_hash_2 (keyv)
    const void *keyv;
{
  struct variable const *key = (struct variable const *) keyv;
  return_STRING_N_HASH_2 (key->name, key->length);
}

static int
variable_hash_cmp (xv, yv)
    const void *xv;
    const void *yv;
{
  struct variable const *x = (struct variable const *) xv;
  struct variable const *y = (struct variable const *) yv;
  int result = x->length - y->length;
  if (result)
    return result;
  return_STRING_N_COMPARE (x->name, y->name, x->length);
}

#ifndef	VARIABLE_BUCKETS
#define VARIABLE_BUCKETS		523
#endif
#ifndef	PERFILE_VARIABLE_BUCKETS
#define	PERFILE_VARIABLE_BUCKETS	23
#endif
#ifndef	SMALL_SCOPE_VARIABLE_BUCKETS
#define	SMALL_SCOPE_VARIABLE_BUCKETS	13
#endif

static struct variable_set global_variable_set;
static struct variable_set_list global_setlist
  = { 0, &global_variable_set };
struct variable_set_list *current_variable_set_list = &global_setlist;

/* Implement variables.  */

void
init_hash_global_variable_set ()
{
  hash_init (&global_variable_set.table, VARIABLE_BUCKETS,
	     variable_hash_1, variable_hash_2, variable_hash_cmp);
}

/* Define variable named NAME with value VALUE in SET.  VALUE is copied.
   LENGTH is the length of NAME, which does not need to be null-terminated.
   ORIGIN specifies the origin of the variable (makefile, command line
   or environment).
   If RECURSIVE is nonzero a flag is set in the variable saying
   that it should be recursively re-expanded.  */

struct variable *
define_variable_in_set (name, length, value, origin, recursive, set, flocp)
     const char *name;
     unsigned int length;
     char *value;
     enum variable_origin origin;
     int recursive;
     struct variable_set *set;
     const struct floc *flocp;
{
  struct variable *v;
  struct variable **var_slot;
  struct variable var_key;

  if (set == NULL)
    set = &global_variable_set;

  var_key.name = (char *) name;
  var_key.length = length;
  var_slot = (struct variable **) hash_find_slot (&set->table, &var_key);

  if (env_overrides && origin == o_env)
    origin = o_env_override;

  v = *var_slot;
  if (! HASH_VACANT (v))
    {
      if (env_overrides && v->origin == o_env)
	/* V came from in the environment.  Since it was defined
	   before the switches were parsed, it wasn't affected by -e.  */
	v->origin = o_env_override;

      /* A variable of this name is already defined.
	 If the old definition is from a stronger source
	 than this one, don't redefine it.  */
      if ((int) origin >= (int) v->origin)
	{
	  if (v->value != 0)
	    free (v->value);
	  v->value = xstrdup (value);
          if (flocp != 0)
            v->fileinfo = *flocp;
          else
            v->fileinfo.filenm = 0;
	  v->origin = origin;
	  v->recursive = recursive;
	}
      return v;
    }

  /* Create a new variable definition and add it to the hash table.  */

  v = (struct variable *) xmalloc (sizeof (struct variable));
  v->name = savestring (name, length);
  v->length = length;
  hash_insert_at (&set->table, v, var_slot);
  v->value = xstrdup (value);
  if (flocp != 0)
    v->fileinfo = *flocp;
  else
    v->fileinfo.filenm = 0;
  v->origin = origin;
  v->recursive = recursive;
  v->expanding = 0;
  v->exp_count = 0;
  v->per_target = 0;
  v->append = 0;
  v->export = v_default;

  v->exportable = 1;
  if (*name != '_' && (*name < 'A' || *name > 'Z')
      && (*name < 'a' || *name > 'z'))
    v->exportable = 0;
  else
    {
      for (++name; *name != '\0'; ++name)
        if (*name != '_' && (*name < 'a' || *name > 'z')
            && (*name < 'A' || *name > 'Z') && !ISDIGIT(*name))
          break;

      if (*name != '\0')
        v->exportable = 0;
    }

  return v;
}

/* If the variable passed in is "special", handle its special nature.
   Currently there are two such variables, both used for introspection:
   .VARIABLES expands to a list of all the variables defined in this instance
   of make.
   .TARGETS expands to a list of all the targets defined in this
   instance of make.
   Returns the variable reference passed in.  */

#define EXPANSION_INCREMENT(_l)  ((((_l) / 500) + 1) * 500)

static struct variable *
handle_special_var (var)
     struct variable *var;
{
  static unsigned long last_var_count = 0;


  /* This one actually turns out to be very hard, due to the way the parser
     records targets.  The way it works is that target information is collected
     internally until make knows the target is completely specified.  It unitl
     it sees that some new construct (a new target or variable) is defined that
     it knows the previous one is done.  In short, this means that if you do
     this:

       all:

       TARGS := $(.TARGETS)

     then $(TARGS) won't contain "all", because it's not until after the
     variable is created that the previous target is completed.

     Changing this would be a major pain.  I think a less complex way to do it
     would be to pre-define the target files as soon as the first line is
     parsed, then come back and do the rest of the definition as now.  That
     would allow $(.TARGETS) to be correct without a major change to the way
     the parser works.

  if (streq (var->name, ".TARGETS"))
    var->value = build_target_list (var->value);
  else
  */

  if (streq (var->name, ".VARIABLES")
      && global_variable_set.table.ht_fill != last_var_count)
    {
      unsigned long max = EXPANSION_INCREMENT (strlen (var->value));
      unsigned long len;
      char *p;
      struct variable **vp = (struct variable **) global_variable_set.table.ht_vec;
      struct variable **end = &vp[global_variable_set.table.ht_size];

      /* Make sure we have at least MAX bytes in the allocated buffer.  */
      var->value = xrealloc (var->value, max);

      /* Walk through the hash of variables, constructing a list of names.  */
      p = var->value;
      len = 0;
      for (; vp < end; ++vp)
        if (!HASH_VACANT (*vp))
          {
            struct variable *v = *vp;
            int l = v->length;

            len += l + 1;
            if (len > max)
              {
                unsigned long off = p - var->value;

                max += EXPANSION_INCREMENT (l + 1);
                var->value = xrealloc (var->value, max);
                p = &var->value[off];
              }

            bcopy (v->name, p, l);
            p += l;
            *(p++) = ' ';
          }
      *(p-1) = '\0';

      /* Remember how many variables are in our current count.  Since we never
         remove variables from the list, this is a reliable way to know whether
         the list is up to date or needs to be recomputed.  */

      last_var_count = global_variable_set.table.ht_fill;
    }

  return var;
}


/* Lookup a variable whose name is a string starting at NAME
   and with LENGTH chars.  NAME need not be null-terminated.
   Returns address of the `struct variable' containing all info
   on the variable, or nil if no such variable is defined.  */

struct variable *
lookup_variable (name, length)
     const char *name;
     unsigned int length;
{
  const struct variable_set_list *setlist;
  struct variable var_key;

  var_key.name = (char *) name;
  var_key.length = length;

  for (setlist = current_variable_set_list;
       setlist != 0; setlist = setlist->next)
    {
      const struct variable_set *set = setlist->set;
      struct variable *v;

      v = (struct variable *) hash_find_item ((struct hash_table *) &set->table, &var_key);
      if (v)
	return v->special ? handle_special_var (v) : v;
    }

#ifdef VMS
  /* since we don't read envp[] on startup, try to get the
     variable via getenv() here.  */
  {
    char *vname = alloca (length + 1);
    char *value;
    strncpy (vname, name, length);
    vname[length] = 0;
    value = getenv (vname);
    if (value != 0)
      {
        char *sptr;
        int scnt;

        sptr = value;
        scnt = 0;

        while ((sptr = strchr (sptr, '$')))
          {
            scnt++;
            sptr++;
          }

        if (scnt > 0)
          {
            char *nvalue;
            char *nptr;

            nvalue = alloca (strlen (value) + scnt + 1);
            sptr = value;
            nptr = nvalue;

            while (*sptr)
              {
                if (*sptr == '$')
                  {
                    *nptr++ = '$';
                    *nptr++ = '$';
                  }
                else
                  {
                    *nptr++ = *sptr;
                  }
                sptr++;
              }

            *nptr = '\0';
            return define_variable (vname, length, nvalue, o_env, 1);

          }

        return define_variable (vname, length, value, o_env, 1);
      }
  }
#endif /* VMS */

  return 0;
}

/* Lookup a variable whose name is a string starting at NAME
   and with LENGTH chars in set SET.  NAME need not be null-terminated.
   Returns address of the `struct variable' containing all info
   on the variable, or nil if no such variable is defined.  */

struct variable *
lookup_variable_in_set (name, length, set)
     const char *name;
     unsigned int length;
     const struct variable_set *set;
{
  struct variable var_key;

  var_key.name = (char *) name;
  var_key.length = length;

  return (struct variable *) hash_find_item ((struct hash_table *) &set->table, &var_key);
}

/* Initialize FILE's variable set list.  If FILE already has a variable set
   list, the topmost variable set is left intact, but the the rest of the
   chain is replaced with FILE->parent's setlist.  If FILE is a double-colon
   rule, then we will use the "root" double-colon target's variable set as the
   parent of FILE's variable set.

   If we're READing a makefile, don't do the pattern variable search now,
   since the pattern variable might not have been defined yet.  */

void
initialize_file_variables (file, reading)
     struct file *file;
     int reading;
{
  register struct variable_set_list *l = file->variables;

  if (l == 0)
    {
      l = (struct variable_set_list *)
	xmalloc (sizeof (struct variable_set_list));
      l->set = (struct variable_set *) xmalloc (sizeof (struct variable_set));
      hash_init (&l->set->table, PERFILE_VARIABLE_BUCKETS,
                 variable_hash_1, variable_hash_2, variable_hash_cmp);
      file->variables = l;
    }

  /* If this is a double-colon, then our "parent" is the "root" target for
     this double-colon rule.  Since that rule has the same name, parent,
     etc. we can just use its variables as the "next" for ours.  */

  if (file->double_colon && file->double_colon != file)
    {
      initialize_file_variables (file->double_colon, reading);
      l->next = file->double_colon->variables;
      return;
    }

  if (file->parent == 0)
    l->next = &global_setlist;
  else
    {
      initialize_file_variables (file->parent, reading);
      l->next = file->parent->variables;
    }

  /* If we're not reading makefiles and we haven't looked yet, see if
     we can find a pattern variable.  */

  if (!reading && !file->pat_searched)
    {
      struct pattern_var *p = lookup_pattern_var (file->name);

      file->pat_searched = 1;
      if (p != 0)
        {
          /* If we found one, insert it between the current target's
             variables and the next set, whatever it is.  */
          file->pat_variables = (struct variable_set_list *)
            xmalloc (sizeof (struct variable_set_list));
          file->pat_variables->set = p->vars->set;
        }
    }

  /* If we have a pattern variable match, set it up.  */

  if (file->pat_variables != 0)
    {
      file->pat_variables->next = l->next;
      l->next = file->pat_variables;
    }
}

/* Pop the top set off the current variable set list,
   and free all its storage.  */

static void
free_variable_name_and_value (item)
     void *item;
{
  struct variable *v = (struct variable *) item;
  free (v->name);
  free (v->value);
}

void
pop_variable_scope ()
{
  struct variable_set_list *setlist = current_variable_set_list;
  struct variable_set *set = setlist->set;

  current_variable_set_list = setlist->next;
  free ((char *) setlist);

  hash_map (&set->table, free_variable_name_and_value);
  hash_free (&set->table, 1);

  free ((char *) set);
}

struct variable_set_list *
create_new_variable_set ()
{
  register struct variable_set_list *setlist;
  register struct variable_set *set;

  set = (struct variable_set *) xmalloc (sizeof (struct variable_set));
  hash_init (&set->table, SMALL_SCOPE_VARIABLE_BUCKETS,
	     variable_hash_1, variable_hash_2, variable_hash_cmp);

  setlist = (struct variable_set_list *)
    xmalloc (sizeof (struct variable_set_list));
  setlist->set = set;
  setlist->next = current_variable_set_list;

  return setlist;
}

/* Create a new variable set and push it on the current setlist.  */

struct variable_set_list *
push_new_variable_scope ()
{
  return (current_variable_set_list = create_new_variable_set());
}

/* Merge SET1 into SET0, freeing unused storage in SET1.  */

static void
merge_variable_sets (to_set, from_set)
     struct variable_set *to_set, *from_set;
{
  struct variable **from_var_slot = (struct variable **) from_set->table.ht_vec;
  struct variable **from_var_end = from_var_slot + from_set->table.ht_size;

  for ( ; from_var_slot < from_var_end; from_var_slot++)
    if (! HASH_VACANT (*from_var_slot))
      {
	struct variable *from_var = *from_var_slot;
	struct variable **to_var_slot
	  = (struct variable **) hash_find_slot (&to_set->table, *from_var_slot);
	if (HASH_VACANT (*to_var_slot))
	  hash_insert_at (&to_set->table, from_var, to_var_slot);
	else
	  {
	    /* GKM FIXME: delete in from_set->table */
	    free (from_var->value);
	    free (from_var);
	  }
      }
}

/* Merge SETLIST1 into SETLIST0, freeing unused storage in SETLIST1.  */

void
merge_variable_set_lists (setlist0, setlist1)
     struct variable_set_list **setlist0, *setlist1;
{
  register struct variable_set_list *list0 = *setlist0;
  struct variable_set_list *last0 = 0;

  while (setlist1 != 0 && list0 != 0)
    {
      struct variable_set_list *next = setlist1;
      setlist1 = setlist1->next;

      merge_variable_sets (list0->set, next->set);

      last0 = list0;
      list0 = list0->next;
    }

  if (setlist1 != 0)
    {
      if (last0 == 0)
	*setlist0 = setlist1;
      else
	last0->next = setlist1;
    }
}

/* Define the automatic variables, and record the addresses
   of their structures so we can change their values quickly.  */

void
define_automatic_variables ()
{
#ifdef WINDOWS32
  extern char* default_shell;
#else
  extern char default_shell[];
#endif
  register struct variable *v;
  char buf[200];

  sprintf (buf, "%u", makelevel);
  (void) define_variable (MAKELEVEL_NAME, MAKELEVEL_LENGTH, buf, o_env, 0);

  sprintf (buf, "%s%s%s",
	   version_string,
	   (remote_description == 0 || remote_description[0] == '\0')
	   ? "" : "-",
	   (remote_description == 0 || remote_description[0] == '\0')
	   ? "" : remote_description);
  (void) define_variable ("MAKE_VERSION", 12, buf, o_default, 0);

#ifdef  __MSDOS__
  /* Allow to specify a special shell just for Make,
     and use $COMSPEC as the default $SHELL when appropriate.  */
  {
    static char shell_str[] = "SHELL";
    const int shlen = sizeof (shell_str) - 1;
    struct variable *mshp = lookup_variable ("MAKESHELL", 9);
    struct variable *comp = lookup_variable ("COMSPEC", 7);

    /* Make $MAKESHELL override $SHELL even if -e is in effect.  */
    if (mshp)
      (void) define_variable (shell_str, shlen,
			      mshp->value, o_env_override, 0);
    else if (comp)
      {
	/* $COMSPEC shouldn't override $SHELL.  */
	struct variable *shp = lookup_variable (shell_str, shlen);

	if (!shp)
	  (void) define_variable (shell_str, shlen, comp->value, o_env, 0);
      }
  }
#endif

  /* This won't override any definition, but it
     will provide one if there isn't one there.  */
  v = define_variable ("SHELL", 5, default_shell, o_default, 0);
  v->export = v_export;		/* Always export SHELL.  */

  /* On MSDOS we do use SHELL from environment, since
     it isn't a standard environment variable on MSDOS,
     so whoever sets it, does that on purpose.  */
#ifndef __MSDOS__
  /* Don't let SHELL come from the environment.  */
  if (*v->value == '\0' || v->origin == o_env || v->origin == o_env_override)
    {
      free (v->value);
      v->origin = o_file;
      v->value = xstrdup (default_shell);
    }
#endif

  /* Make sure MAKEFILES gets exported if it is set.  */
  v = define_variable ("MAKEFILES", 9, "", o_default, 0);
  v->export = v_ifset;

  /* Define the magic D and F variables in terms of
     the automatic variables they are variations of.  */

#ifdef VMS
  define_variable ("@D", 2, "$(dir $@)", o_automatic, 1);
  define_variable ("%D", 2, "$(dir $%)", o_automatic, 1);
  define_variable ("*D", 2, "$(dir $*)", o_automatic, 1);
  define_variable ("<D", 2, "$(dir $<)", o_automatic, 1);
  define_variable ("?D", 2, "$(dir $?)", o_automatic, 1);
  define_variable ("^D", 2, "$(dir $^)", o_automatic, 1);
  define_variable ("+D", 2, "$(dir $+)", o_automatic, 1);
#else
  define_variable ("@D", 2, "$(patsubst %/,%,$(dir $@))", o_automatic, 1);
  define_variable ("%D", 2, "$(patsubst %/,%,$(dir $%))", o_automatic, 1);
  define_variable ("*D", 2, "$(patsubst %/,%,$(dir $*))", o_automatic, 1);
  define_variable ("<D", 2, "$(patsubst %/,%,$(dir $<))", o_automatic, 1);
  define_variable ("?D", 2, "$(patsubst %/,%,$(dir $?))", o_automatic, 1);
  define_variable ("^D", 2, "$(patsubst %/,%,$(dir $^))", o_automatic, 1);
  define_variable ("+D", 2, "$(patsubst %/,%,$(dir $+))", o_automatic, 1);
#endif
  define_variable ("@F", 2, "$(notdir $@)", o_automatic, 1);
  define_variable ("%F", 2, "$(notdir $%)", o_automatic, 1);
  define_variable ("*F", 2, "$(notdir $*)", o_automatic, 1);
  define_variable ("<F", 2, "$(notdir $<)", o_automatic, 1);
  define_variable ("?F", 2, "$(notdir $?)", o_automatic, 1);
  define_variable ("^F", 2, "$(notdir $^)", o_automatic, 1);
  define_variable ("+F", 2, "$(notdir $+)", o_automatic, 1);
}

int export_all_variables;

/* Create a new environment for FILE's commands.
   If FILE is nil, this is for the `shell' function.
   The child's MAKELEVEL variable is incremented.  */

char **
target_environment (file)
     struct file *file;
{
  struct variable_set_list *set_list;
  register struct variable_set_list *s;
  struct hash_table table;
  struct variable **v_slot;
  struct variable **v_end;
  struct variable makelevel_key;
  char **result_0;
  char **result;

  if (file == 0)
    set_list = current_variable_set_list;
  else
    set_list = file->variables;

  hash_init (&table, VARIABLE_BUCKETS,
	     variable_hash_1, variable_hash_2, variable_hash_cmp);

  /* Run through all the variable sets in the list,
     accumulating variables in TABLE.  */
  for (s = set_list; s != 0; s = s->next)
    {
      struct variable_set *set = s->set;
      v_slot = (struct variable **) set->table.ht_vec;
      v_end = v_slot + set->table.ht_size;
      for ( ; v_slot < v_end; v_slot++)
	if (! HASH_VACANT (*v_slot))
	  {
	    struct variable **new_slot;
	    struct variable *v = *v_slot;

	    /* If this is a per-target variable and it hasn't been touched
	       already then look up the global version and take its export
	       value.  */
	    if (v->per_target && v->export == v_default)
	      {
		struct variable *gv;

		gv = lookup_variable_in_set (v->name, strlen(v->name),
                                             &global_variable_set);
		if (gv)
		  v->export = gv->export;
	      }

	    switch (v->export)
	      {
	      case v_default:
		if (v->origin == o_default || v->origin == o_automatic)
		  /* Only export default variables by explicit request.  */
		  continue;

                /* The variable doesn't have a name that can be exported.  */
                if (! v->exportable)
                  continue;

		if (! export_all_variables
		    && v->origin != o_command
		    && v->origin != o_env && v->origin != o_env_override)
		  continue;
		break;

	      case v_export:
		break;

	      case v_noexport:
		continue;

	      case v_ifset:
		if (v->origin == o_default)
		  continue;
		break;
	      }

	    new_slot = (struct variable **) hash_find_slot (&table, v);
	    if (HASH_VACANT (*new_slot))
	      hash_insert_at (&table, v, new_slot);
	  }
    }

  makelevel_key.name = MAKELEVEL_NAME;
  makelevel_key.length = MAKELEVEL_LENGTH;
  hash_delete (&table, &makelevel_key);

  result = result_0 = (char **) xmalloc ((table.ht_fill + 2) * sizeof (char *));

  v_slot = (struct variable **) table.ht_vec;
  v_end = v_slot + table.ht_size;
  for ( ; v_slot < v_end; v_slot++)
    if (! HASH_VACANT (*v_slot))
      {
	struct variable *v = *v_slot;

	/* If V is recursively expanded and didn't come from the environment,
	   expand its value.  If it came from the environment, it should
	   go back into the environment unchanged.  */
	if (v->recursive
	    && v->origin != o_env && v->origin != o_env_override)
	  {
	    char *value = recursively_expand_for_file (v, file);
#ifdef WINDOWS32
	    if (strcmp(v->name, "Path") == 0 ||
		strcmp(v->name, "PATH") == 0)
	      convert_Path_to_windows32(value, ';');
#endif
	    *result++ = concat (v->name, "=", value);
	    free (value);
	  }
	else
	  {
#ifdef WINDOWS32
            if (strcmp(v->name, "Path") == 0 ||
                strcmp(v->name, "PATH") == 0)
              convert_Path_to_windows32(v->value, ';');
#endif
	    *result++ = concat (v->name, "=", v->value);
	  }
      }

  *result = (char *) xmalloc (100);
  (void) sprintf (*result, "%s=%u", MAKELEVEL_NAME, makelevel + 1);
  *++result = 0;

  hash_free (&table, 0);

  return result_0;
}

/* Given a variable, a value, and a flavor, define the variable.
   See the try_variable_definition() function for details on the parameters. */

struct variable *
do_variable_definition (flocp, varname, value, origin, flavor, target_var)
     const struct floc *flocp;
     const char *varname;
     char *value;
     enum variable_origin origin;
     enum variable_flavor flavor;
     int target_var;
{
  char *p, *alloc_value = NULL;
  struct variable *v;
  int append = 0;

  /* Calculate the variable's new value in VALUE.  */

  switch (flavor)
    {
    default:
    case f_bogus:
      /* Should not be possible.  */
      abort ();
    case f_simple:
      /* A simple variable definition "var := value".  Expand the value.
         We have to allocate memory since otherwise it'll clobber the
	 variable buffer, and we may still need that if we're looking at a
         target-specific variable.  */
      p = alloc_value = allocated_variable_expand (value);
      break;
    case f_conditional:
      /* A conditional variable definition "var ?= value".
         The value is set IFF the variable is not defined yet. */
      v = lookup_variable (varname, strlen (varname));
      if (v)
        return v;

      flavor = f_recursive;
      /* FALLTHROUGH */
    case f_recursive:
      /* A recursive variable definition "var = value".
	 The value is used verbatim.  */
      p = value;
      break;
    case f_append:
      {
        /* If we have += but we're in a target variable context, we want to
           append only with other variables in the context of this target.  */
        if (target_var)
          {
            append = 1;
            v = lookup_variable_in_set (varname, strlen (varname),
                                        current_variable_set_list->set);
          }
        else
          v = lookup_variable (varname, strlen (varname));

        if (v == 0)
          {
            /* There was no old value.
               This becomes a normal recursive definition.  */
            p = value;
            flavor = f_recursive;
          }
        else
          {
            /* Paste the old and new values together in VALUE.  */

            unsigned int oldlen, vallen;
            char *val;

            val = value;
            if (v->recursive)
              /* The previous definition of the variable was recursive.
                 The new value is the unexpanded old and new values. */
              flavor = f_recursive;
            else
              /* The previous definition of the variable was simple.
                 The new value comes from the old value, which was expanded
                 when it was set; and from the expanded new value.  Allocate
                 memory for the expansion as we may still need the rest of the
                 buffer if we're looking at a target-specific variable.  */
              val = alloc_value = allocated_variable_expand (val);

            oldlen = strlen (v->value);
            vallen = strlen (val);
            p = (char *) alloca (oldlen + 1 + vallen + 1);
            bcopy (v->value, p, oldlen);
            p[oldlen] = ' ';
            bcopy (val, &p[oldlen + 1], vallen + 1);
          }
      }
    }

#ifdef __MSDOS__
  /* Many Unix Makefiles include a line saying "SHELL=/bin/sh", but
     non-Unix systems don't conform to this default configuration (in
     fact, most of them don't even have `/bin').  On the other hand,
     $SHELL in the environment, if set, points to the real pathname of
     the shell.
     Therefore, we generally won't let lines like "SHELL=/bin/sh" from
     the Makefile override $SHELL from the environment.  But first, we
     look for the basename of the shell in the directory where SHELL=
     points, and along the $PATH; if it is found in any of these places,
     we define $SHELL to be the actual pathname of the shell.  Thus, if
     you have bash.exe installed as d:/unix/bash.exe, and d:/unix is on
     your $PATH, then SHELL=/usr/local/bin/bash will have the effect of
     defining SHELL to be "d:/unix/bash.exe".  */
  if ((origin == o_file || origin == o_override)
      && strcmp (varname, "SHELL") == 0)
    {
      char shellpath[PATH_MAX];
      extern char * __dosexec_find_on_path (const char *, char *[], char *);

      /* See if we can find "/bin/sh.exe", "/bin/sh.com", etc.  */
      if (__dosexec_find_on_path (p, (char **)0, shellpath))
	{
	  char *p;

	  for (p = shellpath; *p; p++)
	    {
	      if (*p == '\\')
		*p = '/';
	    }
	  v = define_variable_loc (varname, strlen (varname),
                                   shellpath, origin, flavor == f_recursive,
                                   flocp);
	}
      else
	{
	  char *shellbase, *bslash;
	  struct variable *pathv = lookup_variable ("PATH", 4);
	  char *path_string;
	  char *fake_env[2];
	  size_t pathlen = 0;

	  shellbase = strrchr (p, '/');
	  bslash = strrchr (p, '\\');
	  if (!shellbase || bslash > shellbase)
	    shellbase = bslash;
	  if (!shellbase && p[1] == ':')
	    shellbase = p + 1;
	  if (shellbase)
	    shellbase++;
	  else
	    shellbase = p;

	  /* Search for the basename of the shell (with standard
	     executable extensions) along the $PATH.  */
	  if (pathv)
	    pathlen = strlen (pathv->value);
	  path_string = (char *)xmalloc (5 + pathlen + 2 + 1);
	  /* On MSDOS, current directory is considered as part of $PATH.  */
	  sprintf (path_string, "PATH=.;%s", pathv ? pathv->value : "");
	  fake_env[0] = path_string;
	  fake_env[1] = (char *)0;
	  if (__dosexec_find_on_path (shellbase, fake_env, shellpath))
	    {
	      char *p;

	      for (p = shellpath; *p; p++)
		{
		  if (*p == '\\')
		    *p = '/';
		}
	      v = define_variable_loc (varname, strlen (varname),
                                       shellpath, origin,
                                       flavor == f_recursive, flocp);
	    }
	  else
	    v = lookup_variable (varname, strlen (varname));

	  free (path_string);
	}
    }
  else
#endif /* __MSDOS__ */
#ifdef WINDOWS32
  if ((origin == o_file || origin == o_override) && streq (varname, "SHELL"))
    {
      extern char *default_shell;

      /* Call shell locator function. If it returns TRUE, then
	 set no_default_sh_exe to indicate sh was found and
         set new value for SHELL variable.  */

      if (find_and_set_default_shell (p))
        {
          v = define_variable_in_set (varname, strlen (varname), default_shell,
                                      origin, flavor == f_recursive,
                                      (target_var
                                       ? current_variable_set_list->set
                                       : NULL),
                                      flocp);
          no_default_sh_exe = 0;
        }
      else
        v = lookup_variable (varname, strlen (varname));
    }
  else
#endif

  /* If we are defining variables inside an $(eval ...), we might have a
     different variable context pushed, not the global context (maybe we're
     inside a $(call ...) or something.  Since this function is only ever
     invoked in places where we want to define globally visible variables,
     make sure we define this variable in the global set.  */

  v = define_variable_in_set (varname, strlen (varname), p,
                              origin, flavor == f_recursive,
                              (target_var
                               ? current_variable_set_list->set : NULL),
                              flocp);
  v->append = append;

  if (alloc_value)
    free (alloc_value);

  return v;
}

/* Try to interpret LINE (a null-terminated string) as a variable definition.

   ORIGIN may be o_file, o_override, o_env, o_env_override,
   or o_command specifying that the variable definition comes
   from a makefile, an override directive, the environment with
   or without the -e switch, or the command line.

   See the comments for parse_variable_definition().

   If LINE was recognized as a variable definition, a pointer to its `struct
   variable' is returned.  If LINE is not a variable definition, NULL is
   returned.  */

struct variable *
try_variable_definition (flocp, line, origin, target_var)
     const struct floc *flocp;
     char *line;
     enum variable_origin origin;
     int target_var;
{
  register int c;
  register char *p = line;
  register char *beg;
  register char *end;
  enum variable_flavor flavor = f_bogus;
  char *name, *expanded_name;
  struct variable *v;

  while (1)
    {
      c = *p++;
      if (c == '\0' || c == '#')
	return 0;
      if (c == '=')
	{
	  end = p - 1;
	  flavor = f_recursive;
	  break;
	}
      else if (c == ':')
	if (*p == '=')
	  {
	    end = p++ - 1;
	    flavor = f_simple;
	    break;
	  }
	else
	  /* A colon other than := is a rule line, not a variable defn.  */
	  return 0;
      else if (c == '+' && *p == '=')
	{
	  end = p++ - 1;
	  flavor = f_append;
	  break;
	}
      else if (c == '?' && *p == '=')
        {
          end = p++ - 1;
          flavor = f_conditional;
          break;
        }
      else if (c == '$')
	{
	  /* This might begin a variable expansion reference.  Make sure we
	     don't misrecognize chars inside the reference as =, := or +=.  */
	  char closeparen;
	  int count;
	  c = *p++;
	  if (c == '(')
	    closeparen = ')';
	  else if (c == '{')
	    closeparen = '}';
	  else
	    continue;		/* Nope.  */

	  /* P now points past the opening paren or brace.
	     Count parens or braces until it is matched.  */
	  count = 0;
	  for (; *p != '\0'; ++p)
	    {
	      if (*p == c)
		++count;
	      else if (*p == closeparen && --count < 0)
		{
		  ++p;
		  break;
		}
	    }
	}
    }

  beg = next_token (line);
  while (end > beg && isblank ((unsigned char)end[-1]))
    --end;
  p = next_token (p);

  /* Expand the name, so "$(foo)bar = baz" works.  */
  name = (char *) alloca (end - beg + 1);
  bcopy (beg, name, end - beg);
  name[end - beg] = '\0';
  expanded_name = allocated_variable_expand (name);

  if (expanded_name[0] == '\0')
    fatal (flocp, _("empty variable name"));

  v = do_variable_definition (flocp, expanded_name, p,
                              origin, flavor, target_var);

  free (expanded_name);

  return v;
}

/* Print information for variable V, prefixing it with PREFIX.  */

static void
print_variable (v, prefix)
     register struct variable *v;
     char *prefix;
{
  const char *origin;

  switch (v->origin)
    {
    case o_default:
      origin = _("default");
      break;
    case o_env:
      origin = _("environment");
      break;
    case o_file:
      origin = _("makefile");
      break;
    case o_env_override:
      origin = _("environment under -e");
      break;
    case o_command:
      origin = _("command line");
      break;
    case o_override:
      origin = _("`override' directive");
      break;
    case o_automatic:
      origin = _("automatic");
      break;
    case o_invalid:
    default:
      abort ();
    }
  fputs ("# ", stdout);
  fputs (origin, stdout);
  if (v->fileinfo.filenm)
    printf (_(" (from `%s', line %lu)"),
            v->fileinfo.filenm, v->fileinfo.lineno);
  putchar ('\n');
  fputs (prefix, stdout);

  /* Is this a `define'?  */
  if (v->recursive && strchr (v->value, '\n') != 0)
    printf ("define %s\n%s\nendef\n", v->name, v->value);
  else
    {
      register char *p;

      printf ("%s %s= ", v->name, v->recursive ? v->append ? "+" : "" : ":");

      /* Check if the value is just whitespace.  */
      p = next_token (v->value);
      if (p != v->value && *p == '\0')
	/* All whitespace.  */
	printf ("$(subst ,,%s)", v->value);
      else if (v->recursive)
	fputs (v->value, stdout);
      else
	/* Double up dollar signs.  */
	for (p = v->value; *p != '\0'; ++p)
	  {
	    if (*p == '$')
	      putchar ('$');
	    putchar (*p);
	  }
      putchar ('\n');
    }
}


/* Print all the variables in SET.  PREFIX is printed before
   the actual variable definitions (everything else is comments).  */

void
print_variable_set (set, prefix)
     register struct variable_set *set;
     char *prefix;
{
  hash_map_arg (&set->table, print_variable, prefix);

  fputs (_("# variable set hash-table stats:\n"), stdout);
  fputs ("# ", stdout);
  hash_print_stats (&set->table, stdout);
  putc ('\n', stdout);
}

/* Print the data base of variables.  */

void
print_variable_data_base ()
{
  puts (_("\n# Variables\n"));

  print_variable_set (&global_variable_set, "");
}


/* Print all the local variables of FILE.  */

void
print_file_variables (file)
     struct file *file;
{
  if (file->variables != 0)
    print_variable_set (file->variables->set, "# ");
}

#ifdef WINDOWS32
void
sync_Path_environment(void)
{
    char* path = allocated_variable_expand("$(Path)");
    static char* environ_path = NULL;

    if (!path)
        return;

    /*
     * If done this before, don't leak memory unnecessarily.
     * Free the previous entry before allocating new one.
     */
    if (environ_path)
        free(environ_path);

    /*
     * Create something WINDOWS32 world can grok
     */
    convert_Path_to_windows32(path, ';');
    environ_path = concat("Path", "=", path);
    putenv(environ_path);
    free(path);
}
#endif
