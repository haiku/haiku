/* Reading/parsing the initialization file.
   Copyright (C) 2005 Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GNU Wget is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wget; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

In addition, as a special exception, the Free Software Foundation
gives permission to link the code of its release of Wget with the
OpenSSL project's "OpenSSL" library (or with modified versions of it
that use the same license as the "OpenSSL" library), and distribute
the linked executables.  You must obey the GNU General Public License
in all respects for all of the code used other than "OpenSSL".  If you
modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do
so, delete this exception statement from your version.  */

#include <config.h>

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif
#include <errno.h>

#ifdef HAVE_PWD_H
# include <pwd.h>
#endif
#include <assert.h>

#include "wget.h"
#include "utils.h"
#include "init.h"
#include "host.h"
#include "netrc.h"
#include "progress.h"
#include "recur.h"		/* for INFINITE_RECURSION */
#include "convert.h"		/* for convert_cleanup */
#include "res.h"		/* for res_cleanup */

#ifndef errno
extern int errno;
#endif

/* We want tilde expansion enabled only when reading `.wgetrc' lines;
   otherwise, it will be performed by the shell.  This variable will
   be set by the wgetrc-reading function.  */

static int enable_tilde_expansion;


#define CMD_DECLARE(func) static int func \
  PARAMS ((const char *, const char *, void *))

CMD_DECLARE (cmd_boolean);
CMD_DECLARE (cmd_bytes);
CMD_DECLARE (cmd_bytes_sum);
#ifdef HAVE_SSL
CMD_DECLARE (cmd_cert_type);
#endif
CMD_DECLARE (cmd_directory_vector);
CMD_DECLARE (cmd_lockable_boolean);
CMD_DECLARE (cmd_number);
CMD_DECLARE (cmd_number_inf);
CMD_DECLARE (cmd_string);
CMD_DECLARE (cmd_file);
CMD_DECLARE (cmd_directory);
CMD_DECLARE (cmd_time);
CMD_DECLARE (cmd_vector);

CMD_DECLARE (cmd_spec_dirstruct);
CMD_DECLARE (cmd_spec_header);
CMD_DECLARE (cmd_spec_htmlify);
CMD_DECLARE (cmd_spec_mirror);
CMD_DECLARE (cmd_spec_prefer_family);
CMD_DECLARE (cmd_spec_progress);
CMD_DECLARE (cmd_spec_recursive);
CMD_DECLARE (cmd_spec_restrict_file_names);
#ifdef HAVE_SSL
CMD_DECLARE (cmd_spec_secure_protocol);
#endif
CMD_DECLARE (cmd_spec_timeout);
CMD_DECLARE (cmd_spec_useragent);

/* List of recognized commands, each consisting of name, place and
   function.  When adding a new command, simply add it to the list,
   but be sure to keep the list sorted alphabetically, as
   command_by_name depends on it.  Also, be sure to add any entries
   that allocate memory (e.g. cmd_string and cmd_vector) to the
   cleanup() function below. */

static struct {
  const char *name;
  void *place;
  int (*action) PARAMS ((const char *, const char *, void *));
} commands[] = {
  { "accept",		&opt.accepts,		cmd_vector },
  { "addhostdir",	&opt.add_hostdir,	cmd_boolean },
  { "alwaysrest",	&opt.always_rest,	cmd_boolean }, /* deprecated */
  { "background",	&opt.background,	cmd_boolean },
  { "backupconverted",	&opt.backup_converted,	cmd_boolean },
  { "backups",		&opt.backups,		cmd_number },
  { "base",		&opt.base_href,		cmd_string },
  { "bindaddress",	&opt.bind_address,	cmd_string },
#ifdef HAVE_SSL
  { "cacertificate",	&opt.ca_cert,		cmd_file },
#endif
  { "cache",		&opt.allow_cache,	cmd_boolean },
#ifdef HAVE_SSL
  { "cadirectory",	&opt.ca_directory,	cmd_directory },
  { "certificate",	&opt.cert_file,		cmd_file },
  { "certificatetype",	&opt.cert_type,		cmd_cert_type },
  { "checkcertificate", &opt.check_cert,	cmd_boolean },
#endif
  { "connecttimeout",	&opt.connect_timeout,	cmd_time },
  { "continue",		&opt.always_rest,	cmd_boolean },
  { "convertlinks",	&opt.convert_links,	cmd_boolean },
  { "cookies",		&opt.cookies,		cmd_boolean },
  { "cutdirs",		&opt.cut_dirs,		cmd_number },
#ifdef ENABLE_DEBUG
  { "debug",		&opt.debug,		cmd_boolean },
#endif
  { "deleteafter",	&opt.delete_after,	cmd_boolean },
  { "dirprefix",	&opt.dir_prefix,	cmd_directory },
  { "dirstruct",	NULL,			cmd_spec_dirstruct },
  { "dnscache",		&opt.dns_cache,		cmd_boolean },
  { "dnstimeout",	&opt.dns_timeout,	cmd_time },
  { "domains",		&opt.domains,		cmd_vector },
  { "dotbytes",		&opt.dot_bytes,		cmd_bytes },
  { "dotsinline",	&opt.dots_in_line,	cmd_number },
  { "dotspacing",	&opt.dot_spacing,	cmd_number },
  { "dotstyle",		&opt.dot_style,		cmd_string },
#ifdef HAVE_SSL
  { "egdfile",		&opt.egd_file,		cmd_file },
#endif
  { "excludedirectories", &opt.excludes,	cmd_directory_vector },
  { "excludedomains",	&opt.exclude_domains,	cmd_vector },
  { "followftp",	&opt.follow_ftp,	cmd_boolean },
  { "followtags",	&opt.follow_tags,	cmd_vector },
  { "forcehtml",	&opt.force_html,	cmd_boolean },
  { "ftppasswd",	&opt.ftp_passwd,	cmd_string }, /* deprecated */
  { "ftppassword",	&opt.ftp_passwd,	cmd_string },
  { "ftpproxy",		&opt.ftp_proxy,		cmd_string },
  { "ftpuser",	        &opt.ftp_user,		cmd_string },
  { "glob",		&opt.ftp_glob,		cmd_boolean },
  { "header",		NULL,			cmd_spec_header },
  { "htmlextension",	&opt.html_extension,	cmd_boolean },
  { "htmlify",		NULL,			cmd_spec_htmlify },
  { "httpkeepalive",	&opt.http_keep_alive,	cmd_boolean },
  { "httppasswd",	&opt.http_passwd,	cmd_string }, /* deprecated */
  { "httppassword",	&opt.http_passwd,	cmd_string },
  { "httpproxy",	&opt.http_proxy,	cmd_string },
  { "httpsproxy",	&opt.https_proxy,	cmd_string },
  { "httpuser",		&opt.http_user,		cmd_string },
  { "ignorelength",	&opt.ignore_length,	cmd_boolean },
  { "ignoretags",	&opt.ignore_tags,	cmd_vector },
  { "includedirectories", &opt.includes,	cmd_directory_vector },
#ifdef ENABLE_IPV6
  { "inet4only",	&opt.ipv4_only,		cmd_boolean },
  { "inet6only",	&opt.ipv6_only,		cmd_boolean },
#endif
  { "input",		&opt.input_filename,	cmd_file },
  { "keepsessioncookies", &opt.keep_session_cookies, cmd_boolean },
  { "limitrate",	&opt.limit_rate,	cmd_bytes },
  { "loadcookies",	&opt.cookies_input,	cmd_file },
  { "logfile",		&opt.lfilename,		cmd_file },
  { "login",		&opt.ftp_user,		cmd_string },/* deprecated*/
  { "mirror",		NULL,			cmd_spec_mirror },
  { "netrc",		&opt.netrc,		cmd_boolean },
  { "noclobber",	&opt.noclobber,		cmd_boolean },
  { "noparent",		&opt.no_parent,		cmd_boolean },
  { "noproxy",		&opt.no_proxy,		cmd_vector },
  { "numtries",		&opt.ntry,		cmd_number_inf },/* deprecated*/
  { "outputdocument",	&opt.output_document,	cmd_file },
  { "pagerequisites",	&opt.page_requisites,	cmd_boolean },
  { "passiveftp",	&opt.ftp_pasv,		cmd_lockable_boolean },
  { "passwd",	        &opt.ftp_passwd,	cmd_string },/* deprecated*/
  { "password",	        &opt.passwd,		cmd_string },
  { "postdata",		&opt.post_data,		cmd_string },
  { "postfile",		&opt.post_file_name,	cmd_file },
  { "preferfamily",	NULL,			cmd_spec_prefer_family },
  { "preservepermissions", &opt.preserve_perm,	cmd_boolean },
#ifdef HAVE_SSL
  { "privatekey",	&opt.private_key,	cmd_file },
  { "privatekeytype",	&opt.private_key_type,	cmd_cert_type },
#endif
  { "progress",		&opt.progress_type,	cmd_spec_progress },
  { "protocoldirectories", &opt.protocol_directories, cmd_boolean },
  { "proxypasswd",	&opt.proxy_passwd,	cmd_string }, /* deprecated */
  { "proxypassword",	&opt.proxy_passwd,	cmd_string },
  { "proxyuser",	&opt.proxy_user,	cmd_string },
  { "quiet",		&opt.quiet,		cmd_boolean },
  { "quota",		&opt.quota,		cmd_bytes_sum },
#ifdef HAVE_SSL
  { "randomfile",	&opt.random_file,	cmd_file },
#endif
  { "randomwait",	&opt.random_wait,	cmd_boolean },
  { "readtimeout",	&opt.read_timeout,	cmd_time },
  { "reclevel",		&opt.reclevel,		cmd_number_inf },
  { "recursive",	NULL,			cmd_spec_recursive },
  { "referer",		&opt.referer,		cmd_string },
  { "reject",		&opt.rejects,		cmd_vector },
  { "relativeonly",	&opt.relative_only,	cmd_boolean },
  { "removelisting",	&opt.remove_listing,	cmd_boolean },
  { "restrictfilenames", NULL,			cmd_spec_restrict_file_names },
  { "retrsymlinks",	&opt.retr_symlinks,	cmd_boolean },
  { "retryconnrefused",	&opt.retry_connrefused,	cmd_boolean },
  { "robots",		&opt.use_robots,	cmd_boolean },
  { "savecookies",	&opt.cookies_output,	cmd_file },
  { "saveheaders",	&opt.save_headers,	cmd_boolean },
#ifdef HAVE_SSL
  { "secureprotocol",	&opt.secure_protocol,	cmd_spec_secure_protocol },
#endif
  { "serverresponse",	&opt.server_response,	cmd_boolean },
  { "spanhosts",	&opt.spanhost,		cmd_boolean },
  { "spider",		&opt.spider,		cmd_boolean },
  { "strictcomments",	&opt.strict_comments,	cmd_boolean },
  { "timeout",		NULL,			cmd_spec_timeout },
  { "timestamping",	&opt.timestamping,	cmd_boolean },
  { "tries",		&opt.ntry,		cmd_number_inf },
  { "useproxy",		&opt.use_proxy,		cmd_boolean },
  { "user",	        &opt.user,		cmd_string },
  { "useragent",	NULL,			cmd_spec_useragent },
  { "verbose",		&opt.verbose,		cmd_boolean },
  { "wait",		&opt.wait,		cmd_time },
  { "waitretry",	&opt.waitretry,		cmd_time }
};

/* Look up CMDNAME in the commands[] and return its position in the
   array.  If CMDNAME is not found, return -1.  */

static int
command_by_name (const char *cmdname)
{
  /* Use binary search for speed.  Wget has ~100 commands, which
     guarantees a worst case performance of 7 string comparisons.  */
  int lo = 0, hi = countof (commands) - 1;

  while (lo <= hi)
    {
      int mid = (lo + hi) >> 1;
      int cmp = strcasecmp (cmdname, commands[mid].name);
      if (cmp < 0)
	hi = mid - 1;
      else if (cmp > 0)
	lo = mid + 1;
      else
	return mid;
    }
  return -1;
}

/* Reset the variables to default values.  */
static void
defaults (void)
{
  char *tmp;

  /* Most of the default values are 0.  Just reset everything, and
     fill in the non-zero values.  Note that initializing pointers to
     NULL this way is technically illegal, but porting Wget to a
     machine where NULL is not all-zero bit pattern will be the least
     of the implementors' worries.  */
  xzero (opt);

  opt.cookies = 1;
  opt.verbose = -1;
  opt.ntry = 20;
  opt.reclevel = 5;
  opt.add_hostdir = 1;
  opt.netrc = 1;
  opt.ftp_glob = 1;
  opt.htmlify = 1;
  opt.http_keep_alive = 1;
  opt.use_proxy = 1;
  tmp = getenv ("no_proxy");
  if (tmp)
    opt.no_proxy = sepstring (tmp);
  opt.allow_cache = 1;

  opt.read_timeout = 900;
  opt.use_robots = 1;

  opt.remove_listing = 1;

  opt.dot_bytes = 1024;
  opt.dot_spacing = 10;
  opt.dots_in_line = 50;

  opt.dns_cache = 1;
  opt.ftp_pasv = 1;

#ifdef HAVE_SSL
  opt.check_cert = 1;
#endif

  /* The default for file name restriction defaults to the OS type. */
#if !defined(WINDOWS) && !defined(__CYGWIN__)
  opt.restrict_files_os = restrict_unix;
#else
  opt.restrict_files_os = restrict_windows;
#endif
  opt.restrict_files_ctrl = 1;
}

/* Return the user's home directory (strdup-ed), or NULL if none is
   found.  */
char *
home_dir (void)
{
  char *home = getenv ("HOME");

  if (!home)
    {
#ifndef WINDOWS
      /* If HOME is not defined, try getting it from the password
         file.  */
      struct passwd *pwd = getpwuid (getuid ());
      if (!pwd || !pwd->pw_dir)
	return NULL;
      home = pwd->pw_dir;
#else  /* WINDOWS */
      /* Under Windows, if $HOME isn't defined, use the directory where
         `wget.exe' resides.  */
      home = ws_mypath ();
#endif /* WINDOWS */
    }

  return home ? xstrdup (home) : NULL;
}

/* Return the path to the user's .wgetrc.  This is either the value of
   `WGETRC' environment variable, or `$HOME/.wgetrc'.

   If the `WGETRC' variable exists but the file does not exist, the
   function will exit().  */
static char *
wgetrc_file_name (void)
{
  char *env, *home;
  char *file = NULL;

  /* Try the environment.  */
  env = getenv ("WGETRC");
  if (env && *env)
    {
      if (!file_exists_p (env))
	{
	  fprintf (stderr, _("%s: WGETRC points to %s, which doesn't exist.\n"),
		   exec_name, env);
	  exit (1);
	}
      return xstrdup (env);
    }

  /* If that failed, try $HOME/.wgetrc.  */
  home = home_dir ();
  if (home)
    file = aprintf ("%s/.wgetrc", home);
  xfree_null (home);

#ifdef WINDOWS
  /* Under Windows, if we still haven't found .wgetrc, look for the file
     `wget.ini' in the directory where `wget.exe' resides; we do this for
     backward compatibility with previous versions of Wget.
     SYSTEM_WGETRC should not be defined under WINDOWS.  */
  if (!file || !file_exists_p (file))
    {
      xfree_null (file);
      file = NULL;
      home = ws_mypath ();
      if (home)
	file = aprintf ("%s/wget.ini", home);
    }
#endif /* WINDOWS */

  if (!file)
    return NULL;
  if (!file_exists_p (file))
    {
      xfree (file);
      return NULL;
    }
  return file;
}

/* Return values of parse_line. */
enum parse_line {
  line_ok,
  line_empty,
  line_syntax_error,
  line_unknown_command
};

static enum parse_line parse_line PARAMS ((const char *, char **,
					   char **, int *));
static int setval_internal PARAMS ((int, const char *, const char *));

/* Initialize variables from a wgetrc file.  Returns zero (failure) if
   there were errors in the file.  */

static int
run_wgetrc (const char *file)
{
  FILE *fp;
  char *line;
  int ln;
  int errcnt = 0;

  fp = fopen (file, "rb");
  if (!fp)
    {
      fprintf (stderr, _("%s: Cannot read %s (%s).\n"), exec_name,
	       file, strerror (errno));
      return 1;			/* not a fatal error */
    }
  enable_tilde_expansion = 1;
  ln = 1;
  while ((line = read_whole_line (fp)) != NULL)
    {
      char *com = NULL, *val = NULL;
      int comind;

      /* Parse the line.  */
      switch (parse_line (line, &com, &val, &comind))
	{
	case line_ok:
	  /* If everything is OK, set the value.  */
	  if (!setval_internal (comind, com, val))
	    {
	      fprintf (stderr, _("%s: Error in %s at line %d.\n"),
		       exec_name, file, ln);
	      ++errcnt;
	    }
	  break;
	case line_syntax_error:
	  fprintf (stderr, _("%s: Syntax error in %s at line %d.\n"),
		   exec_name, file, ln);
	  ++errcnt;
	  break;
	case line_unknown_command:
	  fprintf (stderr, _("%s: Unknown command `%s' in %s at line %d.\n"),
		   exec_name, com, file, ln);
	  ++errcnt;
	  break;
	case line_empty:
	  break;
	default:
	  abort ();
	}
      xfree_null (com);
      xfree_null (val);
      xfree (line);
      ++ln;
    }
  enable_tilde_expansion = 0;
  fclose (fp);

  return errcnt == 0;
}

/* Initialize the defaults and run the system wgetrc and user's own
   wgetrc.  */
void
initialize (void)
{
  char *file;
  int ok = 1;

  /* Load the hard-coded defaults.  */
  defaults ();

  /* If SYSTEM_WGETRC is defined, use it.  */
#ifdef SYSTEM_WGETRC
  if (file_exists_p (SYSTEM_WGETRC))
    ok &= run_wgetrc (SYSTEM_WGETRC);
#endif
  /* Override it with your own, if one exists.  */
  file = wgetrc_file_name ();
  if (!file)
    return;
  /* #### We should canonicalize `file' and SYSTEM_WGETRC with
     something like realpath() before comparing them with `strcmp'  */
#ifdef SYSTEM_WGETRC
  if (!strcmp (file, SYSTEM_WGETRC))
    {
      fprintf (stderr, _("\
%s: Warning: Both system and user wgetrc point to `%s'.\n"),
	       exec_name, file);
    }
  else
#endif
    ok &= run_wgetrc (file);

  /* If there were errors processing either `.wgetrc', abort. */
  if (!ok)
    exit (2);

  xfree (file);
  return;
}

/* Remove dashes and underscores from S, modifying S in the
   process. */

static void
dehyphen (char *s)
{
  char *t = s;			/* t - tortoise */
  char *h = s;			/* h - hare     */
  while (*h)
    if (*h == '_' || *h == '-')
      ++h;
    else
      *t++ = *h++;
  *t = '\0';
}

/* Parse the line pointed by line, with the syntax:
   <sp>* command <sp>* = <sp>* value <sp>*
   Uses malloc to allocate space for command and value.
   If the line is invalid, data is freed and 0 is returned.

   Returns one of line_ok, line_empty, line_syntax_error, or
   line_unknown_command.

   In case of line_ok, *COM and *VAL point to freshly allocated
   strings, and *COMIND points to com's index.  In case of error or
   empty line, their values are unmodified.  */

static enum parse_line
parse_line (const char *line, char **com, char **val, int *comind)
{
  const char *p;
  const char *end = line + strlen (line);
  const char *cmdstart, *cmdend;
  const char *valstart, *valend;

  char *cmdcopy;
  int ind;

  /* Skip leading and trailing whitespace.  */
  while (*line && ISSPACE (*line))
    ++line;
  while (end > line && ISSPACE (end[-1]))
    --end;

  /* Skip empty lines and comments.  */
  if (!*line || *line == '#')
    return line_empty;

  p = line;

  cmdstart = p;
  while (p < end && (ISALNUM (*p) || *p == '_' || *p == '-'))
    ++p;
  cmdend = p;

  /* Skip '=', as well as any space before or after it. */
  while (p < end && ISSPACE (*p))
    ++p;
  if (p == end || *p != '=')
    return line_syntax_error;
  ++p;
  while (p < end && ISSPACE (*p))
    ++p;

  valstart = p;
  valend   = end;

  /* The syntax is valid (even though the command might not be).  Fill
     in the command name and value.  */
  *com = strdupdelim (cmdstart, cmdend);
  *val = strdupdelim (valstart, valend);

  /* The line now known to be syntactically correct.  Check whether
     the command is valid.  */
  BOUNDED_TO_ALLOCA (cmdstart, cmdend, cmdcopy);
  dehyphen (cmdcopy);
  ind = command_by_name (cmdcopy);
  if (ind == -1)
    return line_unknown_command;

  /* Report success to the caller. */
  *comind = ind;
  return line_ok;
}

/* Run commands[comind].action. */

static int
setval_internal (int comind, const char *com, const char *val)
{
  assert (0 <= comind && comind < countof (commands));
  DEBUGP (("Setting %s (%s) to %s\n", com, commands[comind].name, val));
  return ((*commands[comind].action) (com, val, commands[comind].place));
}

/* Run command COM with value VAL.  If running the command produces an
   error, report the error and exit.

   This is intended to be called from main() to modify Wget's behavior
   through command-line switches.  Since COM is hard-coded in main(),
   it is not canonicalized, and this aborts when COM is not found.

   If COMIND's are exported to init.h, this function will be changed
   to accept COMIND directly.  */

void
setoptval (const char *com, const char *val, const char *optname)
{
  /* Prepend "--" to OPTNAME. */
  char *dd_optname = (char *) alloca (2 + strlen (optname) + 1);
  dd_optname[0] = '-';
  dd_optname[1] = '-';
  strcpy (dd_optname + 2, optname);

  assert (val != NULL);
  if (!setval_internal (command_by_name (com), dd_optname, val))
    exit (2);
}

/* Parse OPT into command and value and run it.  For example,
   run_command("foo=bar") is equivalent to setoptval("foo", "bar").
   This is used by the `--execute' flag in main.c.  */

void
run_command (const char *opt)
{
  char *com, *val;
  int comind;
  switch (parse_line (opt, &com, &val, &comind))
    {
    case line_ok:
      if (!setval_internal (comind, com, val))
	exit (2);
      xfree (com);
      xfree (val);
      break;
    default:
      fprintf (stderr, _("%s: Invalid --execute command `%s'\n"),
	       exec_name, opt);
      exit (2);
    }
}

/* Generic helper functions, for use with `commands'. */

/* Forward declarations: */
struct decode_item {
  const char *name;
  int code;
};
static int decode_string PARAMS ((const char *, const struct decode_item *,
				  int, int *));
static int simple_atoi PARAMS ((const char *, const char *, int *));
static int simple_atof PARAMS ((const char *, const char *, double *));

#define CMP1(p, c0) (TOLOWER((p)[0]) == (c0) && (p)[1] == '\0')

#define CMP2(p, c0, c1) (TOLOWER((p)[0]) == (c0)	\
			 && TOLOWER((p)[1]) == (c1)	\
			 && (p)[2] == '\0')

#define CMP3(p, c0, c1, c2) (TOLOWER((p)[0]) == (c0)	\
		     && TOLOWER((p)[1]) == (c1)		\
		     && TOLOWER((p)[2]) == (c2)		\
		     && (p)[3] == '\0')


/* Store the boolean value from VAL to PLACE.  COM is ignored,
   except for error messages.  */
static int
cmd_boolean (const char *com, const char *val, void *place)
{
  int bool_value;

  if (CMP2 (val, 'o', 'n') || CMP3 (val, 'y', 'e', 's') || CMP1 (val, '1'))
    /* "on", "yes" and "1" mean true. */
    bool_value = 1;
  else if (CMP3 (val, 'o', 'f', 'f') || CMP2 (val, 'n', 'o') || CMP1 (val, '0'))
    /* "off", "no" and "0" mean false. */
    bool_value = 0;
  else
    {
      fprintf (stderr,
	       _("%s: %s: Invalid boolean `%s'; use `on' or `off'.\n"),
	       exec_name, com, val);
      return 0;
    }

  *(int *)place = bool_value;
  return 1;
}

/* Store the lockable_boolean {2, 1, 0, -1} value from VAL to PLACE.
   COM is ignored, except for error messages.  Values 2 and -1
   indicate that once defined, the value may not be changed by
   successive wgetrc files or command-line arguments.

   Values: 2 - Enable a particular option for good ("always")
           1 - Enable an option ("on")
           0 - Disable an option ("off")
          -1 - Disable an option for good ("never")

   #### This hack is currently only used for passive FTP because a
   contributor had broken scripts specify --passive-ftp where he
   didn't want it.  It should be removed because the same can now be
   achieved by replacing the wget executable with a script containing:

       exec wget "$@" --no-passive-ftp
*/

static int
cmd_lockable_boolean (const char *com, const char *val, void *place)
{
  int lockable_boolean_value;

  int oldval = *(int *)place;

  /*
   * If a config file said "always" or "never", don't allow command line
   * arguments to override the config file.
   */
  if (oldval == -1 || oldval == 2)
    return 1;

  if (CMP2 (val, 'o', 'n') || CMP3 (val, 'y', 'e', 's') || CMP1 (val, '1'))
    lockable_boolean_value = 1;
  else if (CMP3 (val, 'o', 'f', 'f') || CMP2 (val, 'n', 'o') || CMP1 (val, '0'))
    lockable_boolean_value = 0;
  else if (0 == strcasecmp (val, "always"))
    lockable_boolean_value = 2;
  else if (0 == strcasecmp (val, "never"))
    lockable_boolean_value = -1;
  else
    {
      fprintf (stderr,
	       _("%s: %s: Invalid extended boolean `%s';\n\
use one of `on', `off', `always', or `never'.\n"),
	       exec_name, com, val);
      return 0;
    }

  *(int *)place = lockable_boolean_value;
  return 1;
}

/* Set the non-negative integer value from VAL to PLACE.  With
   incorrect specification, the number remains unchanged.  */
static int
cmd_number (const char *com, const char *val, void *place)
{
  if (!simple_atoi (val, val + strlen (val), place)
      || *(int *) place < 0)
    {
      fprintf (stderr, _("%s: %s: Invalid number `%s'.\n"),
	       exec_name, com, val);
      return 0;
    }
  return 1;
}

/* Similar to cmd_number(), only accepts `inf' as a synonym for 0.  */
static int
cmd_number_inf (const char *com, const char *val, void *place)
{
  if (!strcasecmp (val, "inf"))
    {
      *(int *)place = 0;
      return 1;
    }
  return cmd_number (com, val, place);
}

/* Copy (strdup) the string at COM to a new location and place a
   pointer to *PLACE.  */
static int
cmd_string (const char *com, const char *val, void *place)
{
  char **pstring = (char **)place;

  xfree_null (*pstring);
  *pstring = xstrdup (val);
  return 1;
}

#ifndef WINDOWS
# define ISSEP(c) ((c) == '/')
#else
# define ISSEP(c) ((c) == '/' || (c) == '\\')
#endif

/* Like the above, but handles tilde-expansion when reading a user's
   `.wgetrc'.  In that case, and if VAL begins with `~', the tilde
   gets expanded to the user's home directory.  */
static int
cmd_file (const char *com, const char *val, void *place)
{
  char **pstring = (char **)place;

  xfree_null (*pstring);

  /* #### If VAL is empty, perhaps should set *PLACE to NULL.  */

  if (!enable_tilde_expansion || !(*val == '~' && ISSEP (val[1])))
    {
    noexpand:
      *pstring = xstrdup (val);
    }
  else
    {
      int homelen;
      char *home = home_dir ();
      if (!home)
	goto noexpand;

      homelen = strlen (home);
      while (homelen && ISSEP (home[homelen - 1]))
	home[--homelen] = '\0';

      /* Skip the leading "~/". */
      for (++val; ISSEP (*val); val++)
	;

      *pstring = concat_strings (home, "/", val, (char *) 0);
    }

#ifdef WINDOWS
  /* Convert "\" to "/". */
  {
    char *s;
    for (s = *pstring; *s; s++)
      if (*s == '\\')
	*s = '/';
  }
#endif
  return 1;
}

/* Like cmd_file, but strips trailing '/' characters.  */
static int
cmd_directory (const char *com, const char *val, void *place)
{
  char *s, *t;

  /* Call cmd_file() for tilde expansion and separator
     canonicalization (backslash -> slash under Windows).  These
     things should perhaps be in a separate function.  */
  if (!cmd_file (com, val, place))
    return 0;

  s = *(char **)place;
  t = s + strlen (s);
  while (t > s && *--t == '/')
    *t = '\0';

  return 1;
}

/* Split VAL by space to a vector of values, and append those values
   to vector pointed to by the PLACE argument.  If VAL is empty, the
   PLACE vector is cleared instead.  */

static int
cmd_vector (const char *com, const char *val, void *place)
{
  char ***pvec = (char ***)place;

  if (*val)
    *pvec = merge_vecs (*pvec, sepstring (val));
  else
    {
      free_vec (*pvec);
      *pvec = NULL;
    }
  return 1;
}

static int
cmd_directory_vector (const char *com, const char *val, void *place)
{
  char ***pvec = (char ***)place;

  if (*val)
    {
      /* Strip the trailing slashes from directories.  */
      char **t, **seps;

      seps = sepstring (val);
      for (t = seps; t && *t; t++)
	{
	  int len = strlen (*t);
	  /* Skip degenerate case of root directory.  */
	  if (len > 1)
	    {
	      if ((*t)[len - 1] == '/')
		(*t)[len - 1] = '\0';
	    }
	}
      *pvec = merge_vecs (*pvec, seps);
    }
  else
    {
      free_vec (*pvec);
      *pvec = NULL;
    }
  return 1;
}

/* Engine for cmd_bytes and cmd_bytes_sum: converts a string such as
   "100k" or "2.5G" to a floating point number.  */

static int
parse_bytes_helper (const char *val, double *result)
{
  double number, mult;
  const char *end = val + strlen (val);

  /* Check for "inf".  */
  if (0 == strcmp (val, "inf"))
    {
      *result = 0;
      return 1;
    }

  /* Strip trailing whitespace.  */
  while (val < end && ISSPACE (end[-1]))
    --end;
  if (val == end)
    return 0;

  switch (TOLOWER (end[-1]))
    {
    case 'k':
      --end, mult = 1024.0;
      break;
    case 'm':
      --end, mult = 1048576.0;
      break;
    case 'g':
      --end, mult = 1073741824.0;
      break;
    case 't':
      --end, mult = 1099511627776.0;
      break;
    default:
      /* Not a recognized suffix: assume it's a digit.  (If not,
	 simple_atof will raise an error.)  */
      mult = 1;
    }

  /* Skip leading and trailing whitespace. */
  while (val < end && ISSPACE (*val))
    ++val;
  while (val < end && ISSPACE (end[-1]))
    --end;
  if (val == end)
    return 0;

  if (!simple_atof (val, end, &number) || number < 0)
    return 0;

  *result = number * mult;
  return 1;
}

/* Parse VAL as a number and set its value to PLACE (which should
   point to a wgint).

   By default, the value is assumed to be in bytes.  If "K", "M", or
   "G" are appended, the value is multiplied with 1<<10, 1<<20, or
   1<<30, respectively.  Floating point values are allowed and are
   cast to integer before use.  The idea is to be able to use things
   like 1.5k instead of "1536".

   The string "inf" is returned as 0.

   In case of error, 0 is returned and memory pointed to by PLACE
   remains unmodified.  */

static int
cmd_bytes (const char *com, const char *val, void *place)
{
  double byte_value;
  if (!parse_bytes_helper (val, &byte_value))
    {
      fprintf (stderr, _("%s: %s: Invalid byte value `%s'\n"),
	       exec_name, com, val);
      return 0;
    }
  *(wgint *)place = (wgint)byte_value;
  return 1;
}

/* Like cmd_bytes, but PLACE is interpreted as a pointer to
   SIZE_SUM.  It works by converting the string to double, therefore
   working with values up to 2^53-1 without loss of precision.  This
   value (8192 TB) is large enough to serve for a while.  */

static int
cmd_bytes_sum (const char *com, const char *val, void *place)
{
  double byte_value;
  if (!parse_bytes_helper (val, &byte_value))
    {
      fprintf (stderr, _("%s: %s: Invalid byte value `%s'\n"),
	       exec_name, com, val);
      return 0;
    }
  *(SUM_SIZE_INT *) place = (SUM_SIZE_INT) byte_value;
  return 1;
}

/* Store the value of VAL to *OUT.  The value is a time period, by
   default expressed in seconds, but also accepting suffixes "m", "h",
   "d", and "w" for minutes, hours, days, and weeks respectively.  */

static int
cmd_time (const char *com, const char *val, void *place)
{
  double number, mult;
  const char *end = val + strlen (val);

  /* Strip trailing whitespace.  */
  while (val < end && ISSPACE (end[-1]))
    --end;

  if (val == end)
    {
    err:
      fprintf (stderr, _("%s: %s: Invalid time period `%s'\n"),
	       exec_name, com, val);
      return 0;
    }

  switch (TOLOWER (end[-1]))
    {
    case 's':
      --end, mult = 1;		/* seconds */
      break;
    case 'm':
      --end, mult = 60;		/* minutes */
      break;
    case 'h':
      --end, mult = 3600;	/* hours */
      break;
    case 'd':
      --end, mult = 86400.0;	/* days */
      break;
    case 'w':
      --end, mult = 604800.0;	/* weeks */
      break;
    default:
      /* Not a recognized suffix: assume it belongs to the number.
	 (If not, simple_atof will raise an error.)  */
      mult = 1;
    }

  /* Skip leading and trailing whitespace. */
  while (val < end && ISSPACE (*val))
    ++val;
  while (val < end && ISSPACE (end[-1]))
    --end;
  if (val == end)
    goto err;

  if (!simple_atof (val, end, &number))
    goto err;

  *(double *)place = number * mult;
  return 1;
}

#ifdef HAVE_SSL
static int
cmd_cert_type (const char *com, const char *val, void *place)
{
  static const struct decode_item choices[] = {
    { "pem", keyfile_pem },
    { "der", keyfile_asn1 },
    { "asn1", keyfile_asn1 },
  };
  int ok = decode_string (val, choices, countof (choices), place);
  if (!ok)
    fprintf (stderr, _("%s: %s: Invalid value `%s'.\n"), exec_name, com, val);
  return ok;
}
#endif

/* Specialized helper functions, used by `commands' to handle some
   options specially.  */

static int check_user_specified_header PARAMS ((const char *));

static int
cmd_spec_dirstruct (const char *com, const char *val, void *place_ignored)
{
  if (!cmd_boolean (com, val, &opt.dirstruct))
    return 0;
  /* Since dirstruct behaviour is explicitly changed, no_dirstruct
     must be affected inversely.  */
  if (opt.dirstruct)
    opt.no_dirstruct = 0;
  else
    opt.no_dirstruct = 1;
  return 1;
}

static int
cmd_spec_header (const char *com, const char *val, void *place_ignored)
{
  /* Empty value means reset the list of headers. */
  if (*val == '\0')
    {
      free_vec (opt.user_headers);
      opt.user_headers = NULL;
      return 1;
    }

  if (!check_user_specified_header (val))
    {
      fprintf (stderr, _("%s: %s: Invalid header `%s'.\n"),
	       exec_name, com, val);
      return 0;
    }
  opt.user_headers = vec_append (opt.user_headers, val);
  return 1;
}

static int
cmd_spec_htmlify (const char *com, const char *val, void *place_ignored)
{
  int flag = cmd_boolean (com, val, &opt.htmlify);
  if (flag && !opt.htmlify)
    opt.remove_listing = 0;
  return flag;
}

/* Set the "mirror" mode.  It means: recursive download, timestamping,
   no limit on max. recursion depth, and don't remove listings.  */

static int
cmd_spec_mirror (const char *com, const char *val, void *place_ignored)
{
  int mirror;

  if (!cmd_boolean (com, val, &mirror))
    return 0;
  if (mirror)
    {
      opt.recursive = 1;
      if (!opt.no_dirstruct)
	opt.dirstruct = 1;
      opt.timestamping = 1;
      opt.reclevel = INFINITE_RECURSION;
      opt.remove_listing = 0;
    }
  return 1;
}

/* Validate --prefer-family and set the choice.  Allowed values are
   "IPv4", "IPv6", and "none".  */

static int
cmd_spec_prefer_family (const char *com, const char *val, void *place_ignored)
{
  static const struct decode_item choices[] = {
    { "IPv4", prefer_ipv4 },
    { "IPv6", prefer_ipv6 },
    { "none", prefer_none },
  };
  int ok = decode_string (val, choices, countof (choices),
			  (int *) &opt.prefer_family);
  if (!ok)
    fprintf (stderr, _("%s: %s: Invalid value `%s'.\n"), exec_name, com, val);
  return ok;
}

/* Set progress.type to VAL, but verify that it's a valid progress
   implementation before that.  */

static int
cmd_spec_progress (const char *com, const char *val, void *place_ignored)
{
  if (!valid_progress_implementation_p (val))
    {
      fprintf (stderr, _("%s: %s: Invalid progress type `%s'.\n"),
	       exec_name, com, val);
      return 0;
    }
  xfree_null (opt.progress_type);

  /* Don't call set_progress_implementation here.  It will be called
     in main() when it becomes clear what the log output is.  */
  opt.progress_type = xstrdup (val);
  return 1;
}

/* Set opt.recursive to VAL as with cmd_boolean.  If opt.recursive is
   set to true, also set opt.dirstruct to 1, unless opt.no_dirstruct
   is specified.  */

static int
cmd_spec_recursive (const char *com, const char *val, void *place_ignored)
{
  if (!cmd_boolean (com, val, &opt.recursive))
    return 0;
  else
    {
      if (opt.recursive && !opt.no_dirstruct)
	opt.dirstruct = 1;
    }
  return 1;
}

static int
cmd_spec_restrict_file_names (const char *com, const char *val, void *place_ignored)
{
  int restrict_os = opt.restrict_files_os;
  int restrict_ctrl = opt.restrict_files_ctrl;

  const char *end = strchr (val, ',');
  if (!end)
    end = val + strlen (val);

#define VAL_IS(string_literal) BOUNDED_EQUAL (val, end, string_literal)

  if (VAL_IS ("unix"))
    restrict_os = restrict_unix;
  else if (VAL_IS ("windows"))
    restrict_os = restrict_windows;
  else if (VAL_IS ("nocontrol"))
    restrict_ctrl = 0;
  else
    {
    err:
      fprintf (stderr,
	       _("%s: %s: Invalid restriction `%s', use `unix' or `windows'.\n"),
	       exec_name, com, val);
      return 0;
    }

#undef VAL_IS

  if (*end)
    {
      if (!strcmp (end + 1, "nocontrol"))
	restrict_ctrl = 0;
      else
	goto err;
    }

  opt.restrict_files_os = restrict_os;
  opt.restrict_files_ctrl = restrict_ctrl;
  return 1;
}

#ifdef HAVE_SSL
static int
cmd_spec_secure_protocol (const char *com, const char *val, void *place)
{
  static const struct decode_item choices[] = {
    { "auto", secure_protocol_auto },
    { "sslv2", secure_protocol_sslv2 },
    { "sslv3", secure_protocol_sslv3 },
    { "tlsv1", secure_protocol_tlsv1 },
  };
  int ok = decode_string (val, choices, countof (choices), place);
  if (!ok)
    fprintf (stderr, _("%s: %s: Invalid value `%s'.\n"), exec_name, com, val);
  return ok;
}
#endif

/* Set all three timeout values. */

static int
cmd_spec_timeout (const char *com, const char *val, void *place_ignored)
{
  double value;
  if (!cmd_time (com, val, &value))
    return 0;
  opt.read_timeout = value;
  opt.connect_timeout = value;
  opt.dns_timeout = value;
  return 1;
}

static int
cmd_spec_useragent (const char *com, const char *val, void *place_ignored)
{
  /* Disallow embedded newlines.  */
  if (strchr (val, '\n'))
    {
      fprintf (stderr, _("%s: %s: Invalid value `%s'.\n"),
	       exec_name, com, val);
      return 0;
    }
  xfree_null (opt.useragent);
  opt.useragent = xstrdup (val);
  return 1;
}

/* Miscellaneous useful routines.  */

/* A very simple atoi clone, more useful than atoi because it works on
   delimited strings, and has error reportage.  Returns 1 on success,
   0 on failure.  If successful, stores result to *DEST.  */

static int
simple_atoi (const char *beg, const char *end, int *dest)
{
  int result = 0;
  int negative = 0;
  const char *p = beg;

  while (p < end && ISSPACE (*p))
    ++p;
  if (p < end && (*p == '-' || *p == '+'))
    {
      negative = (*p == '-');
      ++p;
    }
  if (p == end)
    return 0;

  /* Read negative numbers in a separate loop because the most
     negative integer cannot be represented as a positive number.  */

  if (!negative)
    for (; p < end && ISDIGIT (*p); p++)
      {
	int next = (10 * result) + (*p - '0');
	if (next < result)
	  return 0;		/* overflow */
	result = next;
      }
  else
    for (; p < end && ISDIGIT (*p); p++)
      {
	int next = (10 * result) - (*p - '0');
	if (next > result)
	  return 0;		/* underflow */
	result = next;
      }

  if (p != end)
    return 0;

  *dest = result;
  return 1;
}

/* Trivial atof, with error reporting.  Handles "<digits>[.<digits>]",
   doesn't handle exponential notation.  Returns 1 on success, 0 on
   failure.  In case of success, stores its result to *DEST.  */

static int
simple_atof (const char *beg, const char *end, double *dest)
{
  double result = 0;

  int negative = 0;
  int seen_dot = 0;
  int seen_digit = 0;
  double divider = 1;

  const char *p = beg;

  while (p < end && ISSPACE (*p))
    ++p;
  if (p < end && (*p == '-' || *p == '+'))
    {
      negative = (*p == '-');
      ++p;
    }

  for (; p < end; p++)
    {
      char ch = *p;
      if (ISDIGIT (ch))
	{
	  if (!seen_dot)
	    result = (10 * result) + (ch - '0');
	  else
	    result += (ch - '0') / (divider *= 10);
	  seen_digit = 1;
	}
      else if (ch == '.')
	{
	  if (!seen_dot)
	    seen_dot = 1;
	  else
	    return 0;
	}
      else
	return 0;
    }
  if (!seen_digit)
    return 0;
  if (negative)
    result = -result;

  *dest = result;
  return 1;
}

/* Verify that the user-specified header in S is valid.  It must
   contain a colon preceded by non-white-space characters and must not
   contain newlines.  */

static int
check_user_specified_header (const char *s)
{
  const char *p;

  for (p = s; *p && *p != ':' && !ISSPACE (*p); p++);
  /* The header MUST contain `:' preceded by at least one
     non-whitespace character.  */
  if (*p != ':' || p == s)
    return 0;
  /* The header MUST NOT contain newlines.  */
  if (strchr (s, '\n'))
    return 0;
  return 1;
}

/* Decode VAL into a number, according to ITEMS. */

static int
decode_string (const char *val, const struct decode_item *items, int itemcount,
	       int *place)
{
  int i;
  for (i = 0; i < itemcount; i++)
    if (0 == strcasecmp (val, items[i].name))
      {
	*place = items[i].code;
	return 1;
      }
  return 0;
}


void cleanup_html_url PARAMS ((void));
void http_cleanup PARAMS ((void));


/* Free the memory allocated by global variables.  */
void
cleanup (void)
{
  /* Free external resources, close files, etc. */

  {
    extern FILE *output_stream;
    if (output_stream)
      fclose (output_stream);
    /* No need to check for error because Wget flushes its output (and
       checks for errors) after any data arrives.  */
  }

  /* We're exiting anyway so there's no real need to call free()
     hundreds of times.  Skipping the frees will make Wget exit
     faster.

     However, when detecting leaks, it's crucial to free() everything
     because then you can find the real leaks, i.e. the allocated
     memory which grows with the size of the program.  */

#ifdef DEBUG_MALLOC
  convert_cleanup ();
  res_cleanup ();
  http_cleanup ();
  cleanup_html_url ();
  host_cleanup ();
  log_cleanup ();

  {
    extern acc_t *netrc_list;
    free_netrc (netrc_list);
  }
  xfree_null (opt.lfilename);
  xfree_null (opt.dir_prefix);
  xfree_null (opt.input_filename);
  xfree_null (opt.output_document);
  free_vec (opt.accepts);
  free_vec (opt.rejects);
  free_vec (opt.excludes);
  free_vec (opt.includes);
  free_vec (opt.domains);
  free_vec (opt.follow_tags);
  free_vec (opt.ignore_tags);
  xfree_null (opt.progress_type);
  xfree_null (opt.ftp_user);
  xfree_null (opt.ftp_passwd);
  xfree_null (opt.ftp_proxy);
  xfree_null (opt.https_proxy);
  xfree_null (opt.http_proxy);
  free_vec (opt.no_proxy);
  xfree_null (opt.useragent);
  xfree_null (opt.referer);
  xfree_null (opt.http_user);
  xfree_null (opt.http_passwd);
  free_vec (opt.user_headers);
# ifdef HAVE_SSL
  xfree_null (opt.cert_file);
  xfree_null (opt.private_key);
  xfree_null (opt.ca_directory);
  xfree_null (opt.ca_cert);
  xfree_null (opt.random_file);
  xfree_null (opt.egd_file);
# endif
  xfree_null (opt.bind_address);
  xfree_null (opt.cookies_input);
  xfree_null (opt.cookies_output);
  xfree_null (opt.user);
  xfree_null (opt.passwd);
#endif /* DEBUG_MALLOC */
}
