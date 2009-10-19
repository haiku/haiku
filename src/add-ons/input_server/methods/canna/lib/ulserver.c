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
static char rcs_id[] = "@(#) 102.1 $Id: ulserver.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif

#ifndef NO_EXTEND_MENU
#include	<errno.h>
#include "canna.h"

#ifdef luna88k
extern int errno;
#endif

static int uuServerChangeEveryTimeCatch(uiContext d, int retval, mode_context env);
static int uuServerChangeExitCatch(uiContext d, int retval, mode_context env);
static int uuServerChangeQuitCatch(uiContext d, int retval, mode_context env);
static int serverChangeDo(uiContext d, int len);

static int serverChangeDo();

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * ¥µ¡¼¥Ð¤ÎÀÚ¤êÎ¥¤·                                                          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int serverFin(uiContext d)
{
  int retval = 0;
  yomiContext yc = (yomiContext)d->modec;

#ifndef STANDALONE
  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }    
  d->status = 0;
  killmenu(d);

  jrKanjiPipeError();
  
  makeGLineMessageFromString(d, "\244\253\244\312\264\301\273\372\312\321\264\271\245\265\241\274\245\320\244\310\244\316\300\334\302\263\244\362\300\332\244\352\244\336\244\267\244\277");
            /* ¤«¤Ê´Á»úÊÑ´¹¥µ¡¼¥Ð¤È¤ÎÀÜÂ³¤òÀÚ¤ê¤Þ¤·¤¿ */
  currentModeInfo(d);
#endif /* STANDALONE */

  return(retval);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * ¥µ¡¼¥Ð¤ÎÀÚ¤ê´¹¤¨                                                          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef STANDALONE

static
uuServerChangeEveryTimeCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  int len, echoLen, revPos;
  static int lmachinename;
  static WCHAR_T *wmachinename;

  if (!wmachinename) {
    WCHAR_T xxx[30]; /* 30 ¤Ã¤Æ¤Î¤Ï "¥Þ¥·¥óÌ¾?[" ¤è¤ê¤ÏÄ¹¤¤¤Ù¤È¤¤¤¦¤³¤È */
    lmachinename = MBstowcs(xxx, "\245\336\245\267\245\363\314\276?[", 30);
                              /* ¥Þ¥·¥óÌ¾ */
    wmachinename = (WCHAR_T *)malloc((lmachinename + 1)* sizeof(WCHAR_T));
    if (!wmachinename) {
      return -1;
    }
    WStrcpy(wmachinename, xxx);
  }

  if((echoLen = d->kanji_status_return->length) < 0)
    return(retval);

  if (echoLen == 0) {
    d->kanji_status_return->revPos = 0;
    d->kanji_status_return->revLen = 0;
  }

  WStrncpy(d->genbuf + lmachinename,
	   d->kanji_status_return->echoStr, echoLen);
  /* echoStr == d->genbuf ¤À¤È¤Þ¤º¤¤¤Î¤ÇÀè¤ËÆ°¤«¤¹ */
  WStrncpy(d->genbuf, wmachinename, lmachinename);
  revPos = len = lmachinename;
  len += echoLen;
  d->genbuf[len++] = (WCHAR_T)']';

  d->kanji_status_return->gline.line = d->genbuf;
  d->kanji_status_return->gline.length = len;
  if (d->kanji_status_return->revLen) {
    d->kanji_status_return->gline.revPos =
      d->kanji_status_return->revPos + revPos;
    d->kanji_status_return->gline.revLen = d->kanji_status_return->revLen;
  }
  else { /* È¿Å¾ÎÎ°è¤¬¤Ê¤¤¾ì¹ç */
    d->kanji_status_return->gline.revPos = len - 1;
    d->kanji_status_return->gline.revLen = 1;
  }
  d->kanji_status_return->info &= ~(KanjiThroughInfo | KanjiEmptyInfo);
  d->kanji_status_return->info |= KanjiGLineInfo;
  echostrClear(d);
  checkGLineLen(d);

  return retval;
}

static
uuServerChangeExitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  popCallback(d); /* ÆÉ¤ß¤ò pop */

  return(serverChangeDo(d, retval));
}

static
uuServerChangeQuitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  popCallback(d); /* ÆÉ¤ß¤ò pop */

  return prevMenuIfExist(d);
}

extern exp(char *) RkwGetServerName();
#endif /* STANDALONE */

int serverChange(uiContext d)
{
  int retval = 0;
  WCHAR_T *w;
  extern KanjiModeRec yomi_mode;
  extern int defaultContext;
  yomiContext yc = (yomiContext)d->modec;

#ifndef STANDALONE
  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }    
  d->status = 0;

  if ((yc = GetKanjiString(d, (WCHAR_T *)NULL, 0,
		     CANNA_ONLY_ASCII,
		     (int)CANNA_YOMI_CHGMODE_INHIBITTED,
		     (int)CANNA_YOMI_END_IF_KAKUTEI,
		     CANNA_YOMI_INHIBIT_ALL,
		     uuServerChangeEveryTimeCatch, uuServerChangeExitCatch,
		     uuServerChangeQuitCatch))
      == (yomiContext)0) {
    killmenu(d);
    return NoMoreMemory();
  }
  yc->minorMode = CANNA_MODE_ChangingServerMode;
  if(defaultContext != -1) {
    char *servname;
    servname = RkwGetServerName();
    if (servname && (w = WString(servname)) != (WCHAR_T *)0) {
      RomajiStoreYomi(d, w, (WCHAR_T *)0);
      WSfree(w);
      yc->kRStartp = yc->kCurs = 0;
      yc->rStartp = yc->rCurs = 0;
      d->current_mode = &yomi_mode;
      makeYomiReturnStruct(d);
    }
  }
  currentModeInfo(d);
#endif /* STANDALONE */

  return(retval);
}
		 
#ifndef STANDALONE
static
serverChangeDo(uiContext d, int len)
{
/* WCHAR_T ¤ÇÎÉ¤¤¤«¡© 256 ¤ÇÎÉ¤¤¤«¡© */
  WCHAR_T newServerName[256];
  WCHAR_T w1[512];
  char tmpServName[256];
  extern defaultContext;
  char *p;

  d->status = 0;

  if(!len)
    return(serverChange(d));

  WStrncpy(newServerName, d->buffer_return, len);
  newServerName[len] = 0;
#if defined(DEBUG) && !defined(WIN)
  if(iroha_debug)
    printf("iroha_server_name = [%s]\n", newServerName);
#endif

  jrKanjiPipeError();
  WCstombs(tmpServName, newServerName, 256);
  if (RkSetServerName(tmpServName) && (p = index((char *)tmpServName, '@'))) {
#ifdef WIN
    char xxxx[512];
#else
    char xxxx[1024];
#endif
    *p = '\0';
    sprintf(xxxx, "\244\253\244\312\264\301\273\372\312\321\264\271\245\250\245\363\245\270\245\363 %s \244\317\315\370\315\321\244\307\244\255\244\336\244\273\244\363\n",
	    tmpServName);
          /* ¤«¤Ê´Á»úÊÑ´¹¥¨¥ó¥¸¥ó %s ¤ÏÍøÍÑ¤Ç¤­¤Þ¤»¤ó */
    makeGLineMessageFromString(d, xxxx);

    RkSetServerName((char *)0);
    currentModeInfo(d);
    killmenu(d);
    return 0;
  }

  if(defaultContext == -1) {
    if((KanjiInit() != 0) || (defaultContext == -1)) {
      jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\245\265\241\274\245\320\244\310\304\314\277\256\244\307\244\255\244\336\244\273\244\363";
                   /* ¤«¤Ê´Á»úÊÑ´¹¥µ¡¼¥Ð¤ÈÄÌ¿®¤Ç¤­¤Þ¤»¤ó */
      killmenu(d);
      return(GLineNGReturn(d));
    }
    d->contextCache = -1;
  }

  p = RkwGetServerName();
  if (p) { /* ÀäÂÐÀ®¸ù¤¹¤ë¤ó¤À¤±¤É¤Í */
    if ((int)strlen(p) < 256) {
      MBstowcs(newServerName, p, 256);
    }
  }

  MBstowcs(w1, " \244\316\244\253\244\312\264\301\273\372\312\321\264\271\245\265\241\274\245\320\244\313\300\334\302\263\244\267\244\336\244\267\244\277", 512);
              /* ¤Î¤«¤Ê´Á»úÊÑ´¹¥µ¡¼¥Ð¤ËÀÜÂ³¤·¤Þ¤·¤¿ */
  WStrcpy((WCHAR_T *)d->genbuf, (WCHAR_T *)newServerName);
  WStrcat((WCHAR_T *)d->genbuf, (WCHAR_T *)w1);

  makeGLineMessage(d, d->genbuf, WStrlen(d->genbuf));
  killmenu(d);
  currentModeInfo(d);

  return(0);
}

#endif /* STANDALONE */
#endif /* NO_EXTEND_MENU */
