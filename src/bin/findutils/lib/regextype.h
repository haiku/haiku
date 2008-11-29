/* regextype.h -- Decode the name of a regular expression syntax into am
                  option name.

   Copyright 2005 Free Software Foundation, Inc.

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
/* Written by James Youngman <jay@gnu.org>.
 */

int get_regex_type(const char *s);


const char * get_regex_type_name(int ix);
int get_regex_type_flags(int ix);
int get_regex_type_synonym(int ix);
