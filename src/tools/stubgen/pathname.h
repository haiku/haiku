/*
 *  FILE: pathname.h
 *  AUTH: Michael John Radwin <mjr@acm.org>
 *
 *  DESC: two important functions from libgen
 *
 *  DATE: Tue Oct  1 20:47:55 EDT 1996
 *   $Id: pathname.h 10 2002-07-09 12:24:59Z ejakowatz $ 
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

#ifndef __pathname_H__
#define __pathname_H__

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Given a pointer to a null-terminated character string that contains a
 * path name, basename() returns a pointer to the last element of path.
 * Trailing ``/'' characters are deleted.  In doing this, it may place a
 * null byte in the path name after the next to last element, so the
 * content of path must be disposable.
 *
 * If path or *path is zero, pointer to a static constant ``.'' is
 * returned.
 * 
 * If path consists entirely of ``/'' characters, the empty string is
 * returned.  
 */
char * basename(char *path);

/*
 * Given a pointer to a null-terminated character string that contains a
 * file system path name, dirname() returns a string that is the parent
 * directory of that file. In doing this, it may place a null byte in
 * the path name after the next to last element, so the content of path
 * must be disposable. The returned string should not be deallocated by
 * the caller.  Trailing ``/'' characters in the path are not counted as
 * part of the path.  
 *
 * If path or *path is zero, a pointer to a static constant ``.'' is
 * returned.
 *
 * dirname() and basename() together yield a complete path name.
 * dirname (path) is the directory where basename (path) is found.
 */
char * dirname(char *path);

#ifdef	__cplusplus
}
#endif

#endif /* __pathname_H__ */
