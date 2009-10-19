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
static	char	rcs_id[] = "@(#) 102.1 $Id: kigo.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif /* lint */

#include	"canna.h"

#define BYTE1		84	/* JIS¥³¡¼¥ÉÉ½¤ÎÂè°ì¥Ð¥¤¥È¤Î¿ô */
#define BYTE2		94	/* JIS¥³¡¼¥ÉÉ½¤ÎÂèÆó¥Ð¥¤¥È¤Î¿ô */
#define KIGOSU		(((BYTE1 - 1) * BYTE2) + 4)
    				/* µ­¹æ¤ÎÁí¿ô */

#define KIGOSIZE	1	/* µ­¹æ¸õÊä¤ÎÊ¸»ú¿ô */
#define KIGOCOLS	2	/* µ­¹æ¸õÊä¤Î¥³¥é¥à¿ô */
#define KIGOSPACE	2	/* µ­¹æ¤Î´Ö¤Î¶õÇòÊ¸»ú¤Î¥³¥é¥à¿ô */
#define KIGOWIDTH	(KIGOCOLS + KIGOSPACE)
					/* bangomax¤ò·×»»¤¹¤ë¤¿¤á¤Î¿ô */

#define NKAKKOCHARS	1	/* JIS¥³¡¼¥ÉÉ½¼¨ÍÑ³ç¸Ì¤ÎÊ¸»ú¿ô */
#define KAKKOCOLS       2       /* Æ±¥³¥é¥à¿ô */
#define NKCODECHARS	4	/* JIS¥³¡¼¥ÉÉ½¼¨¤½¤Î¤â¤Î¤ÎÊ¸»ú¿ô */
#define KCODECOLS       4       /* Æ±¥³¥é¥à¿ô */
/* JIS¥³¡¼¥ÉÉ½¼¨Á´ÂÎ¤ÎÊ¸»ú¿ô */
#define NKCODEALLCHARS	(NKAKKOCHARS + NKAKKOCHARS + NKCODECHARS)
/* Æ±¥³¥é¥à¿ô */
#define KCODEALLCOLS    (KAKKOCOLS + KAKKOCOLS + KCODECOLS)

static void freeKigoContext(ichiranContext kc);
static void makeKigoGlineStatus(uiContext d);
static int makeKigoInfo(uiContext d, int headkouho);
static int kigoIchiranExitCatch(uiContext d, int retval, mode_context env);
static int kigoIchiranQuitCatch(uiContext d, int retval, mode_context env);
static int KigoNop(uiContext d);
static int KigoForwardKouho(uiContext d);
static int KigoBackwardKouho(uiContext d);
static int KigoPreviousKouhoretsu(uiContext d);
static int KigoNextKouhoretsu(uiContext d);
static int KigoBeginningOfKouho(uiContext d);
static int KigoEndOfKouho(uiContext d);
static int KigoKakutei(uiContext d);
static int KigoBangoKouho(uiContext d);
static int KigoQuit(uiContext d);

static int kigo_curIkouho;

void
initKigoTable(void)
{
}

/* cfunc ichiranContext
 *
 * ichiranContext
 *
 */
inline void
clearKigoContext(ichiranContext p)
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

inline ichiranContext
newKigoContext(void)
{
  ichiranContext kcxt;

  if((kcxt = (ichiranContext)malloc(sizeof(ichiranContextRec)))
                                         == (ichiranContext)NULL) {
#ifndef WIN
    jrKanjiError = "malloc (newKigoContext) ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
#else
    jrKanjiError = "malloc (newKigoContext) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
#endif
    return (ichiranContext)0;
  }
  clearKigoContext(kcxt);

  return kcxt;
}


#ifdef	SOMEONE_USES_THIS
static void
freeKigoContext(ichiranContext kc)
{
  free(kc);
}
#endif	/* SOMEONE_USES_THIS */

/*
 * µ­¹æ°ìÍ÷¹Ô¤òºî¤ë
 */
inline
int getKigoContext(uiContext d, canna_callback_t everyTimeCallback, canna_callback_t exitCallback, canna_callback_t quitCallback, canna_callback_t auxCallback)
{
  extern KanjiModeRec kigo_mode;
  ichiranContext kc;
  int retval = 0;

  if(pushCallback(d, d->modec,
	everyTimeCallback, exitCallback, quitCallback, auxCallback) == 0) {
    jrKanjiError = "malloc (pushCallback) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
                                         /* ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
    return(NG);
  }
  
  if((kc = newKigoContext()) == (ichiranContext)NULL) {
    popCallback(d);
    return(NG);
  }
  kc->next = d->modec;
  d->modec = (mode_context)kc;

  kc->prevMode = d->current_mode;
  d->current_mode = &kigo_mode;

  return(retval);
}

#ifndef NO_EXTEND_MENU
inline void
popKigoMode(uiContext d)
{
  ichiranContext kc = (ichiranContext)d->modec;

  d->modec = kc->next;
  d->current_mode = kc->prevMode;
  freeIchiranContext(kc);
}

/*
 * µ­¹æ°ìÍ÷¹Ô¤Ë´Ø¤¹¤ë¹½Â¤ÂÎ¤ÎÆâÍÆ¤ò¹¹¿·¤¹¤ë
 *
 * ¡¦¥«¥ì¥ó¥È¸õÊä¤Ë¤è¤Ã¤Æ kouhoinfo ¤È glineinfo ¤«¤é¸õÊä°ìÍ÷¹Ô¤òºî¤ë
 * ¡¦¥«¥ì¥ó¥È¸õÊä¤Î¥³¡¼¥É¤ò¥­¥ã¥é¥¯¥¿¤ËÊÑ´¹¤¹¤ë
 *
 * °ú¤­¿ô	RomeStruct
 * Ìá¤êÃÍ	¤Ê¤·
 */
static void
makeKigoGlineStatus(uiContext d)
{
  ichiranContext kc = (ichiranContext)d->modec;
  WCHAR_T *gptr;
  char xxx[3];
  char *yyy = xxx;
  int  i, b1, b2;

  gptr = kc->glineifp->gldata + NKAKKOCHARS;
  
  /* ¥«¥ì¥ó¥Èµ­¹æ¤ÎJIS¥³¡¼¥É¤ò°ìÍ÷¹Ô¤ÎÃæ¤Î¥«¥Ã¥³Æâ¤ËÆþ¤ì¤ë */
  WCstombs(xxx, kc->kouhoifp[*(kc->curIkouho)].khdata, 3);

  for(i=0; i<2; i++, yyy++) {
    b1 = (((unsigned long)*yyy & 0x7f) >> 4);
    b2 = (*yyy & 0x0f);
    *gptr++ = b1 + ((b1 > 0x09) ? ('a' - 10) : '0');
    *gptr++ = b2 + ((b2 > 0x09) ? ('a' - 10) : '0');
  }

  d->kanji_status_return->info |= KanjiGLineInfo;
  d->kanji_status_return->gline.line = kc->glineifp->gldata;
  d->kanji_status_return->gline.length = kc->glineifp->gllen;
  d->kanji_status_return->gline.revPos =
    kc->kouhoifp[*(kc->curIkouho)].khpoint;
  d->kanji_status_return->gline.revLen = KIGOSIZE;

}

/* µ­¹æ°ìÍ÷ÍÑ¤Îglineinfo¤Èkouhoinfo¤òºî¤ë
 *
 * ¡öglineinfo¡ö
 *    int glkosu   : int glhead     : int gllen  : WCHAR_T *gldata
 *    £±¹Ô¤Î¸õÊä¿ô : ÀèÆ¬µ­¹æ¤¬     : £±¹Ô¤ÎÄ¹¤µ : µ­¹æ°ìÍ÷¹Ô¤ÎÊ¸»úÎó
 *                 : ²¿ÈÖÌÜ¤Îµ­¹æ¤« :
 * -------------------------------------------------------------------------
 * 0 | 6           : 0              : 24         : £±¡ù£²¡ú£³¡û£´¡ü£µ¡ý£¶¢¢
 *
 *  ¡ökouhoinfo¡ö
 *    int khretsu  : int khpoint  : WCHAR_T *khdata
 *    Ì¤»ÈÍÑ       : ¹Ô¤ÎÀèÆ¬¤«¤é : µ­¹æ¤ÎÊ¸»ú
 *                 : ²¿¥Ð¥¤¥ÈÌÜ¤« :
 * -------------------------------------------------------------------------
 * 0 | 0           : 0            : ¡ù
 * 1 | 0           : 4            : ¡ú
 * 2 | 0           : 8            : ¡û
 *          :               :
 *
 * °ú¤­¿ô	headkouho	¥«¥ì¥ó¥Èµ­¹æ¸õÊä¹Ô¤ÎÀèÆ¬¸õÊä¤Î°ÌÃÖ
 *					(2121¤«¤é²¿¸ÄÌÜ¤«(2121¤Ï£°ÈÖÌÜ))
 *		uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0
 */
static
int makeKigoInfo(uiContext d, int headkouho)
{
  ichiranContext kc = (ichiranContext)d->modec;
  WCHAR_T *gptr;
  int  i, b1, b2, lnko, cn;
  int  byte1hex = 0xa1;
  int  byte2hex = 0xa1;
  char xxx[3];

  b2 = headkouho % BYTE2;	/* JIS¥³¡¼¥ÉÉ½Ãæ(£Ø¼´)¤Î°ÌÃÖ (ÅÀ-1) */
  b1 = headkouho / BYTE2;	/* JIS¥³¡¼¥ÉÉ½Ãæ(£Ù¼´)¤Î°ÌÃÖ (¶è-1) */

  xxx[2] = '\0';

#if defined(DEBUG) && !defined(WIN)
  if (iroha_debug) {
    printf("kigoinfo = bangomax %d, b1 %d, b2 %d\n", kc->nIkouho, b1, b2);
    printf("kigoinfo = headkouho %d, curIkouho %d\n",
	   headkouho, *(kc->curIkouho));
  }
#endif

  /* µ­¹æ°ìÍ÷ÍÑ¤Îglineinfo¤Èkouhoinfo¤òºî¤ë */
  gptr = kc->glinebufp;

  kc->glineifp->glhead = headkouho;
  kc->glineifp->gldata = gptr;

  /* JIS¥³¡¼¥É¤ÎÉ½¼¨ÎÎ°è¤ò°ìÍ÷¹ÔÃæ¤Ëºî¤ë */
  MBstowcs(gptr, "\241\316", 1);
                 /* ¡Î */
  for(i=0, gptr++; i<NKCODECHARS; i++)
    *gptr++ = ' ';
  MBstowcs(gptr++, "\241\317", 1);
                 /* ¡Ï */

  for(cn=NKCODEALLCHARS, lnko=0;
      b1<BYTE1 && lnko<kc->nIkouho && (headkouho+lnko)<KIGOSU ; b1++) {
    for(; b2<BYTE2 && lnko<kc->nIkouho && (headkouho+lnko)<KIGOSU; b2++, lnko++) {
      
      /* ¶èÀÚ¤ê¤Ë¤Ê¤ë¶õÇò¤ò¥³¥Ô¡¼¤¹¤ë */
      if(lnko != 0) {
	MBstowcs(gptr++, "\241\241", 1);
                         /* ¡¡ */ 
	cn ++;
      }

      kc->kouhoifp[lnko].khpoint = cn;
      kc->kouhoifp[lnko].khdata = gptr;
      
      /* ¸õÊä¤ò¥³¥Ô¡¼¤¹¤ë */
      *xxx = (char)byte1hex + b1;
      *(xxx + 1) = (char)byte2hex + b2;
      MBstowcs(gptr++, xxx, 1);
      cn ++;
    }
    b2 = 0;
  }
  *gptr = (WCHAR_T)0;
  kc->glineifp->glkosu = lnko;
  kc->glineifp->gllen = WStrlen(kc->glineifp->gldata);

  return(0);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * µ­¹æ°ìÍ÷                                                                  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
int kigoIchiranExitCatch(uiContext d, int retval, mode_context env)
     /* ARGSUSED */
{
  popCallback(d);
  retval = YomiExit(d, retval);
  currentModeInfo(d);

  killmenu(d);

  return(retval);
}

static
int kigoIchiranQuitCatch(uiContext d, int retval, mode_context env)
     /* ARGSUSED */
{
  popCallback(d);
  currentModeInfo(d);

  return prevMenuIfExist(d);
}
#endif /* NO_EXTEND_MENU */

int KigoIchiran(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }    

#ifdef NO_EXTEND_MENU
  d->kanji_status_return->info |= KanjiKigoInfo;
  return 0;
#else
  if(makeKigoIchiran(d, CANNA_MODE_KigoMode) == NG)
    return(GLineNGReturn(d));
  else
    return(0);
#endif
}

#ifndef NO_EXTEND_MENU
/*
 * µ­¹æ°ìÍ÷¹Ô¤òÉ½¼¨¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
int makeKigoIchiran(uiContext d, int major_mode)
{
  ichiranContext kc;
  int            headkouho;

  if(d->ncolumns < (KCODEALLCOLS + KIGOCOLS)) {
    NothingChangedWithBeep(d);
    jrKanjiError = "\270\365\312\344\260\354\315\367\315\321\244\316\311\375"
	"\244\254\266\271\244\244\244\316\244\307\265\255\271\346\260\354"
	"\315\367\244\307\244\255\244\336\244\273\244\363";
                   /* ¸õÊä°ìÍ÷ÍÑ¤ÎÉý¤¬¶¹¤¤¤Î¤Çµ­¹æ°ìÍ÷¤Ç¤­¤Þ¤»¤ó */
    return(NG);
  }

  if(getKigoContext(d, NO_CALLBACK, kigoIchiranExitCatch, kigoIchiranQuitCatch, NO_CALLBACK) == NG)
    return(NG);

  kc = (ichiranContext)d->modec;
  kc->majorMode = major_mode;
  kc->minorMode = CANNA_MODE_KigoMode;
  kc->flags |= cannaconf.quickly_escape ? 0 : ICHIRAN_STAY_LONG;

  currentModeInfo(d);

  /* ºÇÂçµ­¹æÉ½¼¨¿ô¤Î¥»¥Ã¥È */
  /* Áí¥«¥é¥à¿ô¤«¤é "¡ÎJIS ¡Ï" Ê¬¤òº¹¤·°ú¤¤¤Æ·×»»¤¹¤ë */
  if((kc->nIkouho =
      (((d->ncolumns - KCODEALLCOLS - KIGOCOLS) / KIGOWIDTH) + 1))
                                                  > KIGOBANGOMAX) {
    kc->nIkouho = KIGOBANGOMAX;
  }

  kc->curIkouho = &kigo_curIkouho;

  if(allocIchiranBuf(d) == NG) { /* µ­¹æ°ìÍ÷¥â¡¼¥É */
    popKigoMode(d);
    popCallback(d);
    return(NG);
  }

  /* ¥«¥ì¥ó¥È¸õÊä¤Î¤¢¤ëµ­¹æ°ìÍ÷¹Ô¤ÎÀèÆ¬¸õÊä¤È¡¢
     °ìÍ÷¹ÔÃæ¤Î¥«¥ì¥ó¥È¸õÊä¤Î°ÌÃÖ¤òµá¤á¤ë */
  if(d->curkigo) {		/* a1a1¤«¤é²¿ÈÖÌÜ¤Îµ­¹æ¤« */
    headkouho = (d->curkigo / kc->nIkouho) * kc->nIkouho;
    *(kc->curIkouho) = d->curkigo % kc->nIkouho;
  } else {
    d->curkigo = 0;
    headkouho = 0;
    *(kc->curIkouho) = 0;
  }

  /* ¤³¤³¤Ë¤¯¤ëÄ¾Á°¤Ë C-t ¤È¤«¤¬ Gline ¤ËÉ½¼¨¤µ¤ì¤Æ¤¤¤ë¾ì¹ç²¼¤Î£±¹Ô¤ò
     ¤ä¤ëÉ¬Í×¤¬½Ð¤Æ¤¯¤ë¡£ */
  d->flags &= ~(PLEASE_CLEAR_GLINE | PCG_RECOGNIZED);

  /* glineinfo¤Èkouhoinfo¤òºî¤ë */
  makeKigoInfo(d, headkouho);

  /* kanji_status_return¤òºî¤ë */
  makeKigoGlineStatus(d);

  return(0);
}

static
int KigoNop(uiContext d)
{
  /* currentModeInfo ¤Ç¥â¡¼¥É¾ðÊó¤¬É¬¤ºÊÖ¤ë¤è¤¦¤Ë¥À¥ß¡¼¤Î¥â¡¼¥É¤òÆþ¤ì¤Æ¤ª¤¯ */
  d->majorMode = d->minorMode = CANNA_MODE_AlphaMode;
  currentModeInfo(d);

  makeKigoGlineStatus(d);
  return 0;
}

/*
 * µ­¹æ°ìÍ÷¹ÔÃæ¤Î¼¡¤Îµ­¹æ¤Ë°ÜÆ°¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0
 */
static
int KigoForwardKouho(uiContext d)
{
  ichiranContext kc = (ichiranContext)d->modec;
  int  headkouho;

  /* ¼¡¤Îµ­¹æ¤Ë¤¹¤ë */
  ++*(kc->curIkouho);
  
  /* °ìÍ÷É½¼¨¤ÎºÇ¸å¤Îµ­¹æ¤À¤Ã¤¿¤é¡¢¼¡¤Î°ìÍ÷¹Ô¤ÎÀèÆ¬µ­¹æ¤ò¥«¥ì¥ó¥Èµ­¹æ¤È¤¹¤ë */
  if((*(kc->curIkouho) >= kc->nIkouho) ||
     (kc->glineifp->glhead + *(kc->curIkouho) >= KIGOSU)) {
    headkouho  = kc->glineifp->glhead + kc->nIkouho;
    if(headkouho >= KIGOSU)
      headkouho = 0;
    *(kc->curIkouho) = 0;
    makeKigoInfo(d, headkouho);
  }

  /* kanji_status_retusrn ¤òºî¤ë */
  makeKigoGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return(0);
}

/*
 * µ­¹æ°ìÍ÷¹ÔÃæ¤ÎÁ°¤Îµ­¹æ¤Ë°ÜÆ°¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0
 */
static
int KigoBackwardKouho(uiContext d)
{
  ichiranContext kc = (ichiranContext)d->modec;
  int  headkouho;

  /* Á°¤Îµ­¹æ¤Ë¤¹¤ë */
  --*(kc->curIkouho);

  /* °ìÍ÷É½¼¨¤ÎÀèÆ¬¤Îµ­¹æ¤À¤Ã¤¿¤é¡¢Á°¤Î°ìÍ÷¹Ô¤ÎºÇ½ªµ­¹æ¤ò¥«¥ì¥ó¥Èµ­¹æ¤È¤¹¤ë */
  if(*(kc->curIkouho) < 0) {
    headkouho  = kc->glineifp->glhead - kc->nIkouho;
    if(headkouho < 0)
      headkouho = ((KIGOSU - 1) / kc->nIkouho) * kc->nIkouho;
    makeKigoInfo(d, headkouho);
    *(kc->curIkouho) = kc->glineifp->glkosu - 1;
  }

  /* kanji_status_retusrn ¤òºî¤ë */
  makeKigoGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return(0);
}

/*
 * Á°µ­¹æ°ìÍ÷Îó¤Ë°ÜÆ°¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0
 */
static
int KigoPreviousKouhoretsu(uiContext d)
{
  ichiranContext kc = (ichiranContext)d->modec;
  int headkouho;

  /** Á°¸õÊäÎó¤Ë¤¹¤ë **/
  headkouho  = kc->glineifp->glhead - kc->nIkouho;
  if(headkouho < 0)
    headkouho = ((KIGOSU -1) / kc->nIkouho) * kc->nIkouho;
  makeKigoInfo(d, headkouho);

  /* *(kc->curIkouho) ¤¬¥«¥ì¥ó¥Èµ­¹æ°ìÍ÷¤Îµ­¹æ¿ô¤è¤êÂç¤­¤¯¤Ê¤Ã¤Æ¤·¤Þ¤Ã¤¿¤é
     ºÇ±¦µ­¹æ¤ò¥«¥ì¥ó¥È¸õÊä¤È¤¹¤ë */
  if(*(kc->curIkouho) >= kc->glineifp->glkosu)
    *(kc->curIkouho) = kc->glineifp->glkosu - 1;

  /* kanji_status_retusrn ¤òºî¤ë */
  makeKigoGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return(0);
}

/*
 * ¼¡µ­¹æ°ìÍ÷Îó¤Ë°ÜÆ°¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0
 */
static
int KigoNextKouhoretsu(uiContext d)
{
  ichiranContext kc = (ichiranContext)d->modec;
  int headkouho;

  /** ¼¡¸õÊäÎó¤Ë¤¹¤ë **/
  headkouho = kc->glineifp->glhead + kc->nIkouho;
  if(headkouho >= KIGOSU)
    headkouho = 0;
  makeKigoInfo(d, headkouho);

  /* *(kc->curIkouho) ¤¬¥«¥ì¥ó¥Èµ­¹æ°ìÍ÷¤Îµ­¹æ¿ô¤è¤êÂç¤­¤¯¤Ê¤Ã¤Æ¤·¤Þ¤Ã¤¿¤é
     ºÇ±¦µ­¹æ¤ò¥«¥ì¥ó¥È¸õÊä¤È¤¹¤ë */
  if(*(kc->curIkouho) >= kc->glineifp->glkosu)
    *(kc->curIkouho) = kc->glineifp->glkosu - 1;

  /* kanji_status_retusrn ¤òºî¤ë */
  makeKigoGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return(0);
}

/*
 * µ­¹æ°ìÍ÷¹ÔÃæ¤ÎÀèÆ¬¤Îµ­¹æ¤Ë°ÜÆ°¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0
 */
static
int KigoBeginningOfKouho(uiContext d)
{
  ichiranContext kc = (ichiranContext)d->modec;

  /* ¸õÊäÎó¤ÎÀèÆ¬¸õÊä¤ò¥«¥ì¥ó¥È¸õÊä¤Ë¤¹¤ë */
  *(kc->curIkouho) = 0;

  /* kanji_status_retusrn ¤òºî¤ë */
  makeKigoGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return(0);
}

/*
 * µ­¹æ°ìÍ÷¹ÔÃæ¤ÎºÇ±¦¤Îµ­¹æ¤Ë°ÜÆ°¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0
 */
static
int KigoEndOfKouho(uiContext d)
{
  ichiranContext kc = (ichiranContext)d->modec;

  /** ¸õÊäÎó¤ÎºÇ±¦¸õÊä¤ò¥«¥ì¥ó¥È¸õÊä¤Ë¤¹¤ë **/
  *(kc->curIkouho) = kc->glineifp->glkosu - 1;

  /* kanji_status_retusrn ¤òºî¤ë */
  makeKigoGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return(0);
}

/*
 * µ­¹æ°ìÍ÷¹ÔÃæ¤«¤éÁªÂò¤µ¤ì¤¿µ­¹æ¤ò³ÎÄê¤¹¤ë
 *
 * ¡¦¼¡¤Ëµ­¹æ°ìÍ÷¤·¤¿»þ¤ËÁ°²ó³ÎÄê¤·¤¿µ­¹æ¤¬¥«¥ì¥ó¥È¸õÊä¤È¤Ê¤ë¤è¤¦¤Ë¡¢
 *   ³ÎÄê¤·¤¿¸õÊä¤ò¥»¡¼¥Ö¤·¤Æ¤ª¤¯
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0
 */
static
int KigoKakutei(uiContext d)
{
  ichiranContext kc = (ichiranContext)d->modec;

  /* ¥«¥ì¥ó¥Èµ­¹æ¤ò¥»¡¼¥Ö¤¹¤ë */
  d->curkigo = kc->glineifp->glhead + *(kc->curIkouho);

  /* ¥¨¥³¡¼¥¹¥È¥ê¥ó¥°¤ò³ÎÄêÊ¸»úÎó¤È¤¹¤ë */
  if (d->n_buffer >= KIGOSIZE) {
    d->nbytes = KIGOSIZE;
    WStrncpy(d->buffer_return, kc->kouhoifp[*(kc->curIkouho)].khdata, 
	    d->nbytes);
    d->buffer_return[KIGOSIZE] = (WCHAR_T)0;
  }
  else {
    d->nbytes = 0;
  }

  if (kc->flags & ICHIRAN_STAY_LONG) {
    kc->flags |= ICHIRAN_NEXT_EXIT;
    d->status = EVERYTIME_CALLBACK;
  }
  else {
    freeIchiranBuf(kc);
    popKigoMode(d);
    GlineClear(d);

    d->status = EXIT_CALLBACK;
  }

  return(d->nbytes);
}

#ifdef	SOMEONE_USES_THIS
/*
 * µ­¹æ°ìÍ÷¹ÔÃæ¤ÎÆþÎÏ¤µ¤ì¤¿ÈÖ¹æ¤Îµ­¹æ¤Ë°ÜÆ°¤¹¤ë  ¡ÚÌ¤»ÈÍÑ¡Û
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0
 */
static
KigoBangoKouho(uiContext d)
{
  ichiranContext kc = (ichiranContext)d->modec;
  int num;

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
    return NothingChangedWithBeep(d);
  }
  /* ÆþÎÏ¥Ç¡¼¥¿¤Ï ¸õÊä¹Ô¤ÎÃæ¤ËÂ¸ºß¤¹¤ë¿ô¤«¡© */
  if(num >= kc->glineifp->glkosu) {
    /* ÆþÎÏ¤µ¤ì¤¿ÈÖ¹æ¤ÏÀµ¤·¤¯¤¢¤ê¤Þ¤»¤ó */
    return NothingChangedWithBeep(d);
  }

  /* ¸õÊäÎó¤ÎÀèÆ¬¸õÊä¤òÆÀ¤ë */
  *(kc->curIkouho) = num;

  /* SelectDirect ¤Î¥«¥¹¥¿¥Þ¥¤¥º¤Î½èÍý */
  if (cannaconf.SelectDirect) /* ON */ {
    return(KigoKakutei(d));
  } else           /* OFF */ {
    /* kanji_status_retusrn ¤òºî¤ë */
    makeKigoGlineStatus(d);

    return(0);
  }
}
#endif	/* SOMEONE_USES_THIS */

/*
 * µ­¹æ°ìÍ÷¹Ô¤ò¾Ãµî¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0
 */
static
int KigoQuit(uiContext d)
{
  ichiranContext kc = (ichiranContext)d->modec;
  BYTE fl = kc->flags;

  freeIchiranBuf(kc);
  popKigoMode(d);
  /* gline ¤ò¥¯¥ê¥¢¤¹¤ë */
  GlineClear(d);
  d->status = (fl & ICHIRAN_NEXT_EXIT) ? EXIT_CALLBACK : QUIT_CALLBACK;
  return 0;
}
#endif /* NO_EXTEND_MENU */

#include	"kigomap.h"
