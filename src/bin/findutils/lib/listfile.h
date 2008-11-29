/* listfile.h -- display a long listing of a file
   Copyright (C) 1991, 1993, 2000 Free Software Foundation, Inc.

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


#if !defined LISTFILE_H
# define LISTFILE_H

void list_file (char *name, char *relname, struct stat *statp, time_t current_time, int output_block_size, FILE *stream);



#endif
