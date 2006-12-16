/* Command line parsing.
   Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.

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
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <sys/types.h>
#ifdef HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif /* HAVE_STRING_H */
#ifdef HAVE_SIGNAL_H
# include <signal.h>
#endif
#ifdef HAVE_NLS
#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif /* HAVE_LOCALE_H */
#endif /* HAVE_NLS */
#include <assert.h>

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#include "wget.h"
#include "utils.h"
#include "init.h"
#include "retr.h"
#include "recur.h"
#include "host.h"
#include "url.h"
#include "progress.h"		/* for progress_handle_sigwinch */
#include "convert.h"

/* On GNU system this will include system-wide getopt.h. */
#include "getopt.h"

#ifndef PATH_SEPARATOR
# define PATH_SEPARATOR '/'
#endif

struct options opt;

extern SUM_SIZE_INT total_downloaded_bytes;
extern char *version_string;

extern struct cookie_jar *wget_cookie_jar;

static RETSIGTYPE redirect_output_signal PARAMS ((int));

const char *exec_name;

/* Initialize I18N/L10N.  That amounts to invoking setlocale, and
   setting up gettext's message catalog using bindtextdomain and
   textdomain.  Does nothing if NLS is disabled or missing.  */

static void
i18n_initialize (void)
{
  /* HAVE_NLS implies existence of functions invoked here.  */
#ifdef HAVE_NLS
  /* Set the current locale.  */
  /* Where possible, sets only LC_MESSAGES and LC_CTYPE.  Other
     categories, such as numeric, time, or collation, break code that
     parses data received from the network and relies on C-locale
     behavior of libc functions.  For example, Solaris strptime fails
     to recognize English month names in non-English locales, which
     breaks http_atotm.  Some implementations of fnmatch perform
     unwanted case folding in non-C locales.  ctype macros, while they
     were used, provided another example against LC_ALL.  */
#if defined(LC_MESSAGES) && defined(LC_CTYPE)
  setlocale (LC_MESSAGES, "");
  setlocale (LC_CTYPE, "");	/* safe because we use safe-ctype */
#else
  setlocale (LC_ALL, "");
#endif
  /* Set the text message domain.  */
  bindtextdomain ("wget", LOCALEDIR);
  textdomain ("wget");
#endif /* HAVE_NLS */
}

/* Definition of command-line options. */

static void print_help PARAMS ((void));
static void print_version PARAMS ((void));

#ifdef HAVE_SSL
# define IF_SSL(x) x
#else
# define IF_SSL(x) NULL
#endif

#ifdef ENABLE_DEBUG
# define IF_DEBUG(x) x
#else
# define IF_DEBUG(x) NULL
#endif

struct cmdline_option {
  const char *long_name;
  char short_name;
  enum {
    OPT_VALUE,
    OPT_BOOLEAN,
    OPT_FUNCALL,
    /* Non-standard options that have to be handled specially in
       main().  */
    OPT__APPEND_OUTPUT,
    OPT__CLOBBER,
    OPT__DONT_REMOVE_LISTING,
    OPT__EXECUTE,
    OPT__NO,
    OPT__PARENT
  } type;
  const void *data;		/* for standard options */
  int argtype;			/* for non-standard options */
};

struct cmdline_option option_data[] =
  {
    { "accept", 'A', OPT_VALUE, "accept", -1 },
    { "append-output", 'a', OPT__APPEND_OUTPUT, NULL, required_argument },
    { "background", 'b', OPT_BOOLEAN, "background", -1 },
    { "backup-converted", 'K', OPT_BOOLEAN, "backupconverted", -1 },
    { "backups", 0, OPT_BOOLEAN, "backups", -1 },
    { "base", 'B', OPT_VALUE, "base", -1 },
    { "bind-address", 0, OPT_VALUE, "bindaddress", -1 },
    { IF_SSL ("ca-certificate"), 0, OPT_VALUE, "cacertificate", -1 },
    { IF_SSL ("ca-directory"), 0, OPT_VALUE, "cadirectory", -1 },
    { "cache", 0, OPT_BOOLEAN, "cache", -1 },
    { IF_SSL ("certificate"), 0, OPT_VALUE, "certificate", -1 },
    { IF_SSL ("certificate-type"), 0, OPT_VALUE, "certificatetype", -1 },
    { IF_SSL ("check-certificate"), 0, OPT_BOOLEAN, "checkcertificate", -1 },
    { "clobber", 0, OPT__CLOBBER, NULL, optional_argument },
    { "connect-timeout", 0, OPT_VALUE, "connecttimeout", -1 },
    { "continue", 'c', OPT_BOOLEAN, "continue", -1 },
    { "convert-links", 'k', OPT_BOOLEAN, "convertlinks", -1 },
    { "cookies", 0, OPT_BOOLEAN, "cookies", -1 },
    { "cut-dirs", 0, OPT_VALUE, "cutdirs", -1 },
    { IF_DEBUG ("debug"), 'd', OPT_BOOLEAN, "debug", -1 },
    { "delete-after", 0, OPT_BOOLEAN, "deleteafter", -1 },
    { "directories", 0, OPT_BOOLEAN, "dirstruct", -1 },
    { "directory-prefix", 'P', OPT_VALUE, "dirprefix", -1 },
    { "dns-cache", 0, OPT_BOOLEAN, "dnscache", -1 },
    { "dns-timeout", 0, OPT_VALUE, "dnstimeout", -1 },
    { "domains", 'D', OPT_VALUE, "domains", -1 },
    { "dont-remove-listing", 0, OPT__DONT_REMOVE_LISTING, NULL, no_argument },
    { "dot-style", 0, OPT_VALUE, "dotstyle", -1 },
    { "egd-file", 0, OPT_VALUE, "egdfile", -1 },
    { "exclude-directories", 'X', OPT_VALUE, "excludedirectories", -1 },
    { "exclude-domains", 0, OPT_VALUE, "excludedomains", -1 },
    { "execute", 'e', OPT__EXECUTE, NULL, required_argument },
    { "follow-ftp", 0, OPT_BOOLEAN, "followftp", -1 },
    { "follow-tags", 0, OPT_VALUE, "followtags", -1 },
    { "force-directories", 'x', OPT_BOOLEAN, "dirstruct", -1 },
    { "force-html", 'F', OPT_BOOLEAN, "forcehtml", -1 },
    { "ftp-password", 0, OPT_VALUE, "ftppassword", -1 },
    { "ftp-user", 0, OPT_VALUE, "ftpuser", -1 },
    { "glob", 0, OPT_BOOLEAN, "glob", -1 },
    { "header", 0, OPT_VALUE, "header", -1 },
    { "help", 'h', OPT_FUNCALL, (void *)print_help, no_argument },
    { "host-directories", 0, OPT_BOOLEAN, "addhostdir", -1 },
    { "html-extension", 'E', OPT_BOOLEAN, "htmlextension", -1 },
    { "htmlify", 0, OPT_BOOLEAN, "htmlify", -1 },
    { "http-keep-alive", 0, OPT_BOOLEAN, "httpkeepalive", -1 },
    { "http-passwd", 0, OPT_VALUE, "httppassword", -1 }, /* deprecated */
    { "http-password", 0, OPT_VALUE, "httppassword", -1 },
    { "http-user", 0, OPT_VALUE, "httpuser", -1 },
    { "ignore-length", 0, OPT_BOOLEAN, "ignorelength", -1 },
    { "ignore-tags", 0, OPT_VALUE, "ignoretags", -1 },
    { "include-directories", 'I', OPT_VALUE, "includedirectories", -1 },
#ifdef ENABLE_IPV6
    { "inet4-only", '4', OPT_BOOLEAN, "inet4only", -1 },
    { "inet6-only", '6', OPT_BOOLEAN, "inet6only", -1 },
#endif
    { "input-file", 'i', OPT_VALUE, "input", -1 },
    { "keep-session-cookies", 0, OPT_BOOLEAN, "keepsessioncookies", -1 },
    { "level", 'l', OPT_VALUE, "reclevel", -1 },
    { "limit-rate", 0, OPT_VALUE, "limitrate", -1 },
    { "load-cookies", 0, OPT_VALUE, "loadcookies", -1 },
    { "mirror", 'm', OPT_BOOLEAN, "mirror", -1 },
    { "no", 'n', OPT__NO, NULL, required_argument },
    { "no-clobber", 0, OPT_BOOLEAN, "noclobber", -1 },
    { "no-parent", 0, OPT_BOOLEAN, "noparent", -1 },
    { "output-document", 'O', OPT_VALUE, "outputdocument", -1 },
    { "output-file", 'o', OPT_VALUE, "logfile", -1 },
    { "page-requisites", 'p', OPT_BOOLEAN, "pagerequisites", -1 },
    { "parent", 0, OPT__PARENT, NULL, optional_argument },
    { "passive-ftp", 0, OPT_BOOLEAN, "passiveftp", -1 },
    { "password", 0, OPT_VALUE, "password", -1 },
    { "post-data", 0, OPT_VALUE, "postdata", -1 },
    { "post-file", 0, OPT_VALUE, "postfile", -1 },
    { "prefer-family", 0, OPT_VALUE, "preferfamily", -1 },
    { "preserve-permissions", 0, OPT_BOOLEAN, "preservepermissions", -1 },
    { IF_SSL ("private-key"), 0, OPT_VALUE, "privatekey", -1 },
    { IF_SSL ("private-key-type"), 0, OPT_VALUE, "privatekeytype", -1 },
    { "progress", 0, OPT_VALUE, "progress", -1 },
    { "protocol-directories", 0, OPT_BOOLEAN, "protocoldirectories", -1 },
    { "proxy", 0, OPT_BOOLEAN, "useproxy", -1 },
    { "proxy__compat", 'Y', OPT_VALUE, "useproxy", -1 }, /* back-compatible */
    { "proxy-passwd", 0, OPT_VALUE, "proxypassword", -1 }, /* deprecated */
    { "proxy-password", 0, OPT_VALUE, "proxypassword", -1 },
    { "proxy-user", 0, OPT_VALUE, "proxyuser", -1 },
    { "quiet", 'q', OPT_BOOLEAN, "quiet", -1 },
    { "quota", 'Q', OPT_VALUE, "quota", -1 },
    { "random-file", 0, OPT_VALUE, "randomfile", -1 },
    { "random-wait", 0, OPT_BOOLEAN, "randomwait", -1 },
    { "read-timeout", 0, OPT_VALUE, "readtimeout", -1 },
    { "recursive", 'r', OPT_BOOLEAN, "recursive", -1 },
    { "referer", 0, OPT_VALUE, "referer", -1 },
    { "reject", 'R', OPT_VALUE, "reject", -1 },
    { "relative", 'L', OPT_BOOLEAN, "relativeonly", -1 },
    { "remove-listing", 0, OPT_BOOLEAN, "removelisting", -1 },
    { "restrict-file-names", 0, OPT_BOOLEAN, "restrictfilenames", -1 },
    { "retr-symlinks", 0, OPT_BOOLEAN, "retrsymlinks", -1 },
    { "retry-connrefused", 0, OPT_BOOLEAN, "retryconnrefused", -1 },
    { "save-cookies", 0, OPT_VALUE, "savecookies", -1 },
    { "save-headers", 0, OPT_BOOLEAN, "saveheaders", -1 },
    { IF_SSL ("secure-protocol"), 0, OPT_VALUE, "secureprotocol", -1 },
    { "server-response", 'S', OPT_BOOLEAN, "serverresponse", -1 },
    { "span-hosts", 'H', OPT_BOOLEAN, "spanhosts", -1 },
    { "spider", 0, OPT_BOOLEAN, "spider", -1 },
    { "strict-comments", 0, OPT_BOOLEAN, "strictcomments", -1 },
    { "timeout", 'T', OPT_VALUE, "timeout", -1 },
    { "timestamping", 'N', OPT_BOOLEAN, "timestamping", -1 },
    { "tries", 't', OPT_VALUE, "tries", -1 },
    { "user", 0, OPT_VALUE, "user", -1 },
    { "user-agent", 'U', OPT_VALUE, "useragent", -1 },
    { "verbose", 'v', OPT_BOOLEAN, "verbose", -1 },
    { "verbose", 0, OPT_BOOLEAN, "verbose", -1 },
    { "version", 'V', OPT_FUNCALL, (void *) print_version, no_argument },
    { "wait", 'w', OPT_VALUE, "wait", -1 },
    { "waitretry", 0, OPT_VALUE, "waitretry", -1 },
  };

#undef IF_DEBUG
#undef IF_SSL

/* Return a string that contains S with "no-" prepended.  The string
   is NUL-terminated and allocated off static storage at Wget
   startup.  */

static char *
no_prefix (const char *s)
{
  static char buffer[1024];
  static char *p = buffer;

  char *cp = p;
  int size = 3 + strlen (s) + 1;  /* "no-STRING\0" */
  if (p + size >= buffer + sizeof (buffer))
    abort ();

  cp[0] = 'n', cp[1] = 'o', cp[2] = '-';
  strcpy (cp + 3, s);
  p += size;
  return cp;
}

/* The arguments that that main passes to getopt_long. */
static struct option long_options[2 * countof (option_data) + 1];
static char short_options[128];

/* Mapping between short option chars and option_data indices. */
static unsigned char optmap[96];

/* Marker for `--no-FOO' values in long_options.  */
#define BOOLEAN_NEG_MARKER 1024

/* Initialize the long_options array used by getopt_long from the data
   in option_data.  */

static void
init_switches (void)
{
  char *p = short_options;
  int i, o = 0;
  for (i = 0; i < countof (option_data); i++)
    {
      struct cmdline_option *opt = &option_data[i];
      struct option *longopt;

      if (!opt->long_name)
	/* The option is disabled. */
	continue;

      longopt = &long_options[o++];
      longopt->name = opt->long_name;
      longopt->val = i;
      if (opt->short_name)
	{
	  *p++ = opt->short_name;
	  optmap[opt->short_name - 32] = longopt - long_options;
	}
      switch (opt->type)
	{
	case OPT_VALUE:
	  longopt->has_arg = required_argument;
          if (opt->short_name)
	    *p++ = ':';
	  break;
	case OPT_BOOLEAN:
	  /* Specify an optional argument for long options, so that
	     --option=off works the same as --no-option, for
	     compatibility with pre-1.10 Wget.  However, don't specify
	     optional arguments short-option booleans because they
	     prevent combining of short options.  */
	  longopt->has_arg = optional_argument;
	  /* For Boolean options, add the "--no-FOO" variant, which is
	     identical to "--foo", except it has opposite meaning and
	     it doesn't allow an argument.  */
	  longopt = &long_options[o++];
	  longopt->name = no_prefix (opt->long_name);
	  longopt->has_arg = no_argument;
	  /* Mask the value so we'll be able to recognize that we're
	     dealing with the false value.  */
	  longopt->val = i | BOOLEAN_NEG_MARKER;
	  break;
	default:
	  assert (opt->argtype != -1);
	  longopt->has_arg = opt->argtype;
	  if (opt->short_name)
	    {
	      if (longopt->has_arg == required_argument)
		*p++ = ':';
	      /* Don't handle optional_argument */
	    }
	}
    }
  /* Terminate short_options. */
  *p = '\0';
  /* No need for xzero(long_options[o]) because its storage is static
     and it will be zeroed by default.  */
  assert (o <= countof (long_options));
}

/* Print the usage message.  */
static void
print_usage (void)
{
  printf (_("Usage: %s [OPTION]... [URL]...\n"), exec_name);
}

/* Print the help message, describing all the available options.  If
   you add an option, be sure to update this list.  */
static void
print_help (void)
{
  /* We split the help text this way to ease translation of individual
     entries.  */
  static const char *help[] = {
    "\n",
    N_("\
Mandatory arguments to long options are mandatory for short options too.\n\n"),
    N_("\
Startup:\n"),
    N_("\
  -V,  --version           display the version of Wget and exit.\n"),
    N_("\
  -h,  --help              print this help.\n"),
    N_("\
  -b,  --background        go to background after startup.\n"),
    N_("\
  -e,  --execute=COMMAND   execute a `.wgetrc'-style command.\n"),
    "\n",

    N_("\
Logging and input file:\n"),
    N_("\
  -o,  --output-file=FILE    log messages to FILE.\n"),
    N_("\
  -a,  --append-output=FILE  append messages to FILE.\n"),
#ifdef ENABLE_DEBUG
    N_("\
  -d,  --debug               print lots of debugging information.\n"),
#endif
    N_("\
  -q,  --quiet               quiet (no output).\n"),
    N_("\
  -v,  --verbose             be verbose (this is the default).\n"),
    N_("\
  -nv, --no-verbose          turn off verboseness, without being quiet.\n"),
    N_("\
  -i,  --input-file=FILE     download URLs found in FILE.\n"),
    N_("\
  -F,  --force-html          treat input file as HTML.\n"),
    N_("\
  -B,  --base=URL            prepends URL to relative links in -F -i file.\n"),
    "\n",

    N_("\
Download:\n"),
    N_("\
  -t,  --tries=NUMBER            set number of retries to NUMBER (0 unlimits).\n"),
    N_("\
       --retry-connrefused       retry even if connection is refused.\n"),
    N_("\
  -O,  --output-document=FILE    write documents to FILE.\n"),
    N_("\
  -nc, --no-clobber              skip downloads that would download to\n\
                                 existing files.\n"),
    N_("\
  -c,  --continue                resume getting a partially-downloaded file.\n"),
    N_("\
       --progress=TYPE           select progress gauge type.\n"),
    N_("\
  -N,  --timestamping            don't re-retrieve files unless newer than\n\
                                 local.\n"),
    N_("\
  -S,  --server-response         print server response.\n"),
    N_("\
       --spider                  don't download anything.\n"),
    N_("\
  -T,  --timeout=SECONDS         set all timeout values to SECONDS.\n"),
    N_("\
       --dns-timeout=SECS        set the DNS lookup timeout to SECS.\n"),
    N_("\
       --connect-timeout=SECS    set the connect timeout to SECS.\n"),
    N_("\
       --read-timeout=SECS       set the read timeout to SECS.\n"),
    N_("\
  -w,  --wait=SECONDS            wait SECONDS between retrievals.\n"),
    N_("\
       --waitretry=SECONDS       wait 1..SECONDS between retries of a retrieval.\n"),
    N_("\
       --random-wait             wait from 0...2*WAIT secs between retrievals.\n"),
    N_("\
  -Y,  --proxy                   explicitly turn on proxy.\n"),
    N_("\
       --no-proxy                explicitly turn off proxy.\n"),
    N_("\
  -Q,  --quota=NUMBER            set retrieval quota to NUMBER.\n"),
    N_("\
       --bind-address=ADDRESS    bind to ADDRESS (hostname or IP) on local host.\n"),
    N_("\
       --limit-rate=RATE         limit download rate to RATE.\n"),
    N_("\
       --no-dns-cache            disable caching DNS lookups.\n"),
    N_("\
       --restrict-file-names=OS  restrict chars in file names to ones OS allows.\n"),
#ifdef ENABLE_IPV6
    N_("\
  -4,  --inet4-only              connect only to IPv4 addresses.\n"),
    N_("\
  -6,  --inet6-only              connect only to IPv6 addresses.\n"),
    N_("\
       --prefer-family=FAMILY    connect first to addresses of specified family,\n\
                                 one of IPv6, IPv4, or none.\n"),
#endif
    N_("\
       --user=USER               set both ftp and http user to USER.\n"),
    N_("\
       --password=PASS           set both ftp and http password to PASS.\n"),
    "\n",

    N_("\
Directories:\n"),
    N_("\
  -nd, --no-directories           don't create directories.\n"),
    N_("\
  -x,  --force-directories        force creation of directories.\n"),
    N_("\
  -nH, --no-host-directories      don't create host directories.\n"),
    N_("\
       --protocol-directories     use protocol name in directories.\n"),
    N_("\
  -P,  --directory-prefix=PREFIX  save files to PREFIX/...\n"),
    N_("\
       --cut-dirs=NUMBER          ignore NUMBER remote directory components.\n"),
    "\n",

    N_("\
HTTP options:\n"),
    N_("\
       --http-user=USER        set http user to USER.\n"),
    N_("\
       --http-password=PASS    set http password to PASS.\n"),
    N_("\
       --no-cache              disallow server-cached data.\n"),
    N_("\
  -E,  --html-extension        save HTML documents with `.html' extension.\n"),
    N_("\
       --ignore-length         ignore `Content-Length' header field.\n"),
    N_("\
       --header=STRING         insert STRING among the headers.\n"),
    N_("\
       --proxy-user=USER       set USER as proxy username.\n"),
    N_("\
       --proxy-password=PASS   set PASS as proxy password.\n"),
    N_("\
       --referer=URL           include `Referer: URL' header in HTTP request.\n"),
    N_("\
       --save-headers          save the HTTP headers to file.\n"),
    N_("\
  -U,  --user-agent=AGENT      identify as AGENT instead of Wget/VERSION.\n"),
    N_("\
       --no-http-keep-alive    disable HTTP keep-alive (persistent connections).\n"),
    N_("\
       --no-cookies            don't use cookies.\n"),
    N_("\
       --load-cookies=FILE     load cookies from FILE before session.\n"),
    N_("\
       --save-cookies=FILE     save cookies to FILE after session.\n"),
    N_("\
       --keep-session-cookies  load and save session (non-permanent) cookies.\n"),
    N_("\
       --post-data=STRING      use the POST method; send STRING as the data.\n"),
    N_("\
       --post-file=FILE        use the POST method; send contents of FILE.\n"),
    "\n",

#ifdef HAVE_SSL
    N_("\
HTTPS (SSL/TLS) options:\n"),
    N_("\
       --secure-protocol=PR     choose secure protocol, one of auto, SSLv2,\n\
                                SSLv3, and TLSv1.\n"),
    N_("\
       --no-check-certificate   don't validate the server's certificate.\n"),
    N_("\
       --certificate=FILE       client certificate file.\n"),
    N_("\
       --certificate-type=TYPE  client certificate type, PEM or DER.\n"),
    N_("\
       --private-key=FILE       private key file.\n"),
    N_("\
       --private-key-type=TYPE  private key type, PEM or DER.\n"),
    N_("\
       --ca-certificate=FILE    file with the bundle of CA's.\n"),
    N_("\
       --ca-directory=DIR       directory where hash list of CA's is stored.\n"),
    N_("\
       --random-file=FILE       file with random data for seeding the SSL PRNG.\n"),
    N_("\
       --egd-file=FILE          file naming the EGD socket with random data.\n"),
    "\n",
#endif /* HAVE_SSL */

    N_("\
FTP options:\n"),
    N_("\
       --ftp-user=USER         set ftp user to USER.\n"),
    N_("\
       --ftp-password=PASS     set ftp password to PASS.\n"),
    N_("\
       --no-remove-listing     don't remove `.listing' files.\n"),
    N_("\
       --no-glob               turn off FTP file name globbing.\n"),
    N_("\
       --no-passive-ftp        disable the \"passive\" transfer mode.\n"),
    N_("\
       --retr-symlinks         when recursing, get linked-to files (not dir).\n"),
    N_("\
       --preserve-permissions  preserve remote file permissions.\n"),
    "\n",

    N_("\
Recursive download:\n"),
    N_("\
  -r,  --recursive          specify recursive download.\n"),
    N_("\
  -l,  --level=NUMBER       maximum recursion depth (inf or 0 for infinite).\n"),
    N_("\
       --delete-after       delete files locally after downloading them.\n"),
    N_("\
  -k,  --convert-links      make links in downloaded HTML point to local files.\n"),
    N_("\
  -K,  --backup-converted   before converting file X, back up as X.orig.\n"),
    N_("\
  -m,  --mirror             shortcut for -N -r -l inf --no-remove-listing.\n"),
    N_("\
  -p,  --page-requisites    get all images, etc. needed to display HTML page.\n"),
    N_("\
       --strict-comments    turn on strict (SGML) handling of HTML comments.\n"),
    "\n",

    N_("\
Recursive accept/reject:\n"),
    N_("\
  -A,  --accept=LIST               comma-separated list of accepted extensions.\n"),
    N_("\
  -R,  --reject=LIST               comma-separated list of rejected extensions.\n"),
    N_("\
  -D,  --domains=LIST              comma-separated list of accepted domains.\n"),
    N_("\
       --exclude-domains=LIST      comma-separated list of rejected domains.\n"),
    N_("\
       --follow-ftp                follow FTP links from HTML documents.\n"),
    N_("\
       --follow-tags=LIST          comma-separated list of followed HTML tags.\n"),
    N_("\
       --ignore-tags=LIST          comma-separated list of ignored HTML tags.\n"),
    N_("\
  -H,  --span-hosts                go to foreign hosts when recursive.\n"),
    N_("\
  -L,  --relative                  follow relative links only.\n"),
    N_("\
  -I,  --include-directories=LIST  list of allowed directories.\n"),
    N_("\
  -X,  --exclude-directories=LIST  list of excluded directories.\n"),
    N_("\
  -np, --no-parent                 don't ascend to the parent directory.\n"),
    "\n",

    N_("Mail bug reports and suggestions to <bug-wget@gnu.org>.\n")
  };

  int i;

  printf (_("GNU Wget %s, a non-interactive network retriever.\n"),
	  version_string);
  print_usage ();

  for (i = 0; i < countof (help); i++)
    fputs (_(help[i]), stdout);

  exit (0);
}

static void
print_version (void)
{
  printf ("GNU Wget %s\n\n", version_string);
  fputs (_("\
Copyright (C) 2005 Free Software Foundation, Inc.\n"), stdout);
  fputs (_("\
This program is distributed in the hope that it will be useful,\n\
but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
GNU General Public License for more details.\n"), stdout);
  fputs (_("\nOriginally written by Hrvoje Niksic <hniksic@xemacs.org>.\n"),
	 stdout);
  exit (0);
}

int
main (int argc, char *const *argv)
{
  char **url, **t;
  int i, ret, longindex;
  int nurl, status;
  int append_to_log = 0;

  i18n_initialize ();

  /* Construct the name of the executable, without the directory part.  */
  exec_name = strrchr (argv[0], PATH_SEPARATOR);
  if (!exec_name)
    exec_name = argv[0];
  else
    ++exec_name;

#ifdef WINDOWS
  /* Drop extension (typically .EXE) from executable filename. */
  windows_main (&argc, (char **) argv, (char **) &exec_name);
#endif

  /* Set option defaults; read the system wgetrc and ~/.wgetrc.  */
  initialize ();

  init_switches ();
  longindex = -1;
  while ((ret = getopt_long (argc, argv,
			     short_options, long_options, &longindex)) != -1)
    {
      int val;
      struct cmdline_option *opt;

      /* If LONGINDEX is unchanged, it means RET is referring a short
	 option.  */
      if (longindex == -1)
	{
	  if (ret == '?')
	    {
	      print_usage ();
	      printf ("\n");
	      printf (_("Try `%s --help' for more options.\n"), exec_name);
	      exit (2);
	    }
	  /* Find the short option character in the mapping.  */
	  longindex = optmap[ret - 32];
	}
      val = long_options[longindex].val;

      /* Use the retrieved value to locate the option in the
	 option_data array, and to see if we're dealing with the
	 negated "--no-FOO" variant of the boolean option "--foo".  */
      opt = &option_data[val & ~BOOLEAN_NEG_MARKER];
      switch (opt->type)
	{
	case OPT_VALUE:
	  setoptval (opt->data, optarg, opt->long_name);
	  break;
	case OPT_BOOLEAN:
	  if (optarg)
	    /* The user has specified a value -- use it. */
	    setoptval (opt->data, optarg, opt->long_name);
	  else
	    {
	      /* NEG is true for `--no-FOO' style boolean options. */
	      int neg = val & BOOLEAN_NEG_MARKER;
	      setoptval (opt->data, neg ? "0" : "1", opt->long_name);
	    }
	  break;
	case OPT_FUNCALL:
	  {
	    void (*func) PARAMS ((void)) = (void (*) PARAMS ((void))) opt->data;
	    func ();
	  }
	  break;
	case OPT__APPEND_OUTPUT:
	  setoptval ("logfile", optarg, opt->long_name);
	  append_to_log = 1;
	  break;
	case OPT__EXECUTE:
	  run_command (optarg);
	  break;
	case OPT__NO:
	  {
	    /* We support real --no-FOO flags now, but keep these
	       short options for convenience and backward
	       compatibility.  */
	    char *p;
	    for (p = optarg; *p; p++)
	      switch (*p)
		{
		case 'v':
		  setoptval ("verbose", "0", opt->long_name);
		  break;
		case 'H':
		  setoptval ("addhostdir", "0", opt->long_name);
		  break;
		case 'd':
		  setoptval ("dirstruct", "0", opt->long_name);
		  break;
		case 'c':
		  setoptval ("noclobber", "1", opt->long_name);
		  break;
		case 'p':
		  setoptval ("noparent", "1", opt->long_name);
		  break;
		default:
		  printf (_("%s: illegal option -- `-n%c'\n"), exec_name, *p);
		  print_usage ();
		  printf ("\n");
		  printf (_("Try `%s --help' for more options.\n"), exec_name);
		  exit (1);
		}
	    break;
	  }
	case OPT__PARENT:
	case OPT__CLOBBER:
	  {
	    /* The wgetrc commands are named noparent and noclobber,
	       so we must revert the meaning of the cmdline options
	       before passing the value to setoptval.  */
	    int flag = 1;
	    if (optarg)
	      flag = (*optarg == '1' || TOLOWER (*optarg) == 'y'
		      || (TOLOWER (optarg[0]) == 'o'
			  && TOLOWER (optarg[1]) == 'n'));
	    setoptval (opt->type == OPT__PARENT ? "noparent" : "noclobber",
		       flag ? "0" : "1", opt->long_name);
	    break;
	  }
	case OPT__DONT_REMOVE_LISTING:
	  setoptval ("removelisting", "0", opt->long_name);
	  break;
	}

      longindex = -1;
    }

  /* All user options have now been processed, so it's now safe to do
     interoption dependency checks. */

  if (opt.reclevel == 0)
    opt.reclevel = INFINITE_RECURSION; /* see recur.h for commentary on this */

  if (opt.page_requisites && !opt.recursive)
    {
      /* Don't set opt.recursive here because it would confuse the FTP
	 code.  Instead, call retrieve_tree below when either
	 page_requisites or recursive is requested.  */
      opt.reclevel = 0;
      if (!opt.no_dirstruct)
	opt.dirstruct = 1;	/* normally handled by cmd_spec_recursive() */
    }

  if (opt.verbose == -1)
    opt.verbose = !opt.quiet;

  /* Sanity checks.  */
  if (opt.verbose && opt.quiet)
    {
      printf (_("Can't be verbose and quiet at the same time.\n"));
      print_usage ();
      exit (1);
    }
  if (opt.timestamping && opt.noclobber)
    {
      printf (_("\
Can't timestamp and not clobber old files at the same time.\n"));
      print_usage ();
      exit (1);
    }
#ifdef ENABLE_IPV6
  if (opt.ipv4_only && opt.ipv6_only)
    {
      printf (_("Cannot specify both --inet4-only and --inet6-only.\n"));
      print_usage ();
      exit (1);
    }
#endif

  nurl = argc - optind;
  if (!nurl && !opt.input_filename)
    {
      /* No URL specified.  */
      printf (_("%s: missing URL\n"), exec_name);
      print_usage ();
      printf ("\n");
      /* #### Something nicer should be printed here -- similar to the
	 pre-1.5 `--help' page.  */
      printf (_("Try `%s --help' for more options.\n"), exec_name);
      exit (1);
    }

  if (opt.background)
    fork_to_background ();

  /* Initialize progress.  Have to do this after the options are
     processed so we know where the log file is.  */
  if (opt.verbose)
    set_progress_implementation (opt.progress_type);

  /* Fill in the arguments.  */
  url = alloca_array (char *, nurl + 1);
  for (i = 0; i < nurl; i++, optind++)
    {
      char *rewritten = rewrite_shorthand_url (argv[optind]);
      if (rewritten)
	url[i] = rewritten;
      else
	url[i] = xstrdup (argv[optind]);
    }
  url[i] = NULL;

  /* Initialize logging.  */
  log_init (opt.lfilename, append_to_log);

  DEBUGP (("DEBUG output created by Wget %s on %s.\n\n", version_string,
	   OS_TYPE));

  /* Open the output filename if necessary.  */
  if (opt.output_document)
    {
      extern FILE *output_stream;
      extern int output_stream_regular;

      if (HYPHENP (opt.output_document))
	output_stream = stdout;
      else
	{
	  struct_fstat st;
	  output_stream = fopen (opt.output_document,
				 opt.always_rest ? "ab" : "wb");
	  if (output_stream == NULL)
	    {
	      perror (opt.output_document);
	      exit (1);
	    }
	  if (fstat (fileno (output_stream), &st) == 0 && S_ISREG (st.st_mode))
	    output_stream_regular = 1;
	}
    }

#ifdef WINDOWS
  ws_startup ();
#endif

  /* Setup the signal handler to redirect output when hangup is
     received.  */
#ifdef HAVE_SIGNAL
  if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
    signal(SIGHUP, redirect_output_signal);
  /* ...and do the same for SIGUSR1.  */
  signal (SIGUSR1, redirect_output_signal);
  /* Writing to a closed socket normally signals SIGPIPE, and the
     process exits.  What we want is to ignore SIGPIPE and just check
     for the return value of write().  */
  signal (SIGPIPE, SIG_IGN);
#ifdef SIGWINCH
  signal (SIGWINCH, progress_handle_sigwinch);
#endif
#endif /* HAVE_SIGNAL */

  status = RETROK;		/* initialize it, just-in-case */
  /* Retrieve the URLs from argument list.  */
  for (t = url; *t; t++)
    {
      char *filename = NULL, *redirected_URL = NULL;
      int dt;

      if ((opt.recursive || opt.page_requisites)
	  && url_scheme (*t) != SCHEME_FTP)
	status = retrieve_tree (*t);
      else
	status = retrieve_url (*t, &filename, &redirected_URL, NULL, &dt);

      if (opt.delete_after && file_exists_p(filename))
	{
	  DEBUGP (("Removing file due to --delete-after in main():\n"));
	  logprintf (LOG_VERBOSE, _("Removing %s.\n"), filename);
	  if (unlink (filename))
	    logprintf (LOG_NOTQUIET, "unlink: %s\n", strerror (errno));
	}

      xfree_null (redirected_URL);
      xfree_null (filename);
    }

  /* And then from the input file, if any.  */
  if (opt.input_filename)
    {
      int count;
      status = retrieve_from_file (opt.input_filename, opt.force_html, &count);
      if (!count)
	logprintf (LOG_NOTQUIET, _("No URLs found in %s.\n"),
		   opt.input_filename);
    }
  /* Print the downloaded sum.  */
  if (opt.recursive || opt.page_requisites
      || nurl > 1
      || (opt.input_filename && total_downloaded_bytes != 0))
    {
      logprintf (LOG_NOTQUIET,
		 _("\nFINISHED --%s--\nDownloaded: %s bytes in %d files\n"),
		 time_str (NULL),
		 with_thousand_seps_sum (total_downloaded_bytes),
		 opt.numurls);
      /* Print quota warning, if exceeded.  */
      if (opt.quota && total_downloaded_bytes > opt.quota)
	logprintf (LOG_NOTQUIET,
		   _("Download quota (%s bytes) EXCEEDED!\n"),
		   with_thousand_seps_sum (opt.quota));
    }

  if (opt.cookies_output)
    save_cookies ();

  if (opt.convert_links && !opt.delete_after)
    convert_all_links ();

  log_close ();
  for (i = 0; i < nurl; i++)
    xfree (url[i]);
  cleanup ();

#ifdef DEBUG_MALLOC
  print_malloc_debug_stats ();
#endif
  if (status == RETROK)
    return 0;
  else
    return 1;
}

#ifdef HAVE_SIGNAL
/* Hangup signal handler.  When wget receives SIGHUP or SIGUSR1, it
   will proceed operation as usual, trying to write into a log file.
   If that is impossible, the output will be turned off.

   #### It is unsafe to do call libc functions from a signal handler.
   What we should do is, set a global variable, and have the code in
   log.c pick it up.  */

static RETSIGTYPE
redirect_output_signal (int sig)
{
  const char *signal_name = (sig == SIGHUP ? "SIGHUP" :
			     (sig == SIGUSR1 ? "SIGUSR1" :
			      "WTF?!"));
  log_request_redirect_output (signal_name);
  progress_schedule_redirect ();
  signal (sig, redirect_output_signal);
}
#endif /* HAVE_SIGNAL */
