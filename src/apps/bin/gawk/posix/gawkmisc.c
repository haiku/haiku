/* gawkmisc.c --- miscellaneous gawk routines that are OS specific.
 
   Copyright (C) 1986, 1988, 1989, 1991 - 1998, 2001 - 2003 the Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

char quote = '\'';
char *defpath = DEFPATH;
char envsep = ':';

#ifndef INVALID_HANDLE
/* FIXME: is this value for INVALID_HANDLE correct? */
#define INVALID_HANDLE -1
#endif

/* gawk_name --- pull out the "gawk" part from how the OS called us */

char *
gawk_name(filespec)
const char *filespec;
{
	char *p;
    
	/* "path/name" -> "name" */
	p = strrchr(filespec, '/');
	return (p == NULL ? (char *) filespec : p + 1);
}

/* os_arg_fixup --- fixup the command line */

void
os_arg_fixup(argcp, argvp)
int *argcp;
char ***argvp;
{
	/* no-op */
	return;
}

/* os_devopen --- open special per-OS devices */

int
os_devopen(name, flag)
const char *name;
int flag;
{
	/* no-op */
	return INVALID_HANDLE;
}

/* optimal_bufsize --- determine optimal buffer size */

/*
 * Enhance this for debugging purposes, as follows:
 *
 * Always stat the file, stat buffer is used by higher-level code.
 *
 * if (AWKBUFSIZE == "exact")
 * 	return the file size
 * else if (AWKBUFSIZE == a number)
 * 	always return that number
 * else
 * 	if the size is < default_blocksize
 * 		return the size
 *	else
 *		return default_blocksize
 *	end if
 * endif
 *
 * Hair comes in an effort to only deal with AWKBUFSIZE
 * once, the first time this routine is called, instead of
 * every time.  Performance, dontyaknow.
 */

size_t
optimal_bufsize(fd, stb)
int fd;
struct stat *stb;
{
	char *val;
	static size_t env_val = 0;
	static short first = TRUE;
	static short exact = FALSE;

	/* force all members to zero in case OS doesn't use all of them. */
	memset(stb, '\0', sizeof(struct stat));

	/* always stat, in case stb is used by higher level code. */
	if (fstat(fd, stb) == -1)
		fatal("can't stat fd %d (%s)", fd, strerror(errno));

	if (first) {
		first = FALSE;

		if ((val = getenv("AWKBUFSIZE")) != NULL) {
			if (strcmp(val, "exact") == 0)
				exact = TRUE;
			else if (ISDIGIT(*val)) {
				for (; *val && ISDIGIT(*val); val++)
					env_val = (env_val * 10) + *val - '0';

				return env_val;
			}
		}
	} else if (! exact && env_val > 0)
		return env_val;
	/* else
	  	fall through */

	/*
	 * System V.n, n < 4, doesn't have the file system block size in the
	 * stat structure. So we have to make some sort of reasonable
	 * guess. We use stdio's BUFSIZ, since that is what it was
	 * meant for in the first place.
	 */
#ifdef HAVE_ST_BLKSIZE
#define DEFBLKSIZE	(stb->st_blksize > 0 ? stb->st_blksize : BUFSIZ)
#else
#define	DEFBLKSIZE	BUFSIZ
#endif

	if (S_ISREG(stb->st_mode)		/* regular file */
	    && 0 < stb->st_size			/* non-zero size */
	    && (stb->st_size < DEFBLKSIZE	/* small file */
		|| exact))			/* or debugging */
		return stb->st_size;		/* use file size */

	return DEFBLKSIZE;
}

/* ispath --- return true if path has directory components */

int
ispath(file)
const char *file;
{
	return (strchr(file, '/') != NULL);
}

/* isdirpunct --- return true if char is a directory separator */

int
isdirpunct(c)
int c;
{
	return (c == '/');
}

/* os_close_on_exec --- set close on exec flag, print warning if fails */

void
os_close_on_exec(fd, name, what, dir)
int fd;
const char *name, *what, *dir;
{
	if (fd <= 2)	/* sanity */
		return;

	if (fcntl(fd, F_SETFD, 1) < 0)
		warning(_("%s %s `%s': could not set close-on-exec: (fcntl: %s)"),
			what, dir, name, strerror(errno));
}

/* os_isdir --- is this an fd on a directory? */

#if ! defined(S_ISDIR) && defined(S_IFDIR)
#define	S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

int
os_isdir(fd)
int fd;
{
	struct stat sbuf;

	return (fstat(fd, &sbuf) == 0 && S_ISDIR(sbuf.st_mode));
}

/* os_is_setuid --- true if running setuid root */

int
os_is_setuid()
{
	long uid, euid;

	uid = getuid();
	euid = geteuid();

	return (euid == 0 && euid != uid);
}

/* os_setbinmode --- set binary mode on file */

int
os_setbinmode (fd, mode)
int fd, mode;
{
	return 0;
}

/* os_restore_mode --- restore the original mode of the console device */

void
os_restore_mode (fd)
int fd;
{
	/* no-op */
	return;
}

#ifdef __CYGWIN__
#include <sys/cygwin.h>

extern int _fmode;
void
cygwin_premain0 (int argc, char **argv, struct per_process *myself)
{
  static struct __cygwin_perfile pf[] =
  {
    {"", O_RDONLY | O_TEXT},
    /*{"", O_WRONLY | O_BINARY},*/
    {NULL, 0}
  };
  cygwin_internal (CW_PERFILE, pf);
}
#endif
