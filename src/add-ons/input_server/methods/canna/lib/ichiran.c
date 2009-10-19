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

/************************************************************************/
/* THIS SOURCE CODE IS MODIFIED FOR TKO BY T.MURAI 1997
/************************************************************************/


#if !defined(lint) && !defined(__CODECENTER__)
static	char	rcs_id[] = "@(#) 102.1 $Id: ichiran.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif /* lint */

#include	<errno.h>
#include	"canna.h"
#include "RK.h"
#include "RKintern.h"

extern int TanNextKouho();

static void clearIchiranContext();
static int IchiranKakutei();
static void getIchiranPreviousKouhoretsu();
static void getIchiranNextKouhoretsu();


#define ICHISIZE 9

static void makeIchiranEchoStrCurChange(yomiContext yc);
static void makeIchiranKanjiStatusReturn(uiContext d, mode_context env, yomiContext yc);
static int ichiranEveryTimeCatch(uiContext d, int retval, mode_context env);
static int ichiranExitCatch(uiContext d, int retval, mode_context env);
static int ichiranQuitCatch(uiContext d, int retval, mode_context env);
static void popIchiranMode(uiContext d);
static void clearIchiranContext(ichiranContext p);
static int makeKouhoIchiran(uiContext d, int nelem, int bangomax, unsigned char inhibit, int currentkouho);
static int IchiranKakuteiThenDo(uiContext d, int func);
static int IchiranQuitThenDo(uiContext d, int func);
static int IchiranConvert(uiContext d);
static void getIchiranPreviousKouhoretsu(uiContext d);
static int IchiranNextPage(uiContext d);
static int IchiranPreviousPage(uiContext d);
static void getIchiranNextKouhoretsu(uiContext d);
static int IchiranBangoKouho(uiContext d);
static int getIchiranBangoKouho(uiContext d);
static int IchiranKakutei(uiContext d);
static int IchiranExtendBunsetsu(uiContext d);
static int IchiranShrinkBunsetsu(uiContext d);
static int IchiranAdjustBunsetsu(uiContext d);
static int IchiranKillToEndOfLine(uiContext d);
static int IchiranDeleteNext(uiContext d);
static int IchiranBubunMuhenkan(uiContext d);
static int IchiranHiragana(uiContext d);
static int IchiranKatakana(uiContext d);
static int IchiranZenkaku(uiContext d);
static int IchiranHankaku(uiContext d);
static int IchiranRomaji(uiContext d);
static int IchiranToUpper(uiContext d);
static int IchiranToLower(uiContext d);
static int IchiranCapitalize(uiContext d);
static int IchiranKanaRotate(uiContext d);
static int IchiranRomajiRotate(uiContext d);
static int IchiranCaseRotateForward(uiContext d);
static char *sbango = 
  "\243\261\241\241\243\262\241\241\243\263\241\241\243\264\241\241\243\265"
  "\241\241\243\266\241\241\243\267\241\241\243\270\241\241\243\271\241\241"
  "\243\341\241\241\243\342\241\241\243\343\241\241\243\344\241\241\243\345"
  "\241\241\243\346";
     /* £±¡¡£²¡¡£³¡¡£´¡¡£µ¡¡£¶¡¡£·¡¡£¸¡¡£¹¡¡£á¡¡£â¡¡£ã¡¡£ä¡¡£å¡¡£æ */
                                                /* ¸õÊä¹Ô¤Î¸õÊäÈÖ¹æ¤ÎÊ¸»úÎó */
static WCHAR_T *bango;

/*  "1.","¡¡2.","¡¡3.","¡¡4.","¡¡5.","¡¡6.","¡¡7.","¡¡8.","¡¡9.",*/
static char  *sbango2[] = {
  "1","\241\2412","\241\2413","\241\2414","\241\2415",
  "\241\2416","\241\2417","\241\2418","\241\2419",
  };

static WCHAR_T *bango2[ICHISIZE];

static char *skuuhaku = "\241\241"; 
			/* ¡¡ */
static WCHAR_T *kuuhaku;

int initIchiran(void)
{
  int i, retval = 0;
  char buf[16];

  retval = setWStrings(&bango, &sbango, 1);
  if (retval != NG) {
    for(i = 0; i < ICHISIZE; i++) {

      /* ¥»¥Ñ¥ì¡¼¥¿¡¼¤Î½èÍý */
      if (cannaconf.indexSeparator &&
	  0x20 <= cannaconf.indexSeparator &&
	  0x80 > cannaconf.indexSeparator)
        sprintf(buf, "%s%c", sbango2[i], (char)cannaconf.indexSeparator);
      else
        sprintf(buf, "%s%c", sbango2[i], (char)DEFAULTINDEXSEPARATOR);
      
      bango2[i] = WString(buf);
    }

    retval = setWStrings(&kuuhaku, &skuuhaku, 1);
  }
  return retval;
}


/*
 * °ìÍ÷¹ÔÉ½¼¨Ãæ¤Î¥«¥ì¥ó¥ÈÊ¸Àá¤Î¸õÊä¤ò¹¹¿·¤¹¤ë
 *
 * ¡¦¥«¥ì¥ó¥È¸õÊä¤òÊÑ¤¨¤ë¡£
 * ¡¦¤³¤ì¤Ë¤È¤â¤Ê¤¤ kugiri ¤â¹¹¿·¤µ¤ì¤ë
 *
 * °ú¤­¿ô	uiContext
 *              yomiContext
 */
static void
makeIchiranEchoStrCurChange(yomiContext yc)
{
  RkwXfer(yc->context, yc->curIkouho);
}

/*
 * ¤«¤Ê´Á»úÊÑ´¹ÍÑ¤Î¹½Â¤ÂÎ¤ÎÆâÍÆ¤ò¹¹¿·¤¹¤ë(¤½¤Î¾ì¤Î¤ß)
 *
 * ¡¦°ìÍ÷¤ò¸Æ¤Ó½Ð¤¹Á°¤Î¾õÂÖ¤Ë¤Ä¤¤¤Æ¤ÎÉ½¼¨Ê¸»úÎó¤òºî¤ë
 *
 * °ú¤­¿ô	uiContext
 *              yomiContext
 */
static void
makeIchiranKanjiStatusReturn(uiContext d, mode_context env, yomiContext yc)
{
  mode_context sv;

  sv = d->modec;
  d->modec = env;
  makeKanjiStatusReturn(d, yc);
  d->modec = sv;
}

#define DEC_COLUMNS(n) ((n) < 10 ? 1 : (n) < 100 ? 2 : (n) < 1000 ? 3 : 4)

/*
 * ¸õÊä¹Ô¤Ë´Ø¤¹¤ë¹½Â¤ÂÎ¤ÎÆâÍÆ¤ò¹¹¿·¤¹¤ë
 *
 * ¡¦glineinfo ¤È kouhoinfo ¤«¤é¸õÊä¹Ô¤òºîÀ®¤·¡¢¥«¥ì¥ó¥È¸õÊäÈÖ¹æ¤òÈ¿Å¾¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	¤Ê¤·
 */
void
makeGlineStatus(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec;
  WCHAR_T *p;
  char str[16];
  int i, cur;

  if (cannaconf.kCount) {
    cur = *(ic->curIkouho) + 1;
  }

  d->kanji_status_return->info |= KanjiGLineInfo;
  d->kanji_status_return->gline.line =
    ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].gldata;
  d->kanji_status_return->gline.length = 
    ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].gllen;
  d->kanji_status_return->gline.revPos = 
    ic->kouhoifp[*(ic->curIkouho)].khpoint;
  if (cannaconf.ReverseWord && ic->inhibit & NUMBERING) {
    p = ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].gldata +
      ic->kouhoifp[*(ic->curIkouho)].khpoint;
    for (i = 0;
         *p != *kuuhaku && *p != ((WCHAR_T)' ') && *p != ((WCHAR_T)0)
	 && i < ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].gllen;
	 i++) {
      p++;
    }
    d->kanji_status_return->gline.revLen = i;
  } else
    d->kanji_status_return->gline.revLen = 1;

  if (cannaconf.kCount && d->kanji_status_return->gline.length) {
    register int a = ic->nIkouho, b = DEC_COLUMNS(cur) + DEC_COLUMNS(a) + 2;
    sprintf(str, " %d/%d", cur, a);
    MBstowcs(d->kanji_status_return->gline.line + 
	     d->kanji_status_return->gline.length - b, str, b + 1);
    /* °Ê²¼¤Ï¤¤¤é¤Ê¤¤¤Î¤Ç¤Ï¡© */
    d->kanji_status_return->gline.length
      = WStrlen(d->kanji_status_return->gline.line);
  }
}

static int ichiranEveryTimeCatch (uiContext, int, mode_context);

static int
ichiranEveryTimeCatch(uiContext d, int retval, mode_context env)
{
  yomiContext yc;

  yc = (yomiContext)env;

  makeIchiranEchoStrCurChange(yc);
  makeIchiranKanjiStatusReturn(d, env, yc);

  return(retval);
}

static int ichiranExitCatch (uiContext, int, mode_context);

static int
ichiranExitCatch(uiContext d, int retval, mode_context env)
{
  yomiContext yc;

  yc = (yomiContext)env;
  yc->kouhoCount = 0;
  /* d->curIkouho¤ò¥«¥ì¥ó¥È¸õÊä¤È¤¹¤ë */
  if ((retval = RkwXfer(yc->context, yc->curIkouho)) == NG) {
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
    jrKanjiError = "\245\253\245\354\245\363\245\310\270\365\312\344\244\362"
	"\274\350\244\352\275\320\244\273\244\336\244\273\244\363\244\307"
	"\244\267\244\277";             
      /* ¥«¥ì¥ó¥È¸õÊä¤ò¼è¤ê½Ð¤»¤Þ¤»¤ó¤Ç¤·¤¿ */
    /* ¥«¥ì¥ó¥È¸õÊä¤¬¼è¤ê½Ð¤»¤Ê¤¤¤¯¤é¤¤¤Ç¤Ï²¿¤È¤â¤Ê¤¤¤¾ */
  }
  else {
    retval = d->nbytes = 0;
  }

  makeIchiranEchoStrCurChange(yc);
  makeIchiranKanjiStatusReturn(d, env, yc);

  freeGetIchiranList(yc->allkouho);
  
  popCallback(d);

  if (!cannaconf.stayAfterValidate && !d->more.todo) {
    d->more.todo = 1;
    d->more.ch = 0;
    d->more.fnum = CANNA_FN_Forward;
  }
  currentModeInfo(d);

  return(retval);
}

static int ichiranQuitCatch (uiContext, int, mode_context);

static int
ichiranQuitCatch(uiContext d, int retval, mode_context env)
{
  yomiContext yc;

  yc = (yomiContext)env;
  yc->kouhoCount = 0;

  if ((retval = RkwXfer(yc->context, yc->curIkouho)) == NG) {
    if(errno == EPIPE) {
      jrKanjiPipeError();
    }
    jrKanjiError = "\245\253\245\354\245\363\245\310\270\365\312\344\244\362"
	"\274\350\244\352\275\320\244\273\244\336\244\273\244\363\244\307"
	"\244\267\244\277";               
           /* ¥«¥ì¥ó¥È¸õÊä¤ò¼è¤ê½Ð¤»¤Þ¤»¤ó¤Ç¤·¤¿ */
    /* ¥«¥ì¥ó¥È¸õÊä¤¬¼è¤ê½Ð¤»¤Ê¤¤¤¯¤é¤¤¤Ç¤Ï²¿¤È¤â¤Ê¤¤¤¾ */
  }
  else {
    retval = d->nbytes = 0;
  }

  makeIchiranEchoStrCurChange(yc);
  makeIchiranKanjiStatusReturn(d, env, yc);

  freeGetIchiranList(yc->allkouho);

  popCallback(d);
  currentModeInfo(d);
  return(retval);
}

void
freeIchiranBuf(ichiranContext ic)
{
  if(ic->glinebufp)
    free(ic->glinebufp);
  if(ic->kouhoifp)
    free(ic->kouhoifp);
  if(ic->glineifp)
    free(ic->glineifp);
}

void
freeGetIchiranList(WCHAR_T **buf)
{
  /* ¸õÊä°ìÍ÷É½¼¨¹ÔÍÑ¤Î¥¨¥ê¥¢¤ò¥Õ¥ê¡¼¤¹¤ë */
  if(buf) {
    if(*buf) {
      free(*buf);
    }
    free(buf);
  }
}

static void
popIchiranMode(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec;

  d->modec = ic->next;
  d->current_mode = ic->prevMode;
  freeIchiranContext(ic);
}

/*
 * ¤¹¤Ù¤Æ¤Î¸õÊä¤ò¼è¤ê½Ð¤·¤Æ¡¢ÇÛÎó¤Ë¤¹¤ë
 */


WCHAR_T **
getIchiranList(int context, int *nelem, int *currentkouho)
{
  WCHAR_T *work, *wptr, **bptr, **buf;
  RkStat st;
  int i;

  /* RkwGetKanjiList ¤ÇÆÀ¤ë¡¢¤¹¤Ù¤Æ¤Î¸õÊä¤Î¤¿¤á¤ÎÎÎ°è¤òÆÀ¤ë */
  if ((work = (WCHAR_T *)malloc(ROMEBUFSIZE * sizeof(WCHAR_T)))
                                               == (WCHAR_T *)NULL) {
#ifndef WIN
    jrKanjiError = "malloc (getIchiranList) ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
#else
    jrKanjiError = "malloc (getIchiranList) \244\307\244\255\244\336\244\273\244\363\244\307\244\267\244\277";
#endif
    return (WCHAR_T **)NULL;
  }

  /* ¤¹¤Ù¤Æ¤Î¸õÊä¤òÆÀ¤ë¡£
     Îã: ¤±¤¤¤«¤ó ¢ª ·Ù´±@·Ê´Ñ@³Ý´§@@ (@¤ÏNULL) */
  if((*nelem = RkwGetKanjiList(context, work, ROMEBUFSIZE)) < 0) {
    jrKanjiError = "\244\271\244\331\244\306\244\316\270\365\312\344\244\316"
	"\274\350\244\352\275\320\244\267\244\313\274\272\307\324\244\267"
	"\244\336\244\267\244\277";
                   /* ¤¹¤Ù¤Æ¤Î¸õÊä¤Î¼è¤ê½Ð¤·¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    free(work);
    return (WCHAR_T **)NULL;
  }

#ifdef	INHIBIT_DUPLICATION
  if (*nelem == 3) {
    WCHAR_T *w1, *w2, *w3;

    w1 = work;
    w2 = w1 + WStrlen(w1);
    w3 = w2 + WStrlen(w2);
    if (!WStrcmp(w1, ++w3)) {
      if (!WStrcmp(w1, ++w2))
	*nelem = 1;
      else
	*nelem = 2;
    }
  }
#endif	/* INHIBIT_DUPLICATION */

  /* makeKouhoIchiran()¤ËÅÏ¤¹¥Ç¡¼¥¿ */
  if((buf = (WCHAR_T **)calloc
      (*nelem + 1, sizeof(WCHAR_T *))) == (WCHAR_T **)NULL) {
    jrKanjiError = "malloc (getIchiranList) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
                                            /* ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
    free(work);
    return (WCHAR_T **)NULL;
  }
  for(wptr = work, bptr = buf, i = 0; *wptr && i++ < *nelem; bptr++) {
    *bptr = wptr;
    while(*wptr++)
      /* EMPTY */
      ;
  }
  *bptr = (WCHAR_T *)0;

  if(RkwGetStat(context, &st) == -1) {
    jrKanjiError = "\245\271\245\306\245\244\245\277\245\271\244\362\274\350"
	"\244\352\275\320\244\273\244\336\244\273\244\363\244\307\244\267"
	"\244\277";
                   /* ¥¹¥Æ¥¤¥¿¥¹¤ò¼è¤ê½Ð¤»¤Þ¤»¤ó¤Ç¤·¤¿ */
    free(work);
    free(buf);
    return (WCHAR_T **)NULL;
  }
  *currentkouho = st.candnum; /* ¥«¥ì¥ó¥È¸õÊä¤Ï²¿ÈÖÌÜ¡© */

  return(buf);
}

/* cfunc ichiranContext
 *
 * ichiranContext ¸õÊä°ìÍ÷ÍÑ¤Î¹½Â¤ÂÎ¤òºî¤ê½é´ü²½¤¹¤ë
 *
 */
ichiranContext
newIchiranContext(void)
{
  ichiranContext icxt;

  if ((icxt = (ichiranContext)malloc(sizeof(ichiranContextRec)))
                                          == (ichiranContext)NULL) {
#ifndef WIN
    jrKanjiError = "malloc (newIchiranContext) ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
#else
    jrKanjiError = "malloc (newIchiranContext) \244\307\244\255\244\336"
	"\244\273\244\363\244\307\244\267\244\277";
#endif
    return (ichiranContext)NULL;
  }
  clearIchiranContext(icxt);

  return icxt;
}

/*
 * ¸õÊä°ìÍ÷¹Ô¤òºî¤ë
 */

int
selectOne(uiContext d, WCHAR_T **buf, int *ck, int nelem, int bangomax, unsigned inhibit, int currentkouho, int allowcallback, canna_callback_t everyTimeCallback, canna_callback_t exitCallback, canna_callback_t quitCallback, canna_callback_t auxCallback)
{
  extern KanjiModeRec ichiran_mode;
  ichiranContext ic;

  if (allowcallback != WITHOUT_LIST_CALLBACK &&
      d->list_func == NULL) {
    allowcallback = WITHOUT_LIST_CALLBACK;
  }

  if(pushCallback(d, d->modec,
	everyTimeCallback, exitCallback, quitCallback, auxCallback) == 0) {
    jrKanjiError = "malloc (pushCallback) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
                                          /* ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
    return(NG);
  }
  
  if((ic = newIchiranContext()) == (ichiranContext)NULL) {
    popCallback(d);
    return(NG);
  }
  ic->majorMode = d->majorMode;
  ic->next = d->modec;
  d->modec = (mode_context)ic;

  ic->prevMode = d->current_mode;
  d->current_mode = &ichiran_mode;
  d->flags &= ~(PLEASE_CLEAR_GLINE | PCG_RECOGNIZED);
  /* ¤³¤³¤Ë¤¯¤ëÄ¾Á°¤Ë C-t ¤È¤«¤¬ Gline ¤ËÉ½¼¨¤µ¤ì¤Æ¤¤¤ë¾ì¹ç¾å¤Î£±¹Ô¤ò
     ¤ä¤ëÉ¬Í×¤¬½Ð¤Æ¤¯¤ë¡£ */

  ic->allkouho = buf;
  ic->curIkouho = ck;
  ic->inhibit = inhibit;
  ic->nIkouho = nelem;

  if (allowcallback != WITHOUT_LIST_CALLBACK) {
    ic->flags |= ICHIRAN_ALLOW_CALLBACK;
    /* listcallback ¤Ç¤ÏÈÖ¹æ¤Ï¤Ä¤±¤Ê¤¤ */
    ic->inhibit |= NUMBERING;
  }

  if (allowcallback == WITHOUT_LIST_CALLBACK) {
    if (makeKouhoIchiran(d, nelem, bangomax, inhibit, currentkouho)   == NG) {
      popIchiranMode(d);
      popCallback(d);
      return(NG);
    }
  }
  else {
    if (cannaconf.kCount) {
      *(ic->curIkouho) += currentkouho;
      if (*(ic->curIkouho) >= ic->nIkouho)
        ic->svIkouho = *(ic->curIkouho) = 0;
    }
    d->list_func(d->client_data, CANNA_LIST_Start, buf, nelem, ic->curIkouho);
  }

  return(0);
}

/*
 * IchiranContext ¤Î½é´ü²½
 */
static void
clearIchiranContext(ichiranContext p)
{
  p->id = ICHIRAN_CONTEXT;
  p->svIkouho = 0;
  p->curIkouho = 0;
  p->nIkouho = 0;
  p->tooSmall = 0;
  p->curIchar = 0;
  p->allkouho = 0;
  p->glinebufp = 0;
  p->kouhoifp = (kouhoinfo *)0;
  p->glineifp = (glineinfo *)0;
  p->flags = (unsigned char)0;
}
  
/*
 * ¸õÊä°ìÍ÷¤Î¥Ç¡¼¥¿¹½Â¤ÂÎ¤òºî¤ë¤¿¤á¤ÎÎÎ°è¤ò³ÎÊÝ¤¹¤ë
 */
int allocIchiranBuf(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec;
  int size;

  /* ¥µ¥¤¥º¤ÎÊ¬¤ÈÈÖ¹æ¤ÎÊ¬¤ÎÎÎ°è¤òÆÀ¤ë*/
  size = ic->nIkouho * (d->ncolumns + 1) * WCHARSIZE; /* ¤¨¤¤¤ä¤Ã */
  if((ic->glinebufp = (WCHAR_T *)malloc(size)) ==  (WCHAR_T *)NULL) {
    jrKanjiError = "malloc (allocIchiranBuf) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
                                             /* ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
    return(NG);
  }

  /* kouhoinfo¤ÎÎÎ°è¤òÆÀ¤ë */
  size = (ic->nIkouho + 1) * sizeof(kouhoinfo);
  if((ic->kouhoifp = (kouhoinfo *)malloc(size)) == (kouhoinfo *)NULL) {
    jrKanjiError = "malloc (allocIchiranBuf) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
                                             /* ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
    free(ic->glinebufp);
    return(NG);
  }

  /* glineinfo¤ÎÎÎ°è¤òÆÀ¤ë */
  size = (ic->nIkouho + 1) * sizeof(glineinfo);
  if((ic->glineifp = (glineinfo *)malloc(size)) == (glineinfo *)NULL) {
    jrKanjiError = "malloc (allocIchiranBuf) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
                                             /* ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
    free(ic->glinebufp);
    free(ic->kouhoifp);
    return(NG);
  }
  return(0);
}

/*
 * ¸õÊä°ìÍ÷¹Ô¤òÉ½¼¨ÍÑ¤Î¥Ç¡¼¥¿¤ò¥Æ¡¼¥Ö¥ë¤ËºîÀ®¤¹¤ë
 *
 * ¡¦glineinfo ¤È kouhoinfo¤òºîÀ®¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
static int
makeKouhoIchiran(uiContext d, int nelem, int bangomax, unsigned char inhibit, int currentkouho)
{
  ichiranContext ic = (ichiranContext)d->modec;
  WCHAR_T **kkptr, *kptr, *gptr, *svgptr;
  int           ko, lnko, cn = 0, svcn, line = 0, dn = 0, svdn;
  int netwidth;

  netwidth = d->ncolumns -
    (cannaconf.kCount ? (DEC_COLUMNS(nelem) * 2 + 2/* 2¤Ï/¤ÈSP¤ÎÊ¬ */) : 0);

  ic->nIkouho = nelem;	/* ¸õÊä¤Î¿ô */

  /* ¥«¥ì¥ó¥È¸õÊä¤ò¥»¥Ã¥È¤¹¤ë */
  ic->svIkouho = *(ic->curIkouho);
  *(ic->curIkouho) += currentkouho;
  if(*(ic->curIkouho) >= ic->nIkouho)
    ic->svIkouho = *(ic->curIkouho) = 0;

  if(allocIchiranBuf(d) == NG)
    return(NG);

  if(d->ncolumns < 1) {
    ic->tooSmall = 1;
    return(0);
  }

  /* glineinfo¤Èkouhoinfo¤òºî¤ë */
  /* 
   ¡öglineinfo¡ö
      int glkosu   : int glhead     : int gllen  : WCHAR_T *gldata
      £±¹Ô¤Î¸õÊä¿ô : ÀèÆ¬¸õÊä¤¬     : £±¹Ô¤ÎÄ¹¤µ : ¸õÊä°ìÍ÷¹Ô¤ÎÊ¸»úÎó
                   : ²¿ÈÖÌÜ¤Î¸õÊä¤« :
   -------------------------------------------------------------------------
   0 | 6           : 0              : 24         : £±¿·£²¿´£³¿Ê£´¿¿£µ¿À£¶¿®
   1 | 4           : 6              : 16         : £±¿Ã£²¿²£³¿­£´¿Ä

    ¡ökouhoinfo¡ö
      int khretsu  : int khpoint  : WCHAR_T *khdata
      ¤Ê¤óÎóÌÜ¤Ë   : ¹Ô¤ÎÀèÆ¬¤«¤é : ¸õÊä¤ÎÊ¸»úÎó
      ¤¢¤ë¸õÊä¤«   : ²¿¥Ð¥¤¥ÈÌÜ¤« :
   -------------------------------------------------------------------------
   0 | 0           : 0            : ¿·
   1 | 0           : 4            : ¿´
             :                :             :
   7 | 1           : 0            : ¿Ã
   8 | 1           : 4            : ¿²
  */

  kkptr = ic->allkouho;
  kptr = *(ic->allkouho);
  gptr = ic->glinebufp;

  /* line -- ²¿ÎóÌÜ¤«
     ko   -- Á´ÂÎ¤ÎÀèÆ¬¤«¤é²¿ÈÖÌÜ¤Î¸õÊä¤«
     lnko -- Îó¤ÎÀèÆ¬¤«¤é²¿ÈÖÌÜ¤Î¸õÊä¤«
     cn   -- Îó¤ÎÀèÆ¬¤«¤é²¿¥Ð¥¤¥ÈÌÜ¤« */

  for(line=0, ko=0; ko<ic->nIkouho; line++) {
    ic->glineifp[line].gldata = gptr; /* ¸õÊä¹Ô¤òÉ½¼¨¤¹¤ë¤¿¤á¤ÎÊ¸»úÎó */
    ic->glineifp[line].glhead = ko;   /* ¤³¤Î¹Ô¤ÎÀèÆ¬¸õÊä¤Ï¡¢Á´ÂÎ¤Ç¤ÎkoÈÖÌÜ */

    ic->tooSmall = 1;
    for (lnko = cn = dn = 0 ;
	dn < netwidth && lnko < bangomax && ko < ic->nIkouho ; lnko++, ko++) {
      ic->tooSmall = 0;
      kptr = kkptr[ko];
      ic->kouhoifp[ko].khretsu = line; /* ²¿¹ÔÌÜ¤ËÂ¸ºß¤¹¤ë¤«¤òµ­Ï¿ */
      ic->kouhoifp[ko].khpoint = cn + (lnko ? 1 : 0);
      ic->kouhoifp[ko].khdata = kptr;  /* ¤½¤ÎÊ¸»úÎó¤Ø¤Î¥Ý¥¤¥ó¥¿ */
      svgptr = gptr;
      svcn = cn;
      svdn = dn;
      /* £²¼ïÎà¤ÎÉ½¼¨¤òÊ¬¤±¤ë */
      if(!(inhibit & (unsigned char)NUMBERING)) {
	/* ÈÖ¹æ¤ò¥³¥Ô¡¼¤¹¤ë */
	if (!cannaconf.indexHankaku) {/* Á´³Ñ */
	  if(lnko == 0) {
	    *gptr++ = *bango; cn ++; dn +=2;
	  } else {
	    WStrncpy(gptr, bango + 1 + BANGOSIZE * (lnko - 1), BANGOSIZE);
	    cn += BANGOSIZE; gptr += BANGOSIZE, dn += BANGOSIZE*2;
	  }
	}
	else{ /* È¾³Ñ */
	  WStrcpy(gptr, bango2[lnko]);
	  if(lnko == 0) {
	    dn +=2;
	  } else {
	    dn +=4;
	  }
	  cn += WStrlen(bango2[lnko]);
	  gptr += WStrlen(bango2[lnko]);
	}
      } else {
	/* ¶õÇò¤ò¥³¥Ô¡¼¤¹¤ë */
	if(lnko) {
	  *gptr++ = *kuuhaku; cn ++; dn +=2;
	}
      }
      /* ¸õÊä¤ò¥³¥Ô¡¼¤¹¤ë */
      for (; *kptr && dn < netwidth ; gptr++, kptr++, cn++) {
	*gptr = *kptr;
	if (WIsG0(*gptr))
	  dn++;
	else if (WIsG1(*gptr))
	  dn += 2;
	else if (WIsG2(*gptr))
	  dn ++;
	else if (WIsG3(*gptr))
	  dn += 2;
      }

      /* ¥«¥é¥à¿ô¤è¤ê¤Ï¤ß¤À¤·¤Æ¤·¤Þ¤¤¤½¤¦¤Ë¤Ê¤Ã¤¿¤Î¤Ç£±¤ÄÌá¤¹ */
      if (dn >= netwidth) {
	if (lnko) {
	  gptr = svgptr;
	  cn = svcn;
	  dn = svdn;
	}
	else {
	  ic->tooSmall = 1;
	}
	break;
      }
    }
    if (ic->tooSmall) {
      return 0;
    }
    if (cannaconf.kCount) {
      for (;dn < d->ncolumns - 1; dn++)
	*gptr++ = ' ';
    }
    /* £±¹Ô½ª¤ï¤ê */
    *gptr++ = 0;
    ic->glineifp[line].glkosu = lnko;
    ic->glineifp[line].gllen = WStrlen(ic->glineifp[line].gldata);
  }

  /* ºÇ¸å¤ËNULL¤òÆþ¤ì¤ë */
  ic->kouhoifp[ko].khretsu = 0;
  ic->kouhoifp[ko].khpoint = 0;
  ic->kouhoifp[ko].khdata  = (WCHAR_T *)NULL;
  ic->glineifp[line].glkosu  = 0;
  ic->glineifp[line].glhead  = 0;
  ic->glineifp[line].gllen   = 0;
  ic->glineifp[line].gldata  = (WCHAR_T *)NULL;

#if defined(DEBUG) && !defined(WIN)
  if (iroha_debug) {
    int i;
    for(i=0; ic->glineifp[i].glkosu; i++)
      printf("%d: %s\n", i, ic->glineifp[i].gldata);
  }
#endif

  return(0);
}

int tanKouhoIchiran(uiContext d, int step)
{
  yomiContext yc = (yomiContext)d->modec;
  ichiranContext ic;
  int nelem, currentkouho, retval = 0;
  unsigned inhibit = 0;
  unsigned char listcallback = (unsigned char)(d->list_func ? 1 : 0);
  int netwidth;

  netwidth = d->ncolumns -
    (cannaconf.kCount ? (DEC_COLUMNS(9999) * 2 + 2/* 2¤Ï / ¤È SP ¤ÎÊ¬ */) : 0);

  /* ¸õÊä°ìÍ÷¹Ô¤¬¶¹¤¯¤Æ¸õÊä°ìÍ÷¤¬½Ð¤»¤Ê¤¤ */
  if (listcallback == 0 && netwidth < 2) {
    /* tooSmall */
    return TanNextKouho(d);
  }

  /* Ãà¼¡´ØÏ¢ */
  yc->status |= CHIKUJI_OVERWRAP;

  /* ¤¹¤Ù¤Æ¤Î¸õÊä¤ò¼è¤ê½Ð¤¹ */
  yc->allkouho = getIchiranList(yc->context, &nelem, &currentkouho);
  if (yc->allkouho == 0) {
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
    TanMuhenkan(d);
    makeGLineMessageFromString(d, jrKanjiError);
    return 0;
  }

  if (!cannaconf.HexkeySelect) {
    inhibit |= (unsigned char)NUMBERING;
  }

  yc->curIkouho = currentkouho;	/* ¸½ºß¤Î¥«¥ì¥ó¥È¸õÊäÈÖ¹æ¤òÊÝÂ¸¤¹¤ë */
  currentkouho = step;	/* ¥«¥ì¥ó¥È¸õÊä¤«¤é²¿ÈÖÌÜ¤ò¥«¥ì¥ó¥È¸õÊä¤È¤¹¤ë¤« */

  /* ¸õÊä°ìÍ÷¤Ë°Ü¹Ô¤¹¤ë */
  if ((retval = selectOne(d, yc->allkouho, &yc->curIkouho, nelem, BANGOMAX,
			  inhibit, currentkouho, WITH_LIST_CALLBACK,
			  ichiranEveryTimeCatch, ichiranExitCatch,
			  ichiranQuitCatch, NO_CALLBACK)) == NG) {
    freeGetIchiranList(yc->allkouho);
    return GLineNGReturn(d);
  }

  ic = (ichiranContext)d->modec;
  if (ic->tooSmall) {
    freeGetIchiranList(yc->allkouho);
    popIchiranMode(d);
    popCallback(d);
    return TanNextKouho(d);
  }

  ic->minorMode = CANNA_MODE_IchiranMode;
  currentModeInfo(d);

  if ( !(ic->flags & ICHIRAN_ALLOW_CALLBACK) ) {
    makeGlineStatus(d);
  }
  /* d->status = EVERYTIME_CALLBACK; */

  return(retval);
}

/*
 * ¸õÊä°ìÍ÷¹Ô¤ÎÉ½¼¨¤ò¶¯À©½ªÎ»¤¹¤ë
 */
int IchiranQuit(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec;
  int retval = 0;

  if (ic->flags & ICHIRAN_ALLOW_CALLBACK &&
      d->list_func) {
    if (ic->flags & ICHIRAN_NEXT_EXIT) {
      d->list_func(d->client_data,
                     CANNA_LIST_Select, (WCHAR_T **)0, 0, (int *)0);
    }
    else {
      d->list_func(d->client_data,
                     CANNA_LIST_Quit, (WCHAR_T **)0, 0, (int *)0);
    }
  }
  
  if (ic->flags & ICHIRAN_NEXT_EXIT) {
    ichiranFin(d); 
    d->status = EXIT_CALLBACK;
  }
  else {
    *(ic->curIkouho) = ic->nIkouho - 1; /* ¤Ò¤é¤¬¤Ê¸õÊä¤Ë¤¹¤ë */
    ichiranFin(d); 
    d->status = QUIT_CALLBACK;
  }
  return(retval);
}

int
IchiranNop(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec;

  if ((ic->flags & ICHIRAN_ALLOW_CALLBACK) && d->list_func) {
    (*d->list_func)
      (d->client_data, CANNA_LIST_Query, (WCHAR_T **)0, 0, (int *)0);
  }

  /* currentModeInfo ¤Ç¥â¡¼¥É¾ðÊó¤¬É¬¤ºÊÖ¤ë¤è¤¦¤Ë¥À¥ß¡¼¤Î¥â¡¼¥É¤òÆþ¤ì¤Æ¤ª¤¯ */
  d->majorMode = d->minorMode = CANNA_MODE_AlphaMode;
  currentModeInfo(d);

  if (!(ic->flags & ICHIRAN_ALLOW_CALLBACK)) {
    makeGlineStatus(d);
  }
  return 0;
}

/*
   IchiranKakuteiThenDo

     -- Do determine from the candidate list, then do one more function.
 */

static int
IchiranKakuteiThenDo(uiContext d, int func)
{
  ichiranContext ic = (ichiranContext)d->modec;
  int retval;
  BYTE ifl = ic->flags;

  if (!ic->prevMode || !ic->prevMode->func ||
      !(*ic->prevMode->func)((uiContext)0/*dummy*/, ic->prevMode, KEY_CHECK,
			     0/*dummy*/, func)) {
    return NothingChangedWithBeep(d);
  }
  retval = IchiranKakutei(d);
  if (ifl & ICHIRAN_STAY_LONG) {
    (void)IchiranQuit(d);
  }
  d->more.todo = 1;
  d->more.ch = d->ch;
  d->more.fnum = func;
  return retval;
}

static
int IchiranQuitThenDo(uiContext d, int func)
{
  ichiranContext ic = (ichiranContext)d->modec;
  int retval;

  if (!ic->prevMode || !ic->prevMode->func ||
      !(*ic->prevMode->func)((uiContext)0/*dummy*/, ic->prevMode, KEY_CHECK,
			     0/*dummy*/, func)) {
    return NothingChangedWithBeep(d);
  }
  retval = IchiranQuit(d);
  d->more.todo = 1;
  d->more.ch = d->ch;
  d->more.fnum = func;
  return retval;
}

/*
 * ¼¡¸õÊä¤Ë°ÜÆ°¤¹¤ë
 *
 * ¡¦¥«¥ì¥ó¥È¸õÊä¤¬ºÇ½ª¸õÊä¤À¤Ã¤¿¤éÀèÆ¬¸õÊä¤ò¥«¥ì¥ó¥È¸õÊä¤È¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
int IchiranForwardKouho(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec;

  if (ic->flags & ICHIRAN_ALLOW_CALLBACK &&
      d->list_func) {
    int res;
    
    res = (*d->list_func)
      (d->client_data, CANNA_LIST_Forward, (WCHAR_T **)0, 0, (int *)0);
    if (res) {
      return 0;
    }
    else { /* CANNA_LIST_Forward was not prepared at the callback func */
      return IchiranKakuteiThenDo(d, CANNA_FN_Forward);
    }
  }

  /* ¼¡¸õÊä¤Ë¤¹¤ë (Ã±¸ì¸õÊä°ìÍ÷¾õÂÖ¤Ç¡¢ºÇ¸å¤Î¸õÊä¤À¤Ã¤¿¤é°ìÍ÷¤ò¤ä¤á¤ë) */
  *(ic->curIkouho) += 1;
  if(*(ic->curIkouho) >= ic->nIkouho) {
    if (cannaconf.QuitIchiranIfEnd
       && (((coreContext)d->modec)->minorMode == CANNA_MODE_IchiranMode)) {
      return(IchiranQuit(d));
    }
    else if (cannaconf.CursorWrap) {
      *(ic->curIkouho) = 0;
    } else {
      *(ic->curIkouho) -= 1;
      return NothingChangedWithBeep(d);
    }
  }

  if(ic->tooSmall) { /* for bushu */
    d->status = AUX_CALLBACK;
    return 0;
  }

  makeGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return 0;
}

/*
 * Á°¸õÊä¤Ë°ÜÆ°¤¹¤ë
 *
 * ¡¦¥«¥ì¥ó¥È¸õÊä¤¬ÀèÆ¬¸õÊä¤À¤Ã¤¿¤éºÇ½ª¸õÊä¤ò¥«¥ì¥ó¥È¸õÊä¤È¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
int IchiranBackwardKouho(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec;
  BYTE mode;

  if (ic->flags & ICHIRAN_ALLOW_CALLBACK &&
      d->list_func) {
    int res;
    res = (*d->list_func)
      (d->client_data, CANNA_LIST_Backward, (WCHAR_T **)0, 0, (int *)0);
    if (res) {
      return 0;
    }
    else { /* CANNA_LIST_Backward was not prepared at the callback func */
      return IchiranKakuteiThenDo(d, CANNA_FN_Backward);
    }
  }

  /* ¸½ºß¤Î¥â¡¼¥É¤òµá¤á¤ë */
  if (cannaconf.QuitIchiranIfEnd)
    mode = ((coreContext)d->modec)->minorMode;

  /* Á°¸õÊä¤Ë¤¹¤ë (Ã±¸ì¸õÊä°ìÍ÷¾õÂÖ¤Ç¡¢ºÇ½é¤Î¸õÊä¤À¤Ã¤¿¤é°ìÍ÷¤ò¤ä¤á¤ë) */
  if(*(ic->curIkouho))
    *(ic->curIkouho) -= 1;
  else {
    if (cannaconf.QuitIchiranIfEnd && (mode == CANNA_MODE_IchiranMode)) {
      return(IchiranQuit(d));
    }
    else if (cannaconf.CursorWrap) {
      *(ic->curIkouho) = ic->nIkouho - 1;
    } else {
      *(ic->curIkouho) = 0;
      return NothingChangedWithBeep(d);
    }
  }

  if(ic->tooSmall) { /* for bushu */
    d->status = AUX_CALLBACK;
    return 0;
  }

  makeGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return 0;
}

/*
   IchiranConvert() will be called when `convert' key is pressed
 */

static int IchiranConvert (uiContext);

static
int IchiranConvert(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec;

  if (ic->flags & ICHIRAN_ALLOW_CALLBACK && d->list_func) {
    (*d->list_func)
      (d->client_data, CANNA_LIST_Convert, (WCHAR_T **)0, 0, (int *)0);
    return 0;
  }
  else {
    return IchiranForwardKouho(d);
  }
}

/*
 * Á°¸õÊäÎó¤Ë°ÜÆ°¤¹¤ë
 *
 * ¡¦¥«¥ì¥ó¥È¸õÊä¤òµá¤á¤Æ¸õÊä°ìÍ÷¤È¤½¤Î¾ì¤Î¸õÊä¤òÉ½¼¨¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
int IchiranPreviousKouhoretsu(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec;

  if (ic->flags & ICHIRAN_ALLOW_CALLBACK &&
      d->list_func) {
    int res;
    res = (*d->list_func)
      (d->client_data, CANNA_LIST_Prev, (WCHAR_T **)0, 0, (int *)0);
    if (res) {
      return 0;
    }
    else { /* CANNA_LIST_Backward was not prepared at the callback func */
      return IchiranKakuteiThenDo(d, CANNA_FN_Prev);
    }
  }

  if(ic->tooSmall) { /* for bushu */
    return(IchiranBackwardKouho(d));
  }

  /* Á°¸õÊäÎó¤Ë¤¹¤ë (*(ic->curIkouho)¤òµá¤á¤ë)*/
  getIchiranPreviousKouhoretsu(d);

  makeGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return 0;
}

/*
 * Á°¸õÊäÎó¤Î¥«¥ì¥ó¥È¸õÊä¤òµá¤á¤ë
 *
 * ¡¦Á°¸õÊäÎóÃæ¤ÎÆ±¤¸¸õÊäÈÖ¹æ¤Î¤â¤Î¤ò¥«¥ì¥ó¥È¸õÊä¤È¤¹¤ë
 * ¡¦¸õÊäÈÖ¹æ¤ÎÆ±¤¸¤â¤Î¤¬¤Ê¤¤»þ¤Ï¡¢¤½¤Î¸õÊäÃæ¤ÎºÇ½ª¸õÊä¤ò¥«¥ì¥ó¥È¸õÊä¤È¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
static void
getIchiranPreviousKouhoretsu(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec;
  int kindex;
  int curretsu, nretsu;

  /* ¥«¥ì¥ó¥È¸õÊä¹Ô¤Î¤Ê¤«¤Ç²¿ÈÖÌÜ¤Î¸õÊä¤«¤Ê¤Î¤«¤òÆÀ¤ë */
  kindex = *(ic->curIkouho) - 
    ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].glhead;
  /* Á°¸õÊäÎó¤òÆÀ¤ë */
  curretsu = ic->kouhoifp[*(ic->curIkouho)].khretsu;
  nretsu = ic->kouhoifp[ic->nIkouho - 1].khretsu + 1;
  if(curretsu == 0) {
    if (cannaconf.CursorWrap)
      curretsu = nretsu;
    else {
      NothingChangedWithBeep(d);
      return;
    }
  }
  curretsu -= 1;
  /* kindex ¤¬¥«¥ì¥ó¥È¸õÊäÎó¤Î¸õÊä¿ô¤è¤êÂç¤­¤¯¤Ê¤Ã¤Æ¤·¤Þ¤Ã¤¿¤é
     ºÇ±¦¸õÊä¤ò¥«¥ì¥ó¥È¸õÊä¤È¤¹¤ë */
  if(ic->glineifp[curretsu].glkosu <= kindex) 
    kindex = ic->glineifp[curretsu].glkosu - 1;
  /* Á°¸õÊäÎó¤ÎÆ±¤¸ÈÖ¹æ¤Ë°ÜÆ°¤¹¤ë */
  *(ic->curIkouho) = kindex + ic->glineifp[curretsu].glhead;
  return;
}

/*
 * ¼¡¸õÊäÎó¤Ë°ÜÆ°¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
int IchiranNextKouhoretsu(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec;

  if (ic->flags & ICHIRAN_ALLOW_CALLBACK &&
      d->list_func) {
    int res;
    res = (*d->list_func)
      (d->client_data, CANNA_LIST_Next, (WCHAR_T **)0, 0, (int *)0);
    if (res) {
      return 0;
    }
    else { /* CANNA_LIST_Backward was not prepared at the callback func */
      return IchiranKakuteiThenDo(d, CANNA_FN_Next);
    }
  }

  if(ic->tooSmall) {
    return(IchiranForwardKouho(d));
  }

  /* ¼¡¸õÊäÎó¤Ë¤¹¤ë (*(ic->curIkouho) ¤òµá¤á¤ë) */
  getIchiranNextKouhoretsu(d);

  makeGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return 0;
}

/*
 * ¼¡¸õÊäÊÇ¤Ë°ÜÆ°¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */

static int IchiranNextPage (uiContext);

static
int IchiranNextPage(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec;

  if (ic->flags & ICHIRAN_ALLOW_CALLBACK &&
      d->list_func) {
    int res;
    res = (*d->list_func)
      (d->client_data, CANNA_LIST_PageDown, (WCHAR_T **)0, 0, (int *)0);
    if (res) {
      return 0;
    }
    else { /* CANNA_LIST_Backward was not prepared at the callback func */
      return IchiranKakuteiThenDo(d, CANNA_FN_PageDown);
    }
  }

  return IchiranNextKouhoretsu(d);
}

/*
 * Á°¸õÊäÊÇ¤Ë°ÜÆ°¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */

static int IchiranPreviousPage (uiContext);

static
int IchiranPreviousPage(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec;

  if (ic->flags & ICHIRAN_ALLOW_CALLBACK &&
      d->list_func) {
    int res;
    res = (*d->list_func)
      (d->client_data, CANNA_LIST_PageUp, (WCHAR_T **)0, 0, (int *)0);
    if (res) {
      return 0;
    }
    else { /* CANNA_LIST_Backward was not prepared at the callback func */
      return IchiranKakuteiThenDo(d, CANNA_FN_PageUp);
    }
  }

  return IchiranPreviousKouhoretsu(d);
}

/*
 * ¼¡¸õÊäÎó¤Ë°ÜÆ°¤¹¤ë
 *
 * ¡¦¼¡¸õÊäÎóÃæ¤ÎÆ±¤¸¸õÊäÈÖ¹æ¤Î¤â¤Î¤ò¥«¥ì¥ó¥È¸õÊä¤È¤¹¤ë
 * ¡¦¸õÊäÈÖ¹æ¤ÎÆ±¤¸¤â¤Î¤¬¤Ê¤¤»þ¤Ï¡¢¤½¤Î¸õÊäÃæ¤ÎºÇ½ª¸õÊä¤ò¥«¥ì¥ó¥È¸õÊä¤È¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
static void
getIchiranNextKouhoretsu(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec;
  int kindex;
  int curretsu, nretsu;

  /* ¥«¥ì¥ó¥È¸õÊä¹Ô¤Î¤Ê¤«¤Ç²¿ÈÖÌÜ¤Î¸õÊä¤«¤Ê¤Î¤«¤òÆÀ¤ë */
  kindex = *(ic->curIkouho) - 
    ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].glhead;
  /* ¼¡¸õÊäÎó¤òÆÀ¤ë */
  curretsu = ic->kouhoifp[*(ic->curIkouho)].khretsu;
  nretsu = ic->kouhoifp[ic->nIkouho - 1].khretsu + 1;
  curretsu += 1;
  if(curretsu >= nretsu) {
    if (cannaconf.CursorWrap)
      curretsu = 0;
    else {
      NothingChangedWithBeep(d);
      return;
    }
  }
  /* kindex ¤¬¥«¥ì¥ó¥È¸õÊäÎó¤Î¸õÊä¿ô¤è¤êÂç¤­¤¯¤Ê¤Ã¤Æ¤·¤Þ¤Ã¤¿¤é
     ºÇ±¦¸õÊä¤ò¥«¥ì¥ó¥È¸õÊä¤È¤¹¤ë */
  if(ic->glineifp[curretsu].glkosu <= kindex) 
    kindex = ic->glineifp[curretsu].glkosu - 1;
  /* Á°¸õÊäÎó¤ÎÆ±¤¸ÈÖ¹æ¤Ë°ÜÆ°¤¹¤ë */
  *(ic->curIkouho) = kindex + ic->glineifp[curretsu].glhead;
  return;
}

/*
 * ¸õÊä¹ÔÃæ¤ÎÀèÆ¬¸õÊä¤Ë°ÜÆ°¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
int IchiranBeginningOfKouho(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec;

  if (ic->flags & ICHIRAN_ALLOW_CALLBACK &&
      d->list_func) {
    int res;
    res = (*d->list_func)
      (d->client_data, CANNA_LIST_BeginningOfLine, (WCHAR_T **)0, 0,(int *)0);
    if (res) {
      return 0;
    }
    else { /* CANNA_LIST_Backward was not prepared at the callback func */
      return IchiranKakuteiThenDo(d, CANNA_FN_BeginningOfLine);
    }
  }

  if(ic->tooSmall) {
    d->status = AUX_CALLBACK;
    return 0;
  }

  /* ¸õÊäÎó¤ÎÀèÆ¬¸õÊä¤ò¥«¥ì¥ó¥È¸õÊä¤Ë¤¹¤ë */
  *(ic->curIkouho) = 
    ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].glhead;

  makeGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return 0;
}

/*
 * ¸õÊä¹ÔÃæ¤ÎºÇ±¦¸õÊä¤Ë°ÜÆ°¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
int IchiranEndOfKouho(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec;

  if (ic->flags & ICHIRAN_ALLOW_CALLBACK &&
      d->list_func) {
    int res;
    res = (*d->list_func)
      (d->client_data, CANNA_LIST_EndOfLine, (WCHAR_T **)0, 0, (int *)0);
    if (res) {
      return 0;
    }
    else { /* CANNA_LIST_Backward was not prepared at the callback func */
      return IchiranKakuteiThenDo(d, CANNA_FN_EndOfLine);
    }
  }

  if(ic->tooSmall) {
    d->status = AUX_CALLBACK;
    return 0;
  }

  /* ¸õÊäÎó¤ÎºÇ±¦¸õÊä¤ò¥«¥ì¥ó¥È¸õÊä¤Ë¤¹¤ë */
  *(ic->curIkouho) = 
    ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].glhead
    + ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].glkosu - 1;

  makeGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return 0;
}

/*
 * ¸õÊä¹ÔÃæ¤ÎÆþÎÏ¤µ¤ì¤¿ÈÖ¹æ¤Î¸õÊä¤Ë°ÜÆ°¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */

static int getIchiranBangoKouho (uiContext);
static int IchiranBangoKouho (uiContext);

static
int IchiranBangoKouho(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec;
  int zflag, retval = 0;

  if(ic->tooSmall) {
    d->status = AUX_CALLBACK;
    return(retval);
  }

  /* d->status = EVERYTIME_CALLBACK; */

  if (cannaconf.HexkeySelect && !(ic->inhibit & NUMBERING)) {
    /* ÆþÎÏ¤µ¤ì¤¿ÈÖ¹æ¤Î¸õÊä¤ò¥«¥ì¥ó¥È¸õÊä¤È¤¹¤ë */
    if((zflag = getIchiranBangoKouho(d)) == NG)
      goto insert;

    /* SelectDirect ¤Î¥«¥¹¥¿¥Þ¥¤¥º¤Î½èÍý */
  do_selection:
    if (cannaconf.SelectDirect) /* ON */ {
      if(zflag) /* £°¤¬ÆþÎÏ¤µ¤ì¤¿ */
	retval = IchiranQuit(d);
      else
	retval = IchiranKakutei(d);
    } else {          /* OFF */
      makeGlineStatus(d);
      /* d->status = EVERYTIME_CALLBACK; */
    }
    return(retval);
  }
  else {
#ifdef CANNA_LIST_Insert /* ÀäÂÐÄêµÁ¤µ¤ì¤Æ¤¤¤ë¤ó¤À¤±¤É¤Í */
    if (ic->flags & ICHIRAN_ALLOW_CALLBACK && d->list_func) {
      int res = (*d->list_func) /* list_func ¤ò¸Æ¤Ó½Ð¤¹ */
	(d->client_data, CANNA_LIST_Insert, (WCHAR_T **)0, d->ch, (int *)0);
      if (res) { /* d->ch ¤¬¥¢¥×¥ê¥±¡¼¥·¥ç¥óÂ¦¤Ç½èÍý¤µ¤ì¤¿ */
	if (res == CANNA_FN_FunctionalInsert) {
	  zflag = 1; /* 0 ¤¸¤ã¤Ê¤±¤ì¤Ð¤¤¤¤ */
	  goto do_selection;
	}
	else if (res != CANNA_FN_Nop) {
	  /* ¥¢¥×¥ê¥±¡¼¥·¥ç¥óÂ¦¤«¤éÍ×µá¤·¤ÆÍè¤¿µ¡Ç½¤òÂ³¤±¤Æ¼Â¹Ô¤¹¤ë */
	  d->more.todo = 1;
	  d->more.ch = d->ch;
	  d->more.fnum = CANNA_FN_FunctionalInsert;
	}
	return 0;
      }else{ /* CANNA_LIST_Insert was not processed at the callback func */
	/* continue to the 'insert:' tag.. */
      }
    }
#endif

  insert:
    if(!(ic->inhibit & CHARINSERT) && cannaconf.allowNextInput) {
      BYTE ifl = ic->flags;
      retval = IchiranKakutei(d);
      if (ifl & ICHIRAN_STAY_LONG) {
	(void)IchiranQuit(d);
      }
      d->more.todo = 1;
      d->more.ch = d->ch;
      d->more.fnum = CANNA_FN_FunctionalInsert;
    } else {
      NothingChangedWithBeep(d);
    }
    return(retval);
  }
}

/*
 * ¸õÊä¹ÔÃæ¤ÎÆþÎÏ¤µ¤ì¤¿ÈÖ¹æ¤Î¸õÊä¤Ë°ÜÆ°¤¹¤ë
 *
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	£°¤¬ÆþÎÏ¤µ¤ì¤¿¤é              £±¤òÊÖ¤¹
 * 		£±¡Á£¹¡¢£á¡Á£æ¤¬ÆþÎÏ¤µ¤ì¤¿¤é  £°¤òÊÖ¤¹
 * 		¥¨¥é¡¼¤À¤Ã¤¿¤é              ¡¼£±¤òÊÖ¤¹
 */
static int
getIchiranBangoKouho(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec;
  int num, kindex;

  /* ÆþÎÏ¥Ç¡¼¥¿¤Ï £°¡Á£¹ £á¡Á£æ ¤«¡© */
  if(((0x30 <= d->ch) && (d->ch <= 0x39))
     || ((0x61 <= d->ch) && (d->ch <= 0x66))) {
    if((0x30 <= d->ch) && (d->ch <= 0x39))
      num = (int)(d->ch & 0x0f);
    else if((0x61 <= d->ch) && (d->ch <= 0x66))
      num = (int)(d->ch - 0x57);
  } 
  else {
    /* ÆþÎÏ¤µ¤ì¤¿ÈÖ¹æ¤ÏÀµ¤·¤¯¤¢¤ê¤Þ¤»¤ó */
    return(NG);
  }
  /* ÆþÎÏ¥Ç¡¼¥¿¤Ï ¸õÊä¹Ô¤ÎÃæ¤ËÂ¸ºß¤¹¤ë¿ô¤«¡© */
  if(num > ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].glkosu) {
    /* ÆþÎÏ¤µ¤ì¤¿ÈÖ¹æ¤ÏÀµ¤·¤¯¤¢¤ê¤Þ¤»¤ó */
    return(NG);
  }

  /* ÆþÎÏ¤µ¤ì¤¿¿ô¤¬£°¤Ç SelectDirect ¤¬ ON ¤Ê¤éÆÉ¤ß¤ËÌá¤·¤Æ£±¤òÊÖ¤¹ */
  if(num == 0) {
    if (cannaconf.SelectDirect)
      return(1);
    else {
      /* ÆþÎÏ¤µ¤ì¤¿ÈÖ¹æ¤ÏÀµ¤·¤¯¤¢¤ê¤Þ¤»¤ó */
      return(NG);
    }  
  } else {
    /* ¸õÊäÎó¤ÎÀèÆ¬¸õÊä¤òÆÀ¤ë */
    kindex = ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].glhead;
    *(ic->curIkouho) = kindex + (num - 1);
  }

  return(0);
}

/*
 * ¸õÊä¹ÔÃæ¤«¤éÁªÂò¤µ¤ì¤¿¸õÊä¤ò¥«¥ì¥ó¥È¸õÊä¤È¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */

// static IchiranKakutei (uiContext);

static int
IchiranKakutei(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec;
  int retval = 0;
  WCHAR_T *kakuteiStrings;

  if ((ic->flags & ICHIRAN_ALLOW_CALLBACK) && d->list_func) {
    if (ic->flags & ICHIRAN_STAY_LONG) {
      d->list_func(d->client_data,
                     CANNA_LIST_Query, (WCHAR_T **)0, 0, (int *)0);
    }
    else {
      d->list_func(d->client_data,
                     CANNA_LIST_Select, (WCHAR_T **)0, 0, (int *)0);
    }
  }

  kakuteiStrings = ic->allkouho[*(ic->curIkouho)];
  retval = d->nbytes = WStrlen(kakuteiStrings);
  WStrcpy(d->buffer_return, kakuteiStrings);

  if (ic->flags & ICHIRAN_STAY_LONG) {
    ic->flags |= ICHIRAN_NEXT_EXIT; 
    d->status = EVERYTIME_CALLBACK;
  }
  else {
    ichiranFin(d);

    d->status = EXIT_CALLBACK;
  }

  return(retval);
}

/*
 * ¸õÊä¹ÔÉ½¼¨¥â¡¼¥É¤«¤éÈ´¤±¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
void
ichiranFin(uiContext d)
{
  ichiranContext ic = (ichiranContext)d->modec; 

  /* ¸õÊä°ìÍ÷É½¼¨¹ÔÍÑ¤Î¥¨¥ê¥¢¤ò¥Õ¥ê¡¼¤¹¤ë */
  freeIchiranBuf(ic);

  popIchiranMode(d);

  /* gline ¤ò¥¯¥ê¥¢¤¹¤ë */
  GlineClear(d);
}

static int IchiranExtendBunsetsu (uiContext);

static
int IchiranExtendBunsetsu(uiContext d)
{
  return IchiranQuitThenDo(d, CANNA_FN_Extend);
}

static int IchiranShrinkBunsetsu (uiContext);

static
int IchiranShrinkBunsetsu(uiContext d)
{
  return IchiranQuitThenDo(d, CANNA_FN_Shrink);
}

static int IchiranAdjustBunsetsu (uiContext);

static
int IchiranAdjustBunsetsu(uiContext d)
{
  return IchiranQuitThenDo(d, CANNA_FN_AdjustBunsetsu);
}

static int IchiranKillToEndOfLine (uiContext);

static
int IchiranKillToEndOfLine(uiContext d)
{
  return IchiranKakuteiThenDo(d, CANNA_FN_KillToEndOfLine);
}

static int IchiranDeleteNext (uiContext);

static
int IchiranDeleteNext(uiContext d)
{
  return IchiranKakuteiThenDo(d, CANNA_FN_DeleteNext);
}

static int IchiranBubunMuhenkan (uiContext);

static
int IchiranBubunMuhenkan(uiContext d)
{
  return IchiranQuitThenDo(d, CANNA_FN_BubunMuhenkan);
}

static int IchiranHiragana (uiContext);

static
int IchiranHiragana(uiContext d)
{
  return IchiranQuitThenDo(d, CANNA_FN_Hiragana);
}

static int IchiranKatakana (uiContext);

static
int IchiranKatakana(uiContext d)
{
  return IchiranQuitThenDo(d, CANNA_FN_Katakana);
}

static int IchiranZenkaku (uiContext);

static
int IchiranZenkaku(uiContext d)
{
  return IchiranQuitThenDo(d, CANNA_FN_Zenkaku);
}

static int IchiranHankaku (uiContext);

static
int IchiranHankaku(uiContext d)
{
  return IchiranQuitThenDo(d, CANNA_FN_Hankaku);
}

static int IchiranRomaji (uiContext);

static
int IchiranRomaji(uiContext d)
{
  return IchiranQuitThenDo(d, CANNA_FN_Romaji);
}

static int IchiranToUpper (uiContext);

static
int IchiranToUpper(uiContext d)
{
  return IchiranQuitThenDo(d, CANNA_FN_ToUpper);
}

static int IchiranToLower (uiContext);

static
int IchiranToLower(uiContext d)
{
  return IchiranQuitThenDo(d, CANNA_FN_ToLower);
}

static int IchiranCapitalize (uiContext);

static
int IchiranCapitalize(uiContext d)
{
  return IchiranQuitThenDo(d, CANNA_FN_Capitalize);
}

static int IchiranKanaRotate (uiContext);

static
int IchiranKanaRotate(uiContext d)
{
  return IchiranQuitThenDo(d, CANNA_FN_KanaRotate);
}

static int IchiranRomajiRotate (uiContext);

static
int IchiranRomajiRotate(uiContext d)
{
  return IchiranQuitThenDo(d, CANNA_FN_RomajiRotate);
}

static int IchiranCaseRotateForward (uiContext);

static
int IchiranCaseRotateForward(uiContext d)
{
  return IchiranQuitThenDo(d, CANNA_FN_CaseRotate);
}

#include	"ichiranmap.h"
