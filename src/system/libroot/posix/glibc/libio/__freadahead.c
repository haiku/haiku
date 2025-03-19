/* Retrieve information about a FILE stream.
   Copyright (C) 2007-2025 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <stdio_ext.h>
#include "libioP.h"

size_t
__freadahead (FILE *fp)
{
  if (fp->_IO_write_ptr > fp->_IO_write_base)
    return 0;
  return (fp->_IO_read_end - fp->_IO_read_ptr)
         + (fp->_flags & _IO_IN_BACKUP ? fp->_IO_save_end - fp->_IO_save_base :
            0);
}
