/* Copyright 1992 NEC Corporation, Tokyo, Japan.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of NEC
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  NEC Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * NEC CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN 
 * NO EVENT SHALL NEC CORPORATION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
 * OTHER TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
 * PERFORMANCE OF THIS SOFTWARE. 
 */
//Modified by T.Murai

#include "canna.h"

#ifdef NO_EXTEND_MENU
static struct funccfunc onoff_funcs[] = {{0, 0},};
#else
static struct funccfunc onoff_funcs[] = {
  {CANNA_FN_Forward		,IchiranForwardKouho		},
  {CANNA_FN_Backward		,IchiranBackwardKouho		},
  {CANNA_FN_Next		,IchiranNextKouhoretsu		},
  {CANNA_FN_Prev		,IchiranPreviousKouhoretsu	},
  {CANNA_FN_BeginningOfLine	,IchiranBeginningOfKouho	},
  {CANNA_FN_EndOfLine		,IchiranEndOfKouho		},
  {CANNA_FN_DeletePrevious	,IchiranQuit			},
  {CANNA_FN_Henkan		,OnOffSelect			},
  {CANNA_FN_HenkanOrInsert	,OnOffSelect			},
  {CANNA_FN_HenkanOrNothing	,OnOffSelect			},
  {CANNA_FN_Kakutei		,OnOffKakutei			},
  {CANNA_FN_Quit		,IchiranQuit			},
  {CANNA_FN_Nop			,IchiranNop			},
  {CANNA_FN_FuncSequence	,DoFuncSequence			},
  {CANNA_FN_UseOtherKeymap	,UseOtherKeymap			},
  {0				,0				},
};
#endif /* NO_EXTEND_MENU */

extern int searchfunc(...);
KanjiModeRec onoff_mode = {
  searchfunc,
  default_kmap,
  CANNA_KANJIMODE_TABLE_SHARED,
  onoff_funcs,
};
