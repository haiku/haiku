/* Compute look-ahead criteria for bison,

   Copyright (C) 1984, 1986, 1989, 2000, 2002, 2004 Free Software
   Foundation, Inc.

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

#ifndef LALR_H_
# define LALR_H_

# include <bitset.h>
# include <bitsetv.h>

/* Import the definition of RULE_T. */
# include "gram.h"

/* Import the definition of CORE, TRANSITIONS and REDUCTIONS. */
# include "state.h"

/* Compute how to make the finite state machine deterministic; find
   which rules need lookahead in each state, and which lookahead
   tokens they accept.  */

void lalr (void);

/* Release the information related to lookaheads.  Can be performed
   once the action tables are computed.  */

void lalr_free (void);


/* lalr() builds these data structures. */

/* GOTO_MAP, FROM_STATE and TO_STATE -- record each shift transition
   which accepts a variable (a nonterminal).

   FROM_STATE[T] -- state number which a transition leads from.
   TO_STATE[T] -- state number it leads to.

   All the transitions that accept a particular variable are grouped
   together and GOTO_MAP[I - NTOKENS] is the index in FROM_STATE and
   TO_STATE of the first of them.  */

typedef short int goto_number;
# define GOTO_NUMBER_MAXIMUM SHRT_MAX

extern goto_number *goto_map;
extern state_number *from_state;
extern state_number *to_state;


#endif /* !LALR_H_ */
