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

#if !defined(lint) && !defined(__CODECENTER__)
static char yomimap_id[] = "@(#) 102.1 $Id: yomimap.h 10525 2004-12-23 21:23:50Z korli $";
#endif /* lint */

#include "canna.h"

static struct funccfunc yomi_funcs[] = {
  {CANNA_FN_FunctionalInsert	,YomiInsert		},
  {CANNA_FN_QuotedInsert	,YomiQuotedInsert	},
  {CANNA_FN_Forward		,YomiForward		},
  {CANNA_FN_Backward		,YomiBackward		},
  {CANNA_FN_Next		,YomiNextJishu		},
  {CANNA_FN_Prev		,YomiPreviousJishu	},
  {CANNA_FN_BeginningOfLine	,YomiBeginningOfLine	},
  {CANNA_FN_EndOfLine		,YomiEndOfLine		},
  {CANNA_FN_DeleteNext		,YomiDeleteNext		},
  {CANNA_FN_DeletePrevious	,YomiDeletePrevious	},
  {CANNA_FN_KillToEndOfLine	,YomiKillToEndOfLine	},
  {CANNA_FN_Henkan		,YomiHenkan		},
  {CANNA_FN_HenkanOrInsert	,YomiHenkanNaive	},
  {CANNA_FN_HenkanOrNothing	,YomiHenkanOrNothing	},
  {CANNA_FN_Kakutei		,YomiKakutei		},
  {CANNA_FN_Quit		,YomiQuit		},
  {CANNA_FN_ConvertAsHex	,ConvertAsHex		},
  {CANNA_FN_ConvertAsBushu	,ConvertAsBushu		},
  {CANNA_FN_KouhoIchiran	,ConvertAsBushu		},
  {CANNA_FN_ToUpper		,YomiToUpper		},
  {CANNA_FN_ToLower		,YomiToLower		},
  {CANNA_FN_Capitalize		,YomiCapitalize		},
  {CANNA_FN_Zenkaku		,YomiZenkaku		},
  {CANNA_FN_Hankaku		,YomiHankaku		},
  {CANNA_FN_Hiragana		,YomiHiraganaJishu	},
  {CANNA_FN_Katakana		,YomiKatakanaJishu	},
  {CANNA_FN_Romaji		,YomiRomajiJishu	},
  {CANNA_FN_KanaRotate		,YomiKanaRotate		},
  {CANNA_FN_RomajiRotate	,YomiRomajiRotate	},
  {CANNA_FN_CaseRotate		,YomiCaseRotateForward	},
  {CANNA_FN_Mark		,YomiMark		},
  {CANNA_FN_BubunKakutei	,YomiBubunKakutei	},
  {CANNA_FN_BaseHiragana	,YomiBaseHira		},
  {CANNA_FN_BaseKatakana	,YomiBaseKata		},
  {CANNA_FN_BaseKana		,YomiBaseKana		},
  {CANNA_FN_BaseEisu		,YomiBaseEisu		},
  {CANNA_FN_BaseZenkaku		,YomiBaseZen		},
  {CANNA_FN_BaseHankaku		,YomiBaseHan		},
  {CANNA_FN_BaseKakutei		,YomiBaseKakutei	},
  {CANNA_FN_BaseHenkan		,YomiBaseHenkan		},
  {CANNA_FN_BaseHiraKataToggle	,YomiBaseHiraKataToggle	},
  {CANNA_FN_BaseZenHanToggle	,YomiBaseZenHanToggle	},
  {CANNA_FN_BaseKanaEisuToggle	,YomiBaseKanaEisuToggle	},
  {CANNA_FN_BaseKakuteiHenkanToggle ,YomiBaseKakuteiHenkanToggle	},
  {CANNA_FN_BaseRotateForward	,YomiBaseRotateForw	},
  {CANNA_FN_BaseRotateBackward	,YomiBaseRotateBack	},
  {CANNA_FN_TemporalMode	,YomiModeBackup		},
  {CANNA_FN_Nop			,YomiNop		},
  {CANNA_FN_FuncSequence	,DoFuncSequence		},
  {CANNA_FN_UseOtherKeymap	,UseOtherKeymap		},
  {0				,0			},
};

KanjiModeRec yomi_mode = {
  Yomisearchfunc,
  default_kmap,
  CANNA_KANJIMODE_TABLE_SHARED,
  yomi_funcs,
};

