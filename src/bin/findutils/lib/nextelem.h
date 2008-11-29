/* Return the next element of a path.
   Copyright (C) 1992, 2005 Free Software Foundation, Inc.

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

/* Written by David MacKenzie <djm@gnu.org>,
   inspired by John P. Rouillard <rouilj@cs.umb.edu>.  */

#ifndef INC_NEXTELEM_H
#define INC_NEXTELEM_H 1

char *next_element (const char *path, int curdir_ok);

#endif
