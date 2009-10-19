/*
 *  FILE: pathname.c
 *  AUTH: Michael John Radwin <mjr@acm.org>
 *
 *  DESC: two important functions from libgen.  Largely influenced
 *        by the gnu dirutils.
 *
 *  DATE: Tue Oct  1 20:48:28 EDT 1996
 *   $Id: pathname.c 10 2002-07-09 12:24:59Z ejakowatz $
 *
 *  Copyright (c) 1996-1998  Michael John Radwin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>

#ifdef WIN32
#define PATH_SEPARATOR_STR  "\\"
#define PATH_SEPARATOR_CHAR '\\'
#else
#define PATH_SEPARATOR_STR  "/"
#define PATH_SEPARATOR_CHAR '/'
#endif

static const char rcsid[] = "$Id: pathname.c 10 2002-07-09 12:24:59Z ejakowatz $";

static void
strip_trailing_slashes(char *path)
{
    int last = strlen (path) - 1;
    while ((last > 0) && (path[last] == PATH_SEPARATOR_CHAR))
	path[last--] = '\0';
}

char *
basename(char *path)
{
    char *slash;

    if ((path == NULL) || (path[0] == '\0'))
	return ".";

    strip_trailing_slashes(path);
    if (path[0] == '\0')
	return PATH_SEPARATOR_STR;

    slash = strrchr(path, PATH_SEPARATOR_CHAR);
    return (slash) ? slash + 1 : path;
}

char *
dirname(char *path)
{
    char *slash;

    if ((path == NULL) || (path[0] == '\0'))
	return ".";

    strip_trailing_slashes(path);
    if (path[0] == '\0')
	return PATH_SEPARATOR_STR;

    slash = strrchr(path, PATH_SEPARATOR_CHAR);
    if (slash == NULL)
	return ".";

    /* Remove any trailing slashes and final element. */
    while (slash > path && *slash == PATH_SEPARATOR_CHAR)
        --slash;
    slash[1] = 0;

    return path;
}
