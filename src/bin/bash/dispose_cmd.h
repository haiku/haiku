/* dispose_cmd.h -- Functions appearing in dispose_cmd.c. */

/* Copyright (C) 1993 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2, or (at your option) any later
   version.

   Bash is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License along
   with Bash; see the file COPYING.  If not, write to the Free Software
   Foundation, 59 Temple Place, Suite 330, Boston, MA 02111 USA. */

#if !defined (_DISPOSE_CMD_H_)
#define _DISPOSE_CMD_H_

#include "stdc.h"

extern void dispose_command __P((COMMAND *));
extern void dispose_word __P((WORD_DESC *));
extern void dispose_words __P((WORD_LIST *));
extern void dispose_word_array __P((char **));
extern void dispose_redirects __P((REDIRECT *));

#if defined (COND_COMMAND)
extern void dispose_cond_node __P((COND_COM *));
#endif

#endif /* !_DISPOSE_CMD_H_ */
