/* Save the list of files in a directory, with additional information.

   Copyright 1997, 1999, 2001, 2003, 2005 Free Software Foundation, Inc.

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
 * Based on savedir.h by David MacKenzie <djm@gnu.org>.
 */

#if !defined SAVEDIRINFO_H_
# define SAVEDIRINFO_H_


typedef enum tagSaveDirControlFlags
  {
    SavedirSort = 1
  } 
SaveDirControlFlags;


typedef enum tagSaveDirDataFlags
  {
    SavedirHaveFileType = 1
  } 
SaveDirDataFlags;


/* We keep the name and the type in a structure together 
 * to allow us to sort them together.
 */
struct savedir_direntry
{
  int      flags;		/* from SaveDirDataFlags */
  char     *name;		/* the name of the directory entry */
  mode_t   type_info;		/* the type (or zero if unknown) */
};

struct savedir_dirinfo
{
  char *buffer;			/* The names are stored here. */
  size_t size;			/* The total number of results. */
  struct savedir_direntry *entries;	/* The results themselves */
};


struct savedir_extrainfo
{
  mode_t type_info;
};

/* savedirinfo() is the old interface. */
char *savedirinfo (const char *dir, struct savedir_extrainfo **extra);

/* savedir() is the 'new' interface, but the function has the same name
 * as the function from findutils 4.1.7 and 4.1.20.
 */
struct savedir_dirinfo * xsavedir(const char *dir, int flags);
void free_dirinfo(struct savedir_dirinfo *p);

#endif
