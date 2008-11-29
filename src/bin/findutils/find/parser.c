/* parser.c -- convert the command line args into an expression tree.
   Copyright (C) 1990, 1991, 1992, 1993, 1994, 2000, 2001, 2003, 
                 2004, 2005, 2007 Free Software Foundation, Inc.

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
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <fnmatch.h>
#include "modechange.h"
#include "modetype.h"
#include "xstrtol.h"
#include "xalloc.h"
#include "quote.h"
#include "quotearg.h"
#include "buildcmd.h"
#include "nextelem.h"
#include "stdio-safer.h"
#include "regextype.h"
#include "gnulib-version.h"

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

/* The presence of unistd.h is assumed by gnulib these days, so we 
 * might as well assume it too. 
 */
/* We need <unistd.h> for isatty(). */
#include <unistd.h> 

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

#if !defined (isascii) || defined (STDC_HEADERS)
#ifdef isascii
#undef isascii
#endif
#define isascii(c) 1
#endif

#define ISDIGIT(c) (isascii ((unsigned char)c) && isdigit ((unsigned char)c))
#define ISUPPER(c) (isascii ((unsigned char)c) && isupper ((unsigned char)c))

#ifndef HAVE_ENDGRENT
#define endgrent()
#endif
#ifndef HAVE_ENDPWENT
#define endpwent()
#endif

static boolean parse_amin          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_and           PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_anewer        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_atime         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_cmin          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_cnewer        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_comma         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_ctime         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_daystart      PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_delete        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_d             PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_depth         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_empty         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_exec          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_execdir       PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_false         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_fls           PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_fprintf       PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_follow        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_fprint        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_fprint0       PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_fstype        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_gid           PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_group         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_help          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_ilname        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_iname         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_inum          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_ipath         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_iregex        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_iwholename    PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_links         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_lname         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_ls            PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_maxdepth      PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_mindepth      PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_mmin          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_mtime         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_name          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_negate        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_newer         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_noleaf        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_nogroup       PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_nouser        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_nowarn        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_ok            PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_okdir         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_or            PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_path          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_perm          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_print0        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_printf        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_prune         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_regex         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_regextype     PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_samefile      PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_size          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_true          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_type          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_uid           PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_used          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_user          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_version       PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_wholename     PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_xdev          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_ignore_race   PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_noignore_race PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_warn          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_xtype         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_quit          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));



boolean parse_print             PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));



static boolean insert_type PARAMS((char **argv, int *arg_ptr, const struct parser_table *entry, PRED_FUNC which_pred));
static boolean insert_regex PARAMS((char *argv[], int *arg_ptr, const struct parser_table *entry, int regex_options));
static boolean insert_fprintf PARAMS((FILE *fp, const struct parser_table *entry, PRED_FUNC func, char *argv[], int *arg_ptr));

static struct segment **make_segment PARAMS((struct segment **segment, char *format, int len, int kind));
static boolean insert_exec_ok PARAMS((const char *action, const struct parser_table *entry, char *argv[], int *arg_ptr));
static boolean get_num_days PARAMS((char *str, uintmax_t *num_days, enum comparison_type *comp_type));
static boolean insert_time PARAMS((char **argv, int *arg_ptr, const struct parser_table* entry, PRED_FUNC pred));
static boolean get_num PARAMS((char *str, uintmax_t *num, enum comparison_type *comp_type));
static boolean insert_num PARAMS((char *argv[], int *arg_ptr, const struct parser_table *entry));
static FILE *open_output_file PARAMS((char *path));
static boolean stream_is_tty(FILE *fp);

#ifdef DEBUG
char *find_pred_name PARAMS((PRED_FUNC pred_func));
#endif /* DEBUG */


#define PASTE(x,y) x##y
#define STRINGIFY(s) #s

#define PARSE_OPTION(what,suffix) \
  { (ARG_OPTION), (what), PASTE(parse_,suffix), NULL }

#define PARSE_POSOPT(what,suffix) \
  { (ARG_POSITIONAL_OPTION), (what), PASTE(parse_,suffix), NULL }

#define PARSE_TEST(what,suffix) \
  { (ARG_TEST), (what), PASTE(parse_,suffix), PASTE(pred_,suffix) }

#define PARSE_TEST_NP(what,suffix) \
  { (ARG_TEST), (what), PASTE(parse_,suffix), NULL }

#define PARSE_ACTION(what,suffix) \
  { (ARG_ACTION), (what), PASTE(parse_,suffix), PASTE(pred_,suffix) }

#define PARSE_ACTION_NP(what,suffix) \
  { (ARG_ACTION), (what), PASTE(parse_,suffix), NULL }

#define PARSE_PUNCTUATION(what,suffix) \
  { (ARG_PUNCTUATION), (what), PASTE(parse_,suffix), PASTE(pred_,suffix) }


/* GNU find predicates that are not mentioned in POSIX.2 are marked `GNU'.
   If they are in some Unix versions of find, they are marked `Unix'. */

static struct parser_table const parse_table[] =
{
 PARSE_PUNCTUATION("!",                     negate),
  PARSE_PUNCTUATION("not",                   negate),	     /* GNU */
  PARSE_PUNCTUATION("(",                     openparen),
  PARSE_PUNCTUATION(")",                     closeparen),
  PARSE_PUNCTUATION(",",                     comma),	     /* GNU */
  PARSE_PUNCTUATION("a",                     and),
  PARSE_TEST       ("amin",                  amin),	     /* GNU */
  PARSE_PUNCTUATION("and",                   and),		/* GNU */
  PARSE_TEST       ("anewer",                anewer),	     /* GNU */
  PARSE_TEST       ("atime",                 atime),
  PARSE_TEST       ("cmin",                  cmin),	     /* GNU */
  PARSE_TEST       ("cnewer",                cnewer),	     /* GNU */
  PARSE_TEST       ("ctime",                 ctime),
  PARSE_POSOPT     ("daystart",              daystart),	     /* GNU */
  PARSE_ACTION     ("delete",                delete), /* GNU, Mac OS, FreeBSD */
  PARSE_OPTION     ("d",                     d), /* Mac OS X, FreeBSD, NetBSD, OpenBSD, but deprecated  in favour of -depth */
  PARSE_OPTION     ("depth",                 depth),
  PARSE_TEST       ("empty",                 empty),	     /* GNU */
  {ARG_ACTION,      "exec",    parse_exec, pred_exec}, /* POSIX */
  PARSE_ACTION     ("execdir",               execdir), /* *BSD, GNU */
  PARSE_ACTION     ("fls",                   fls),	     /* GNU */
  PARSE_POSOPT     ("follow",                follow),  /* GNU, Unix */
  PARSE_ACTION     ("fprint",                fprint),	     /* GNU */
  PARSE_ACTION     ("fprint0",               fprint0),	     /* GNU */
  {ARG_ACTION,      "fprintf", parse_fprintf, pred_fprintf}, /* GNU */
  PARSE_TEST       ("fstype",                fstype),  /* GNU, Unix */
  PARSE_TEST       ("gid",                   gid),	     /* GNU */
  PARSE_TEST       ("group",                 group),
  PARSE_OPTION     ("ignore_readdir_race",   ignore_race),   /* GNU */
  PARSE_TEST       ("ilname",                ilname),	     /* GNU */
  PARSE_TEST       ("iname",                 iname),	     /* GNU */
  PARSE_TEST       ("inum",                  inum),    /* GNU, Unix */
  PARSE_TEST       ("ipath",                 ipath), /* GNU, deprecated in favour of iwholename */
  PARSE_TEST_NP    ("iregex",                iregex),	     /* GNU */
  PARSE_TEST_NP    ("iwholename",            iwholename),    /* GNU */
  PARSE_TEST       ("links",                 links),
  PARSE_TEST       ("lname",                 lname),	     /* GNU */
  PARSE_ACTION     ("ls",                    ls),      /* GNU, Unix */
  PARSE_OPTION     ("maxdepth",              maxdepth),	     /* GNU */
  PARSE_OPTION     ("mindepth",              mindepth),	     /* GNU */
  PARSE_TEST       ("mmin",                  mmin),	     /* GNU */
  PARSE_OPTION     ("mount",                 xdev),	    /* Unix */
  PARSE_TEST       ("mtime",                 mtime),
  PARSE_TEST       ("name",                  name),
#ifdef UNIMPLEMENTED_UNIX	                    
  PARSE(ARG_UNIMPLEMENTED, "ncpio",          ncpio),	    /* Unix */
#endif				                    
  PARSE_TEST       ("newer",                 newer),
  PARSE_OPTION     ("noleaf",                noleaf),	     /* GNU */
  PARSE_TEST       ("nogroup",               nogroup),
  PARSE_TEST       ("nouser",                nouser),
  PARSE_OPTION     ("noignore_readdir_race", noignore_race), /* GNU */
  PARSE_OPTION     ("nowarn",                nowarn),	     /* GNU */
  PARSE_PUNCTUATION("o",                     or),
  PARSE_PUNCTUATION("or",                    or),	     /* GNU */
  PARSE_ACTION     ("ok",                    ok),
  PARSE_ACTION     ("okdir",                 okdir), /* GNU (-execdir is BSD) */
  PARSE_TEST       ("path",                  path), /* GNU, HP-UX, GNU prefers wholename */
  PARSE_TEST       ("perm",                  perm),
  PARSE_ACTION     ("print",                 print),
  PARSE_ACTION     ("print0",                print0),	     /* GNU */
  {ARG_ACTION,      "printf",   parse_printf, NULL},	     /* GNU */
  PARSE_ACTION     ("prune",                 prune),
  PARSE_ACTION     ("quit",                  quit),	     /* GNU */
  PARSE_TEST       ("regex",                 regex),	     /* GNU */
  PARSE_OPTION     ("regextype",             regextype),     /* GNU */
  PARSE_TEST       ("samefile",              samefile),	     /* GNU */
  PARSE_TEST       ("size",                  size),
  PARSE_TEST       ("type",                  type),
  PARSE_TEST       ("uid",                   uid),	     /* GNU */
  PARSE_TEST       ("used",                  used),	     /* GNU */
  PARSE_TEST       ("user",                  user),
  PARSE_OPTION     ("warn",                  warn),	     /* GNU */
  PARSE_TEST_NP    ("wholename",             wholename), /* GNU, replaces -path */
  PARSE_OPTION     ("xdev",                  xdev),
  PARSE_TEST       ("xtype",                 xtype),	     /* GNU */
#ifdef UNIMPLEMENTED_UNIX
  /* It's pretty ugly for find to know about archive formats.
     Plus what it could do with cpio archives is very limited.
     Better to leave it out.  */
  PARSE(ARG_UNIMPLEMENTED,      "cpio",                  cpio),	/* Unix */
#endif
  /* gnulib's stdbool.h might have made true and false into macros, 
   * so we can't leave named 'true' and 'false' tokens, so we have 
   * to expeant the relevant entries longhand. 
   */
  {ARG_TEST, "false",                 parse_false,   pred_false}, /* GNU */
  {ARG_TEST, "true",                  parse_true,    pred_true }, /* GNU */

  /* Various other cases that don't fit neatly into our macro scheme. */
  {ARG_TEST, "help",                  parse_help,    NULL},       /* GNU */
  {ARG_TEST, "-help",                 parse_help,    NULL},       /* GNU */
  {ARG_TEST, "version",               parse_version, NULL},	  /* GNU */
  {ARG_TEST, "-version",              parse_version, NULL},	  /* GNU */
  {0, 0, 0, 0}
};


static const char *first_nonoption_arg = NULL;



void
parse_begin_user_args (char **args, int argno, const struct predicate *last, const struct predicate *predicates)
{
  (void) args;
  (void) argno;
  (void) last;
  (void) predicates;
  first_nonoption_arg = NULL;
}

void 
parse_end_user_args (char **args, int argno, const struct predicate *last, const struct predicate *predicates)
{
  /* does nothing */
  (void) args;
  (void) argno;
  (void) last;
  (void) predicates;
}




/* Return a pointer to the parser function to invoke for predicate
   SEARCH_NAME.
   Return NULL if SEARCH_NAME is not a valid predicate name. */

const struct parser_table*
find_parser (char *search_name)
{
  int i;
  const char *original_arg = search_name;
  
  if (*search_name == '-')
    search_name++;
  for (i = 0; parse_table[i].parser_name != 0; i++)
    {
      if (strcmp (parse_table[i].parser_name, search_name) == 0)
	{
	  /* If this is an option, but we have already had a 
	   * non-option argument, the user may be under the 
	   * impression that the behaviour of the option 
	   * argument is conditional on some preceding 
	   * tests.  This might typically be the case with,
	   * for example, -maxdepth.
	   *
	   * The options -daystart and -follow are exempt 
	   * from this treatment, since their positioning
	   * in the command line does have an effect on 
	   * subsequent tests but not previous ones.  That
	   * might be intentional on the part of the user.
	   */
	  if (parse_table[i].type != ARG_POSITIONAL_OPTION)
	    {
	      /* Something other than -follow/-daystart.
	       * If this is an option, check if it followed
	       * a non-option and if so, issue a warning.
	       */
	      if (parse_table[i].type == ARG_OPTION)
		{
		  if ((first_nonoption_arg != NULL)
		      && options.warnings )
		    {
		      /* option which follows a non-option */
		      error (0, 0,
			     _("warning: you have specified the %s "
			       "option after a non-option argument %s, "
			       "but options are not positional (%s affects "
			       "tests specified before it as well as those "
			       "specified after it).  Please specify options "
			       "before other arguments.\n"),
			     original_arg,
			     first_nonoption_arg,
			     original_arg);
		    }
		}
	      else
		{
		  /* Not an option or a positional option,
		   * so remember we've seen it in order to 
		   * use it in a possible future warning message.
		   */
		  if (first_nonoption_arg == NULL)
		    {
		      first_nonoption_arg = original_arg;
		    }
		}
	    }
	  
	  return &parse_table[i];
	}
    }
  return NULL;
}

/* The parsers are responsible to continue scanning ARGV for
   their arguments.  Each parser knows what is and isn't
   allowed for itself.
   
   ARGV is the argument array.
   *ARG_PTR is the index to start at in ARGV,
   updated to point beyond the last element consumed.
 
   The predicate structure is updated with the new information. */

static boolean
parse_amin (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;
  uintmax_t num;
  enum comparison_type c_type;
  time_t t;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  if (!get_num_days (argv[*arg_ptr], &num, &c_type))
    return false;
  t = options.cur_day_start + DAYSECS - num * 60;
  our_pred = insert_primary (entry);
  our_pred->args.info.kind = c_type;
  our_pred->args.info.negative = t < 0;
  our_pred->args.info.l_val = t;
  (*arg_ptr)++;
  return true;
}

static boolean
parse_and (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) argv;
  (void) arg_ptr;
  
  our_pred = get_new_pred (entry);
  our_pred->pred_func = pred_and;
#ifdef	DEBUG
  our_pred->p_name = find_pred_name (pred_and);
#endif	/* DEBUG */
  our_pred->p_type = BI_OP;
  our_pred->p_prec = AND_PREC;
  our_pred->need_stat = our_pred->need_type = false;
  return true;
}

static boolean
parse_anewer (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;
  struct stat stat_newer;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  if ((*options.xstat) (argv[*arg_ptr], &stat_newer))
    error (1, errno, "%s", argv[*arg_ptr]);
  our_pred = insert_primary (entry);
  our_pred->args.time = stat_newer.st_mtime;
  (*arg_ptr)++;
  return true;
}

static boolean
parse_atime (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_time (argv, arg_ptr, entry, pred_atime);
}

boolean
parse_closeparen (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) argv;
  (void) arg_ptr;
  
  our_pred = get_new_pred (entry);
  our_pred->pred_func = pred_closeparen;
#ifdef	DEBUG
  our_pred->p_name = find_pred_name (pred_closeparen);
#endif	/* DEBUG */
  our_pred->p_type = CLOSE_PAREN;
  our_pred->p_prec = NO_PREC;
  our_pred->need_stat = our_pred->need_type = false;
  return true;
}

static boolean
parse_cmin (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;
  uintmax_t num;
  enum comparison_type c_type;
  time_t t;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  if (!get_num_days (argv[*arg_ptr], &num, &c_type))
    return false;
  t = options.cur_day_start + DAYSECS - num * 60;
  our_pred = insert_primary (entry);
  our_pred->args.info.kind = c_type;
  our_pred->args.info.negative = t < 0;
  our_pred->args.info.l_val = t;
  (*arg_ptr)++;
  return true;
}

static boolean
parse_cnewer (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;
  struct stat stat_newer;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  if ((*options.xstat) (argv[*arg_ptr], &stat_newer))
    error (1, errno, "%s", argv[*arg_ptr]);
  our_pred = insert_primary (entry);
  our_pred->args.time = stat_newer.st_mtime;
  (*arg_ptr)++;
  return true;
}

static boolean
parse_comma (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) argv;
  (void) arg_ptr;

  our_pred = get_new_pred (entry);
  our_pred->pred_func = pred_comma;
#ifdef	DEBUG
  our_pred->p_name = find_pred_name (pred_comma);
#endif /* DEBUG */
  our_pred->p_type = BI_OP;
  our_pred->p_prec = COMMA_PREC;
  our_pred->need_stat = our_pred->need_type = false;
  return true;
}

static boolean
parse_ctime (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_time (argv, arg_ptr, entry, pred_ctime);
}

static boolean
parse_daystart (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct tm *local;

  (void) entry;
  (void) argv;
  (void) arg_ptr;

  if (options.full_days == false)
    {
      options.cur_day_start += DAYSECS;
      local = localtime (&options.cur_day_start);
      options.cur_day_start -= (local
			? (local->tm_sec + local->tm_min * 60
			   + local->tm_hour * 3600)
			: options.cur_day_start % DAYSECS);
      options.full_days = true;
    }
  return true;
}

static boolean
parse_delete (const struct parser_table* entry, char *argv[], int *arg_ptr)
{
  struct predicate *our_pred;
  (void) argv;
  (void) arg_ptr;
  
  our_pred = insert_primary (entry);
  our_pred->side_effects = our_pred->no_default_print = true;
  /* -delete implies -depth */
  options.do_dir_first = false;
  return true;
}

static boolean
parse_depth (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  (void) entry;
  (void) argv;
  (void) arg_ptr;

  options.do_dir_first = false;
  return true;
}
 
static boolean
parse_d (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  (void) argv;
  (void) arg_ptr;
  
  if (options.warnings)
    {
      error (0, 0,
	     _("warning: the -d option is deprecated; please use -depth instead, because the latter is a POSIX-compliant feature."));
    }
  return parse_depth(entry, argv, arg_ptr);
}
 
static boolean
parse_empty (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  (void) argv;
  (void) arg_ptr;

  insert_primary (entry);
  return true;
}

static boolean
parse_exec (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_exec_ok ("-exec", entry, argv, arg_ptr);
}

static boolean
parse_execdir (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_exec_ok ("-execdir", entry, argv, arg_ptr);
}

static boolean
parse_false (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;
  
  (void) argv;
  (void) arg_ptr;

  our_pred = insert_primary (entry);
  our_pred->need_stat = our_pred->need_type = false;
  our_pred->side_effects = our_pred->no_default_print = false;
  return true;
}

static boolean
parse_fls (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  our_pred = insert_primary (entry);
  our_pred->args.stream = open_output_file (argv[*arg_ptr]);
  our_pred->side_effects = our_pred->no_default_print = true;
  (*arg_ptr)++;
  return true;
}

static boolean 
parse_fprintf (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  FILE *fp;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  if (argv[*arg_ptr + 1] == NULL)
    {
      /* Ensure we get "missing arg" message, not "invalid arg".  */
      (*arg_ptr)++;
      return false;
    }
  fp = open_output_file (argv[*arg_ptr]);
  (*arg_ptr)++;
  return insert_fprintf (fp, entry, pred_fprintf, argv, arg_ptr);
}

static boolean
parse_follow (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  (void) entry;
  (void) argv;
  (void) arg_ptr;

  set_follow_state(SYMLINK_ALWAYS_DEREF);
  return true;
}

static boolean
parse_fprint (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  our_pred = insert_primary (entry);
  our_pred->args.printf_vec.segment = NULL;
  our_pred->args.printf_vec.stream = open_output_file (argv[*arg_ptr]);
  our_pred->args.printf_vec.dest_is_tty = stream_is_tty(our_pred->args.printf_vec.stream);
  our_pred->args.printf_vec.quote_opts = clone_quoting_options (NULL);
  our_pred->side_effects = our_pred->no_default_print = true;
  our_pred->need_stat = our_pred->need_type = false;
  (*arg_ptr)++;
  return true;
}

static boolean
parse_fprint0 (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  our_pred = insert_primary (entry);
  our_pred->args.stream = open_output_file (argv[*arg_ptr]);
  our_pred->side_effects = our_pred->no_default_print = true;
  our_pred->need_stat = our_pred->need_type = false;
  (*arg_ptr)++;
  return true;
}

static boolean
parse_fstype (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  our_pred = insert_primary (entry);
  our_pred->args.str = argv[*arg_ptr];
  (*arg_ptr)++;
  return true;
}

static boolean
parse_gid (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_num (argv, arg_ptr, entry);
}

static boolean
parse_group (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct group *cur_gr;
  struct predicate *our_pred;
  gid_t gid;
  int gid_len;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  cur_gr = getgrnam (argv[*arg_ptr]);
  endgrent ();
  if (cur_gr != NULL)
    gid = cur_gr->gr_gid;
  else
    {
      gid_len = strspn (argv[*arg_ptr], "0123456789");
      if ((gid_len == 0) || (argv[*arg_ptr][gid_len] != '\0'))
	return false;
      gid = atoi (argv[*arg_ptr]);
    }
  our_pred = insert_primary (entry);
  our_pred->args.gid = gid;
  (*arg_ptr)++;
  return true;
}

static boolean
parse_help (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  (void) entry;
  (void) argv;
  (void) arg_ptr;
  
  printf (_("\
Usage: %s [path...] [expression]\n"), program_name);
  puts (_("\n\
default path is the current directory; default expression is -print\n\
expression may consist of: operators, options, tests, and actions:\n"));
  puts (_("\
operators (decreasing precedence; -and is implicit where no others are given):\n\
      ( EXPR )   ! EXPR   -not EXPR   EXPR1 -a EXPR2   EXPR1 -and EXPR2\n\
      EXPR1 -o EXPR2   EXPR1 -or EXPR2   EXPR1 , EXPR2\n"));
  puts (_("\
positional options (always true): -daystart -follow -regextype\n\n\
normal options (always true, specified before other expressions):\n\
      -depth --help -maxdepth LEVELS -mindepth LEVELS -mount -noleaf\n\
      --version -xdev -ignore_readdir_race -noignore_readdir_race\n"));
  puts (_("\
tests (N can be +N or -N or N): -amin N -anewer FILE -atime N -cmin N\n\
      -cnewer FILE -ctime N -empty -false -fstype TYPE -gid N -group NAME\n\
      -ilname PATTERN -iname PATTERN -inum N -iwholename PATTERN -iregex PATTERN\n\
      -links N -lname PATTERN -mmin N -mtime N -name PATTERN -newer FILE"));
  puts (_("\
      -nouser -nogroup -path PATTERN -perm [+-]MODE -regex PATTERN\n\
      -wholename PATTERN -size N[bcwkMG] -true -type [bcdpflsD] -uid N\n\
      -used N -user NAME -xtype [bcdpfls]\n"));
  puts (_("\
actions: -delete -print0 -printf FORMAT -fprintf FILE FORMAT -print \n\
      -fprint0 FILE -fprint FILE -ls -fls FILE -prune -quit\n\
      -exec COMMAND ; -exec COMMAND {} + -ok COMMAND ;\n\
      -execdir COMMAND ; -execdir COMMAND {} + -okdir COMMAND ;\n\
"));
  puts (_("Report (and track progress on fixing) bugs via the findutils bug-reporting\n\
page at http://savannah.gnu.org/ or, if you have no web access, by sending\n\
email to <bug-findutils@gnu.org>."));
  exit (0);
}

static boolean
parse_ilname (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  our_pred = insert_primary (entry);
  our_pred->args.str = argv[*arg_ptr];
  (*arg_ptr)++;
  return true;
}


/* sanity check the fnmatch() function to make sure
 * it really is the GNU version. 
 */
static boolean 
fnmatch_sanitycheck(void)
{
  /* fprintf(stderr, "Performing find sanity check..."); */
  if (0 != fnmatch("foo", "foo", 0)
      || 0 == fnmatch("Foo", "foo", 0)
      || 0 != fnmatch("Foo", "foo", FNM_CASEFOLD))
    {
      error (1, 0, _("sanity check of the fnmatch() library function failed."));
      /* fprintf(stderr, "FAILED\n"); */
      return false;
    }

  /* fprintf(stderr, "OK\n"); */
  return true;
}


static boolean
check_name_arg(const char *pred, const char *arg)
{
  if (options.warnings && strchr(arg, '/'))
    {
      error(0, 0,_("warning: Unix filenames usually don't contain slashes (though pathnames do).  That means that '%s %s' will probably evaluate to false all the time on this system.  You might find the '-wholename' test more useful, or perhaps '-samefile'.  Alternatively, if you are using GNU grep, you could use 'find ... -print0 | grep -FzZ %s'."),
	    pred, arg, arg);
    }
  return true;			/* allow it anyway */
}



static boolean
parse_iname (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  if (!check_name_arg("-iname", argv[*arg_ptr]))
    return false;

  fnmatch_sanitycheck();
  
  our_pred = insert_primary (entry);
  our_pred->need_stat = our_pred->need_type = false;
  our_pred->args.str = argv[*arg_ptr];
  (*arg_ptr)++;
  return true;
}

static boolean
parse_inum (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_num (argv, arg_ptr, entry);
}

/* -ipath is deprecated (at RMS's request) in favour of 
 * -iwholename.   See the node "GNU Manuals" in standards.texi
 * for the rationale for this (basically, GNU prefers the use 
 * of the phrase "file name" to "path name"
 */
static boolean
parse_ipath (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  error (0, 0,
	 _("warning: the predicate -ipath is deprecated; please use -iwholename instead."));
  
  return parse_iwholename(entry, argv, arg_ptr);
}

static boolean
parse_iwholename (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;

  fnmatch_sanitycheck();
  
  our_pred = insert_primary_withpred (entry, pred_ipath);
  our_pred->need_stat = our_pred->need_type = false;
  our_pred->args.str = argv[*arg_ptr];
  (*arg_ptr)++;
  return true;
}

static boolean
parse_iregex (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_regex (argv, arg_ptr, entry, RE_ICASE|options.regex_options);
}

static boolean
parse_links (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_num (argv, arg_ptr, entry);
}

static boolean
parse_lname (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) argv;
  (void) arg_ptr;
  
  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;

  fnmatch_sanitycheck();
  
  our_pred = insert_primary (entry);
  our_pred->args.str = argv[*arg_ptr];
  (*arg_ptr)++;
  return true;
}

static boolean
parse_ls (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) &argv;
  (void) &arg_ptr;

  our_pred = insert_primary (entry);
  our_pred->side_effects = our_pred->no_default_print = true;
  return true;
}

static boolean
parse_maxdepth (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  int depth_len;
  (void) entry;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  depth_len = strspn (argv[*arg_ptr], "0123456789");
  if ((depth_len == 0) || (argv[*arg_ptr][depth_len] != '\0'))
    return false;
  options.maxdepth = atoi (argv[*arg_ptr]);
  if (options.maxdepth < 0)
    return false;
  (*arg_ptr)++;
  return true;
}

static boolean
parse_mindepth (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  int depth_len;
  (void) entry;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  depth_len = strspn (argv[*arg_ptr], "0123456789");
  if ((depth_len == 0) || (argv[*arg_ptr][depth_len] != '\0'))
    return false;
  options.mindepth = atoi (argv[*arg_ptr]);
  if (options.mindepth < 0)
    return false;
  (*arg_ptr)++;
  return true;
}

static boolean
parse_mmin (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;
  uintmax_t num;
  enum comparison_type c_type;
  time_t t;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  if (!get_num_days (argv[*arg_ptr], &num, &c_type))
    return false;
  t = options.cur_day_start + DAYSECS - num * 60;
  our_pred = insert_primary (entry);
  our_pred->args.info.kind = c_type;
  our_pred->args.info.negative = t < 0;
  our_pred->args.info.l_val = t;
  (*arg_ptr)++;
  return true;
}

static boolean
parse_mtime (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_time (argv, arg_ptr, entry, pred_mtime);
}

static boolean
parse_name (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) argv;
  (void) arg_ptr;
  
  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  if (!check_name_arg("-name", argv[*arg_ptr]))
    return false;
  fnmatch_sanitycheck();
  
  our_pred = insert_primary (entry);
  our_pred->need_stat = our_pred->need_type = false;
  our_pred->args.str = argv[*arg_ptr];
  (*arg_ptr)++;
  return true;
}

static boolean
parse_negate (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) &argv;
  (void) &arg_ptr;

  our_pred = get_new_pred_chk_op (entry);
  our_pred->pred_func = pred_negate;
#ifdef	DEBUG
  our_pred->p_name = find_pred_name (pred_negate);
#endif	/* DEBUG */
  our_pred->p_type = UNI_OP;
  our_pred->p_prec = NEGATE_PREC;
  our_pred->need_stat = our_pred->need_type = false;
  return true;
}

static boolean
parse_newer (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;
  struct stat stat_newer;

  (void) argv;
  (void) arg_ptr;
  
  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  if ((*options.xstat) (argv[*arg_ptr], &stat_newer))
    error (1, errno, "%s", argv[*arg_ptr]);
  our_pred = insert_primary (entry);
  our_pred->args.time = stat_newer.st_mtime;
  (*arg_ptr)++;
  return true;
}

static boolean
parse_noleaf (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  (void) &argv;
  (void) &arg_ptr;
  (void) entry;
  
  options.no_leaf_check = true;
  return true;
}

#ifdef CACHE_IDS
/* Arbitrary amount by which to increase size
   of `uid_unused' and `gid_unused'. */
#define ALLOC_STEP 2048

/* Boolean: if uid_unused[n] is nonzero, then UID n has no passwd entry. */
char *uid_unused = NULL;

/* Number of elements in `uid_unused'. */
unsigned uid_allocated;

/* Similar for GIDs and group entries. */
char *gid_unused = NULL;
unsigned gid_allocated;
#endif

static boolean
parse_nogroup (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) &argv;
  (void) &arg_ptr;
  
  our_pred = insert_primary (entry);
#ifdef CACHE_IDS
  if (gid_unused == NULL)
    {
      struct group *gr;

      gid_allocated = ALLOC_STEP;
      gid_unused = xmalloc (gid_allocated);
      memset (gid_unused, 1, gid_allocated);
      setgrent ();
      while ((gr = getgrent ()) != NULL)
	{
	  if ((unsigned) gr->gr_gid >= gid_allocated)
	    {
	      unsigned new_allocated = (unsigned) gr->gr_gid + ALLOC_STEP;
	      gid_unused = xrealloc (gid_unused, new_allocated);
	      memset (gid_unused + gid_allocated, 1,
		      new_allocated - gid_allocated);
	      gid_allocated = new_allocated;
	    }
	  gid_unused[(unsigned) gr->gr_gid] = 0;
	}
      endgrent ();
    }
#endif
  return true;
}

static boolean
parse_nouser (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;
  (void) argv;
  (void) arg_ptr;
  

  our_pred = insert_primary (entry);
#ifdef CACHE_IDS
  if (uid_unused == NULL)
    {
      struct passwd *pw;

      uid_allocated = ALLOC_STEP;
      uid_unused = xmalloc (uid_allocated);
      memset (uid_unused, 1, uid_allocated);
      setpwent ();
      while ((pw = getpwent ()) != NULL)
	{
	  if ((unsigned) pw->pw_uid >= uid_allocated)
	    {
	      unsigned new_allocated = (unsigned) pw->pw_uid + ALLOC_STEP;
	      uid_unused = xrealloc (uid_unused, new_allocated);
	      memset (uid_unused + uid_allocated, 1,
		      new_allocated - uid_allocated);
	      uid_allocated = new_allocated;
	    }
	  uid_unused[(unsigned) pw->pw_uid] = 0;
	}
      endpwent ();
    }
#endif
  return true;
}

static boolean
parse_nowarn (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  (void) argv;
  (void) arg_ptr;
  (void) entry;
  
  options.warnings = false;
  return true;;
}

static boolean
parse_ok (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_exec_ok ("-ok", entry, argv, arg_ptr);
}

static boolean
parse_okdir (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_exec_ok ("-okdir", entry, argv, arg_ptr);
}

boolean
parse_openparen (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) argv;
  (void) arg_ptr;
  
  our_pred = get_new_pred_chk_op (entry);
  our_pred->pred_func = pred_openparen;
#ifdef	DEBUG
  our_pred->p_name = find_pred_name (pred_openparen);
#endif	/* DEBUG */
  our_pred->p_type = OPEN_PAREN;
  our_pred->p_prec = NO_PREC;
  our_pred->need_stat = our_pred->need_type = false;
  return true;
}

static boolean
parse_or (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) argv;
  (void) arg_ptr;
  
  our_pred = get_new_pred (entry);
  our_pred->pred_func = pred_or;
#ifdef	DEBUG
  our_pred->p_name = find_pred_name (pred_or);
#endif	/* DEBUG */
  our_pred->p_type = BI_OP;
  our_pred->p_prec = OR_PREC;
  our_pred->need_stat = our_pred->need_type = false;
  return true;
}

/* -path is deprecated (at RMS's request) in favour of 
 * -iwholename.   See the node "GNU Manuals" in standards.texi
 * for the rationale for this (basically, GNU prefers the use 
 * of the phrase "file name" to "path name".
 *
 * We do not issue a warning that this usage is deprecated
 * since HPUX find supports this predicate also.
 */
static boolean
parse_path (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return parse_wholename(entry, argv, arg_ptr);
}

static boolean
parse_wholename (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  our_pred = insert_primary_withpred (entry, pred_path);
  our_pred->need_stat = our_pred->need_type = false;
  our_pred->args.str = argv[*arg_ptr];
  (*arg_ptr)++;
  return true;
}

static boolean
parse_perm (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  mode_t perm_val[2];
  int mode_start = 0;
  boolean havekind = false;
  enum permissions_type kind = PERM_EXACT;
  struct mode_change *change = NULL;
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;

  switch (argv[*arg_ptr][0])
    {
    case '-':
      mode_start = 1;
      kind = PERM_AT_LEAST;
      havekind = true;
      break;
      
     case '+':
       change = mode_compile (argv[*arg_ptr]);
       if (NULL == change)
	 {
	   /* Most likely the caller is an old script that is still
	    * using the obsolete GNU syntax '-perm +MODE'.  This old
	    * syntax was withdrawn in favor of '-perm /MODE' because
	    * it is incompatible with POSIX in some cases, but we
	    * still support uses of it that are not incompatible with
	    * POSIX.
	    */
	   mode_start = 1;
	   kind = PERM_ANY;
	 }
       else
	 {
	   /* This is a POSIX-compatible usage */
	   mode_start = 0;
	   kind = PERM_EXACT;
	 }
       havekind = true;
       break;
      
    case '/':			/* GNU extension */
       mode_start = 1;
       kind = PERM_ANY;
       havekind = true;
       break;
       
    default:
      /* For example, '-perm 0644', which is valid and matches 
       * only files whose mode is exactly 0644.
       *
       * We do nothing here, because mode_start and kind are already
       * correctly set.
       */
      break;
    }

  if (NULL == change)
    {
      change = mode_compile (argv[*arg_ptr] + mode_start);
      if (NULL == change)
	error (1, 0, _("invalid mode `%s'"), argv[*arg_ptr]);
    }
  perm_val[0] = mode_adjust (0, false, 0, change, NULL);
  perm_val[1] = mode_adjust (0, true, 0, change, NULL);
  free (change);
  
  our_pred = insert_primary (entry);

  if (havekind)
    {
      our_pred->args.perm.kind = kind;
    }
  else
    {
  
      switch (argv[*arg_ptr][0])
	{
	case '-':
	  our_pred->args.perm.kind = PERM_AT_LEAST;
	  break;
	case '+':
	  our_pred->args.perm.kind = PERM_ANY;
	  break;
	default:
	  our_pred->args.perm.kind = PERM_EXACT;
	  break;
	}
    }
  
  if (('/' == argv[*arg_ptr][0]) && (0 == perm_val[0]) && (0 == perm_val[1]))
    {
      /* The meaning of -perm /000 will change in the future.
       * It currently matches no files, but like -perm -000 it
       * should match all files.
       */
      error (0, 0,
	     _("warning: you have specified a mode pattern %s which is "
	       "equivalent to 000. The meaning of -perm /000 will soon be "
	       "changed to be consistent with -perm -000; that is, at the "
	       "moment it matches no files but it will soon be changed to "
	       "match all files."),
	     argv[*arg_ptr]);
    }

  memcpy (our_pred->args.perm.val, perm_val, sizeof perm_val);
  (*arg_ptr)++;
  return true;
}

boolean
parse_print (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) argv;
  (void) arg_ptr;
  
  our_pred = insert_primary (entry);
  /* -print has the side effect of printing.  This prevents us
     from doing undesired multiple printing when the user has
     already specified -print. */
  our_pred->side_effects = our_pred->no_default_print = true;
  our_pred->need_stat = our_pred->need_type = false;
  our_pred->args.printf_vec.segment = NULL;
  our_pred->args.printf_vec.stream = stdout;
  our_pred->args.printf_vec.dest_is_tty = stream_is_tty(stdout);
  our_pred->args.printf_vec.quote_opts = clone_quoting_options (NULL);
  
  return true;
}

static boolean
parse_print0 (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) argv;
  (void) arg_ptr;
  
  our_pred = insert_primary (entry);
  /* -print0 has the side effect of printing.  This prevents us
     from doing undesired multiple printing when the user has
     already specified -print0. */
  our_pred->side_effects = our_pred->no_default_print = true;
  our_pred->need_stat = our_pred->need_type = false;
  return true;
}

static boolean
parse_printf (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  return insert_fprintf (stdout, entry, pred_fprintf, argv, arg_ptr);
}

static boolean
parse_prune (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) argv;
  (void) arg_ptr;
  
  our_pred = insert_primary (entry);
  our_pred->need_stat = our_pred->need_type = false;
  /* -prune has a side effect that it does not descend into
     the current directory. */
  our_pred->side_effects = true;
  our_pred->no_default_print = false;
  return true;
}

static boolean 
parse_quit  (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred = insert_primary (entry);
  (void) argv;
  (void) arg_ptr;
  our_pred->need_stat = our_pred->need_type = false;
  our_pred->side_effects = true; /* Exiting is a side effect... */
  our_pred->no_default_print = false; /* Don't inhibit the default print, though. */
  return true;
}


static boolean 
parse_regextype (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;

  /* collect the regex type name */
  options.regex_options = get_regex_type(argv[*arg_ptr]);
  (*arg_ptr)++;

  return true;
}


static boolean
parse_regex (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_regex (argv, arg_ptr, entry, options.regex_options);
}

static boolean
insert_regex (char **argv, int *arg_ptr, const struct parser_table *entry, int regex_options)
{
  struct predicate *our_pred;
  struct re_pattern_buffer *re;
  const char *error_message;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  our_pred = insert_primary_withpred (entry, pred_regex);
  our_pred->need_stat = our_pred->need_type = false;
  re = (struct re_pattern_buffer *)
    xmalloc (sizeof (struct re_pattern_buffer));
  our_pred->args.regex = re;
  re->allocated = 100;
  re->buffer = (unsigned char *) xmalloc (re->allocated);
  re->fastmap = NULL;

  re_set_syntax(regex_options);
  re->syntax = regex_options;
  re->translate = NULL;
  
  error_message = re_compile_pattern (argv[*arg_ptr], strlen (argv[*arg_ptr]),
				      re);
  if (error_message)
    error (1, 0, "%s", error_message);
  (*arg_ptr)++;
  return true;
}

static boolean
parse_size (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;
  uintmax_t num;
  enum comparison_type c_type;
  int blksize = 512;
  int len;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  len = strlen (argv[*arg_ptr]);
  if (len == 0)
    error (1, 0, _("invalid null argument to -size"));
  switch (argv[*arg_ptr][len - 1])
    {
    case 'b':
      blksize = 512;
      argv[*arg_ptr][len - 1] = '\0';
      break;

    case 'c':
      blksize = 1;
      argv[*arg_ptr][len - 1] = '\0';
      break;

    case 'k':
      blksize = 1024;
      argv[*arg_ptr][len - 1] = '\0';
      break;

    case 'M':			/* Megabytes */
      blksize = 1024*1024;
      argv[*arg_ptr][len - 1] = '\0';
      break;

    case 'G':			/* Gigabytes */
      blksize = 1024*1024*1024;
      argv[*arg_ptr][len - 1] = '\0';
      break;

    case 'w':
      blksize = 2;
      argv[*arg_ptr][len - 1] = '\0';
      break;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      break;

    default:
      error (1, 0, _("invalid -size type `%c'"), argv[*arg_ptr][len - 1]);
    }
  if (!get_num (argv[*arg_ptr], &num, &c_type))
    return false;
  our_pred = insert_primary (entry);
  our_pred->args.size.kind = c_type;
  our_pred->args.size.blocksize = blksize;
  our_pred->args.size.size = num;
  (*arg_ptr)++;
  return true;
}


static boolean
parse_samefile (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;
  struct stat st;
  
  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  if ((*options.xstat) (argv[*arg_ptr], &st))
    error (1, errno, "%s", argv[*arg_ptr]);
  
  our_pred = insert_primary (entry);
  our_pred->args.fileid.ino = st.st_ino;
  our_pred->args.fileid.dev = st.st_dev;
  our_pred->need_type = false;
  our_pred->need_stat = true;
  (*arg_ptr)++;
  return true;
}


static boolean
parse_true (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) argv;
  (void) arg_ptr;
  
  our_pred = insert_primary (entry);
  our_pred->need_stat = our_pred->need_type = false;
  return true;
}

static boolean
parse_type (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_type (argv, arg_ptr, entry, pred_type);
}

static boolean
parse_uid (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_num (argv, arg_ptr, entry);
}

static boolean
parse_used (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;
  uintmax_t num_days;
  enum comparison_type c_type;
  time_t t;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  if (!get_num (argv[*arg_ptr], &num_days, &c_type))
    return false;
  t = num_days * DAYSECS;
  our_pred = insert_primary (entry);
  our_pred->args.info.kind = c_type;
  our_pred->args.info.negative = t < 0;
  our_pred->args.info.l_val = t;
  (*arg_ptr)++;
  return true;
}

static boolean
parse_user (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct passwd *cur_pwd;
  struct predicate *our_pred;
  uid_t uid;
  int uid_len;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  cur_pwd = getpwnam (argv[*arg_ptr]);
  endpwent ();
  if (cur_pwd != NULL)
    uid = cur_pwd->pw_uid;
  else
    {
      uid_len = strspn (argv[*arg_ptr], "0123456789");
      if ((uid_len == 0) || (argv[*arg_ptr][uid_len] != '\0'))
	return false;
      uid = atoi (argv[*arg_ptr]);
    }
  our_pred = insert_primary (entry);
  our_pred->args.uid = uid;
  (*arg_ptr)++;
  return true;
}

static boolean
parse_version (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  extern char *version_string;
  int features = 0;
  
  (void) argv;
  (void) arg_ptr;
  (void) entry;
  
  fflush (stderr);
  printf (_("GNU find version %s\n"), version_string);
  printf (_("Built using GNU gnulib version %s\n"), gnulib_version);
  printf (_("Features enabled: "));
  
#if CACHE_IDS
  printf("CACHE_IDS ");
  ++features;
#endif
#if DEBUG
  printf("DEBUG ");
  ++features;
#endif
#if DEBUG_STAT
  printf("DEBUG_STAT ");
  ++features;
#endif
#if defined(USE_STRUCT_DIRENT_D_TYPE) && defined(HAVE_STRUCT_DIRENT_D_TYPE)
  printf("D_TYPE ");
  ++features;
#endif
#if defined(O_NOFOLLOW)
  printf("O_NOFOLLOW(%s) ",
	 (options.open_nofollow_available ? "enabled" : "disabled"));
  ++features;
#endif
#if defined(LEAF_OPTIMISATION)
  printf("LEAF_OPTIMISATION ");
  ++features;
#endif
  if (0 == features)
    {
      /* For the moment, leave this as English in case someone wants
	 to parse these strings. */
      printf("none");
    }
  printf("\n");
  
  exit (0);
}

static boolean
parse_xdev (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  (void) argv;
  (void) arg_ptr;
  (void) entry;
  options.stay_on_filesystem = true;
  return true;
}

static boolean
parse_ignore_race (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  (void) argv;
  (void) arg_ptr;
  (void) entry;
  options.ignore_readdir_race = true;
  return true;
}

static boolean
parse_noignore_race (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  (void) argv;
  (void) arg_ptr;
  (void) entry;
  options.ignore_readdir_race = false;
  return true;
}

static boolean
parse_warn (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  (void) argv;
  (void) arg_ptr;
  (void) entry;
  options.warnings = true;
  return true;
}

static boolean
parse_xtype (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  (void) argv;
  (void) arg_ptr;
  return insert_type (argv, arg_ptr, entry, pred_xtype);
}

static boolean
insert_type (char **argv, int *arg_ptr, const struct parser_table *entry, PRED_FUNC which_pred)
{
  mode_t type_cell;
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL)
      || (strlen (argv[*arg_ptr]) != 1))
    return false;
  switch (argv[*arg_ptr][0])
    {
    case 'b':			/* block special */
      type_cell = S_IFBLK;
      break;
    case 'c':			/* character special */
      type_cell = S_IFCHR;
      break;
    case 'd':			/* directory */
      type_cell = S_IFDIR;
      break;
    case 'f':			/* regular file */
      type_cell = S_IFREG;
      break;
#ifdef S_IFLNK
    case 'l':			/* symbolic link */
      type_cell = S_IFLNK;
      break;
#endif
#ifdef S_IFIFO
    case 'p':			/* pipe */
      type_cell = S_IFIFO;
      break;
#endif
#ifdef S_IFSOCK
    case 's':			/* socket */
      type_cell = S_IFSOCK;
      break;
#endif
#ifdef S_IFDOOR
    case 'D':			/* Solaris door */
      type_cell = S_IFDOOR;
      break;
#endif
    default:			/* None of the above ... nuke 'em. */
      return false;
    }
  our_pred = insert_primary_withpred (entry, which_pred);

  /* Figure out if we will need to stat the file, because if we don't
   * need to follow symlinks, we can avoid a stat call by using 
   * struct dirent.d_type.
   */
  if (which_pred == pred_xtype)
    {
      our_pred->need_stat = true;
      our_pred->need_type = false;
    }
  else
    {
      our_pred->need_stat = false; /* struct dirent is enough */
      our_pred->need_type = true;
    }
  our_pred->args.type = type_cell;
  (*arg_ptr)++;			/* Move on to next argument. */
  return true;
}


/* Return true if the file accessed via FP is a terminal.
 */
static boolean 
stream_is_tty(FILE *fp)
{
  int fd = fileno(fp);
  if (-1 == fd)
    {
      return false; /* not a valid stream */
    }
  else
    {
      return isatty(fd) ? true : false;
    }
  
}



/* If true, we've determined that the current fprintf predicate
   uses stat information. */
static boolean fprintf_stat_needed;

/* XXX: do we need to pass FUNC to this function? */
static boolean
insert_fprintf (FILE *fp, const struct parser_table *entry, PRED_FUNC func, char **argv, int *arg_ptr)
{
  char *format;			/* Beginning of unprocessed format string. */
  register char *scan;		/* Current address in scanning `format'. */
  register char *scan2;		/* Address inside of element being scanned. */
  struct segment **segmentp;	/* Address of current segment. */
  struct predicate *our_pred;

  format = argv[(*arg_ptr)++];

  fprintf_stat_needed = false;	/* Might be overridden later. */
  our_pred = insert_primary_withpred (entry, func);
  our_pred->side_effects = our_pred->no_default_print = true;
  our_pred->args.printf_vec.stream = fp;
  our_pred->args.printf_vec.dest_is_tty = stream_is_tty(fp);
  our_pred->args.printf_vec.quote_opts = clone_quoting_options (NULL);
  segmentp = &our_pred->args.printf_vec.segment;
  *segmentp = NULL;

  for (scan = format; *scan; scan++)
    {
      if (*scan == '\\')
	{
	  scan2 = scan + 1;
	  if (*scan2 >= '0' && *scan2 <= '7')
	    {
	      register int n, i;

	      for (i = n = 0; i < 3 && (*scan2 >= '0' && *scan2 <= '7');
		   i++, scan2++)
		n = 8 * n + *scan2 - '0';
	      scan2--;
	      *scan = n;
	    }
	  else
	    {
	      switch (*scan2)
		{
		case 'a':
		  *scan = 7;
		  break;
		case 'b':
		  *scan = '\b';
		  break;
		case 'c':
		  make_segment (segmentp, format, scan - format, KIND_STOP);
		  our_pred->need_stat = fprintf_stat_needed;
		  return true;
		case 'f':
		  *scan = '\f';
		  break;
		case 'n':
		  *scan = '\n';
		  break;
		case 'r':
		  *scan = '\r';
		  break;
		case 't':
		  *scan = '\t';
		  break;
		case 'v':
		  *scan = '\v';
		  break;
		case '\\':
		  /* *scan = '\\'; * it already is */
		  break;
		default:
		  error (0, 0,
			 _("warning: unrecognized escape `\\%c'"), *scan2);
		  scan++;
		  continue;
		}
	    }
	  segmentp = make_segment (segmentp, format, scan - format + 1,
				   KIND_PLAIN);
	  format = scan2 + 1;	/* Move past the escape. */
	  scan = scan2;		/* Incremented immediately by `for'. */
	}
      else if (*scan == '%')
	{
	  if (scan[1] == 0)
	    {
	      /* Trailing %.  We don't like those. */
	      error (1, 0, _("error: %s at end of format string"), scan);
	    }
	  else if (scan[1] == '%')
	    {
	      segmentp = make_segment (segmentp, format, scan - format + 1,
				       KIND_PLAIN);
	      scan++;
	      format = scan + 1;
	      continue;
	    }
	  /* Scan past flags, width and precision, to verify kind. */
	  for (scan2 = scan; *++scan2 && strchr ("-+ #", *scan2);)
	    /* Do nothing. */ ;
	  while (ISDIGIT (*scan2))
	    scan2++;
	  if (*scan2 == '.')
	    for (scan2++; ISDIGIT (*scan2); scan2++)
	      /* Do nothing. */ ;
	  if (strchr ("abcdDfFgGhHiklmMnpPstuUyY", *scan2))
	    {
	      segmentp = make_segment (segmentp, format, scan2 - format,
				       (int) *scan2);
	      scan = scan2;
	      format = scan + 1;
	    }
	  else if (strchr ("ACT", *scan2) && scan2[1])
	    {
	      segmentp = make_segment (segmentp, format, scan2 - format,
				       *scan2 | (scan2[1] << 8));
	      scan = scan2 + 1;
	      format = scan + 1;
	      continue;
	    }
	  else
	    {
	      /* An unrecognized % escape.  Print the char after the %. */
	      error (0, 0, _("warning: unrecognized format directive `%%%c'"),
		     *scan2);
	      segmentp = make_segment (segmentp, format, scan - format,
				       KIND_PLAIN);
	      format = scan + 1;
	      continue;
	    }
	}
    }

  if (scan > format)
    make_segment (segmentp, format, scan - format, KIND_PLAIN);
  our_pred->need_type = false;
  our_pred->need_stat = fprintf_stat_needed;
  return true;
}

/* Create a new fprintf segment in *SEGMENT, with type KIND,
   from the text in FORMAT, which has length LEN.
   Return the address of the `next' pointer of the new segment. */

static struct segment **
make_segment (struct segment **segment, char *format, int len, int kind)
{
  char *fmt;

  *segment = (struct segment *) xmalloc (sizeof (struct segment));

  (*segment)->kind = kind;
  (*segment)->next = NULL;
  (*segment)->text_len = len;

  fmt = (*segment)->text = xmalloc (len + sizeof "d");
  strncpy (fmt, format, len);
  fmt += len;

  switch (kind & 0xff)
    {
    case KIND_PLAIN:		/* Plain text string, no % conversion. */
    case KIND_STOP:		/* Terminate argument, no newline. */
      break;

    case 'a':			/* atime in `ctime' format */
    case 'A':			/* atime in user-specified strftime format */
    case 'c':			/* ctime in `ctime' format */
    case 'C':			/* ctime in user-specified strftime format */
    case 'F':			/* filesystem type */
    case 'g':			/* group name */
    case 'i':			/* inode number */
    case 'l':			/* object of symlink */
    case 'M':			/* mode in `ls -l' format (eg., "drwxr-xr-x") */
    case 's':			/* size in bytes */
    case 't':			/* mtime in `ctime' format */
    case 'T':			/* mtime in user-specified strftime format */
    case 'u':			/* user name */
    case 'y':			/* file type */
    case 'Y':			/* symlink pointed file type */
      fprintf_stat_needed = true;
      /* FALLTHROUGH */
    case 'f':			/* basename of path */
    case 'h':			/* leading directories part of path */
    case 'H':			/* ARGV element file was found under */
    case 'p':			/* pathname */
    case 'P':			/* pathname with ARGV element stripped */
      *fmt++ = 's';
      break;

      /* Numeric items that one might expect to honour 
       * #, 0, + flags but which do not.
       */
    case 'G':			/* GID number */
    case 'U':			/* UID number */
    case 'b':			/* size in 512-byte blocks */
    case 'D':                   /* Filesystem device on which the file exits */
    case 'k':			/* size in 1K blocks */
    case 'n':			/* number of links */
      fprintf_stat_needed = true;
      *fmt++ = 's';
      break;
      
      /* Numeric items that DO honour #, 0, + flags.
       */
    case 'd':			/* depth in search tree (0 = ARGV element) */
      *fmt++ = 'd';
      break;

    case 'm':			/* mode as octal number (perms only) */
      *fmt++ = 'o';
      fprintf_stat_needed = true;
      break;
    }
  *fmt = '\0';

  return &(*segment)->next;
}

static void 
check_path_safety(const char *action)
{
  const char *path = getenv("PATH");
  char *s;
  s = next_element(path, 1);
  while ((s = next_element ((char *) NULL, 1)) != NULL)
    {
      if (0 == strcmp(s, "."))
	{
	  error(1, 0, _("The current directory is included in the PATH environment variable, which is insecure in combination with the %s action of find.  Please remove the current directory from your $PATH (that is, remove \".\" or leading or trailing colons)"),
		action);
	}
    }
}


/* handles both exec and ok predicate */
#if defined(NEW_EXEC)
/* handles both exec and ok predicate */
static boolean
new_insert_exec_ok (const char *action,
		    const struct parser_table *entry,
		    char **argv,
		    int *arg_ptr)
{
  int start, end;		/* Indexes in ARGV of start & end of cmd. */
  int i;			/* Index into cmd args */
  int saw_braces;		/* True if previous arg was '{}'. */
  boolean allow_plus;		/* True if + is a valid terminator */
  int brace_count;		/* Number of instances of {}. */
  PRED_FUNC func = entry->pred_func;
  enum BC_INIT_STATUS bcstatus;
  
  struct predicate *our_pred;
  struct exec_val *execp;	/* Pointer for efficiency. */

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;

  our_pred = insert_primary_withpred (entry, func);
  our_pred->side_effects = our_pred->no_default_print = true;
  execp = &our_pred->args.exec_vec;

  if ((func != pred_okdir) && (func != pred_ok))
    {
      allow_plus = true;
      execp->close_stdin = false;
    }
  else
    {
      allow_plus = false;
      /* If find reads stdin (i.e. for -ok and similar), close stdin
       * in the child to prevent some script from consiming the output
       * intended for find.
       */
      execp->close_stdin = true;
    }
  
  
  if ((func == pred_execdir) || (func == pred_okdir))
    {
      options.ignore_readdir_race = false;
      check_path_safety(action);
      execp->use_current_dir = true;
    }
  else
    {
      execp->use_current_dir = false;
    }
  
  our_pred->args.exec_vec.multiple = 0;

  /* Count the number of args with path replacements, up until the ';'. 
   * Also figure out if the command is terminated by ";" or by "+".
   */
  start = *arg_ptr;
  for (end = start, saw_braces=0, brace_count=0;
       (argv[end] != NULL)
       && ((argv[end][0] != ';') || (argv[end][1] != '\0'));
       end++)
    {
      /* For -exec and -execdir, "{} +" can terminate the command. */
      if ( allow_plus
	   && argv[end][0] == '+' && argv[end][1] == 0
	   && saw_braces)
	{
	  our_pred->args.exec_vec.multiple = 1;
	  break;
	}
      
      saw_braces = 0;
      if (strstr (argv[end], "{}"))
	{
	  saw_braces = 1;
	  ++brace_count;
	  
	  if (0 == end && (func == pred_execdir || func == pred_okdir))
	    {
	      /* The POSIX standard says that {} replacement should
	       * occur even in the utility name.  This is insecure
	       * since it means we will be executing a command whose
	       * name is chosen according to whatever find finds in
	       * the filesystem.  That can be influenced by an
	       * attacker.  Hence for -execdir and -okdir this is not
	       * allowed.  We can specify this as those options are 
	       * not defined by POSIX.
	       */
	      error(1, 0, _("You may not use {} within the utility name for -execdir and -okdir, because this is a potential security problem."));
	    }
	}
    }
  
  /* Fail if no command given or no semicolon found. */
  if ((end == start) || (argv[end] == NULL))
    {
      *arg_ptr = end;
      free(our_pred);
      return false;
    }

  if (our_pred->args.exec_vec.multiple && brace_count > 1)
    {
	
      const char *suffix;
      if (func == pred_execdir)
	suffix = "dir";
      else
	suffix = "";

      error(1, 0,
	    _("Only one instance of {} is supported with -exec%s ... +"),
	    suffix);
    }

  /* We use a switch statement here so that 
   * the compiler warns us when we forget to handle a 
   * newly invented enum value.
   */
  bcstatus = bc_init_controlinfo(&execp->ctl);
  switch (bcstatus) 
    {
    case BC_INIT_ENV_TOO_BIG:
      error(1, 0, 
	    _("The environment is too large for exec()."));
      break;
    case BC_INIT_OK:
      /* Good news.  Carry on. */
      break;
    }
  bc_use_sensible_arg_max(&execp->ctl);


  execp->ctl.exec_callback = launch;

  if (our_pred->args.exec_vec.multiple)
    {
      /* "+" terminator, so we can just append our arguments after the
       * command and initial arguments.
       */
      execp->replace_vec = NULL;
      execp->ctl.replace_pat = NULL;
      execp->ctl.rplen = 0;
      execp->ctl.lines_per_exec = 0; /* no limit */
      execp->ctl.args_per_exec = 0; /* no limit */
      
      /* remember how many arguments there are */
      execp->ctl.initial_argc = (end-start) - 1;

      /* execp->state = xmalloc(sizeof struct buildcmd_state); */
      bc_init_state(&execp->ctl, &execp->state, execp);
  
      /* Gather the initial arguments.  Skip the {}. */
      for (i=start; i<end-1; ++i)
	{
	  bc_push_arg(&execp->ctl, &execp->state,
		      argv[i], strlen(argv[i])+1,
		      NULL, 0,
		      1);
	}
    }
  else
    {
      /* Semicolon terminator - more than one {} is supported, so we
       * have to do brace-replacement.
       */
      execp->num_args = end - start;
      
      execp->ctl.replace_pat = "{}";
      execp->ctl.rplen = strlen(execp->ctl.replace_pat);
      execp->ctl.lines_per_exec = 0; /* no limit */
      execp->ctl.args_per_exec = 0; /* no limit */
      execp->replace_vec = xmalloc(sizeof(char*)*execp->num_args);


      /* execp->state = xmalloc(sizeof(*(execp->state))); */
      bc_init_state(&execp->ctl, &execp->state, execp);

      /* Remember the (pre-replacement) arguments for later. */
      for (i=0; i<execp->num_args; ++i)
	{
	  execp->replace_vec[i] = argv[i+start];
	}
    }
  
  if (argv[end] == NULL)
    *arg_ptr = end;
  else
    *arg_ptr = end + 1;
  
  return true;
}
#else
/* handles both exec and ok predicate */
static boolean
old_insert_exec_ok (boolean (*func) (/* ??? */), char **argv, int *arg_ptr)
{
  int start, end;		/* Indexes in ARGV of start & end of cmd. */
  int num_paths;		/* Number of args with path replacements. */
  int path_pos;			/* Index in array of path replacements. */
  int vec_pos;			/* Index in array of args. */
  struct predicate *our_pred;
  struct exec_val *execp;	/* Pointer for efficiency. */

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;

  /* Count the number of args with path replacements, up until the ';'. */
  start = *arg_ptr;
  for (end = start, num_paths = 0;
       (argv[end] != NULL)
       && ((argv[end][0] != ';') || (argv[end][1] != '\0'));
       end++)
    if (strstr (argv[end], "{}"))
      num_paths++;
  /* Fail if no command given or no semicolon found. */
  if ((end == start) || (argv[end] == NULL))
    {
      *arg_ptr = end;
      return false;
    }

  our_pred = insert_primary (func);
  our_pred->side_effects = our_pred->no_default_print = true;
  execp = &our_pred->args.exec_vec;
  execp->usercontext = our_pred;
  execp->use_current_dir = false;
  execp->paths =
    (struct path_arg *) xmalloc (sizeof (struct path_arg) * (num_paths + 1));
  execp->vec = (char **) xmalloc (sizeof (char *) * (end - start + 1));
  /* Record the positions of all args, and the args with path replacements. */
  for (end = start, path_pos = vec_pos = 0;
       (argv[end] != NULL)
       && ((argv[end][0] != ';') || (argv[end][1] != '\0'));
       end++)
    {
      register char *p;
      
      execp->paths[path_pos].count = 0;
      for (p = argv[end]; *p; ++p)
	if (p[0] == '{' && p[1] == '}')
	  {
	    execp->paths[path_pos].count++;
	    ++p;
	  }
      if (execp->paths[path_pos].count)
	{
	  execp->paths[path_pos].offset = vec_pos;
	  execp->paths[path_pos].origarg = argv[end];
	  path_pos++;
	}
      execp->vec[vec_pos++] = argv[end];
    }
  execp->paths[path_pos].offset = -1;
  execp->vec[vec_pos] = NULL;

  if (argv[end] == NULL)
    *arg_ptr = end;
  else
    *arg_ptr = end + 1;
  return true;
}
#endif



static boolean
insert_exec_ok (const char *action, const struct parser_table *entry, char **argv, int *arg_ptr)
{
#if defined(NEW_EXEC)
  return new_insert_exec_ok(action, entry, argv, arg_ptr);
#else
  return old_insert_exec_ok(func, argv, arg_ptr);
#endif
}



/* Get a number of days and comparison type.
   STR is the ASCII representation.
   Set *NUM_DAYS to the number of days, taken as being from
   the current moment (or possibly midnight).  Thus the sense of the
   comparison type appears to be reversed.
   Set *COMP_TYPE to the kind of comparison that is requested.

   Return true if all okay, false if input error.

   Used by -atime, -ctime and -mtime (parsers) to
   get the appropriate information for a time predicate processor. */

static boolean
get_num_days (char *str, uintmax_t *num_days, enum comparison_type *comp_type)
{
  boolean r = get_num (str, num_days, comp_type);
  if (r)
    switch (*comp_type)
      {
      case COMP_LT: *comp_type = COMP_GT; break;
      case COMP_GT: *comp_type = COMP_LT; break;
      default: break;
      }
  return r;
}

/* Insert a time predicate PRED.
   ARGV is a pointer to the argument array.
   ARG_PTR is a pointer to an index into the array, incremented if
   all went well.

   Return true if input is valid, false if not.

   A new predicate node is assigned, along with an argument node
   obtained with malloc.

   Used by -atime, -ctime, and -mtime parsers. */

static boolean
insert_time (char **argv, int *arg_ptr, const struct parser_table* entry, PRED_FUNC pred)
{
  struct predicate *our_pred;
  uintmax_t num_days;
  enum comparison_type c_type;
  time_t t;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  if (!get_num_days (argv[*arg_ptr], &num_days, &c_type))
    return false;

  /* Figure out the timestamp value we are looking for. */
  t = ( options.cur_day_start - num_days * DAYSECS
		   + ((c_type == COMP_GT) ? DAYSECS - 1 : 0));

  if (1)
    {
      /* We introduce a scope in which 'val' can be declared, for the 
       * benefit of compilers that are really C89 compilers
       * which support intmax_t because config.h #defines it
       */
      intmax_t val = ( (intmax_t)options.cur_day_start - num_days * DAYSECS
		       + ((c_type == COMP_GT) ? DAYSECS - 1 : 0));
      t = val;
      
      /* Check for possibility of an overflow */
      if ( (intmax_t)t != val ) 
	{
	  error (1, 0, "arithmetic overflow while converting %s days to a number of seconds", argv[*arg_ptr]);
	}
    }
  
  our_pred = insert_primary_withpred (entry, pred);
  our_pred->args.info.kind = c_type;
  our_pred->args.info.negative = t < 0;
  our_pred->args.info.l_val = t;
  (*arg_ptr)++;
#ifdef	DEBUG
  fprintf (stderr, "inserting %s\n", our_pred->p_name);
  fprintf (stderr, "    type: %s    %s  ",
	  (c_type == COMP_GT) ? "gt" :
	  ((c_type == COMP_LT) ? "lt" : ((c_type == COMP_EQ) ? "eq" : "?")),
	  (c_type == COMP_GT) ? " >" :
	  ((c_type == COMP_LT) ? " <" : ((c_type == COMP_EQ) ? ">=" : " ?")));
  t = our_pred->args.info.l_val;
  fprintf (stderr, "%ju %s", (uintmax_t) our_pred->args.info.l_val, ctime (&t));
  if (c_type == COMP_EQ)
    {
      t = our_pred->args.info.l_val += DAYSECS;
      fprintf (stderr, "                 <  %ju %s",
	      (uintmax_t) our_pred->args.info.l_val, ctime (&t));
      our_pred->args.info.l_val -= DAYSECS;
    }
#endif	/* DEBUG */
  return true;
}

/* Get a number with comparison information.
   The sense of the comparison information is 'normal'; that is,
   '+' looks for a count > than the number and '-' less than.
   
   STR is the ASCII representation of the number.
   Set *NUM to the number.
   Set *COMP_TYPE to the kind of comparison that is requested.
 
   Return true if all okay, false if input error.  */

static boolean
get_num (char *str, uintmax_t *num, enum comparison_type *comp_type)
{
  if (str == NULL)
    return false;
  switch (str[0])
    {
    case '+':
      *comp_type = COMP_GT;
      str++;
      break;
    case '-':
      *comp_type = COMP_LT;
      str++;
      break;
    default:
      *comp_type = COMP_EQ;
      break;
    }

  return xstrtoumax (str, NULL, 10, num, "") == LONGINT_OK;
}

/* Insert a number predicate.
   ARGV is a pointer to the argument array.
   *ARG_PTR is an index into ARGV, incremented if all went well.
   *PRED is the predicate processor to insert.

   Return true if input is valid, false if error.
   
   A new predicate node is assigned, along with an argument node
   obtained with malloc.

   Used by -inum and -links parsers. */

static boolean
insert_num (char **argv, int *arg_ptr, const struct parser_table *entry)
{
  struct predicate *our_pred;
  uintmax_t num;
  enum comparison_type c_type;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;
  if (!get_num (argv[*arg_ptr], &num, &c_type))
    return false;
  our_pred = insert_primary (entry);
  our_pred->args.info.kind = c_type;
  our_pred->args.info.l_val = num;
  (*arg_ptr)++;
#ifdef	DEBUG
  fprintf (stderr, "inserting %s\n", our_pred->p_name);
  fprintf (stderr, "    type: %s    %s  ",
	  (c_type == COMP_GT) ? "gt" :
	  ((c_type == COMP_LT) ? "lt" : ((c_type == COMP_EQ) ? "eq" : "?")),
	  (c_type == COMP_GT) ? " >" :
	  ((c_type == COMP_LT) ? " <" : ((c_type == COMP_EQ) ? " =" : " ?")));
  fprintf (stderr, "%ju\n", our_pred->args.info.l_val);
#endif	/* DEBUG */
  return true;
}

static FILE *
open_output_file (char *path)
{
  FILE *f;

  if (!strcmp (path, "/dev/stderr"))
    return stderr;
  else if (!strcmp (path, "/dev/stdout"))
    return stdout;
  f = fopen_safer (path, "w");
  if (f == NULL)
    error (1, errno, "%s", path);
  return f;
}
