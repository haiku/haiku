/* Locations for Bison
   Copyright (C) 2002 Free Software Foundation, Inc.

   This file is part of Bison, the GNU Compiler Compiler.

   Bison is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   Bison is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bison; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef LOCATION_H_
# define LOCATION_H_

/* A boundary between two characters.  */
typedef struct
{
  /* The name of the file that contains the boundary.  */
  char const *file;

  /* The (origin-1) line that contains the boundary.  */
  int line;

  /* The (origin-1) column just after the boundary.  This is neither a
     byte count, nor a character count; it is a column count.  */
  int column;

} boundary;

/* Return nonzero if A and B are equal boundaries.  */
static inline bool
equal_boundaries (boundary a, boundary b)
{
  return (a.column == b.column
	  && a.line == b.line
	  && a.file == b.file);
}

/* A location, that is, a region of source code.  */
typedef struct
{
  /* Boundary just before the location starts.  */
  boundary start;

  /* Boundary just after the location ends.  */
  boundary end;

} location;

#define YYLTYPE location

extern location const empty_location;

void location_print (FILE *, location);

#endif /* ! defined LOCATION_H_ */
