/* Emulate waitpid on systems that just have wait.
   Copyright (C) 1991, 1993, 2000, 2003, 2004, 2007 Free Software
   Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#ifndef errno
extern int errno;
#endif


#if defined _MSC_VER || defined __MINGW32__
/* Native Woe32 API.  */
#include <process.h>
#else
/* Unix API.  */
#include "wait.h"
#endif

#define WAITPID_CHILDREN 8
static pid_t waited_pid[WAITPID_CHILDREN];
static int waited_status[WAITPID_CHILDREN];

pid_t
waitpid (pid_t pid, int *stat_loc, int options)
{
  int i;
  pid_t p;

  if (!options && (pid == -1 || 0 < pid))
    {
      /* If we have already waited for this child, return it immediately.  */
      for (i = 0;  i < WAITPID_CHILDREN;  i++)
	{
	  p = waited_pid[i];
	  if (p && (p == pid || pid == -1))
	    {
	      waited_pid[i] = 0;
	      goto success;
	    }
	}

      /* The child has not returned yet; wait for it, accumulating status.  */
      for (i = 0;  i < WAITPID_CHILDREN;  i++)
	if (! waited_pid[i])
	  {
	    p = wait (&waited_status[i]);
	    if (p < 0)
	      return p;
	    if (p == pid || pid == -1)
	      goto success;
	    waited_pid[i] = p;
	  }
    }

  /* We cannot emulate this wait call, e.g. because of too many children.  */
  errno = EINVAL;
  return -1;

success:
  if (stat_loc)
    *stat_loc = waited_status[i];
  return p;
}
