/* config.h - configuration defines for thttpd and libhttpd
**
** Copyright © 1995,1998,1999,2000,2001 by Jef Poskanzer <jef@mail.acme.com>.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

#ifndef _CONFIG_H_
#define _CONFIG_H_


/* The following configuration settings are sorted in order of decreasing
** likelihood that you'd want to change them - most likely first, least
** likely last.
**
** In case you're not familiar with the convention, "#ifdef notdef"
** is a Berkeleyism used to indicate temporarily disabled code.
** The idea here is that you re-enable it by just moving it outside
** of the ifdef.
*/

/* CONFIGURE: CGI programs must match this pattern to get executed.  It's
** a simple shell-style wildcard pattern, with * meaning any string not
** containing a slash, ** meaning any string at all, and ? meaning any
** single character; or multiple such patterns separated by |.  The
** patterns get checked against the filename part of the incoming URL.
**
** Restricting CGI programs to a single directory lets the site administrator
** review them for security holes, and is strongly recommended.  If there
** are individual users that you trust, you can enable their directories too.
**
** You can also specify a CGI pattern on the command line, with the -c flag.
** Such a pattern overrides this compiled-in default.
**
** If no CGI pattern is specified, neither here nor on the command line,
** then CGI programs cannot be run at all.  If you want to disable CGI
** as a security measure that's how you do it, just don't define any
** pattern here and don't run with the -c flag.
*/
#ifdef notdef
/* Some sample patterns.  Allow programs only in one central directory: */
#define CGI_PATTERN "/cgi-bin/*"
/* Allow programs in a central directory, or anywhere in a trusted
** user's tree: */
#define CGI_PATTERN "/cgi-bin/*|/jef/**"
/* Allow any program ending with a .cgi: */
#define CGI_PATTERN "**.cgi"
/* When virtual hosting, enable the central directory on every host: */
#define CGI_PATTERN "/*/cgi-bin/*"
#endif

/* CONFIGURE: How many seconds to allow CGI programs to run before killing
** them.  This is in case someone writes a CGI program that goes into an
** infinite loop, or does a massive database lookup that would take hours,
** or whatever.  If you don't want any limit, comment this out, but that's
** probably a really bad idea.
*/
//#define CGI_TIMELIMIT 30

/* CONFIGURE: Maximum number of simultaneous CGI programs allowed.
** If this many are already running, then attempts to run more will
** return an HTTP 503 error.  If this is not defined then there's
** no limit (and you'd better have a lot of memory).  This can also be
** set in the runtime config file.
*/
#ifdef notdef
#define CGI_LIMIT 50
#endif

/* CONFIGURE: How many seconds to allow for reading the initial request
** on a new connection.
*/
#define IDLE_READ_TIMELIMIT 60

/* CONFIGURE: How many seconds before an idle connection gets closed.
*/
#define IDLE_SEND_TIMELIMIT 300

/* CONFIGURE: The syslog facility to use.  Using this you can set up your
** syslog.conf so that all thttpd messages go into a separate file.  Note
** that even if you use the -l command line flag to send logging to a
** file, errors still get sent via syslog.
*/
#define LOG_FACILITY LOG_DAEMON

/* CONFIGURE: Tilde mapping.  Many URLs use ~username to indicate a
** user's home directory.  thttpd provides two options for mapping
** this construct to an actual filename.
**
** 1) Map ~username to <prefix>/username.  This is the recommended choice.
** Each user gets a subdirectory in the main chrootable web tree, and
** the tilde construct points there.  The prefix could be something
** like "users", or it could be empty.  See also the makeweb program
** for letting users create their own web subdirectories.
**
** 2) Map ~username to <user's homedir>/<postfix>.  The postfix would be
** the name of a subdirectory off of the user's actual home dir, something
** like "public_html".  This is what Apache and other servers do.  The problem
** is, you can't do this and chroot() at the same time, so it's inherently
** a security hole.  This is strongly dis-recommended, but it's here because
** some people really want it.  Use at your own risk.
**
** You can also leave both options undefined, and thttpd will not do
** anything special about tildes.  Enabling both options is an error.
*/
//#ifdef notdef
//#define TILDE_MAP_1 "users"
//#define TILDE_MAP_2 "public_html"
//#endif

/* CONFIGURE: The file to use for authentication.  If this is defined then
** thttpd checks for this file in the local directory before every fetch.
** If the file exists then authentication is done, otherwise the fetch
** proceeds as usual.
**
** If you undefine this then thttpd will not implement authentication
** at all and will not check for auth files, which saves a bit of CPU time.
*/
//#define AUTH_FILE ".htpasswd"

/* CONFIGURE: The default character set name to use with text MIME types.
** This gets substituted into the MIME types where they have a "%s".
**
** You can override this in the config file with the "charset" setting,
** or on the command like with the -T flag.
*/
#define DEFAULT_CHARSET "iso-8859-1"


/* Most people won't want to change anything below here. */

/* CONFIGURE: This controls the SERVER_NAME environment variable that gets
** passed to CGI programs.  By default thttpd does a gethostname(), which
** gives the host's canonical name.  If you want to always use some other name
** you can define it here.
**
** Alternately, if you want to run the same thttpd binary on multiple
** machines, and want to build in alternate names for some or all of
** them, you can define a list of canonical name to altername name
** mappings.  thttpd seatches the list and when it finds a match on
** the canonical name, that alternate name gets used.  If no match
** is found, the canonical name gets used.
**
** If both SERVER_NAME and SERVER_NAME_LIST are defined here, thttpd searches
** the list as above, and if no match is found then SERVER_NAME gets used.
**
** In any case, if thttpd is started with the -h flag, that name always
** gets used.
*/
#ifdef notdef
#define SERVER_NAME "your.hostname.here"
#define SERVER_NAME_LIST \
    "canonical.name.here/alternate.name.here", \
    "canonical.name.two/alternate.name.two"
#endif

/* CONFIGURE: Undefine this if you want thttpd to hide its specific version
** when returning into to browsers.  Instead it'll just say "thttpd" with
** no version.
*/
//#define SHOW_SERVER_VERSION

/* CONFIGURE: Define this if you want to always chroot(), without having
** to give the -r command line flag.  Some people like this as a security
** measure, to prevent inadvertant exposure by accidentally running without -r.
** You can still disable it at runtime with the -nor flag.
*/
#ifdef notdef
#define ALWAYS_CHROOT
#endif

/* CONFIGURE: Define this if you want to always do virtual hosting, without
** having to give the -v command line flag.  You can still disable it at
** runtime with the -nov flag.
*/
#ifdef notdef
#define ALWAYS_VHOST
#endif

/* CONFIGURE: If you're using the vhost feature and you have a LOT of
** virtual hostnames (like, hundreds or thousands), you will want to
** enable this feature.  It avoids a problem with most Unix filesystems,
** where if there are a whole lot of items in a directory then name lookup
** becomes very slow.  This feature makes thttpd use subdirectories
** based on the first characters of each hostname.  You can set it to use
** from one to three characters.  If the hostname starts with "www.", that
** part is skipped over.  Dots are also skipped over, and if the name isn't
** long enough then "_"s are used.  Here are some examples of how hostnames
** would get turned into directory paths, for each different setting:
** 1: www.acme.com ->    a/www.acme.com
** 1: foobar.acme.com -> f/foobar.acme.com
** 2: www.acme.com ->    a/c/www.acme.com
** 2: foobar.acme.com -> f/o/foobar.acme.com
** 3: www.acme.com ->    a/c/m/www.acme.com
** 3: foobar.acme.com -> f/o/o/foobar.acme.com
** 3: m.tv ->            m/t/v/m.tv
** 4: m.tv ->            m/t/v/_/m.tv
** Note that if you compile this setting in but then forget to set up
** the corresponding subdirectories, the only error indication you'll
** get is a "404 Not Found" when you try to visit a site.  So be careful.
*/
#ifdef notdef
#define VHOST_DIRLEVELS 1
#define VHOST_DIRLEVELS 2
#define VHOST_DIRLEVELS 3
#endif

/* CONFIGURE: Define this if you want to always use a global passwd file,
** without having to give the -P command line flag.  You can still disable
** it at runtime with the -noP flag.
*/
#ifdef notdef
#define ALWAYS_GLOBAL_PASSWD
#endif

/* CONFIGURE: When started as root, the default username to switch to after
** initializing.  If this user (or the one specified by the -u flag) does
** not exist, the program will refuse to run.
*/
#define DEFAULT_USER "nobody"

/* CONFIGURE: When started as root, the program can automatically chdir()
** to the home directory of the user specified by -u or DEFAULT_USER.
** An explicit -d still overrides this.
*/
#ifdef notdef
#define USE_USER_DIR
#endif

/* CONFIGURE: If this is defined, some of the built-in error pages will
** have more explicit information about exactly what the problem is.
** Some sysadmins don't like this, for security reasons.
*/
#define EXPLICIT_ERROR_PAGES

/* CONFIGURE: Subdirectory for custom error pages.  The error filenames are
** $WEBDIR/$ERR_DIR/err%d.html - if virtual hosting is enabled then
** $WEBDIR/hostname/$ERR_DIR/err%d.html is searched first.  This allows
** different custom error pages for each virtual hosting web server.  If
** no custom page for a given error can be found, the built-in error page
** is generated.  If ERR_DIR is not defined at all, only the built-in error
** pages will be generated.
*/
#define ERR_DIR "errors"

/* CONFIGURE: Define this if you want a standard HTML tail containing
** $SERVER_SOFTWARE and $SERVER_ADDRESS to be appended to the custom error
** pages.  (It is always appended to the built-in error pages.)
*/
#define ERR_APPEND_SERVER_INFO

/* CONFIGURE: nice(2) value to use for CGI programs.  If this is undefined,
** CGI programs run at normal priority.
*/
//#define CGI_NICE 10

/* CONFIGURE: $PATH to use for CGI programs.
*/
#define CGI_PATH "/usr/local/bin:/usr/ucb:/bin:/usr/bin"

/* CONFIGURE: If defined, $LD_LIBRARY_PATH to use for CGI programs.
*/
#ifdef notdef
#define CGI_LD_LIBRARY_PATH "/usr/local/lib:/usr/lib"
#endif

/* CONFIGURE: How often to run the occasional cleanup job.
*/
#define OCCASIONAL_TIME 120

/* CONFIGURE: Seconds between stats syslogs.  If this is undefined then
** no stats are accumulated and no stats syslogs are done.
*/
#define STATS_TIME 3600

/* CONFIGURE: The mmap cache tries to keep the total number of mapped
** files below this number, so you don't run out of kernel file descriptors.
** If you have reconfigured your kernel to have more descriptors, you can
** raise this and thttpd will keep more maps cached.  However it's not
** a hard limit, thttpd will go over it if you really are accessing
** a whole lot of files.
*/
#define DESIRED_MAX_MAPPED_FILES 1000

/* CONFIGURE: The mmap cache also tries to keep the total mapped bytes
** below this number, so you don't run out of address space.  Again
** it's not a hard limit, thttpd will go over it if you really are
** accessing a bunch of large files.
*/
#define DESIRED_MAX_MAPPED_BYTES 1000000000

/* You almost certainly don't want to change anything below here. */

/* CONFIGURE: When throttling CGI programs, we don't know how many bytes
** they send back to the client because it would be inefficient to
** interpose a counter.  CGI programs are much more expensive than
** regular files to serve, so we set an arbitrary and high byte count
** that gets applied to all CGI programs for throttling purposes.
*/
#define CGI_BYTECOUNT 25000

/* CONFIGURE: The default port to listen on.  80 is the standard HTTP port.
*/
#define DEFAULT_PORT 80

/* CONFIGURE: A list of index filenames to check.  The files are searched
** for in this order.
*/
//#define INDEX_NAMES "index.html", "index.htm", "index.xhtml", "index.xht", "Default.htm", "index.cgi"

/* CONFIGURE: If this is defined then thttpd will automatically generate
** index pages for directories that don't have an explicit index file.
** If you want to disable this behavior site-wide, perhaps for security
** reasons, just undefine this.  Note that you can disable indexing of
** individual directories by merely doing a "chmod 711" on them - the
** standard Unix file permission to allow file access but disable "ls".
*/
#define GENERATE_INDEXES

/* CONFIGURE: Whether to log unknown request headers.  Most sites will not
** want to log them, which will save them a bit of CPU time.
*/
//#ifdef notdef
//#define LOG_UNKNOWN_HEADERS
//#endif

/* CONFIGURE: Whether to fflush() the log file after each request.  If
** this is turned off there's a slight savings in CPU cycles.
*/
#define FLUSH_LOG_EVERY_TIME

/* CONFIGURE: Time between updates of the throttle table's rolling averages. */
#define THROTTLE_TIME 2

/* CONFIGURE: The listen() backlog queue length.  The 1024 doesn't actually
** get used, the kernel uses its maximum allowed value.  This is a config
** parameter only in case there's some OS where asking for too high a queue
** length causes an error.  Note that on many systems the maximum length is
** way too small - see http://www.acme.com/software/thttpd/notes.html
*/
#define LISTEN_BACKLOG 1024

/* CONFIGURE: Maximum number of throttle patterns that any single URL can
** be included in.  This has nothing to do with the number of throttle
** patterns that you can define, which is unlimited.
*/
#define MAXTHROTTLENUMS 10

/* CONFIGURE: Number of file descriptors to reserve for uses other than
** connections.  Currently this is 10, representing one for the listen fd,
** one for dup()ing at connection startup time, one for reading the file,
** one for syslog, and possibly one for the regular log file, which is
** five, plus a factor of two for who knows what.
*/
#define SPARE_FDS 10

/* CONFIGURE: How many milliseconds to leave a connection open while doing a
** lingering close.
*/
#define LINGER_TIME 500

/* CONFIGURE: Maximum number of symbolic links to follow before
** assuming there's a loop.
*/
#define MAX_LINKS 32

/* CONFIGURE: You don't even want to know.
*/
#define MIN_WOULDBLOCK_DELAY 100L

#endif /* _CONFIG_H_ */
