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
static char jishumap_id[] = "@(#) 102.1 $Id: jishumap.h 14875 2005-11-12 21:25:31Z bonefish $";
#endif /* lint */

#include "canna.h"

static struct funccfunc jishu_funcs[] = {
  {CANNA_FN_FunctionalInsert	,JishuYomiInsert	},
  {CANNA_FN_Next		,JishuNextJishu		},
  {CANNA_FN_Prev		,JishuPreviousJishu	},
  {CANNA_FN_DeletePrevious	,JishuQuit		},
  {CANNA_FN_Henkan		,JishuKanjiHenkan	},
  {CANNA_FN_HenkanOrInsert	,JishuKanjiHenkanOInsert},
  {CANNA_FN_HenkanOrNothing	,JishuKanjiHenkanONothing},
  {CANNA_FN_Kakutei		,YomiKakutei		},
  {CANNA_FN_Extend		,JishuExtend		},
  {CANNA_FN_Shrink		,JishuShrink		},
  {CANNA_FN_Quit		,JishuQuit		},
  {CANNA_FN_BubunMuhenkan	,JishuQuit		},
  {CANNA_FN_Zenkaku		,JishuZenkaku		},
  {CANNA_FN_Hankaku		,JishuHankaku		},
  {CANNA_FN_ToUpper		,JishuToUpper		},
  {CANNA_FN_ToLower		,JishuToLower		},
  {CANNA_FN_Hiragana		,JishuHiragana		},
  {CANNA_FN_Katakana		,JishuKatakana		},
  {CANNA_FN_Romaji		,JishuRomaji		},
  {CANNA_FN_Capitalize		,JishuCapitalize	},
  {CANNA_FN_Forward		,TbForward		},
  {CANNA_FN_Backward		,TbBackward		},
  {CANNA_FN_BeginningOfLine	,TbBeginningOfLine	},
  {CANNA_FN_EndOfLine		,TbEndOfLine		},
  {CANNA_FN_KanaRotate		,JishuKanaRotate	},
  {CANNA_FN_RomajiRotate	,JishuRomajiRotate	},
  {CANNA_FN_CaseRotate		,JishuCaseRotateForward	},
  {CANNA_FN_Nop			,JishuNop		},
  {CANNA_FN_FuncSequence	,DoFuncSequence		},
  {CANNA_FN_UseOtherKeymap	,UseOtherKeymap		},
  {0				,0			},
};
extern int searchfunc(...);

KanjiModeRec jishu_mode = {
  searchfunc,
  default_kmap,
  CANNA_KANJIMODE_TABLE_SHARED,
  jishu_funcs,
};
