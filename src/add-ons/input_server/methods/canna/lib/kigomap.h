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
// Modified by T.Murai

#if !defined(lint) && !defined(__CODECENTER__)
static char kigomap_id[] = "@(#) 102.1 $Id: kigomap.h 14875 2005-11-12 21:25:31Z bonefish $";
#endif /* lint */

#ifdef NO_EXTEND_MENU
static struct funccfunc kigo_funcs[] = {{0, 0},};
#else
extern int DoFuncSequence();
extern int UseOtherKeymap();

static struct funccfunc kigo_funcs[] = {
  {CANNA_FN_KigouMode		,KigoQuit		},
  {CANNA_FN_Forward		,KigoForwardKouho	},
  {CANNA_FN_Backward		,KigoBackwardKouho	},
  {CANNA_FN_Next		,KigoNextKouhoretsu	},
  {CANNA_FN_Prev		,KigoPreviousKouhoretsu	},
  {CANNA_FN_BeginningOfLine	,KigoBeginningOfKouho	},
  {CANNA_FN_EndOfLine		,KigoEndOfKouho		},
  {CANNA_FN_DeletePrevious	,KigoQuit		},
  {CANNA_FN_Henkan		,KigoForwardKouho	},
  {CANNA_FN_HenkanOrInsert	,KigoForwardKouho	},
  {CANNA_FN_HenkanOrNothing	,KigoForwardKouho	},
  {CANNA_FN_Kakutei		,KigoKakutei		},
  {CANNA_FN_Quit		,KigoQuit		},
  {CANNA_FN_Nop			,KigoNop		},
  {CANNA_FN_FuncSequence	,DoFuncSequence		},
  {CANNA_FN_UseOtherKeymap	,UseOtherKeymap		},
  {0				,0			},
};
#endif /* NO_EXTEND_MENU */

extern int searchfunc(...);

KanjiModeRec kigo_mode = {
  searchfunc,
  default_kmap,
  CANNA_KANJIMODE_TABLE_SHARED,
  kigo_funcs,
};
