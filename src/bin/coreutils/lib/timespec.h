/* timespec -- System time interface

   Copyright (C) 2000, 2002, 2004, 2005, 2007 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#if ! defined TIMESPEC_H
# define TIMESPEC_H

# include <time.h>

/* Return negative, zero, positive if A < B, A == B, A > B, respectively.
   Assume the nanosecond components are in range, or close to it.  */
static inline int
timespec_cmp (struct timespec a, struct timespec b)
{
  return (a.tv_sec < b.tv_sec ? -1
	  : a.tv_sec > b.tv_sec ? 1
	  : a.tv_nsec - b.tv_nsec);
}

void gettime (struct timespec *);
int settime (struct timespec const *);

#endif
