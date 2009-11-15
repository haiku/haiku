/* Internationalization related declarations.
   Copyright (C) 2008, 2009 Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

GNU Wget is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wget.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef WGET_EXITS_H
#define WGET_EXITS_H

#include "wget.h"


void inform_exit_status (uerr_t err);

int get_exit_status (void);


#endif /* WGET_EXITS_H */
