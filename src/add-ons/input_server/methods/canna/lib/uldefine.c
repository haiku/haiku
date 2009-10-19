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
static char rcs_id[] = "@(#) 102.1 $Id: uldefine.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif

#include	<errno.h>
#include 	"canna.h"
#include "RK.h"
#include "RKintern.h"

// static void clearTourokuContext(tourokuContext p);
static tourokuContext newTourokuContext(void);
static int uuTTangoEveryTimeCatch(uiContext d, int retval, mode_context env);
static int uuTTangoExitCatch(uiContext d, int retval, mode_context env);
static int uuT2TangoEveryTimeCatch(uiContext d, int retval, mode_context env);
static int uuT2TangoExitCatch(uiContext d, int retval, mode_context nyc);
static int uuT2TangoQuitCatch(uiContext d, int retval, mode_context env);
static int uuTMakeDicYesCatch(uiContext d, int retval, mode_context env);
static int uuTMakeDicQuitCatch(uiContext d, int retval, mode_context env);
static int uuTMakeDicNoCatch(uiContext d, int retval, mode_context env);
static int dicTourokuDo(uiContext d);
static struct dicname *findUsrDic(void);
static int checkUsrDic(uiContext d);
static int dicTourokuTangoPre(uiContext d);
static int acDicTourokuTangoPre(uiContext d, int dn, mode_context dm);
static int uuTYomiEveryTimeCatch(uiContext d, int retval, mode_context env);
static int uuTYomiExitCatch(uiContext d, int retval, mode_context env);
static int uuTYomiQuitCatch(uiContext d, int retval, mode_context env);
static int dicTourokuYomi(uiContext d);
static int acDicTourokuYomi(uiContext d, int dn, mode_context dm);
static int dicTourokuYomiDo(uiContext d, canna_callback_t quitfunc);
static int uuTHinshiExitCatch(uiContext d, int retval, mode_context env);
static int uuTHinshiQuitCatch(uiContext d, int retval, mode_context env);
extern int specialfunc (uiContext, int);

#if !defined(NO_EXTEND_MENU) && !defined(WIN)
#ifdef luna88k
extern int errno;
#endif


static char *shinshitbl1[] = 
{
  "\314\276\273\354",                 /* Ì¾»ì */
  "\270\307\315\255\314\276\273\354", /* ¸ÇÍ­Ì¾»ì */
  "\306\260\273\354",                 /* Æ°»ì */
  "\267\301\315\306\273\354",         /* ·ÁÍÆ»ì */
  "\267\301\315\306\306\260\273\354", /* ·ÁÍÆÆ°»ì */
  "\311\373\273\354",                 /* Éû»ì */
  "\244\275\244\316\302\276",         /* ¤½¤ÎÂ¾ */
};

static char *shinshitbl2[] = 
{
  "\303\261\264\301\273\372",           /* Ã±´Á»ú */
  "\277\364\273\354",                   /* ¿ô»ì */
  "\317\242\302\316\273\354",           /* Ï¢ÂÎ»ì */
  "\300\334\302\263\273\354\241\246\264\266\306\260\273\354",/* ÀÜÂ³»ì¡¦´¶Æ°»ì */
};


static int tblflag;

#define TABLE1 1
#define TABLE2 2

#define HINSHI1_SZ (sizeof(shinshitbl1) / sizeof(char *))
#define HINSHI2_SZ (sizeof(shinshitbl2) / sizeof(char *))

#define SONOTA       HINSHI1_SZ - 1

static WCHAR_T *hinshitbl1[HINSHI1_SZ];
static WCHAR_T *hinshitbl2[HINSHI2_SZ];

static WCHAR_T *b1, *b2;

int
initHinshiTable(void)
{
  int retval = 0;

  retval = setWStrings(hinshitbl1, shinshitbl1, HINSHI1_SZ);
  if (retval != NG) {
    retval = setWStrings(hinshitbl2, shinshitbl2, HINSHI2_SZ);
    b1 = WString("\303\261\270\354?[");
                  /* Ã±¸ì */
    b2 = WString("]");
    if (!b1 || !b2) {
      retval = NG;
    }
  }
  return retval;
}

static void
clearTango(uiContext d)
{
  tourokuContext tc = (tourokuContext)d->modec;

  tc->tango_buffer[0] = 0;
  tc->tango_len = 0;
}

void
clearYomi(uiContext d)
{
  tourokuContext tc = (tourokuContext)d->modec;

  tc->yomi_buffer[0] = 0;
  tc->yomi_len = 0;
}

inline void
clearTourokuContext(tourokuContext p)
{
  p->id = TOUROKU_CONTEXT;
  p->genbuf[0] = 0;
  p->qbuf[0] = 0;
  p->tango_buffer[0] = 0;
  p->tango_len = 0;
  p->yomi_buffer[0] = 0;
  p->yomi_len = 0;
  p->curHinshi = 0;
  p->newDic = (struct dicname *)0;
  p->hcode[0] = 0;
  p->katsuyou = 0;
  p->workDic2 = (deldicinfo *)0;
  p->workDic3 = (deldicinfo *)0;
  p->udic = (WCHAR_T **)0;
  p->delContext = 0;

//  return(0);
}
  
static tourokuContext
newTourokuContext(void)
{
  tourokuContext tcxt;

  if ((tcxt = (tourokuContext)malloc(sizeof(tourokuContextRec)))
                                          == (tourokuContext)NULL) {
#ifndef WIN
    jrKanjiError = "malloc (newTourokuContext) ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
#else
    jrKanjiError = "malloc (newTourokuContext) \244\307\244\255\244\336"
	"\244\273\244\363\244\307\244\267\244\277";
#endif
    return (tourokuContext)NULL;
  }
  clearTourokuContext(tcxt);

  return tcxt;
}

int getTourokuContext(uiContext d)
{
  tourokuContext tc;
  int retval = 0;

  if (pushCallback(d, d->modec, NO_CALLBACK, NO_CALLBACK,
                                  NO_CALLBACK, NO_CALLBACK) == 0) {
#ifndef WIN
    jrKanjiError = "malloc (pushCallback) ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
#else
    jrKanjiError = "malloc (pushCallback) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
#endif
    return(NG);
  }

  if((tc = newTourokuContext()) == (tourokuContext)NULL) {
    popCallback(d);
    return(NG);
  }
  tc->majorMode = d->majorMode;
  tc->next = d->modec;
  d->modec = (mode_context)tc;

  tc->prevMode = d->current_mode;

  return(retval);
}

void
popTourokuMode(uiContext d)
{
  tourokuContext tc = (tourokuContext)d->modec;

  d->modec = tc->next;
  d->current_mode = tc->prevMode;
  free(tc);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Ã±¸ìÅÐÏ¿¤ÎÃ±¸ì¤ÎÆþÎÏ                                                      *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
int uuTTangoEveryTimeCatch(uiContext d, int retval, mode_context env)
{
  tourokuContext tc = (tourokuContext)env;
  int len, echoLen, revPos;
  WCHAR_T tmpbuf[ROMEBUFSIZE];
  /* I will leave this stack located array becase this will not be
     compiled in case WIN is defined.  1996.6.5 kon */

  retval = d->nbytes = 0;

#ifdef DEBUG
  checkModec(d);
#endif
  if((echoLen = d->kanji_status_return->length) < 0 || d->more.todo)
    return(retval);

  if (echoLen == 0) {
    d->kanji_status_return->revPos = 0;
    d->kanji_status_return->revLen = 0;
  }

  if(d->kanji_status_return->info & KanjiGLineInfo &&
     d->kanji_status_return->gline.length > 0) {
    echostrClear(d);
    return 0;
  }

  WStrncpy(tmpbuf, d->kanji_status_return->echoStr, echoLen);
  tmpbuf[echoLen] = (WCHAR_T)'\0';

  WStrcpy(d->genbuf, b1);
  WStrcat(d->genbuf, tmpbuf);
  WStrcat(d->genbuf, b2);

  revPos = WStrlen(b1);

  len = revPos + echoLen + 1;
  WStrcpy(d->genbuf + len, tc->genbuf); /* ¥á¥Ã¥»¡¼¥¸ */

  len += WStrlen(tc->genbuf);
  tc->genbuf[0] = 0;
  d->kanji_status_return->gline.line = d->genbuf;
  d->kanji_status_return->gline.length = len;
  if (d->kanji_status_return->revLen) {
    d->kanji_status_return->gline.revPos =
      d->kanji_status_return->revPos + revPos;
    d->kanji_status_return->gline.revLen = d->kanji_status_return->revLen;
  }
  else { /* È¿Å¾ÎÎ°è¤¬¤Ê¤¤¾ì¹ç */
    d->kanji_status_return->gline.revPos = len - WStrlen(b2);
    d->kanji_status_return->gline.revLen = 1;
  }
  d->kanji_status_return->info |= KanjiGLineInfo;
  d->kanji_status_return->length = 0;

  echostrClear(d);
#ifdef WIN
  if (checkGLineLen(d) < 0) {
    echostrClear(d);
  }
#else
  checkGLineLen(d);
#endif

  return retval;
}

static
int uuTTangoExitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  tourokuContext tc;

  popCallback(d); /* ÆÉ¤ß¤ò pop */

  tc = (tourokuContext)d->modec;

  WStrncpy(tc->tango_buffer, d->buffer_return, retval);
  tc->tango_buffer[retval] = (WCHAR_T)'\0';
  tc->tango_len = retval;

  return(dicTourokuYomi(d));
}

int uuTTangoQuitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  popCallback(d); /* ÆÉ¤ß¤ò pop */

  freeAndPopTouroku(d);
  GlineClear(d);
  currentModeInfo(d);
  return prevMenuIfExist(d);
}

static
int uuT2TangoEveryTimeCatch(uiContext d, int retval, mode_context env)
{
  yomiContext nyc;
  int echoLen, pos, offset;
  WCHAR_T tmpbuf[ROMEBUFSIZE];
  /* I will leave this stack located array becase this will not be
     compiled in case WIN is defined.  1996.6.5 kon */

  nyc = (yomiContext)env;

#ifdef DEBUG
  checkModec(d);
#endif
  if(d->kanji_status_return->info & KanjiThroughInfo) {
    extern KanjiModeRec yomi_mode;
    _do_func_slightly(d, 0, (mode_context)nyc, &yomi_mode);
  } else if(retval > 0){
    /* ÁÞÆþ¤¹¤ë */
    generalReplace(nyc->kana_buffer, nyc->kAttr, &nyc->kRStartp,
		   &nyc->kCurs, &nyc->kEndp, 0, d->buffer_return,
		   retval, HENKANSUMI);
    generalReplace(nyc->romaji_buffer, nyc->rAttr, &nyc->rStartp,
		   &nyc->rCurs, &nyc->rEndp, 0, d->buffer_return,
		   retval, 0);
    nyc->rStartp = nyc->rCurs;
    nyc->kRStartp = nyc->kCurs;
  }

  d->kanji_status_return->info &= ~(KanjiThroughInfo | KanjiEmptyInfo);
  if((echoLen = d->kanji_status_return->length) < 0)
    return(retval);

  WStrncpy(tmpbuf, d->kanji_status_return->echoStr, echoLen);

  WStrncpy(d->genbuf, nyc->kana_buffer, pos = offset = nyc->kCurs);

  WStrncpy(d->genbuf + pos, tmpbuf, echoLen);
  pos += echoLen;
  WStrncpy(d->genbuf + pos, nyc->kana_buffer + offset, nyc->kEndp - offset);
  pos += nyc->kEndp - offset;
  if (d->kanji_status_return->revLen == 0 && /* È¿Å¾É½¼¨ÉôÊ¬¤Ê¤·¤Ç... */
      nyc->kEndp - offset) { /* ¸å¤í¤Ë¤¯¤Ã¤Ä¤±¤ëÉôÊ¬¤¬¤¢¤ë¤Î¤Ê¤é */
    d->kanji_status_return->revLen = 1;
    d->kanji_status_return->revPos = offset + echoLen;
  }
  else {
    d->kanji_status_return->revPos += offset;
  }
  d->kanji_status_return->echoStr = d->genbuf;
  d->kanji_status_return->length = pos;

  return retval;
}

/************************************************
 *  Ã±¸ìÅÐÏ¿¥â¡¼¥É¤òÈ´¤±¤ëºÝ¤ËÉ¬Í×¤Ê½èÍý¤ò¹Ô¤¦  *
 ************************************************/
static
int uuT2TangoExitCatch(uiContext d, int retval, mode_context nyc)
/* ARGSUSED */
{
  yomiContext yc;

  popCallback(d); /* ÆÉ¤ß¤ò pop */

  yc = (yomiContext)d->modec;
  d->nbytes = retval = yc->kEndp;
  WStrncpy(d->buffer_return, yc->kana_buffer, retval);
  d->buffer_return[retval] = (WCHAR_T)'\0';

  RomajiClearYomi(d);
  popYomiMode(d);
  d->status = EXIT_CALLBACK;

  return retval;
}

static
int uuT2TangoQuitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  popCallback(d); /* ÆÉ¤ß¤ò pop */

  popYomiMode(d);

  d->status = QUIT_CALLBACK;

  return(0);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Ã±¸ìÅÐÏ¿¤Î¼­½ñºîÀ®                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
int uuTMakeDicYesCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  int err = 0, perr = 0;
  tourokuContext tc;
  WCHAR_T **dp;
  extern int defaultContext;

  popCallback(d); /* yesNo ¤ò¥Ý¥Ã¥× */

  tc = (tourokuContext)d->modec;

  if(defaultContext < 0) {
    if((KanjiInit() < 0) || (defaultContext < 0)) {
      jrKanjiError = KanjiInitError();
      freeAndPopTouroku(d);
      defineEnd(d);
      return(GLineNGReturn(d));
    }
  }
  /* ¼­½ñ¤òºî¤ë */
  if (RkwCreateDic(defaultContext, tc->newDic->name, 0x80) < 0) {
    err++; if (errno == EPIPE) perr++;
    MBstowcs(d->genbuf, "\274\255\275\361\244\316\300\270\300\256\244\313"
	"\274\272\307\324\244\267\244\336\244\267\244\277", 256);
                          /* ¼­½ñ¤ÎÀ¸À®¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
  } else if(RkwMountDic(defaultContext, tc->newDic->name, 0) < 0) {
    err++; if (errno == EPIPE) perr++;
    MBstowcs(d->genbuf, "\274\255\275\361\244\316\245\336\245\246\245\363"
	"\245\310\244\313\274\272\307\324\244\267\244\336\244\267\244\277", 256);
                          /* ¼­½ñ¤Î¥Þ¥¦¥ó¥È¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */ 
  } else if(d->contextCache != -1 && 
    RkwMountDic(d->contextCache, tc->newDic->name, 0) < 0) {
    err++; if (errno == EPIPE) perr++;
    MBstowcs(d->genbuf, "\274\255\275\361\244\316\245\336\245\246\245\363"
	"\245\310\244\313\274\272\307\324\244\267\244\336\244\267\244\277", 256);
                          /* ¼­½ñ¤Î¥Þ¥¦¥ó¥È¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
  }

  if(err) {
    if (perr) {
      jrKanjiPipeError();
    }
    makeGLineMessage(d, d->genbuf, WStrlen(d->genbuf));
    freeAndPopTouroku(d);
    defineEnd(d);
    currentModeInfo(d);
    return(0);
  }

  tc->newDic->dicflag = DIC_MOUNTED;

  /* ¼­½ñ¤Î¸õÊä¤ÎºÇ¸å¤ËÄÉ²Ã¤¹¤ë */
  dp = tc->udic;
  if (dp) {
    while (*dp) {
      dp++;
    }
    *dp++ = WString(tc->newDic->name);
    *dp = 0;
  }

  return(dicTourokuTango(d, uuTTangoQuitCatch));
}

static
int uuTMakeDicQuitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  popCallback(d); /* yesNo ¤ò¥Ý¥Ã¥× */

  freeAndPopTouroku(d);

  return prevMenuIfExist(d);
}

static
int uuTMakeDicNoCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  popCallback(d); /* yesNo ¤ò¥Ý¥Ã¥× */

  freeAndPopTouroku(d);
  defineEnd(d);
  currentModeInfo(d);

  GlineClear(d);
  defineEnd(d);
  return(retval);
}

/*
  ¥æ¡¼¥¶¼­½ñ¤Ç¥Þ¥¦¥ó¥È¤µ¤ì¤Æ¤¤¤ë¤â¤Î¤ò¼è¤ê½Ð¤¹½èÍý
 */
WCHAR_T **
getUserDicName(uiContext d)
/* ARGSUSED */
{
  int nmudic; /* ¥Þ¥¦¥ó¥È¤µ¤ì¤Æ¤¤¤ë¥æ¡¼¥¶¼­½ñ¤Î¿ô */
  struct dicname *p;
  WCHAR_T **tourokup, **tp;
  extern int defaultContext;

  if(defaultContext < 0) {
    if((KanjiInit() < 0) || (defaultContext < 0)) {
      jrKanjiError = KanjiInitError();
      return (WCHAR_T **)0;
    }
  }

  for (nmudic = 0, p = kanjidicnames ; p ; p = p->next) {
    if (p->dictype == DIC_USER && p->dicflag == DIC_MOUNTED) {
      nmudic++;
    }
  }

  /* return BUFFER ¤Î alloc */
  if ((tourokup = (WCHAR_T **)calloc(nmudic + 2, sizeof(WCHAR_T *)))
                                                  == (WCHAR_T **)NULL) {
    /* + 2 ¤Ê¤Î¤Ï 1 ¸ÄÁý¤¨¤ë²ÄÇ½À­¤¬¤¢¤ë¤Î¤ÈÂÇ¤Á»ß¤á¥Þ¡¼¥¯¤ò¤¤¤ì¤ë¤¿¤á */
#ifndef WIN
    jrKanjiError = "malloc (getUserDicName) ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
#else
    jrKanjiError = "malloc (getUserDicName) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
#endif
    return (WCHAR_T **)0;
  }

  for (tp = tourokup + nmudic, p = kanjidicnames ; p ; p = p->next) {
    if (p->dictype == DIC_USER && p->dicflag == DIC_MOUNTED) {
      *--tp = WString(p->name);
    }
  }
  tourokup[nmudic] = (WCHAR_T *)0;

  return (WCHAR_T **)tourokup;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Ã±¸ìÅÐÏ¿¤ÎÃ±¸ì¤ÎÆþÎÏ                                                      *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* »Ï¤á¤Ë¸Æ¤Ð¤ì¤ë´Ø¿ô */

int dicTouroku(uiContext d)
{
  tourokuContext tc;
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }    
  if(dicTourokuDo(d) < 0) {
    defineEnd(d);
    return(GLineNGReturn(d));
  }

  tc = (tourokuContext)d->modec;

  /* ¼­½ñ¤¬Ìµ¤±¤ì¤Ð¼­½ñ¤òºî¤ë */
  if(!*tc->udic) {
    checkUsrDic(d);
    return(0); /* ¼­½ñ¤òºî¤ë¤«¤É¤¦¤«¤ò¼ÁÌä¤¹¤ë¥â¡¼¥É¤ËÆþ¤ë(¤«¤â)¡£
		  ¡Ö¤«¤â¡×¤Ã¤Æ¤Î¤Ï¡¢checkUsrDic ¤Ç¤Ê¤Ë¤¬¤·¤«¤ÎÌäÂê¤¬
		  È¯À¸¤·¤¿¾ì¹ç¤ÏÆþ¤é¤Ê¤¤¤³¤È¤òÉ½¤¹¡£ */
  }
  tblflag = TABLE1;
  return(dicTourokuTango(d, uuTTangoQuitCatch));
}

static
int dicTourokuDo(uiContext d)
{
  tourokuContext tc;
  WCHAR_T **up;

  d->status = 0;

  /* ¥æ¡¼¥¶¼­½ñ¤Ç¥Þ¥¦¥ó¥È¤µ¤ì¤Æ¤¤¤ë¤â¤Î¤ò¼è¤Ã¤Æ¤¯¤ë */
  if((up = getUserDicName(d)) == 0) {
    return(NG);
  }

  if (getTourokuContext(d) < 0) {
    if (up) {
      WCHAR_T **p = up;

      for ( ; *p; p++) {
        WSfree(*p);
      }
      free(up);
    }
    return(NG);
  }

  tc = (tourokuContext)d->modec;

  tc->udic = up;

  return(0);
}

/*
 * Ã±¸ìÅÐÏ¿ÍÑ¼­½ñ¤Î¥µ¡¼¥Á
 *  ¥«¥¹¥¿¥Þ¥¤¥º¥Õ¥¡¥¤¥ë¤Ç°ìÈÖ»Ï¤á¤Ë»ØÄê¤µ¤ì¤Æ¤¤¤ë
 *  Ã±¸ìÅÐÏ¿ÍÑ¼­½ñ¤ò¸«ÉÕ¤±¤ë
 */
static struct dicname *
findUsrDic(void)
{
  struct dicname *res = (struct dicname *)0, *p;

  for (p = kanjidicnames ; p ; p = p->next) {
    if (p->dictype == DIC_USER) {
      res = p;
    }
  }
  return res;
}

/* 
 * ¥Þ¥¦¥ó¥È¤µ¤ì¤Æ¤¤¤ë¼­½ñ¤Î¥Á¥§¥Ã¥¯
 * ¡¦¥«¥¹¥¿¥Þ¥¤¥º¥Õ¥¡¥¤¥ë¤ÇÃ±¸ìÅÐÏ¿ÍÑ¼­½ñ¤È¤·¤Æ»ØÄê¤µ¤ì¤Æ¤¤¤Æ¡¢
 *   ¥Þ¥¦¥ó¥È¤Ë¼ºÇÔ¤·¤Æ¤¤¤ë¼­½ñ¤¬¤¢¤ë
 *     ¢ª ¼­½ñ¤òºî¤ë(ºî¤ë¼­½ñ¤Ï£±¤Ä¤À¤±)
 * ¡¦¥«¥¹¥¿¥Þ¥¤¥º¥Õ¥¡¥¤¥ë¤ÇÃ±¸ìÅÐÏ¿ÍÑ¼­½ñ¤È¤·¤Æ»ØÄê¤µ¤ì¤Æ¤¤¤ë¼­½ñ¤¬¤Ê¤¤
 * ¡¦¥«¥¹¥¿¥Þ¥¤¥º¥Õ¥¡¥¤¥ë¤ÇÃ±¸ìÅÐÏ¿ÍÑ¼­½ñ¤È¤·¤Æ»ØÄê¤µ¤ì¤Æ¤¤¤Æ¡¢
 *   ¥Þ¥¦¥ó¥È¤µ¤ì¤Æ¤¤¤ë¼­½ñ¤¬¤Ê¤¤
 */
static
int checkUsrDic(uiContext d)
{
  tourokuContext tc = (tourokuContext)d->modec;
  coreContext ync;
  struct dicname *u;
  WCHAR_T xxxx[512];
  /* I will leave this stack located array becase this will not be
     compiled in case WIN is defined.  1996.6.5 kon */

  u = findUsrDic();

  /* ¥«¥¹¥¿¥Þ¥¤¥º¥Õ¥¡¥¤¥ë¤Ç¡¢Ã±¸ìÅÐÏ¿ÍÑ¼­½ñ¤Ï»ØÄê¤µ¤ì¤Æ¤¤¤ë¤¬¡¢
     ¥Þ¥¦¥ó¥È¤Ë¼ºÇÔ¤·¤Æ¤¤¤ë¤È¤­                                */
  if (u) {
    if (u->dicflag == DIC_MOUNT_FAILED) {
      char tmpbuf[1024];
      sprintf(tmpbuf, "\303\261\270\354\305\320\317\277\315\321\274\255"
	"\275\361\244\254\244\242\244\352\244\336\244\273\244\363\241\243"
	"\274\255\275\361(%s)\244\362\272\356\300\256\244\267\244\336\244"
	"\271\244\253?(y/n)",
	      u->name);
                /* Ã±¸ìÅÐÏ¿ÍÑ¼­½ñ¤¬¤¢¤ê¤Þ¤»¤ó¡£¼­½ñ(%s)¤òºîÀ®¤·¤Þ¤¹¤« */
      makeGLineMessageFromString(d, tmpbuf);
      tc->newDic = u; /* ºî¤ë¼­½ñ */
      if(getYesNoContext(d,
			 NO_CALLBACK, uuTMakeDicYesCatch,
			 uuTMakeDicQuitCatch, uuTMakeDicNoCatch) < 0) {
	defineEnd(d);
	return(GLineNGReturn(d));
      }
      makeGLineMessage(d, d->genbuf, WStrlen(d->genbuf));
      ync = (coreContext)d->modec;
      ync->majorMode = CANNA_MODE_ExtendMode;
      ync->minorMode = CANNA_MODE_TourokuMode;
    }
  }

  /* ¥«¥¹¥¿¥Þ¥¤¥º¥Õ¥¡¥¤¥ë¤Ç¡¢Ã±¸ìÅÐÏ¿ÍÑ¼­½ñ¤¬»ØÄê¤µ¤ì¤Æ¤¤¤Ê¤¤¤«¡¢
     »ØÄê¤Ï¤µ¤ì¤Æ¤¤¤ë¤¬¥Þ¥¦¥ó¥È¤µ¤ì¤Æ¤¤¤Ê¤¤¤È¤­                  */
  if (!u || u->dicflag == DIC_NOT_MOUNTED) {
    MBstowcs(xxxx, "\303\261\270\354\305\320\317\277\315\321\274\255\275\361"
	"\244\254\273\330\304\352\244\265\244\354\244\306\244\244\244\336"
	"\244\273\244\363", 512);
                 /* Ã±¸ìÅÐÏ¿ÍÑ¼­½ñ¤¬»ØÄê¤µ¤ì¤Æ¤¤¤Þ¤»¤ó */
    WStrcpy(d->genbuf, xxxx);
    makeGLineMessage(d, d->genbuf, WStrlen(d->genbuf));
    freeAndPopTouroku(d);
    defineEnd(d);
    currentModeInfo(d);
  }

  return(0);
}

int dicTourokuTango(uiContext d, canna_callback_t quitfunc)
{
  tourokuContext tc = (tourokuContext)d->modec;
  yomiContext yc, yc2;
  int retval = 0;

  yc = GetKanjiString(d, (WCHAR_T *)0, 0,
		      CANNA_NOTHING_RESTRICTED,
		      (int)CANNA_YOMI_CHGMODE_INHIBITTED,
		      (int)CANNA_YOMI_END_IF_KAKUTEI,
		      CANNA_YOMI_INHIBIT_NONE,
		      uuTTangoEveryTimeCatch, uuTTangoExitCatch,
		      quitfunc);
  if (yc == (yomiContext)0) {
    freeAndPopTouroku(d);
    defineEnd(d);
    currentModeInfo(d);
    return NoMoreMemory();
  }
  yc2 = GetKanjiString(d, (WCHAR_T *)0, 0,
		      CANNA_NOTHING_RESTRICTED,
		      (int)CANNA_YOMI_CHGMODE_INHIBITTED,
		      (int)!CANNA_YOMI_END_IF_KAKUTEI,
		      CANNA_YOMI_INHIBIT_NONE,
		      uuT2TangoEveryTimeCatch, uuT2TangoExitCatch,
		      uuT2TangoQuitCatch);
  if (yc2 == (yomiContext)0) {
    popYomiMode(d);  /* yc1 ¤ò pop ¤¹¤ë */
    popCallback(d);
    freeAndPopTouroku(d);
    defineEnd(d);
    currentModeInfo(d);
    return NoMoreMemory();
  }
  yc2->generalFlags |= CANNA_YOMI_DELETE_DONT_QUIT;

  yc2->majorMode = CANNA_MODE_ExtendMode;
  yc2->minorMode = CANNA_MODE_TourokuMode;
  currentModeInfo(d);

  return(retval);
}

static
int dicTourokuTangoPre(uiContext d)
{
  return dicTourokuTango(d, uuTTangoQuitCatch);
}

static
int acDicTourokuTangoPre(uiContext d, int dn, mode_context dm)
/* ARGSUSED */
{
  popCallback(d);
  return dicTourokuTangoPre(d);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Ã±¸ìÅÐÏ¿¤ÎÆÉ¤ß¤ÎÆþÎÏ                                                      *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
int uuTYomiEveryTimeCatch(uiContext d, int retval, mode_context env)
{
  tourokuContext tc = (tourokuContext)env;
  int len, echoLen, revPos;
  WCHAR_T tmpbuf[ROMEBUFSIZE];

  retval = d->nbytes = 0;

  if((echoLen = d->kanji_status_return->length) < 0)
    return(retval);

  if (echoLen == 0) {
    d->kanji_status_return->revPos = 0;
    d->kanji_status_return->revLen = 0;
  }

  /* ¼è¤ê¤¢¤¨¤º echoStr ¤¬ d->genbuf ¤«¤â¤·¤ì¤Ê¤¤¤Î¤Ç copy ¤·¤Æ¤ª¤¯ */
  WStrncpy(tmpbuf, d->kanji_status_return->echoStr, echoLen);

  d->kanji_status_return->info &= ~(KanjiThroughInfo | KanjiEmptyInfo);
  revPos = MBstowcs(d->genbuf, "\303\261\270\354[", ROMEBUFSIZE);
                               /* Ã±¸ì */
  WStrcpy(d->genbuf + revPos, tc->tango_buffer);
  revPos += WStrlen(tc->tango_buffer);
  revPos += MBstowcs(d->genbuf + revPos, "] \306\311\244\337?[", ROMEBUFSIZE - revPos);
                                           /* ÆÉ¤ß */
  WStrncpy(d->genbuf + revPos, tmpbuf, echoLen);
  len = echoLen + revPos;
  d->genbuf[len++] = (WCHAR_T) ']';
  WStrcpy(d->genbuf + len, tc->genbuf);
  len += WStrlen(tc->genbuf);
  tc->genbuf[0] = 0;
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
  d->kanji_status_return->info |= KanjiGLineInfo;
  echostrClear(d);
#ifdef WIN
  if (checkGLineLen(d) < 0) {
    echostrClear(d);
  }
#else
  checkGLineLen(d);
#endif

  return retval;
}

static
int uuTYomiExitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  tourokuContext tc;

  popCallback(d); /* ÆÉ¤ß¤ò pop */

  tc = (tourokuContext)d->modec;

  WStrncpy(tc->yomi_buffer, d->buffer_return, retval);
  tc->yomi_buffer[retval] = (WCHAR_T)'\0';
  tc->yomi_len = retval;

  return(dicTourokuHinshi(d));
}

static int uuTYomiQuitCatch (uiContext, int, mode_context);

static
int uuTYomiQuitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  popCallback(d); /* ÆÉ¤ß¤ò pop */

  clearTango(d);
  clearYomi(d);

  return(dicTourokuTango(d, uuTTangoQuitCatch));
}

static
int dicTourokuYomi(uiContext d)
{
  return(dicTourokuYomiDo(d, uuTYomiQuitCatch));
}

static
int acDicTourokuYomi(uiContext d, int dn, mode_context dm)
/* ARGSUSED */
{
  popCallback(d);
  return dicTourokuYomi(d);
}

static
int dicTourokuYomiDo(uiContext d, canna_callback_t quitfunc)
{
  yomiContext yc;
  tourokuContext tc = (tourokuContext)d->modec;
  int retval = 0;

  if(tc->tango_len < 1) {
    clearTango(d);
    return canna_alert(d, "\303\261\270\354\244\362\306\376\316\317\244\267\244\306\244\257\244\300\244\265\244\244", acDicTourokuTangoPre);
                         /* Ã±¸ì¤òÆþÎÏ¤·¤Æ¤¯¤À¤µ¤¤ */
  }

  yc = GetKanjiString(d, (WCHAR_T *)0, 0,
		      CANNA_NOTHING_RESTRICTED,
		      (int)CANNA_YOMI_CHGMODE_INHIBITTED,
		      (int)CANNA_YOMI_END_IF_KAKUTEI,
		      (CANNA_YOMI_INHIBIT_HENKAN | CANNA_YOMI_INHIBIT_ASHEX |
		      CANNA_YOMI_INHIBIT_ASBUSHU),
		      uuTYomiEveryTimeCatch, uuTYomiExitCatch,
		      quitfunc);
  if (yc == (yomiContext)0) {
    freeAndPopTouroku(d);
    defineEnd(d);
    currentModeInfo(d);
    return NoMoreMemory();
  }
  yc->majorMode = CANNA_MODE_ExtendMode;
  yc->minorMode = CANNA_MODE_TourokuMode;
  currentModeInfo(d);

  return(retval);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Ã±¸ìÅÐÏ¿¤ÎÉÊ»ì¤ÎÁªÂò                                                      *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
int uuTHinshiExitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  forichiranContext fc;
  tourokuContext tc;
  int cur;

  d->nbytes = 0;

  popCallback(d); /* °ìÍ÷¤ò pop */

  fc = (forichiranContext)d->modec;
  cur = fc->curIkouho;

  popForIchiranMode(d);
  popCallback(d);

  if (tblflag == TABLE1 && cur == SONOTA) {
    tblflag = TABLE2;
    return dicTourokuHinshi(d);
  }

  if (tblflag == TABLE2) {
    cur += SONOTA;
  }

  tc = (tourokuContext)d->modec;

  tc->curHinshi = cur;

  return(dicTourokuHinshiDelivery(d));
}

static
int uuTHinshiQuitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  popCallback(d); /* °ìÍ÷¤ò pop */

  popForIchiranMode(d);
  popCallback(d);

  if (tblflag == TABLE2) {
    tblflag = TABLE1;
    return dicTourokuHinshi(d);
  }

  clearYomi(d);

  return(dicTourokuYomi(d));
}

int dicTourokuHinshi(uiContext d)
{
  tourokuContext tc = (tourokuContext)d->modec;
  forichiranContext fc;
  ichiranContext ic;
  unsigned inhibit = 0;
  int currentkouho, retval = 0, numkouho;

  d->status = 0;

  if(tc->yomi_len < 1) {
    return canna_alert(d, "\306\311\244\337\244\362\306\376\316\317\244\267"
	"\244\306\244\257\244\300\244\265\244\244", acDicTourokuYomi);
                          /* ÆÉ¤ß¤òÆþÎÏ¤·¤Æ¤¯¤À¤µ¤¤ */
  }

  if((retval = getForIchiranContext(d)) < 0) {
    freeDic(tc);
    defineEnd(d);
    return(GLineNGReturnTK(d));
  }

  fc = (forichiranContext)d->modec;

  /* selectOne ¤ò¸Æ¤Ö¤¿¤á¤Î½àÈ÷ */
  if (tblflag == TABLE2) {
    fc->allkouho = hinshitbl2;
    numkouho = HINSHI2_SZ;
  }
  else {
    fc->allkouho = hinshitbl1;
    numkouho = HINSHI1_SZ;
  }

  fc->curIkouho = 0;
  currentkouho = 0;
  if (!cannaconf.HexkeySelect)
    inhibit |= ((unsigned char)NUMBERING | (unsigned char)CHARINSERT); 
  else
    inhibit |= (unsigned char)CHARINSERT; 

  if((retval = selectOne(d, fc->allkouho, &fc->curIkouho, numkouho,
		 BANGOMAX, inhibit, currentkouho, WITH_LIST_CALLBACK,
		 NO_CALLBACK, uuTHinshiExitCatch, 
		 uuTHinshiQuitCatch, uiUtilIchiranTooSmall)) < 0) {
    popForIchiranMode(d);
    popCallback(d);
    freeDic(tc);
    defineEnd(d);
    return(GLineNGReturnTK(d));
  }

  ic = (ichiranContext)d->modec;
  ic->majorMode = CANNA_MODE_ExtendMode;
  ic->minorMode = CANNA_MODE_TourokuHinshiMode;
  currentModeInfo(d);

  /* ¸õÊä°ìÍ÷¹Ô¤¬¶¹¤¯¤Æ¸õÊä°ìÍ÷¤¬½Ð¤»¤Ê¤¤ */
  if(ic->tooSmall) {
    d->status = AUX_CALLBACK;
    return(retval);
  }

  if ( !(ic->flags & ICHIRAN_ALLOW_CALLBACK) ) {
    makeGlineStatus(d);
  }

  /* d->status = ICHIRAN_EVERYTIME; */

  return(retval);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * jrKanjiControl ÍÑ                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int dicTourokuControl(uiContext d, WCHAR_T *tango, canna_callback_t quitfunc)
{
  tourokuContext tc;

  if(dicTourokuDo(d) < 0) {
    return(GLineNGReturn(d));
  }

  tc = (tourokuContext)d->modec;

  if(!*tc->udic) {
    if(checkUsrDic(d) < 0) 
      return(GLineNGReturn(d));
    else
      return(0);
  }

  if(tango == 0 || tango[0] == 0) {
    return(dicTourokuTango(d, quitfunc));
  }

  WStrcpy(tc->tango_buffer, tango);
  tc->tango_len = WStrlen(tc->tango_buffer);

  return(dicTourokuYomiDo(d, quitfunc));
}
#endif /* NO_EXTEND_MENU */
