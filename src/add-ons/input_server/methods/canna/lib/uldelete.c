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
static char rcs_id[] = "@(#) 102.1 $Id: uldelete.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif


#if !defined(NO_EXTEND_MENU) && !defined(WIN)
#include	<errno.h>
#include 	"canna.h"
#include "RK.h"
#include "RKintern.h"

static int uuSYomiEveryTimeCatch(uiContext d, int retval, mode_context env);
static int uuSYomiExitCatch(uiContext d, int retval, mode_context env);
static int uuSYomiQuitCatch(uiContext d, int retval, mode_context env);
static int dicSakujoYomi(uiContext d);
static int acDicSakujoYomi(uiContext d, int dn, mode_context dm);
static int acDicSakujoDictionary(uiContext d, int dn, mode_context dm);
static WCHAR_T **getMountDicName(uiContext d, int *num_return);
static void CloseDeleteContext(tourokuContext tc);
static int getEffectDic(tourokuContext tc);
static int uuSTangoExitCatch(uiContext d, int retval, mode_context env);
static int uuSTangoQuitCatch(uiContext d, int retval, mode_context env);
static int dicSakujoBgnBun(uiContext d, RkStat *st);
static int dicSakujoEndBun(uiContext d);
static int dicSakujoTango(uiContext d);
static int getDeleteDic(mountContext mc);
static int uuSDicExitCatch(uiContext d, int retval, mode_context env);
static int uuSDicQuitCatch(uiContext d, int retval, mode_context env);
static int dicSakujoDictionary(uiContext d);
static int uuSDeleteYesCatch(uiContext d, int retval, mode_context env);
static int uuSDeleteQuitCatch(uiContext d, int retval, mode_context env);
static int uuSDeleteNoCatch(uiContext d, int retval, mode_context env);
static int dicSakujoDo(uiContext d);

extern int RkwGetServerVersion (int *, int *);
extern int RkwChmodDic (int, char *, int);

static int dicSakujoYomi (uiContext),
           dicSakujoEndBun (uiContext),
           dicSakujoTango (uiContext),
           dicSakujoDictionary (uiContext),
           dicSakujoDo (uiContext);

void
freeWorkDic3(tourokuContext tc)
{
  if (tc->workDic3) {
    free(tc->workDic3);
    tc->workDic3 = (deldicinfo *)0;
  }
}

void
freeWorkDic(tourokuContext tc)
{
  if (tc->workDic2) {
    free(tc->workDic2);
    tc->workDic2 = (deldicinfo *)0;
  }
  freeWorkDic3(tc);
}

void
freeDic(tourokuContext tc)
{
  if (tc->udic) {
    WCHAR_T **p = tc->udic;

    for ( ; *p; p++) {
      WSfree(*p);
    }
    free(tc->udic);
  }
  freeWorkDic(tc);
}

void
freeAndPopTouroku(uiContext d)
{
  tourokuContext tc = (tourokuContext)d->modec;

  freeDic(tc);
  popTourokuMode(d);
  popCallback(d);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Ã±¸ìºï½ü¤ÎÆÉ¤ß¤ÎÆþÎÏ                                                      *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static
int uuSYomiEveryTimeCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  int len, echoLen, revPos;
  WCHAR_T tmpbuf[ROMEBUFSIZE];

  retval = 0;
  if((echoLen = d->kanji_status_return->length) < 0)
    return(retval);

  if (echoLen == 0) {
    d->kanji_status_return->revPos = 0;
    d->kanji_status_return->revLen = 0;
  }

  /* ¼è¤ê¤¢¤¨¤º echoStr ¤¬ d->genbuf ¤«¤â¤·¤ì¤Ê¤¤¤Î¤Ç copy ¤·¤Æ¤ª¤¯ */
  WStrncpy(tmpbuf, d->kanji_status_return->echoStr, echoLen);

  revPos = MBstowcs(d->genbuf, "\306\311\244\337?[", ROMEBUFSIZE);
				/* ÆÉ¤ß */
  WStrncpy(d->genbuf + revPos, tmpbuf, echoLen);
  *(d->genbuf + revPos + echoLen) = (WCHAR_T) ']';
  len = revPos + echoLen + 1;
  *(d->genbuf + len) = (WCHAR_T) '\0';
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
int uuSYomiExitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  tourokuContext tc;

  popCallback(d); /* ÆÉ¤ß¤ò pop */

  tc = (tourokuContext)d->modec;

  WStrncpy(tc->yomi_buffer, d->buffer_return, retval);
  tc->yomi_buffer[retval] = (WCHAR_T)'\0';
  tc->yomi_len = WStrlen(tc->yomi_buffer);

  return dicSakujoTango(d);
}

static
int uuSYomiQuitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  popCallback(d); /* ÆÉ¤ß¤ò pop */

  clearYomi(d);

  freeAndPopTouroku(d);
  GlineClear(d);
  currentModeInfo(d);

  return prevMenuIfExist(d);
}

static
int dicSakujoYomi(uiContext d)
{
  yomiContext yc;

  d->status = 0;

  yc = GetKanjiString(d, (WCHAR_T *)NULL, 0,
	       CANNA_NOTHING_RESTRICTED,
	       (int)CANNA_YOMI_CHGMODE_INHIBITTED,
	       (int)CANNA_YOMI_END_IF_KAKUTEI,
	       (CANNA_YOMI_INHIBIT_HENKAN | CANNA_YOMI_INHIBIT_ASHEX |
	       CANNA_YOMI_INHIBIT_ASBUSHU),
	       uuSYomiEveryTimeCatch, uuSYomiExitCatch,
	       uuSYomiQuitCatch);
  if (yc == (yomiContext)0) {
    deleteEnd(d);
    return NoMoreMemory();
  }
  yc->majorMode = CANNA_MODE_ExtendMode;
  yc->minorMode = CANNA_MODE_DeleteDicMode;
  currentModeInfo(d);

  return 0;
}

static
int acDicSakujoYomi(uiContext d, int dn, mode_context dm)
/* ARGSUSED */
{
  popCallback(d);
  return dicSakujoYomi(d);
}

static
int acDicSakujoDictionary(uiContext d, int dn, mode_context dm)
/* ARGSUSED */
{
  popCallback(d);
  return dicSakujoDictionary(d);
}

/*
 * ¥Þ¥¦¥ó¥È¤µ¤ì¤Æ¤¤¤ë¼­½ñ¤«¤é WRITE ¸¢¤Î¤¢¤ë¤â¤Î¤ò¼è¤ê½Ð¤¹
 */
static
WCHAR_T **
getMountDicName(uiContext d, int *num_return)
/* ARGSUSED */
{
  int nmmdic, check, majv, minv;
  struct dicname *p;
  WCHAR_T **tourokup, **tp;
  extern int defaultContext;

  if (defaultContext < 0) {
    if ((KanjiInit() < 0) || (defaultContext < 0)) {
#ifdef STANDALONE
#ifndef WIN
      jrKanjiError = "¤«¤Ê´Á»úÊÑ´¹¤Ç¤­¤Þ¤»¤ó";
#else
      jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\307\244\255\244\336\244\273\244\363";
#endif
#else
#ifndef WIN
      jrKanjiError = "¤«¤Ê´Á»úÊÑ´¹¥µ¡¼¥Ð¤ÈÄÌ¿®¤Ç¤­¤Þ¤»¤ó";
#else
      jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\245\265"
	"\241\274\245\320\244\310\304\314\277\256\244\307\244\255\244\336"
	"\244\273\244\363";    /* ¤«¤Ê´Á»úÊÑ´¹¥µ¡¼¥Ð¤ÈÄÌ¿®¤Ç¤­¤Þ¤»¤ó */
#endif
#endif /* STANDALONE */
      return 0;
    }
  }

  /* ¥µ¡¼¥Ð¤Î Version ¤Ë¤è¤Ã¤Æ¼è¤ê½Ð¤¹¼­½ñ¤òÊ¬¤±¤ë */
  RkwGetServerVersion(&majv, &minv);

  if (canna_version(majv, minv) < canna_version(3, 2)) {
    /* Version3.2 ¤è¤êÁ°¤Î¥µ¡¼¥Ð¤Î¾ì¹ç */
    for (nmmdic = 0, p = kanjidicnames; p; p = p->next) {
      if (p->dicflag == DIC_MOUNTED && p->dictype == DIC_USER) {
        nmmdic++;
      }
    }
  }
  else {
    /* Version3.2 °Ê¹ß¤Î¥µ¡¼¥Ð¤Î¾ì¹ç */
    for (nmmdic = 0, p = kanjidicnames ; p ; p = p->next) {
      if (p->dicflag == DIC_MOUNTED) {
        check = RkwChmodDic(defaultContext, p->name, 0);
        if (check >= 0 && (check & RK_ENABLE_WRITE)) {
          nmmdic++;
        }
      }
    }
  }

  /* return BUFFER ¤Î alloc */
  if ((tourokup = (WCHAR_T **)calloc(nmmdic + 1, sizeof(WCHAR_T *)))
                                                  == (WCHAR_T **)NULL) {
    /* + 1 ¤Ê¤Î¤ÏÂÇ¤Á»ß¤á¥Þ¡¼¥¯¤ò¤¤¤ì¤ë¤¿¤á */
    jrKanjiError = "malloc (getMountDicName) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
                       /* ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
    return 0;
  }

  if (canna_version(majv, minv) < canna_version(3, 2)) {
    /* Version3.2 ¤è¤êÁ°¤Î¥µ¡¼¥Ð¤Î¾ì¹ç */
    for (tp = tourokup + nmmdic, p = kanjidicnames ; p ; p = p->next) {
      if (p->dicflag == DIC_MOUNTED && p->dictype == DIC_USER) {
        *--tp = WString(p->name);
      }
    }
  }
  else {
    /* Version3.2 °Ê¹ß¤Î¥µ¡¼¥Ð¤Î¾ì¹ç */
    for (tp = tourokup + nmmdic, p = kanjidicnames ; p ; p = p->next) {
      if (p->dicflag == DIC_MOUNTED) {
        check = RkwChmodDic(defaultContext, p->name, 0);
        if (check >= 0 && (check & RK_ENABLE_WRITE)) {
          *--tp = WString(p->name);
        }
      }
    }
  }
  tourokup[nmmdic] = (WCHAR_T *)0;
  *num_return = nmmdic;

  return tourokup;
}

int dicSakujo(uiContext d)
{
  WCHAR_T **mp, **p;
  tourokuContext tc;
  int num;
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }    
  d->status = 0;

  /* ¥Þ¥¦¥ó¥È¤µ¤ì¤Æ¤¤¤ë¼­½ñ¤Ç WRITE ¸¢¤Î¤¢¤ë¤â¤Î¤ò¼è¤Ã¤Æ¤¯¤ë */
  if ((mp = getMountDicName(d, &num)) != 0) {
    if (getTourokuContext(d) != NG) {
      tc = (tourokuContext)d->modec;

      tc->udic = mp;
      if(!*mp) {
        makeGLineMessageFromString(d, "\303\261\270\354\272\357\275\374"
	"\262\304\307\275\244\312\274\255\275\361\244\254\302\270\272\337"
	"\244\267\244\336\244\273\244\363");           
			/* Ã±¸ìºï½ü²ÄÇ½¤Ê¼­½ñ¤¬Â¸ºß¤·¤Þ¤»¤ó */
     
        freeAndPopTouroku(d);
        deleteEnd(d);
        currentModeInfo(d);
        return 0;
      }
      tc->nudic = num;
      return dicSakujoYomi(d);
    }
    for ( p = mp; *p; p++) {
      WSfree(*p);
    }
    free(mp);
  }
  deleteEnd(d);
  return GLineNGReturn(d);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Ã±¸ìºï½ü¤ÎÃ±¸ì¤ÎÆþÎÏ                                                      *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void
CloseDeleteContext(tourokuContext tc)
{
  if(tc->delContext >= 0) {
    if (RkwCloseContext(tc->delContext) < 0) {
      if (errno == EPIPE) {
	jrKanjiPipeError();
      }
    }
  }
#ifdef DEBUG
  else
    printf("ERROR: delContext < 0\n");
#endif
}

/*
 * »ØÄê¤µ¤ì¤¿Ã±¸ì¤¬ÅÐÏ¿¤µ¤ì¤Æ¤¤¤ë¼­½ñ¤ò¼è¤ê½Ð¤¹
 */
static
int getEffectDic(tourokuContext tc)
{
  int workContext, currentkouho, nbunsetsu, nelem = tc->nudic;
  WCHAR_T **mdic, **cands, **work;
  char dicname[1024], tmpbuf[64];
  RkLex lex[5];
  deldicinfo *dic;

  dic = (deldicinfo *)malloc((nelem + 1) * sizeof(deldicinfo));
  if (dic == (deldicinfo *)NULL) {
#ifndef WIN
    jrKanjiError = "malloc (getEffectDic) ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
#else
    jrKanjiError = "malloc (getEffectDic) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
#endif
    return NG;
  }
  tc->workDic2 = dic;

  if ((workContext = RkwCreateContext()) == NG) {
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
#ifndef WIN
    jrKanjiError = "¼­½ñ¸¡º÷ÍÑ¥³¥ó¥Æ¥¯¥¹¥È¤òºîÀ®¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
#else
    jrKanjiError = "\274\255\275\361\270\241\272\367\315\321\245\263\245\363"
	"\245\306\245\257\245\271\245\310\244\362\272\356\300\256\244\307"
	"\244\255\244\336\244\273\244\363\244\307\244\267\244\277";
#endif
    return NG;
  }

#ifdef STANDALONE
  if ((RkwSetDicPath(workContext, "iroha")) == NG) {
#ifndef WIN
    jrKanjiError = "¼­½ñ¥Ç¥£¥ì¥¯¥È¥ê¤òÀßÄê¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
#else
    jrKanjiError = "\274\255\275\361\245\307\245\243\245\354\245\257\245\310\245\352\244\362\300\337\304\352\244\307\244\255\244\336\244\273\244\363\244\307\244\267\244\277";
#endif
    CloseDeleteContext(tc);
    return NG;
  }
#endif /* STANDALONE */

  for (mdic = tc->udic; *mdic; mdic++) {
    WCstombs(dicname, *mdic, sizeof(dicname));
    if (RkwMountDic(workContext, dicname, 0) == NG) {
      if (errno == EPIPE) {
        jrKanjiPipeError();
      }
      jrKanjiError = "\274\255\275\361\270\241\272\367\315\321\245\263\245\363"
	"\245\306\245\257\245\271\245\310\244\313\274\255\275\361\244\362"
	"\245\336\245\246\245\363\245\310\244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
              /* ¼­½ñ¸¡º÷ÍÑ¥³¥ó¥Æ¥¯¥¹¥È¤Ë¼­½ñ¤ò¥Þ¥¦¥ó¥È¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
      RkwCloseContext(workContext);
      return NG;
    }

    nbunsetsu = RkwBgnBun(workContext, tc->yomi_buffer, tc->yomi_len, 0);
    if (nbunsetsu == 1) {
      if ((cands = getIchiranList(workContext, &nelem, &currentkouho)) != 0) {
        work = cands;
        while (*work) {
          if (WStrcmp(*work, tc->tango_buffer) == 0) {
            dic->name = *mdic;
            if (RkwXfer(workContext, currentkouho) == NG) {
              if (errno == EPIPE)
                jrKanjiPipeError();
              jrKanjiError = "\245\253\245\354\245\363\245\310\270\365\312\344"
			     "\244\362\274\350\244\352\275\320\244\273\244\336"
			     "\244\273\244\363\244\307\244\267\244\277";
               /* ¥«¥ì¥ó¥È¸õÊä¤ò¼è¤ê½Ð¤»¤Þ¤»¤ó¤Ç¤·¤¿ */
              freeGetIchiranList(cands);
              RkwEndBun(workContext, 0);
              RkwUnmountDic(workContext, dicname);
              RkwCloseContext(workContext);
              return NG;
            }
            if (RkwGetLex(workContext, lex, 5) <= 0) {
              if (errno == EPIPE)
                jrKanjiPipeError();
              jrKanjiError = "\267\301\302\326\301\307\276\360\312\363\244\362"
		"\274\350\244\352\275\320\244\273\244\336\244\273\244\363"
		"\244\307\244\267\244\277";
                /* ·ÁÂÖÁÇ¾ðÊó¤ò¼è¤ê½Ð¤»¤Þ¤»¤ó¤Ç¤·¤¿ */
              freeGetIchiranList(cands);
              RkwEndBun(workContext, 0);
              RkwUnmountDic(workContext, dicname);
              RkwCloseContext(workContext);
              return NG;
            }
            sprintf((char *)tmpbuf, "#%d#%d", lex[0].rownum, lex[0].colnum);
            MBstowcs(dic->hcode, tmpbuf, 16);
            dic++;
            break;
          }
          work++;
        }
        freeGetIchiranList(cands);
      }
    }

    if (RkwEndBun(workContext, 0) == NG) {
      if (errno == EPIPE) {
        jrKanjiPipeError();
      }
      jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\316"
	"\275\252\316\273\244\313\274\272\307\324\244\267\244\336\244\267"
	"\244\277";
       /* ¤«¤Ê´Á»úÊÑ´¹¤Î½ªÎ»¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
      RkwUnmountDic(workContext, dicname);
      RkwCloseContext(workContext);
      return NG;
    }

    if (RkwUnmountDic(workContext, dicname) == NG) {
      if (errno == EPIPE) {
        jrKanjiPipeError();
      }
      jrKanjiError = "\274\255\275\361\270\241\272\367\315\321\244\316\274\255"
	"\275\361\244\362\245\242\245\363\245\336\245\246\245\363\245\310"
	"\244\307\244\255\244\336\244\273\244\363\244\307\244\267\244\277";
       /* ¼­½ñ¸¡º÷ÍÑ¤Î¼­½ñ¤ò¥¢¥ó¥Þ¥¦¥ó¥È¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
      RkwCloseContext(workContext);
      return NG;
    }
  }

  if (RkwCloseContext(workContext) < 0) {
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
    jrKanjiError = "\274\255\275\361\270\241\272\367\315\321\244\316\245\263"
	"\245\363\245\306\245\257\245\271\245\310\244\362\245\257\245\355"
	"\241\274\245\272\244\307\244\255\244\336\244\273\244\363\244\307"
	"\244\267\244\277";
     /* ¼­½ñ¸¡º÷ÍÑ¤Î¥³¥ó¥Æ¥¯¥¹¥È¤ò¥¯¥í¡¼¥º¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
    return NG;
  }

  dic->name = (WCHAR_T *)0;
  tc->nworkDic2 = dic - tc->workDic2;
  return 0;
}

static
int uuSTangoExitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  forichiranContext fc;
  tourokuContext tc;

  popCallback(d); /* °ìÍ÷¤ò pop */

  fc = (forichiranContext)d->modec;
  freeGetIchiranList(fc->allkouho);

  popForIchiranMode(d);
  popCallback(d);

  tc = (tourokuContext)d->modec;
  WStrcpy(tc->tango_buffer, d->buffer_return);
  tc->tango_buffer[d->nbytes] = 0;
  tc->tango_len = d->nbytes;

  d->nbytes = 0;

  if (getEffectDic(tc) == NG) {
    freeDic(tc);
    deleteEnd(d);
    return GLineNGReturnTK(d);
  }

  return dicSakujoDictionary(d);
}

static
int uuSTangoQuitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  forichiranContext fc;

  popCallback(d); /* °ìÍ÷¤ò pop */

  fc = (forichiranContext)d->modec;
  freeGetIchiranList(fc->allkouho);

  popForIchiranMode(d);
  popCallback(d);

  clearYomi(d);
  return dicSakujoYomi(d);
}

/*
 * ÆÉ¤ß¤ò»ØÄê¤µ¤ì¤¿¼­½ñ¤«¤éÊÑ´¹¤¹¤ë
 */
static
int dicSakujoBgnBun(uiContext d, RkStat *st)
{
  tourokuContext tc = (tourokuContext)d->modec;
  int nbunsetsu;
  char dicname[1024];
  WCHAR_T **mdic;

  if(!tc) {
#if !defined(DEBUG) && !defined(WIN)
    printf("tc = NULL\n");
#endif
  }
  if(!tc->udic) {
#if !defined(DEBUG) && !defined(WIN)
    printf("tc->udic = NULL\n");
#endif
  }

  if((tc->delContext = RkwCreateContext())== NG) {
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
    jrKanjiError = "\303\261\270\354\272\357\275\374\315\321\244\316\245\263"
	"\245\363\245\306\245\257\245\271\245\310\244\362\272\356\300\256"
	"\244\307\244\255\244\336\244\273\244\363";
     /* Ã±¸ìºï½üÍÑ¤Î¥³¥ó¥Æ¥¯¥¹¥È¤òºîÀ®¤Ç¤­¤Þ¤»¤ó */
    return(NG);
  }

#ifdef STANDALONE
  if ((RkwSetDicPath(tc->delContext, "iroha")) == NG) {
#ifndef WIN
    jrKanjiError = "¼­½ñ¥Ç¥£¥ì¥¯¥È¥ê¤òÀßÄê¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
#else
    jrKanjiError = "\274\255\275\361\245\307\245\243\245\354\245\257\245\310\245\352\244\362\300\337\304\352\244\307\244\255\244\336\244\273\244\363\244\307\244\267\244\277";
#endif
    CloseDeleteContext(tc);
    return NG;
  }
#endif /* STANDALONE */

  for (mdic = tc->udic; *mdic; mdic++) {
    WCstombs(dicname, *mdic, sizeof(dicname));
    if (RkwMountDic(tc->delContext, dicname, 0) == NG) {
      if (errno == EPIPE) {
        jrKanjiPipeError();
      }
      jrKanjiError = "\303\261\270\354\272\357\275\374\315\321\244\316\274"
	"\255\275\361\244\362\245\336\245\246\245\363\245\310\244\307\244"
	"\255\244\336\244\273\244\363\244\307\244\267\244\277";
        /* Ã±¸ìºï½üÍÑ¤Î¼­½ñ¤ò¥Þ¥¦¥ó¥È¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
      CloseDeleteContext(tc);
      return(NG);
    }
  }

  if((nbunsetsu = RkwBgnBun(tc->delContext, tc->yomi_buffer, tc->yomi_len, 0))
	== -1) {
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
    jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\313"
	"\274\272\307\324\244\267\244\336\244\267\244\277";
      /* ¤«¤Ê´Á»úÊÑ´¹¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    CloseDeleteContext(tc);
    return(NG);
  }
  
  if(RkwGetStat(tc->delContext, st) == -1) {
    RkwEndBun(tc->delContext, 0); /* 0:³Ø½¬¤·¤Ê¤¤ */
    if(errno == EPIPE)
      jrKanjiPipeError();
    jrKanjiError = "\245\271\245\306\245\244\245\277\245\271\244\362\274\350"
	"\244\352\275\320\244\273\244\336\244\273\244\363\244\307\244\267"
	"\244\277";
               /* ¥¹¥Æ¥¤¥¿¥¹¤ò¼è¤ê½Ð¤»¤Þ¤»¤ó¤Ç¤·¤¿ */
    /* ¥¢¥ó¥Þ¥¦¥ó¥È¤·¤Æ¤Ê¤¤ */
    CloseDeleteContext(tc);
    return(NG);
  }

  return(nbunsetsu);
}

static
int dicSakujoEndBun(uiContext d)
{
  tourokuContext tc = (tourokuContext)d->modec;

  if(RkwEndBun(tc->delContext, 0) == -1) {	/* 0:³Ø½¬¤·¤Ê¤¤ */
    if(errno == EPIPE)
      jrKanjiPipeError();
    jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\316"
	"\275\252\316\273\244\313\274\272\307\324\244\267\244\336\244\267"
	"\244\277";
       /* ¤«¤Ê´Á»úÊÑ´¹¤Î½ªÎ»¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    return(NG);
  }

  return(0);
}

static
int dicSakujoTango(uiContext d)
{
  tourokuContext tc = (tourokuContext)d->modec;
  forichiranContext fc;
  ichiranContext ic;
  WCHAR_T **allDelCands;
  BYTE inhibit = 0;
  int nbunsetsu, nelem, currentkouho, retval = 0;
  RkStat st;

  if(tc->yomi_len < 1) {
    return canna_alert(d, "\306\311\244\337\244\362\306\376\316\317\244\267"
	"\244\306\244\257\244\300\244\265\244\244", acDicSakujoYomi);
		/* ÆÉ¤ß¤òÆþÎÏ¤·¤Æ¤¯¤À¤µ¤¤ */ 
  }

  if((nbunsetsu = dicSakujoBgnBun(d, &st)) == NG) {
    freeDic(tc);
    deleteEnd(d);
    return(GLineNGReturnTK(d));
  }
  if((nbunsetsu != 1) || (st.maxcand == 0)) {
    /* ¸õÊä¤¬¤Ê¤¤ */
    if(dicSakujoEndBun(d) == NG) {
      freeDic(tc);
      CloseDeleteContext(tc);
      deleteEnd(d);
      return(GLineNGReturnTK(d));
    }

    makeGLineMessageFromString(d, "\244\263\244\316\306\311\244\337\244\307"
	"\305\320\317\277\244\265\244\354\244\277\303\261\270\354\244\317"
	"\302\270\272\337\244\267\244\336\244\273\244\363");
         /* ¤³¤ÎÆÉ¤ß¤ÇÅÐÏ¿¤µ¤ì¤¿Ã±¸ì¤ÏÂ¸ºß¤·¤Þ¤»¤ó */
    CloseDeleteContext(tc);
    freeAndPopTouroku(d);
    deleteEnd(d);
    currentModeInfo(d);
    return(0);
  }

  /* ¤¹¤Ù¤Æ¤Î¸õÊä¤ò¼è¤ê½Ð¤¹ */
  if((allDelCands = 
      getIchiranList(tc->delContext, &nelem, &currentkouho)) == 0) {
    freeDic(tc);
    dicSakujoEndBun(d);
    CloseDeleteContext(tc);
    deleteEnd(d);
    return(GLineNGReturnTK(d));
  }

  if (dicSakujoEndBun(d) == NG) {
    freeDic(tc); 
    CloseDeleteContext(tc);
    deleteEnd(d);
    return GLineNGReturnTK(d);
  }
  CloseDeleteContext(tc);

  if(getForIchiranContext(d) == NG) {
    freeDic(tc);
    freeGetIchiranList(allDelCands);
    deleteEnd(d);
    return(GLineNGReturnTK(d));
  }

  fc = (forichiranContext)d->modec;
  fc->allkouho = allDelCands;

  if (!cannaconf.HexkeySelect)
    inhibit |= ((BYTE)NUMBERING | (BYTE)CHARINSERT);
  else
    inhibit |= (BYTE)CHARINSERT;

  fc->curIkouho = currentkouho;	/* ¸½ºß¤Î¥«¥ì¥ó¥È¸õÊäÈÖ¹æ¤òÊÝÂ¸¤¹¤ë */
  currentkouho = 0;	/* ¥«¥ì¥ó¥È¸õÊä¤«¤é²¿ÈÖÌÜ¤ò¥«¥ì¥ó¥È¸õÊä¤È¤¹¤ë¤« */

  /* ¸õÊä°ìÍ÷¤Ë°Ü¹Ô¤¹¤ë */
  if((retval = selectOne(d, fc->allkouho, &fc->curIkouho, nelem, BANGOMAX,
               inhibit, currentkouho, WITHOUT_LIST_CALLBACK,
	       NO_CALLBACK, uuSTangoExitCatch,
	       uuSTangoQuitCatch, uiUtilIchiranTooSmall)) == NG) {
    freeDic(tc);
    freeGetIchiranList(fc->allkouho);
    deleteEnd(d);
    return(GLineNGReturnTK(d));
  }

  ic = (ichiranContext)d->modec;
  ic->majorMode = CANNA_MODE_ExtendMode;
  ic->minorMode = CANNA_MODE_DeleteDicMode;
  currentModeInfo(d);

  if(ic->tooSmall) {
    d->status = AUX_CALLBACK;
    return(retval);
  }

  makeGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return(retval);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Ã±¸ìºï½ü¤Î¼­½ñ°ìÍ÷                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
int getDeleteDic(mountContext mc)
{
  tourokuContext tc = (tourokuContext)mc->next;
  int i, num = 0;
  deldicinfo *dic, *srcp;

  /* ¤Þ¤º¡¢Ã±¸ìºï½ü¤¹¤ë¼­½ñ¤Î¿ô¤ò¿ô¤¨¤ë */
  for (i = 0; mc->mountList[i]; i++) {
    if (mc->mountOldStatus[i] != mc->mountNewStatus[i]) {
      num++;
    }
  }

  dic = (deldicinfo *)malloc((num + 1) * sizeof(deldicinfo));
  if (dic != (deldicinfo *)NULL) {
    tc->workDic3 = dic;

    /* ¤É¤Î¼­½ñ¤«¤éÃ±¸ì¤òºï½ü¤¹¤ë¤« */
    srcp = tc->workDic2;
    for (i = 0; mc->mountList[i]; i++, srcp++) {
      if (mc->mountOldStatus[i] != mc->mountNewStatus[i]) {
        *dic++ = *srcp;
      }
    }
    dic->name = (WCHAR_T *)0;
    tc->nworkDic3 = dic - tc->workDic3;
    return 0;
  }
  jrKanjiError ="malloc (uuSDicExitCatch) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
                 /* ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
  return NG;
}


static
int uuSDicExitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  mountContext mc;
  tourokuContext tc;

  d->nbytes = 0;

  popCallback(d); /* °ìÍ÷¤ò pop */

  mc = (mountContext)d->modec;

  if (getDeleteDic(mc) == NG) {
    popMountMode(d);
    popCallback(d);
    tc = (tourokuContext)d->modec;
    freeDic(tc);
    deleteEnd(d);
    return GLineNGReturnTK(d);
  }

  popMountMode(d);
  popCallback(d);

  tc = (tourokuContext)d->modec;
  /* ¼­½ñ¤¬ÁªÂò¤µ¤ì¤Ê¤«¤Ã¤¿¤È¤­¤Ï¡¢¥á¥Ã¥»¡¼¥¸¤ò½Ð¤·¡¢
     ²¿¤«¤Î¥­¡¼¤¬ÆþÎÏ¤µ¤ì¤¿¤é¡¢ ¼­½ñÁªÂò¤ËÌá¤ë¡£     */
  if (tc->nworkDic3 == 0) {
    return canna_alert(d, "\274\255\275\361\244\362\301\252\302\362\244\267"
	"\244\306\244\257\244\300\244\265\244\244", acDicSakujoDictionary);
		/* ¼­½ñ¤òÁªÂò¤·¤Æ¤¯¤À¤µ¤¤ */
  }

  return dicSakujoDo(d);
}

static
int uuSDicQuitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  popCallback(d); /* °ìÍ÷¤ò pop */

  popMountMode(d);
  popCallback(d);

  freeWorkDic((tourokuContext)d->modec);
  return dicSakujoTango(d);
}

static
int dicSakujoDictionary(uiContext d)
{
  tourokuContext tc = (tourokuContext)d->modec;
  mountContext mc;
  ichiranContext ic;
  deldicinfo *work;
  BYTE inhibit = 0;
  int retval, i, upnelem = tc->nworkDic2;
  char *dicLbuf, **dicLp, *wptr;
  BYTE *soldp, *snewp;
  WCHAR_T *xxxx[100];

  retval = d->nbytes = 0;
  d->status = 0;

  if (upnelem == 1) {
    work
      = (deldicinfo *)malloc((1 /* upnelem(==1) */ + 1) * sizeof(deldicinfo));
    if (work != (deldicinfo *)NULL) {
      tc->workDic3 = work;
      *work++ = *tc->workDic2; /* ¹½Â¤ÂÎ¤ÎÂåÆþ */
      work->name = (WCHAR_T *)0;
      tc->nworkDic3 = 1; /* work - tc->workDic3 == 1 */
      return dicSakujoDo(d);
    }
    jrKanjiError = "malloc (dicSakujoDictionary) \244\307\244\255\244\336"
	"\244\273\244\363\244\307\244\267\244\277";
                 /* ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
    freeDic(tc);
    deleteEnd(d);
    return GLineNGReturnTK(d);
  }

  if ((dicLbuf = (char *)malloc(ROMEBUFSIZE)) != (char *)NULL) {
    if ((dicLp = (char **)calloc(upnelem + 1, sizeof(char *)))
                                               != (char **)NULL) {
      wptr = dicLbuf;
      for (work = tc->workDic2; work->name; work++) {
        i = WCstombs(wptr, work->name, ROMEBUFSIZE);
        wptr += i;
        *wptr++ = '\0';
      }
      for (wptr = dicLbuf, i = 0; i < upnelem ; i++) {
        dicLp[i] = wptr;
        while (*wptr++)
          /* EMPTY */
          ;
      }
      dicLp[i] = (char *)NULL;

      /* ¸½ºß¤Î¾õÂÖ¤Ï¤¹¤Ù¤Æ off ¤Ë¤·¤Æ¤ª¤¯ */
      if ((soldp = (BYTE *)calloc(upnelem + 1, sizeof(BYTE)))
                                               != (BYTE *)NULL) {
        if ((snewp = (BYTE *)calloc(upnelem + 1, sizeof(BYTE)))
                                                 != (BYTE *)NULL) {
          if ((retval = getMountContext(d)) != NG) {
            mc = (mountContext)d->modec;
            mc->mountOldStatus = soldp;
            mc->mountNewStatus = snewp;
            mc->mountList = dicLp;

            /* selectOnOff ¤ò¸Æ¤Ö¤¿¤á¤Î½àÈ÷ */

            mc->curIkouho = 0;
            if (!cannaconf.HexkeySelect)
              inhibit |= ((BYTE)NUMBERING | (BYTE)CHARINSERT);
            else
              inhibit |= (BYTE)CHARINSERT;

            retval = setWStrings(xxxx, mc->mountList, 0);
            if (retval == NG) {
              popMountMode(d);
              popCallback(d);
              deleteEnd(d);
              return GLineNGReturnTK(d);
            }
            if ((retval = selectOnOff(d, xxxx, &mc->curIkouho, upnelem,
		            BANGOMAX, 0, mc->mountOldStatus,
		            (int(*)(...))NO_CALLBACK, (int(*)(...))uuSDicExitCatch, 
                            (int(*)(...))uuSDicQuitCatch,
							(int(*)(...))uiUtilIchiranTooSmall)) == NG) {
              popMountMode(d);
              popCallback(d);
              deleteEnd(d);
              return GLineNGReturnTK(d);
            }

            ic = (ichiranContext)d->modec;
            ic->majorMode = CANNA_MODE_ExtendMode;
            ic->minorMode = CANNA_MODE_DeleteDicMode;
            currentModeInfo(d);

            /* ¸õÊä°ìÍ÷¹Ô¤¬¶¹¤¯¤Æ¸õÊä°ìÍ÷¤¬½Ð¤»¤Ê¤¤ */
            if (ic->tooSmall) {
              jrKanjiError = "\274\255\275\361\260\354\315\367\315\321\244\316"
		"\311\375\244\254\266\271\244\244\244\316\244\307\274\255"
		"\275\361\244\316\301\252\302\362\244\254\244\307\244\255"
		"\244\336\244\273\244\363";
                /* ¼­½ñ°ìÍ÷ÍÑ¤ÎÉý¤¬¶¹¤¤¤Î¤Ç¼­½ñ¤ÎÁªÂò¤¬¤Ç¤­¤Þ¤»¤ó */
              ichiranFin(d);
              popCallback(d); /* OnOff ¤ò¥Ý¥Ã¥× */
              popMountMode(d);
              popCallback(d);
              currentModeInfo(d);
              freeDic(tc);
              deleteEnd(d);
              return GLineNGReturnTK(d);
            }

            makeGlineStatus(d);
            /* d->status = ICHIRAN_EVERYTIME; */

            return(retval);
          }
          free(snewp);
        }
        free(soldp);
      }
      free(dicLp);
    }
    free(dicLbuf);
  }
  jrKanjiError = "malloc (dicSakujoDictionary) \244\307\244\255\244\336"
	"\244\273\244\363\244\307\244\267\244\277";
     /* ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
  freeDic(tc);
  deleteEnd(d);
  return GLineNGReturnTK(d);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Ã±¸ìºï½ü                                                                  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
int uuSDeleteYesCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  tourokuContext tc;
  char dicname[1024];
  deldicinfo *dic;
  int bufcnt, l;
  extern int defaultContext;

  deleteEnd(d);
  popCallback(d); /* yesNo ¤ò¥Ý¥Ã¥× */

  tc = (tourokuContext)d->modec;

  if(defaultContext == -1) {
    if((KanjiInit() < 0) || (defaultContext == -1)) {
#ifdef STANDALONE
#ifndef WIN
      jrKanjiError = "¤«¤Ê´Á»úÊÑ´¹¤Ç¤­¤Þ¤»¤ó";
#else
      jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\307\244\255\244\336\244\273\244\363";
#endif
#else
#ifndef WIN
      jrKanjiError = "¤«¤Ê´Á»úÊÑ´¹¥µ¡¼¥Ð¤ÈÄÌ¿®¤Ç¤­¤Þ¤»¤ó";
#else
      jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\245\265"
	"\241\274\245\320\244\310\304\314\277\256\244\307\244\255\244\336"
	"\244\273\244\363";
#endif
#endif
      freeAndPopTouroku(d);
      return(GLineNGReturn(d));
    }
  }

  /* ¼­½ñ¤«¤éÃ±¸ì¤òºï½ü¤¹¤ë */
  /* Ã±¸ìºï½üÍÑ¤Î°ì¹Ô¤òºî¤ë(³Æ¼­½ñ¶¦ÄÌ) */
  WStraddbcpy(d->genbuf, tc->yomi_buffer, ROMEBUFSIZE);
  l = WStrlen(tc->yomi_buffer);
  d->genbuf[l] = (WCHAR_T)' ';
  l += 1;
  for (dic = tc->workDic3; dic->name; dic++) {
    /* Ã±¸ìºï½üÍÑ¤Î°ì¹Ô¤òºî¤ë(³Æ¼­½ñ¸ÇÍ­) */
    WStrcpy(d->genbuf + l, dic->hcode);
    bufcnt = l + WStrlen(dic->hcode);
    d->genbuf[bufcnt] = (WCHAR_T)' ';
    bufcnt += 1;
    WStraddbcpy(d->genbuf + bufcnt, tc->tango_buffer, 
                                                 ROMEBUFSIZE - bufcnt);

    WCstombs(dicname, dic->name, sizeof(dicname));
    if (RkwDeleteDic(defaultContext, dicname, d->genbuf) == NG) {
      if (errno == EPIPE)
        jrKanjiPipeError();
      MBstowcs(d->genbuf, "\303\261\270\354\272\357\275\374\244\307\244\255"
	"\244\336\244\273\244\363\244\307\244\267\244\277", 512);
		/* Ã±¸ìºï½ü¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
      goto close;
    }
    if (cannaconf.auto_sync) {
      RkwSync(defaultContext, dicname);
    }
  }

  /* ºï½ü¤Î´°Î»¤òÉ½¼¨¤¹¤ë */
  l = MBstowcs(d->genbuf, "\241\330", ROMEBUFSIZE);
			/* ¡Ø */
  WStrcpy(d->genbuf + l, tc->tango_buffer);
  l += WStrlen(tc->tango_buffer);
  l += MBstowcs(d->genbuf + l, "\241\331(", ROMEBUFSIZE - l);
				/* ¡Ù */
  WStrcpy(d->genbuf + l, tc->yomi_buffer);
  l += WStrlen(tc->yomi_buffer);
  l += MBstowcs(d->genbuf + l, ")\244\362\274\255\275\361 ", ROMEBUFSIZE - l);
			/* ¤ò¼­½ñ */
  dic = tc->workDic3;
  WStrcpy(d->genbuf + l, dic->name);
  l += WStrlen(dic->name);
  for (dic++; dic->name; dic++) {
    l += MBstowcs(d->genbuf + l, " \244\310 ", ROMEBUFSIZE - l);
				/* ¤È */
    WStrcpy(d->genbuf + l, dic->name);
    l += WStrlen(dic->name);
  }
  l += MBstowcs(d->genbuf + l, " \244\253\244\351\272\357\275\374\244\267"
	"\244\336\244\267\244\277", ROMEBUFSIZE - l);
			/* ¤«¤éºï½ü¤·¤Þ¤·¤¿ */

 close:
  makeGLineMessage(d, d->genbuf, WStrlen(d->genbuf));

  freeAndPopTouroku(d);

  currentModeInfo(d);

  return(0);
}

static
int uuSDeleteQuitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  tourokuContext tc = (tourokuContext)env;

  popCallback(d); /* yesNo ¤ò¥Ý¥Ã¥× */

  if (tc->nworkDic2 == 1) {
    freeWorkDic(tc);
    return dicSakujoTango(d);
  }
  freeWorkDic3(tc);
  return dicSakujoDictionary(d);
}

static
int uuSDeleteNoCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  popCallback(d); /* yesNo ¤ò¥Ý¥Ã¥× */

  freeAndPopTouroku(d);
  deleteEnd(d);
  currentModeInfo(d);

  GlineClear(d);

  return(retval);
}

static
int dicSakujoDo(uiContext d)
{
  tourokuContext tc = (tourokuContext)d->modec;
  int l;
  deldicinfo *dic;

  l = MBstowcs(d->genbuf, "\241\330", ROMEBUFSIZE);
			/* ¡Ø */
  WStrcpy(d->genbuf + l, tc->tango_buffer);
  l += WStrlen(tc->tango_buffer);
  l += MBstowcs(d->genbuf + l, "\241\331(", ROMEBUFSIZE - l);
				/* ¡Ù */
  WStrcpy(d->genbuf + l, tc->yomi_buffer);
  l += WStrlen(tc->yomi_buffer);
  l += MBstowcs(d->genbuf + l, ")\244\362\274\255\275\361 ", ROMEBUFSIZE - l);
			/* ¤ò¼­½ñ */
  dic = tc->workDic3;
  WStrcpy(d->genbuf + l, dic->name);
  l += WStrlen(dic->name);
  for (dic++; dic->name; dic++) {
    l += MBstowcs(d->genbuf + l, " \244\310¤È ", ROMEBUFSIZE - l);
				/* ¤È */
    WStrcpy(d->genbuf + l, dic->name);
    l += WStrlen(dic->name);
  }
  l += MBstowcs(d->genbuf + l, " \244\253\244\351\272\357\275\374\244\267"
	"\244\336\244\271\244\253?(y/n)", ROMEBUFSIZE - l);
		/* ¤«¤éºï½ü¤·¤Þ¤¹¤« */
  if (getYesNoContext(d,
	     NO_CALLBACK, uuSDeleteYesCatch,
	     uuSDeleteQuitCatch, uuSDeleteNoCatch) == NG) {
    freeDic(tc);
    deleteEnd(d);
    return(GLineNGReturnTK(d));
  }
  makeGLineMessage(d, d->genbuf, WStrlen(d->genbuf));

  return(0);
}
#endif /* !NO_EXTEND_MENU && !WIN */
