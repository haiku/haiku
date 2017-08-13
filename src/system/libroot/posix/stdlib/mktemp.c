/*
 * Copyright (c) 1987, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdint.h>

#include <errno_private.h>


static int _gettemp(char *, int *, int, int);

static const char padchar[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";


#if 0
int
mkstemps(char *path, int slen)
{
	int fd;
	return _gettemp(path, &fd, 0, slen) ? fd : -1;
}
#endif


int
mkstemp(char *path)
{
	int fd;
	if (_gettemp(path, &fd, 0, 0))
		return fd;

	return -1;
}


char *
mkdtemp(path)
	char *path;
{
	return (_gettemp(path, (int *)NULL, 1, 0) ? path : (char *)NULL);
}


char *
mktemp(char *path)
{
	if (_gettemp(path, (int *)NULL, 0, 0))
		return path;

	return NULL;
}


static int
_gettemp(char *path, int *doopen, int domkdir, int slen)
{
	char *start, *trv, *suffp;
	char *pad;
	struct stat sbuf;
	int rval;
	static unsigned int seed = 0;

	if (doopen != NULL && domkdir) {
		__set_errno(EINVAL);
		return 0;
	}

	for (trv = path; *trv != '\0'; ++trv)
		;

	trv -= slen;
	suffp = trv;
	--trv;
	if (trv < path) {
		__set_errno(EINVAL);
		return 0;
	}

	/* Fill space with random characters */
	if (seed == 0) {
		/* Select a pseudo-random seed on first call to avoid
		to generate the same sequence of pattern */
		struct timeval tv;
		gettimeofday(&tv, 0);
		seed = (getpid() << 16) ^ getuid() ^ tv.tv_sec ^ tv.tv_usec;
	}
	while (trv >= path && *trv == 'X') {
		uint32_t value = rand_r(&seed) % (sizeof(padchar) - 1);
		*trv-- = padchar[value];
	}
	start = trv + 1;

	/*
	 * check the target directory.
	 */
	if (doopen != NULL || domkdir) {
		for (; trv > path; --trv) {
			if (*trv == '/') {
				*trv = '\0';
				rval = stat(path, &sbuf);
				*trv = '/';
				if (rval != 0)
					return 0;
				if (!S_ISDIR(sbuf.st_mode)) {
					__set_errno(ENOTDIR);
					return 0;
				}
				break;
			}
		}
	}

	for (;;) {
		if (doopen) {
			if ((*doopen = open(path, O_CREAT|O_EXCL|O_RDWR, 0600)) >= 0)
				return 1;
			if (errno != EEXIST)
				return 0;
		} else if (domkdir) {
			if (mkdir(path, 0700) == 0)
				return 1;
			if (errno != EEXIST)
				return 0;
		} else if (lstat(path, &sbuf))
			return errno == ENOENT;

		/* If we have a collision, cycle through the space of filenames */
		for (trv = start;;) {
			if (*trv == '\0' || trv == suffp)
				return 0;
			pad = strchr(padchar, *trv);
			if (pad == NULL || *++pad == '\0')
				*trv++ = padchar[0];
			else {
				*trv++ = *pad;
				break;
			}
		}
	}

	/* not reached */
}
