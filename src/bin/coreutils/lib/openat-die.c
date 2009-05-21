/* Report a save- or restore-cwd failure in our openat replacement and then exit.

   Copyright (C) 2005, 2006, 2008-2009 Free Software Foundation, Inc.

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

#include <config.h>

#include "openat.h"

#include <stdlib.h>

#include "error.h"
#include "exitfail.h"

#include "gettext.h"
#define _(msgid) gettext (msgid)

void
openat_save_fail (int errnum)
{
  error (exit_failure, errnum,
	 _("unable to record current working directory"));

  /* The `noreturn' attribute cannot be applied to error, since it returns
     when its first argument is 0.  To help compilers understand that this
     function does not return, call abort.  Also, the abort is a
     safety feature if exit_failure is 0 (which shouldn't happen).  */
  abort ();
}

void
openat_restore_fail (int errnum)
{
  error (exit_failure, errnum,
	 _("failed to return to initial working directory"));

  /* As above.  */
  abort ();
}
