/*
 * main.c -- Expression tree constructors and main program for gawk. 
 */

/* 
 * Copyright (C) 1986, 1988, 1989, 1991-2003 the Free Software Foundation, Inc.
 * 
 * This file is part of GAWK, the GNU implementation of the
 * AWK Programming Language.
 * 
 * GAWK is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * GAWK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

#include "awk.h"
#include "getopt.h"
#ifdef TANDEM
#include "ptchlvl.h"	/* blech */
#else
#include "patchlev.h"
#endif

#ifndef O_BINARY
#include <fcntl.h>
#endif

#ifdef HAVE_MCHECK_H
#include <mcheck.h>
#endif

#define DEFAULT_PROFILE		"awkprof.out"	/* where to put profile */
#define DEFAULT_VARFILE		"awkvars.out"	/* where to put vars */

static const char *varfile = DEFAULT_VARFILE;

static void usage P((int exitval, FILE *fp)) ATTRIBUTE_NORETURN;
static void copyleft P((void)) ATTRIBUTE_NORETURN;
static void cmdline_fs P((char *str));
static void init_args P((int argc0, int argc, char *argv0, char **argv));
static void init_vars P((void));
static void add_src P((struct src **data, long *num, long *alloc, enum srctype stype, char *val));
static RETSIGTYPE catchsig P((int sig)) ATTRIBUTE_NORETURN;
static void nostalgia P((void)) ATTRIBUTE_NORETURN;
static void version P((void)) ATTRIBUTE_NORETURN;
static void init_fds P((void));
static void init_groupset P((void));

/* These nodes store all the special variables AWK uses */
NODE *ARGC_node, *ARGIND_node, *ARGV_node, *BINMODE_node, *CONVFMT_node;
NODE *ENVIRON_node, *ERRNO_node, *FIELDWIDTHS_node, *FILENAME_node, *FNR_node;
NODE *FS_node, *IGNORECASE_node, *NF_node, *NR_node, *OFMT_node, *OFS_node;
NODE *ORS_node, *PROCINFO_node, *RLENGTH_node, *RSTART_node, *RS_node;
NODE *RT_node, *SUBSEP_node, *LINT_node, *TEXTDOMAIN_node;

long NF;
long NR;
long FNR;
int BINMODE;
int IGNORECASE;
char *OFS;
char *ORS;
char *OFMT;
char *TEXTDOMAIN;
int MRL;	/* See -mr option for use of this variable */

/*
 * CONVFMT is a convenience pointer for the current number to string format.
 * We must supply an initial value to avoid recursion problems of
 *	set_CONVFMT -> fmt_index -> r_force_string: gets NULL CONVFMT
 * Fun, fun, fun, fun.
 */
char *CONVFMT = "%.6g";


int errcount = 0;		/* error counter, used by yyerror() */

NODE *Nnull_string;		/* The global null string */

/* The name the program was invoked under, for error messages */
const char *myname;

/* A block of AWK code to be run before running the program */
NODE *begin_block = NULL;

/* A block of AWK code to be run after the last input file */
NODE *end_block = NULL;

int exiting = FALSE;		/* Was an "exit" statement executed? */
int exit_val = 0;		/* optional exit value */

#if defined(YYDEBUG) || defined(GAWKDEBUG)
extern int yydebug;
#endif

struct src *srcfiles = NULL;	/* source file name(s) */
long numfiles = -1;		/* how many source files */
static long allocfiles;		/* for how many is *srcfiles allocated */

#define	srcfiles_add(stype, val) \
	add_src(&srcfiles, &numfiles, &allocfiles, stype, val)

static struct src *preassigns = NULL;	/* requested via -v or -F */
static long numassigns = -1;		/* how many of them */
static long allocassigns;		/* for how many is allocated */

#define	preassigns_add(stype, val) \
	add_src(&preassigns, &numassigns, &allocassigns, stype, val)

#undef do_lint
#undef do_lint_old

int do_traditional = FALSE;	/* no gnu extensions, add traditional weirdnesses */
int do_posix = FALSE;		/* turn off gnu and unix extensions */
int do_lint = FALSE;		/* provide warnings about questionable stuff */
int do_lint_old = FALSE;	/* warn about stuff not in V7 awk */
int do_intl = FALSE;		/* dump locale-izable strings to stdout */
int do_non_decimal_data = FALSE;	/* allow octal/hex C style DATA. Use with caution! */
int do_nostalgia = FALSE;	/* provide a blast from the past */
int do_intervals = FALSE;	/* allow {...,...} in regexps */
int do_profiling = FALSE;	/* profile and pretty print the program */
int do_dump_vars = FALSE;	/* dump all global variables at end */
int do_tidy_mem = FALSE;	/* release vars when done */

int in_begin_rule = FALSE;	/* we're in a BEGIN rule */
int in_end_rule = FALSE;	/* we're in a END rule */
int whiny_users = FALSE;	/* do things that whiny users want */
#ifdef MBS_SUPPORT
int gawk_mb_cur_max = 1;	/* MB_CUR_MAX value, see comment in main() */
#endif

int output_is_tty = FALSE;	/* control flushing of output */

extern const char *version_string;	/* current version, for printing */

#if defined (HAVE_GETGROUPS) && defined(NGROUPS_MAX) && NGROUPS_MAX > 0
GETGROUPS_T *groupset;		/* current group set */
int ngroups;			/* size of said set */
#endif

/* The parse tree is stored here.  */
NODE *expression_value;

#if _MSC_VER == 510
void (*lintfunc) P((va_list va_alist, ...)) = warning;
#else
#if defined(HAVE_STDARG_H) && defined(__STDC__) && __STDC__
void (*lintfunc) P((const char *mesg, ...)) = warning;
#else
void (*lintfunc) () = warning;
#endif
#endif

static const struct option optab[] = {
	{ "compat",		no_argument,		& do_traditional,	1 },
	{ "traditional",	no_argument,		& do_traditional,	1 },
	{ "lint",		optional_argument,	NULL,		'l' },
	{ "lint-old",		no_argument,		& do_lint_old,	1 },
	{ "posix",		no_argument,		& do_posix,	1 },
	{ "nostalgia",		no_argument,		& do_nostalgia,	1 },
	{ "gen-po",		no_argument,		& do_intl,	1 },
	{ "non-decimal-data",	no_argument,		& do_non_decimal_data, 1 },
	{ "profile",		optional_argument,	NULL,		'p' },
	{ "copyleft",		no_argument,		NULL,		'C' },
	{ "copyright",		no_argument,		NULL,		'C' },
	{ "field-separator",	required_argument,	NULL,		'F' },
	{ "file",		required_argument,	NULL,		'f' },
	{ "re-interval",	no_argument,		& do_intervals,	1 },
	{ "source",		required_argument,	NULL,		's' },
	{ "dump-variables",	optional_argument,	NULL,		'd' },
	{ "assign",		required_argument,	NULL,		'v' },
	{ "version",		no_argument,		NULL,		'V' },
	{ "usage",		no_argument,		NULL,		'u' },
	{ "help",		no_argument,		NULL,		'u' },
#ifdef GAWKDEBUG
	{ "parsedebug",		no_argument,		NULL,		'D' },
#endif
	{ NULL, 0, NULL, '\0' }
};

#ifdef NO_LINT
#define do_lint 0
#define do_lint_old 0
#endif

/* main --- process args, parse program, run it, clean up */

int
main(int argc, char **argv)
{
	int c;
	char *scan;
	/* the + on the front tells GNU getopt not to rearrange argv */
	const char *optlist = "+F:f:v:W;m:D";
	int stopped_early = FALSE;
	int old_optind;
	extern int optind;
	extern int opterr;
	extern char *optarg;
	int i;

	/* do these checks early */
	if (getenv("TIDYMEM") != NULL)
		do_tidy_mem = TRUE;

	if (getenv("WHINY_USERS") != NULL)
		whiny_users = TRUE;

#ifdef HAVE_MCHECK_H
	if (do_tidy_mem)
		mtrace();
#endif /* HAVE_MCHECK_H */
	
#if defined(LC_CTYPE)
	setlocale(LC_CTYPE, "");
#endif
#if defined(LC_COLLATE)
	setlocale(LC_COLLATE, "");
#endif
#if HAVE_LC_MESSAGES && defined(LC_MESSAGES)
	setlocale(LC_MESSAGES, "");
#endif
#if defined(LC_NUMERIC)
	/*
	 * Force the issue here.  According to POSIX 2001, decimal
	 * point is used for parsing source code and for command-line
	 * assignments and the locale value for processing input,
	 * number to string conversion, and printing output.
	 */
	setlocale(LC_NUMERIC, "C");
#endif
#if defined(LC_TIME)
	setlocale(LC_TIME, "");
#endif

#ifdef MBS_SUPPORT
	/*
	 * In glibc, MB_CUR_MAX is actually a function.  This value is
	 * tested *a lot* in many speed-critical places in gawk. Caching
	 * this value once makes a speed difference.
	 */
	gawk_mb_cur_max = MB_CUR_MAX;
#endif

	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	(void) signal(SIGFPE, catchsig);
	(void) signal(SIGSEGV, catchsig);
#ifdef SIGBUS
	(void) signal(SIGBUS, catchsig);
#endif

	myname = gawk_name(argv[0]);
        argv[0] = (char *) myname;
	os_arg_fixup(&argc, &argv); /* emulate redirection, expand wildcards */

	/* remove sccs gunk */
	if (strncmp(version_string, "@(#)", 4) == 0)
		version_string += 4;

	if (argc < 2)
		usage(1, stderr);

	/* Robustness: check that file descriptors 0, 1, 2 are open */
	init_fds();

	/* init array handling. */
	array_init();

	/* we do error messages ourselves on invalid options */
	opterr = FALSE;

	/* option processing. ready, set, go! */
	for (optopt = 0, old_optind = 1;
	     (c = getopt_long(argc, argv, optlist, optab, NULL)) != EOF;
	     optopt = 0, old_optind = optind) {
		if (do_posix)
			opterr = TRUE;

		switch (c) {
		case 'F':
			preassigns_add(PRE_ASSIGN_FS, optarg);
			break;

		case 'f':
			/*
			 * a la MKS awk, allow multiple -f options.
			 * this makes function libraries real easy.
			 * most of the magic is in the scanner.
			 *
			 * The following is to allow for whitespace at the end
			 * of a #! /bin/gawk line in an executable file
			 */
			scan = optarg;
			if (argv[optind-1] != optarg)
				while (ISSPACE(*scan))
					scan++;
			srcfiles_add(SOURCEFILE,
				(*scan == '\0' ? argv[optind++] : optarg));
			break;

		case 'v':
			preassigns_add(PRE_ASSIGN, optarg);
			break;

		case 'm':
			/*
			 * Research awk extension.
			 *	-mf nnn		set # fields, gawk ignores
			 *	-mr nnn		set record length, ditto
			 */
			if (do_lint)
				lintwarn(_("`-m[fr]' option irrelevant in gawk"));
			if (optarg[0] != 'r' && optarg[0] != 'f')
				warning(_("-m option usage: `-m[fr] nnn'"));
			/*
			 * Set fixed length records for Tandem,
			 * ignored on other platforms (see io.c:get_a_record).
			 */
			if (optarg[0] == 'r') {
				if (ISDIGIT(optarg[1]))
					MRL = atoi(optarg+1);
				else {
					MRL = atoi(argv[optind]);
					optind++;
				}
			} else if (optarg[1] == '\0')
				optind++;
			break;

		case 'W':       /* gawk specific options - now in getopt_long */
			fprintf(stderr, _("%s: option `-W %s' unrecognized, ignored\n"),
				argv[0], optarg);
			break;

		/* These can only come from long form options */
		case 'C':
			copyleft();
			break;

		case 'd':
			do_dump_vars = TRUE;
			if (optarg != NULL && optarg[0] != '\0')
				varfile = optarg;
			break;

		case 'l':
#ifndef NO_LINT
			do_lint = LINT_ALL;
			if (optarg != NULL) {
				if (strcmp(optarg, "fatal") == 0)
					lintfunc = r_fatal;
				else if (strcmp(optarg, "invalid") == 0)
					do_lint = LINT_INVALID;
			}
#endif
			break;

		case 'p':
			do_profiling = TRUE;
			if (optarg != NULL)
				set_prof_file(optarg);
			else
				set_prof_file(DEFAULT_PROFILE);
			break;

		case 's':
			if (optarg[0] == '\0')
				warning(_("empty argument to `--source' ignored"));
			else
				srcfiles_add(CMDLINE, optarg);
			break;

		case 'u':
			usage(0, stdout);	/* per coding stds */
			break;

		case 'V':
			version();
			break;

		case 0:
			/*
			 * getopt_long found an option that sets a variable
			 * instead of returning a letter. Do nothing, just
			 * cycle around for the next one.
			 */
			break;

		case 'D':
#ifdef GAWKDEBUG
			yydebug = 2;
			break;
#endif
			/* if not debugging, fall through */

		case '?':
		default:
			/*
			 * New behavior.  If not posix, an unrecognized
			 * option stops argument processing so that it can
			 * go into ARGV for the awk program to see. This
			 * makes use of ``#! /bin/gawk -f'' easier.
			 *
			 * However, it's never simple. If optopt is set,
			 * an option that requires an argument didn't get the
			 * argument. We care because if opterr is 0, then
			 * getopt_long won't print the error message for us.
			 */
			if (! do_posix
			    && (optopt == '\0' || strchr(optlist, optopt) == NULL)) {
				/*
				 * can't just do optind--. In case of an
				 * option with >= 2 letters, getopt_long
				 * won't have incremented optind.
				 */
				optind = old_optind;
				stopped_early = TRUE;
				goto out;
			} else if (optopt != '\0')
				/* Use 1003.2 required message format */
				fprintf(stderr,
					_("%s: option requires an argument -- %c\n"),
					myname, optopt);
			/* else
				let getopt print error message for us */
			break;
		}
	}
out:

	if (do_nostalgia)
		nostalgia();

	/* check for POSIXLY_CORRECT environment variable */
	if (! do_posix && getenv("POSIXLY_CORRECT") != NULL) {
		do_posix = TRUE;
		if (do_lint)
			lintwarn(
	_("environment variable `POSIXLY_CORRECT' set: turning on `--posix'"));
	}

	if (do_posix) {
		if (do_traditional)	/* both on command line */
			warning(_("`--posix' overrides `--traditional'"));
		else
			do_traditional = TRUE;
			/*
			 * POSIX compliance also implies
			 * no GNU extensions either.
			 */
	}

	if (do_traditional && do_non_decimal_data) {
		do_non_decimal_data = FALSE;
		warning(_("`--posix'/`--traditional' overrides `--non-decimal-data'"));
	}

	if (do_lint && os_is_setuid())
		warning(_("running %s setuid root may be a security problem"), myname);

	/*
	 * Force profiling if this is pgawk.
	 * Don't bother if the command line already set profiling up.
	 */
	if (! do_profiling)
		init_profiling(& do_profiling, DEFAULT_PROFILE);

	/* load group set */
	init_groupset();

	/* initialize the null string */
	Nnull_string = make_string("", 0);
	Nnull_string->numbr = 0.0;
	Nnull_string->type = Node_val;
	Nnull_string->flags = (PERM|STRCUR|STRING|NUMCUR|NUMBER);

	/*
	 * Tell the regex routines how they should work.
	 * Do this before initializing variables, since
	 * they could want to do a regexp compile.
	 */
	resetup();

	/* Set up the special variables */
	init_vars();

	/* Set up the field variables */
	init_fields();

	/* Now process the pre-assignments */
	for (i = 0; i <= numassigns; i++)
		if (preassigns[i].stype == PRE_ASSIGN)
			(void) arg_assign(preassigns[i].val, TRUE);
		else	/* PRE_ASSIGN_FS */
			cmdline_fs(preassigns[i].val);
	free(preassigns);

	if ((BINMODE & 1) != 0)
		if (os_setbinmode(fileno(stdin), O_BINARY) == -1)
			fatal(_("can't set binary mode on stdin (%s)"), strerror(errno));
	if ((BINMODE & 2) != 0) {
		if (os_setbinmode(fileno(stdout), O_BINARY) == -1)
			fatal(_("can't set binary mode on stdout (%s)"), strerror(errno));
		if (os_setbinmode(fileno(stderr), O_BINARY) == -1)
			fatal(_("can't set binary mode on stderr (%s)"), strerror(errno));
	}

#ifdef GAWKDEBUG
	setbuf(stdout, (char *) NULL);	/* make debugging easier */
#endif
	if (isatty(fileno(stdout)))
		output_is_tty = TRUE;
	/* No -f or --source options, use next arg */
	if (numfiles == -1) {
		if (optind > argc - 1 || stopped_early) /* no args left or no program */
			usage(1, stderr);
		srcfiles_add(CMDLINE, argv[optind]);
		optind++;
	}

	init_args(optind, argc, (char *) myname, argv);
	(void) tokexpand();

	/* Read in the program */
	if (yyparse() != 0 || errcount != 0)
		exit(1);

	free(srcfiles);

	if (do_intl)
		exit(0);

	if (do_lint && begin_block == NULL && expression_value == NULL
	     && end_block == NULL)
		lintwarn(_("no program text at all!"));

	if (do_lint)
		shadow_funcs();

	init_profiling_signals();

#if defined(LC_NUMERIC)
	/* See comment above. */
	setlocale(LC_NUMERIC, "");
#endif

	/* Whew. Finally, run the program. */
	if (begin_block != NULL) {
		in_begin_rule = TRUE;
		(void) interpret(begin_block);
	}
	in_begin_rule = FALSE;
	if (! exiting && (expression_value != NULL || end_block != NULL))
		do_input();
	if (end_block != NULL) {
		in_end_rule = TRUE;
		(void) interpret(end_block);
	}
	in_end_rule = FALSE;
	if (close_io() != 0 && exit_val == 0)
		exit_val = 1;

	if (do_profiling) {
		dump_prog(begin_block, expression_value, end_block);
		dump_funcs();
	}

	if (do_dump_vars)
		dump_vars(varfile);

	if (do_tidy_mem)
		release_all_vars();

	exit(exit_val);		/* more portable */
	return exit_val;	/* to suppress warnings */
}

/* add_src --- add one element to *srcfiles or *preassigns */

static void
add_src(struct src **data, long *num, long *alloc, enum srctype stype, char *val)
{
#define INIT_SRC 4

	++*num;

	if (*data == NULL) {
		emalloc(*data, struct src *, INIT_SRC * sizeof(struct src), "add_src");
		*alloc = INIT_SRC;
	} else if (*num >= *alloc) {
		(*alloc) *= 2;
		erealloc(*data, struct src *, (*alloc) * sizeof(struct src), "add_src");
	}

	(*data)[*num].stype = stype;
	(*data)[*num].val = val;

#undef INIT_SRC
}

/* usage --- print usage information and exit */

static void
usage(int exitval, FILE *fp)
{

	/* Not factoring out common stuff makes it easier to translate. */
	fprintf(fp, _("Usage: %s [POSIX or GNU style options] -f progfile [--] file ...\n"),
		myname);
	fprintf(fp, _("Usage: %s [POSIX or GNU style options] [--] %cprogram%c file ...\n"),
		myname, quote, quote);

	/* GNU long options info. This is too many options. */

	fputs(_("POSIX options:\t\tGNU long options:\n"), fp);
	fputs(_("\t-f progfile\t\t--file=progfile\n"), fp);
	fputs(_("\t-F fs\t\t\t--field-separator=fs\n"), fp);
	fputs(_("\t-v var=val\t\t--assign=var=val\n"), fp);
	fputs(_("\t-m[fr] val\n"), fp);
	fputs(_("\t-W compat\t\t--compat\n"), fp);
	fputs(_("\t-W copyleft\t\t--copyleft\n"), fp);
	fputs(_("\t-W copyright\t\t--copyright\n"), fp);
	fputs(_("\t-W dump-variables[=file]\t--dump-variables[=file]\n"), fp);
	fputs(_("\t-W gen-po\t\t--gen-po\n"), fp);
	fputs(_("\t-W help\t\t\t--help\n"), fp);
	fputs(_("\t-W lint[=fatal]\t\t--lint[=fatal]\n"), fp);
	fputs(_("\t-W lint-old\t\t--lint-old\n"), fp);
	fputs(_("\t-W non-decimal-data\t--non-decimal-data\n"), fp);
#ifdef NOSTALGIA
	fputs(_("\t-W nostalgia\t\t--nostalgia\n"), fp);
#endif
#ifdef GAWKDEBUG
	fputs(_("\t-W parsedebug\t\t--parsedebug\n"), fp);
#endif
	fputs(_("\t-W profile[=file]\t--profile[=file]\n"), fp);
	fputs(_("\t-W posix\t\t--posix\n"), fp);
	fputs(_("\t-W re-interval\t\t--re-interval\n"), fp);
	fputs(_("\t-W source=program-text\t--source=program-text\n"), fp);
	fputs(_("\t-W traditional\t\t--traditional\n"), fp);
	fputs(_("\t-W usage\t\t--usage\n"), fp);
	fputs(_("\t-W version\t\t--version\n"), fp);


	/* This is one string to make things easier on translators. */
	fputs(_("\nTo report bugs, see node `Bugs' in `gawk.info', which is\n\
section `Reporting Problems and Bugs' in the printed version.\n\n"), fp);

	/* ditto */
	fputs(_("gawk is a pattern scanning and processing language.\n\
By default it reads standard input and writes standard output.\n\n"), fp);

	/* ditto */
	fputs(_("Examples:\n\tgawk '{ sum += $1 }; END { print sum }' file\n\
\tgawk -F: '{ print $1 }' /etc/passwd\n"), fp);

	fflush(fp);

	if (ferror(fp))
		exit(1);

	exit(exitval);
}

/* copyleft --- print out the short GNU copyright information */

static void
copyleft()
{
	static const char blurb_part1[] =
	  N_("Copyright (C) 1989, 1991-%d Free Software Foundation.\n\
\n\
This program is free software; you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License as published by\n\
the Free Software Foundation; either version 2 of the License, or\n\
(at your option) any later version.\n\
\n");
	static const char blurb_part2[] =
	  N_("This program is distributed in the hope that it will be useful,\n\
but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
GNU General Public License for more details.\n\
\n");
	static const char blurb_part3[] =
	  N_("You should have received a copy of the GNU General Public License\n\
along with this program; if not, write to the Free Software\n\
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.\n");
 
	/* multiple blurbs are needed for some brain dead compilers. */
	printf(_(blurb_part1), 2003);	/* Last update year */
	fputs(_(blurb_part2), stdout);
	fputs(_(blurb_part3), stdout);
	fflush(stdout);

	if (ferror(stdout))
		exit(1);

	exit(0);
}

/* cmdline_fs --- set FS from the command line */

static void
cmdline_fs(char *str)
{
	register NODE **tmp;

	tmp = get_lhs(FS_node, (Func_ptr *) 0, FALSE);
	unref(*tmp);
	/*
	 * Only if in full compatibility mode check for the stupid special
	 * case so -F\t works as documented in awk book even though the shell
	 * hands us -Ft.  Bleah!
	 *
	 * Thankfully, Posix didn't propagate this "feature".
	 */
	if (str[0] == 't' && str[1] == '\0') {
		if (do_lint)
			lintwarn(_("-Ft does not set FS to tab in POSIX awk"));
		if (do_traditional && ! do_posix)
			str[0] = '\t';
	}
	*tmp = make_str_node(str, strlen(str), SCAN); /* do process escapes */
	set_FS();
}

/* init_args --- set up ARGV from stuff on the command line */

static void
init_args(int argc0, int argc, char *argv0, char **argv)
{
	int i, j;
	NODE **aptr;

	ARGV_node = install("ARGV", node((NODE *) NULL, Node_var_array, (NODE *) NULL));
	aptr = assoc_lookup(ARGV_node, tmp_number(0.0), FALSE);
	*aptr = make_string(argv0, strlen(argv0));
	(*aptr)->flags |= MAYBE_NUM;
	for (i = argc0, j = 1; i < argc; i++) {
		aptr = assoc_lookup(ARGV_node, tmp_number((AWKNUM) j), FALSE);
		*aptr = make_string(argv[i], strlen(argv[i]));
		(*aptr)->flags |= MAYBE_NUM;
		j++;
	}
	ARGC_node = install("ARGC",
			node(make_number((AWKNUM) j), Node_var, (NODE *) NULL));
}

/*
 * Set all the special variables to their initial values.
 * Note that some of the variables that have set_FOO routines should
 * *N*O*T* have those routines called upon initialization, and thus
 * they have NULL entries in that field. This is notably true of FS
 * and IGNORECASE.
 */
struct varinit {
	NODE **spec;
	const char *name;
	NODETYPE type;
	const char *strval;
	AWKNUM numval;
	Func_ptr assign;
};
static const struct varinit varinit[] = {
{&CONVFMT_node,	"CONVFMT",	Node_CONVFMT,		"%.6g",	0,  set_CONVFMT },
{&NF_node,	"NF",		Node_NF,		NULL,	-1, NULL },
{&FIELDWIDTHS_node, "FIELDWIDTHS", Node_FIELDWIDTHS,	"",	0,  NULL },
{&NR_node,	"NR",		Node_NR,		NULL,	0,  set_NR },
{&FNR_node,	"FNR",		Node_FNR,		NULL,	0,  set_FNR },
{&FS_node,	"FS",		Node_FS,		" ",	0,  NULL },
{&RS_node,	"RS",		Node_RS,		"\n",	0,  set_RS },
{&IGNORECASE_node, "IGNORECASE", Node_IGNORECASE,	NULL,	0,  NULL },
{&FILENAME_node, "FILENAME",	Node_var,		"",	0,  NULL },
{&OFS_node,	"OFS",		Node_OFS,		" ",	0,  set_OFS },
{&ORS_node,	"ORS",		Node_ORS,		"\n",	0,  set_ORS },
{&OFMT_node,	"OFMT",		Node_OFMT,		"%.6g",	0,  set_OFMT },
{&RLENGTH_node, "RLENGTH",	Node_var,		NULL,	0,  NULL },
{&RSTART_node,	"RSTART",	Node_var,		NULL,	0,  NULL },
{&SUBSEP_node,	"SUBSEP",	Node_var,		"\034",	0,  NULL },
{&ARGIND_node,	"ARGIND",	Node_var,		NULL,	0,  NULL },
{&ERRNO_node,	"ERRNO",	Node_var,		NULL,	0,  NULL },
{&RT_node,	"RT",		Node_var,		"",	0,  NULL },
{&BINMODE_node,	"BINMODE",	Node_BINMODE,		NULL,	0,  NULL },
{&LINT_node,	"LINT",		Node_LINT,		NULL,	0,  NULL },
{&TEXTDOMAIN_node,	"TEXTDOMAIN",		Node_TEXTDOMAIN,	"messages",	0,  set_TEXTDOMAIN },
{0,		NULL,		Node_illegal,		NULL,	0,  NULL },
};

/* init_vars --- actually initialize everything in the symbol table */

static void
init_vars()
{
	register const struct varinit *vp;

	for (vp = varinit; vp->name; vp++) {
		*(vp->spec) = install((char *) vp->name,
		  node(vp->strval == NULL ? make_number(vp->numval)
				: make_string((char *) vp->strval,
					strlen(vp->strval)),
		       vp->type, (NODE *) NULL));
		if (vp->assign)
			(*(vp->assign))();
	}
}

/* load_environ --- populate the ENVIRON array */

NODE *
load_environ()
{
#if ! defined(TANDEM)
#if ! (defined(MSDOS) && !defined(DJGPP)) && ! defined(OS2) && ! (defined(VMS) && defined(__DECC))
	extern char **environ;
#endif
	register char *var, *val;
	NODE **aptr;
	register int i;
#endif /* TANDEM */

	ENVIRON_node = install("ENVIRON", 
			node((NODE *) NULL, Node_var_array, (NODE *) NULL));
#if ! defined(TANDEM)
	for (i = 0; environ[i] != NULL; i++) {
		static char nullstr[] = "";

		var = environ[i];
		val = strchr(var, '=');
		if (val != NULL)
			*val++ = '\0';
		else
			val = nullstr;
		aptr = assoc_lookup(ENVIRON_node, tmp_string(var, strlen(var)),
				    FALSE);
		*aptr = make_string(val, strlen(val));
		(*aptr)->flags |= MAYBE_NUM;

		/* restore '=' so that system() gets a valid environment */
		if (val != nullstr)
			*--val = '=';
	}
	/*
	 * Put AWKPATH into ENVIRON if it's not there.
	 * This allows querying it from within awk programs.
	 */
	if (getenv("AWKPATH") == NULL) {
		aptr = assoc_lookup(ENVIRON_node, tmp_string("AWKPATH", 7), FALSE);
		*aptr = make_string(defpath, strlen(defpath));
	}
#endif /* TANDEM */
	return ENVIRON_node;
}

/* load_procinfo --- populate the PROCINFO array */

NODE *
load_procinfo()
{
	int i;
	NODE **aptr;
	char name[100];
	AWKNUM value;

	PROCINFO_node = install("PROCINFO",
			node((NODE *) NULL, Node_var_array, (NODE *) NULL));

#ifdef GETPGRP_VOID
#define getpgrp_arg() /* nothing */
#else
#define getpgrp_arg() getpid()
#endif

	value = getpgrp(getpgrp_arg());
	aptr = assoc_lookup(PROCINFO_node, tmp_string("pgrpid", 6), FALSE);
	*aptr = make_number(value);

	/*
	 * could put a lot of this into a table, but then there's
	 * portability problems declaring all the functions. so just
	 * do it the slow and stupid way. sigh.
	 */

	value = getpid();
	aptr = assoc_lookup(PROCINFO_node, tmp_string("pid", 3), FALSE);
	*aptr = make_number(value);

	value = getppid();
	aptr = assoc_lookup(PROCINFO_node, tmp_string("ppid", 4), FALSE);
	*aptr = make_number(value);

	value = getuid();
	aptr = assoc_lookup(PROCINFO_node, tmp_string("uid", 3), FALSE);
	*aptr = make_number(value);

	value = geteuid();
	aptr = assoc_lookup(PROCINFO_node, tmp_string("euid", 4), FALSE);
	*aptr = make_number(value);

	value = getgid();
	aptr = assoc_lookup(PROCINFO_node, tmp_string("gid", 3), FALSE);
	*aptr = make_number(value);

	value = getegid();
	aptr = assoc_lookup(PROCINFO_node, tmp_string("egid", 4), FALSE);
	*aptr = make_number(value);

	aptr = assoc_lookup(PROCINFO_node, tmp_string("FS", 2), FALSE);
	*aptr = (using_fieldwidths() ? make_string("FIELDWIDTHS", 11) :
				make_string("FS", 2) );

#if defined (HAVE_GETGROUPS) && defined(NGROUPS_MAX) && NGROUPS_MAX > 0
	for (i = 0; i < ngroups; i++) {
		sprintf(name, "group%d", i + 1);
		value = groupset[i];
		aptr = assoc_lookup(PROCINFO_node, tmp_string(name, strlen(name)), FALSE);
		*aptr = make_number(value);
	}
	if (groupset) {
		free(groupset);
		groupset = NULL;
	}
#endif
	return PROCINFO_node;
}

/* arg_assign --- process a command-line assignment */

int
arg_assign(char *arg, int initing)
{
	char *cp, *cp2;
	int badvar;
	Func_ptr after_assign = NULL;
	NODE *var;
	NODE *it;
	NODE **lhs;

	cp = strchr(arg, '=');

	if (cp == NULL) {
		if (! initing)
			return FALSE;	/* This is file name, not assignment. */

		fprintf(stderr,
			_("%s: `%s' argument to `-v' not in `var=value' form\n\n"),
			myname, arg);
		usage(1, stderr);
	}

	*cp++ = '\0';

	/* first check that the variable name has valid syntax */
	badvar = FALSE;
	if (! ISALPHA(arg[0]) && arg[0] != '_')
		badvar = TRUE;
	else
		for (cp2 = arg+1; *cp2; cp2++)
			if (! ISALNUM(*cp2) && *cp2 != '_') {
				badvar = TRUE;
				break;
			}

	if (badvar) {
		if (initing)
			fatal(_("`%s' is not a legal variable name"), arg);

		if (do_lint)
			lintwarn(_("`%s' is not a variable name, looking for file `%s=%s'"),
				arg, arg, cp);
	} else {
		/*
		 * Recent versions of nawk expand escapes inside assignments.
		 * This makes sense, so we do it too.
		 */
		it = make_str_node(cp, strlen(cp), SCAN);
		it->flags |= MAYBE_NUM;
#ifdef LC_NUMERIC
		setlocale(LC_NUMERIC, "C");
		(void) force_number(it);
		setlocale(LC_NUMERIC, "");
#endif /* LC_NUMERIC */
		var = variable(arg, FALSE, Node_var);
		lhs = get_lhs(var, &after_assign, FALSE);
		unref(*lhs);
		*lhs = it;
		if (after_assign != NULL)
			(*after_assign)();
	}

	*--cp = '=';	/* restore original text of ARGV */

	return ! badvar;
}

/* catchsig --- catch signals */

static RETSIGTYPE
catchsig(int sig)
{
	if (sig == SIGFPE) {
		fatal(_("floating point exception"));
	} else if (sig == SIGSEGV
#ifdef SIGBUS
	        || sig == SIGBUS
#endif
	) {
		set_loc(__FILE__, __LINE__);
		msg(_("fatal error: internal error"));
		/* fatal won't abort() if not compiled for debugging */
		abort();
	} else
		cant_happen();
	/* NOTREACHED */
}

/* nostalgia --- print the famous error message and die */

static void
nostalgia()
{
	/*
	 * N.B.: This string is not gettextized, on purpose.
	 * So there.
	 */
	fprintf(stderr, "awk: bailing out near line 1\n");
	fflush(stderr);
	abort();
}

/* version --- print version message */

static void
version()
{
	printf("%s.%s\n", version_string, PATCHLEVEL);
	/*
	 * Per GNU coding standards, print copyright info,
	 * then exit successfully, do nothing else.
	 */
	copyleft();
	exit(0);
}

/* init_fds --- check for 0, 1, 2, open on /dev/null if possible */

static void
init_fds()
{
	struct stat sbuf;
	int fd;
	int newfd;

	/* maybe no stderr, don't bother with error mesg */
	for (fd = 0; fd <= 2; fd++) {
		if (fstat(fd, &sbuf) < 0) {
#if MAKE_A_HEROIC_EFFORT
			if (do_lint)
				lintwarn(_("no pre-opened fd %d"), fd);
#endif
			newfd = devopen("/dev/null", "r+");
#ifdef MAKE_A_HEROIC_EFFORT
			if (do_lint && newfd < 0)
				lintwarn(_("could not pre-open /dev/null for fd %d"), fd);
#endif
		}
	}
}

/* init_groupset --- initialize groupset */

static void
init_groupset()
{
#if defined(HAVE_GETGROUPS) && defined(NGROUPS_MAX) && NGROUPS_MAX > 0
#ifdef GETGROUPS_NOT_STANDARD
	/* For systems that aren't standards conformant, use old way. */
	ngroups = NGROUPS_MAX;
#else
	/*
	 * If called with 0 for both args, return value is
	 * total number of groups.
	 */
	ngroups = getgroups(0, NULL);
#endif
	if (ngroups == -1)
		fatal(_("could not find groups: %s"), strerror(errno));
	else if (ngroups == 0)
		return;

	/* fill in groups */
	emalloc(groupset, GETGROUPS_T *, ngroups * sizeof(GETGROUPS_T), "init_groupset");

	ngroups = getgroups(ngroups, groupset);
	if (ngroups == -1)
		fatal(_("could not find groups: %s"), strerror(errno));
#endif
}
