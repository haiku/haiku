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
static char rcs_id[] = "@(#) 102.1 $Id: ulhinshi.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif

#include <errno.h>
#include "canna.h"
#include "RK.h"
#include "RKintern.h"

#ifndef NO_EXTEND_MENU
#ifdef luna88k
extern int errno;
#endif

static void WSprintf(WCHAR_T *to_buf, WCHAR_T *x1, WCHAR_T *x2, WCHAR_T *from_buf);
static void EWStrcpy(WCHAR_T *buf, char *xxxx);
static int EWStrcmp(WCHAR_T *buf, char *xxxx);
static int EWStrncmp(WCHAR_T *buf, char *xxxx, int len);
static int uuTHinshiYNQuitCatch(uiContext d, int retval, mode_context env);
static int uuTHinshi2YesCatch(uiContext d, int retval, mode_context env);
static int uuTHinshi2NoCatch(uiContext d, int retval, mode_context env);
static int uuTHinshi1YesCatch(uiContext d, int retval, mode_context env);
static int uuTHinshi1NoCatch(uiContext d, int retval, mode_context env);
static int uuTHinshiQYesCatch(uiContext d, int retval, mode_context env);
static int uuTHinshiQNoCatch(uiContext d, int retval, mode_context env);
static int makeHinshi(uiContext d);
static int tourokuYes(uiContext d);
static int tourokuNo(uiContext d);
static void makeDoushi(uiContext d);
static int uuTDicExitCatch(uiContext d, int retval, mode_context env);
static int uuTDicQuitCatch(uiContext d, int retval, mode_context env);
static int tangoTouroku(uiContext d);

static char *e_message[] = {
//#ifndef WIN
#ifdef WIN
  /*0*/"¤µ¤é¤ËºÙ¤«¤¤ÉÊ»ìÊ¬¤±¤Î¤¿¤á¤Î¼ÁÌä¤ò¤·¤Æ¤âÎÉ¤¤¤Ç¤¹¤«?(y/n)",
  /*1*/"ÆÉ¤ß¤È¸õÊä¤ò ½ª»ß·Á¤ÇÆþÎÏ¤·¤Æ¤¯¤À¤µ¤¤¡£",
  /*2*/"ÆÉ¤ß¤È¸õÊä¤Î ³èÍÑ¤¬°ã¤¤¤Þ¤¹¡£ÆþÎÏ¤·¤Ê¤ª¤·¤Æ¤¯¤À¤µ¤¤¡£",
  /*3*/"ÆÉ¤ß¤È¸õÊä¤ò ½ª»ß·Á¤ÇÆþÎÏ¤·¤Æ¤¯¤À¤µ¤¤¡£Îã) Áá¤¤",
  /*4*/"ÆÉ¤ß¤È¸õÊä¤ò ½ª»ß·Á¤ÇÆþÎÏ¤·¤Æ¤¯¤À¤µ¤¤¡£Îã) ÀÅ¤«¤À",
  /*5*/"¡Ö",
  /*6*/"¤¹¤ë¡×¤ÏÀµ¤·¤¤¤Ç¤¹¤«?(y/n)",
  /*7*/"¤Ê¡×¤ÏÀµ¤·¤¤¤Ç¤¹¤«?(y/n)",
  /*8*/"¡×¤Ï¿ÍÌ¾¤Ç¤¹¤«?(y/n)",
  /*9*/"¡×¤ÏÃÏÌ¾¤Ç¤¹¤«?(y/n)",
  /*10*/"¤Ê¤¤¡×¤ÏÀµ¤·¤¤¤Ç¤¹¤«?(y/n)",
  /*11*/"¡×¤ÏÌ¾»ì¤È¤·¤Æ»È¤¤¤Þ¤¹¤«?(y/n)",
  /*12*/"¡×¤ÏÀµ¤·¤¤¤Ç¤¹¤«?(y/n)",
  /*13*/"¤È¡×¤ÏÀµ¤·¤¤¤Ç¤¹¤«?(y/n)",
#ifdef STANDALONE
  /*14*/"¤«¤Ê´Á»úÊÑ´¹¤Ç¤­¤Þ¤»¤ó",
#else
  /*14*/"¤«¤Ê´Á»úÊÑ´¹¥µ¡¼¥Ð¤ÈÄÌ¿®¤Ç¤­¤Þ¤»¤ó",
#endif
  /*15*/"Ã±¸ìÅÐÏ¿¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿",
  /*16*/"¡Ø",
  /*17*/"¡Ù",
  /*18*/"¡Ê",
  /*19*/"¡Ë¤òÅÐÏ¿¤·¤Þ¤·¤¿",
  /*20*/"Ã±¸ìÅÐÏ¿¤Ë¼ºÇÔ¤·¤Þ¤·¤¿",
#else
  /*0*/"\244\265\244\351\244\313\272\331\244\253\244\244\311\312\273\354\312\254\244\261\244\316\244\277\244\341\244\316\274\301\314\344\244\362\244\267\244\306\244\342\316\311\244\244\244\307\244\271\244\253?(y/n)",
       /* ¤µ¤é¤ËºÙ¤«¤¤ÉÊ»ìÊ¬¤±¤Î¤¿¤á¤Î¼ÁÌä¤ò¤·¤Æ¤âÎÉ¤¤¤Ç¤¹¤« */

  /*1*/"\306\311\244\337\244\310\270\365\312\344\244\362\40\275\252\273\337\267\301\244\307\306\376\316\317\244\267\244\306\244\257\244\300\244\265\244\244\241\243",
       /* ÆÉ¤ß¤È¸õÊä¤ò ½ª»ß·Á¤ÇÆþÎÏ¤·¤Æ¤¯¤À¤µ¤¤¡£*/

  /*2*/"\306\311\244\337\244\310\270\365\312\344\244\316\40\263\350\315\321\244\254\260\343\244\244\244\336\244\271\241\243\306\376\316\317\244\267\244\312\244\252\244\267\244\306\244\257\244\300\244\265\244\244\241\243",
       /* ÆÉ¤ß¤È¸õÊä¤Î ³èÍÑ¤¬°ã¤¤¤Þ¤¹¡£ÆþÎÏ¤·¤Ê¤ª¤·¤Æ¤¯¤À¤µ¤¤¡£*/

  /*3*/"\306\311\244\337\244\310\270\365\312\344\244\362\40\275\252\273\337\267\301\244\307\306\376\316\317\244\267\244\306\244\257\244\300\244\265\244\244\241\243\316\343) \301\341\244\244",
       /* ÆÉ¤ß¤È¸õÊä¤ò ½ª»ß·Á¤ÇÆþÎÏ¤·¤Æ¤¯¤À¤µ¤¤¡£Îã) Áá¤¤ */

  /*4*/"\306\311\244\337\244\310\270\365\312\344\244\362\40\275\252\273\337\267\301\244\307\306\376\316\317\244\267\244\306\244\257\244\300\244\265\244\244\241\243\316\343) \300\305\244\253\244\300",
       /* ÆÉ¤ß¤È¸õÊä¤ò ½ª»ß·Á¤ÇÆþÎÏ¤·¤Æ¤¯¤À¤µ¤¤¡£Îã) ÀÅ¤«¤À */

  /*5*/"\241\326",  /* ¡Ö */

  /*6*/"\244\271\244\353\241\327\244\317\300\265\244\267\244\244\244\307\244\271\244\253?(y/n)",
       /* ¤¹¤ë¡×¤ÏÀµ¤·¤¤¤Ç¤¹¤« */

  /*7*/"\244\312\241\327\244\317\300\265\244\267\244\244\244\307\244\271\244\253?(y/n)",
       /* ¤Ê¡×¤ÏÀµ¤·¤¤¤Ç¤¹¤« */

  /*8*/"\241\327\244\317\277\315\314\276\244\307\244\271\244\253?(y/n)",
       /* ¡×¤Ï¿ÍÌ¾¤Ç¤¹¤« */

  /*9*/"\241\327\244\317\303\317\314\276\244\307\244\271\244\253?(y/n)",
       /* ¡×¤ÏÃÏÌ¾¤Ç¤¹¤« */

  /*10*/"\244\312\244\244\241\327\244\317\300\265\244\267\244\244\244\307\244\271\244\253?(y/n)",
       /* ¤Ê¤¤¡×¤ÏÀµ¤·¤¤¤Ç¤¹¤« */

  /*11*/"\241\327\244\317\314\276\273\354\244\310\244\267\244\306\273\310\244\244\244\336\244\271\244\253?(y/n)",
       /* ¡×¤ÏÌ¾»ì¤È¤·¤Æ»È¤¤¤Þ¤¹¤« */

  /*12*/"\241\327\244\317\300\265\244\267\244\244\244\307\244\271\244\253?(y/n)",
       /* ¡×¤ÏÀµ¤·¤¤¤Ç¤¹¤« */

  /*13*/"\244\310\241\327\244\317\300\265\244\267\244\244\244\307\244\271\244\253?(y/n)",
       /* ¤È¡×¤ÏÀµ¤·¤¤¤Ç¤¹¤« */

#ifdef STANDALONE
  /*14*/"\244\253\244\312\264\301\273\372\312\321\264\271\244\307\244\255\244\336\244\273\244\363",
       /* ¤«¤Ê´Á»úÊÑ´¹¤Ç¤­¤Þ¤»¤ó */
#else
  /*14*/"\244\253\244\312\264\301\273\372\312\321\264\271\245\265\241\274\245\320\244\310\304\314\277\256\244\307\244\255\244\336\244\273\244\363",
       /* ¤«¤Ê´Á»úÊÑ´¹¥µ¡¼¥Ð¤ÈÄÌ¿®¤Ç¤­¤Þ¤»¤ó */
#endif

  /*15*/"\303\261\270\354\305\320\317\277\244\307\244\255\244\336\244\273\244\363\244\307\244\267\244\277",
       /* Ã±¸ìÅÐÏ¿¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */

  /*16*/"\241\330", /* ¡Ø */

  /*17*/"\241\331", /* ¡Ù */

  /*18*/"\241\312", /* ¡Ê */

  /*19*/"\241\313\244\362\305\320\317\277\244\267\244\336\244\267\244\277",
       /* ¡Ë¤òÅÐÏ¿¤·¤Þ¤·¤¿ */

  /*20*/"\303\261\270\354\305\320\317\277\244\313\274\272\307\324\244\267\244\336\244\267\244\277",
       /* Ã±¸ìÅÐÏ¿¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
#endif
};

#define message_num (sizeof(e_message) / sizeof(char *))
static WCHAR_T *message[message_num];

#ifdef WIN
static char sgyouA[] = "¤«¤¬¤µ¤¿¤Ê¤Ð¤Þ¤é¤ï";
static char sgyouI[] = "¤­¤®¤·¤Á¤Ë¤Ó¤ß¤ê¤¤";
static char sgyouU[] = "¤¯¤°¤¹¤Ä¤Ì¤Ö¤à¤ë¤¦";
#else
static char sgyouA[] = "\244\253\244\254\244\265\244\277\244\312\244\320\244\336\244\351\244\357";
                       /* ¤«¤¬¤µ¤¿¤Ê¤Ð¤Þ¤é¤ï */

static char sgyouI[] = "\244\255\244\256\244\267\244\301\244\313\244\323\244\337\244\352\244\244";
                       /* ¤­¤®¤·¤Á¤Ë¤Ó¤ß¤ê¤¤ */

static char sgyouU[] = "\244\257\244\260\244\271\244\304\244\314\244\326\244\340\244\353\244\246";
                       /* ¤¯¤°¤¹¤Ä¤Ì¤Ö¤à¤ë¤¦ */
#endif


#define KAGYOU 0
#define GAGYOU 1
#define SAGYOU 2
#define TAGYOU 3
#define NAGYOU 4
#define BAGYOU 5
#define MAGYOU 6
#define RAGYOU 7
#define WAGYOU 8


static WCHAR_T *gyouA;
static WCHAR_T *gyouI;
static WCHAR_T *gyouU;

/* Á´¤Æ¤Î¥á¥Ã¥»¡¼¥¸¤ò"unsigned char"¤«¤é"WCHAR_T"¤ËÊÑ´¹¤¹¤ë */
int
initHinshiMessage(void)
{
  int i;

  for(i = 0; i < message_num; i++) {
    message[i] = WString(e_message[i]);
    if(!message[i]) {
      return(-1);
    }
  }
  return 0;
}

/* WSprintf(to_buf, x1, x2, from_buf)
   :WSprintf(to_buf,"x1%sx2",from_buf);
 */
static void
WSprintf(WCHAR_T *to_buf, WCHAR_T *x1, WCHAR_T *x2, WCHAR_T *from_buf)
{
    WStrcpy(to_buf, x1);
    WStrcat(to_buf, from_buf);
    WStrcat(to_buf, x2);
}
#endif /* NO_EXTEND_MENU */

#ifndef WIN
void
EWStrcat(WCHAR_T *buf, char *xxxx)
{
  WCHAR_T x[1024];

  MBstowcs(x, xxxx, 1024);
  WStrcat(buf, x);
}
#endif

#ifndef NO_EXTEND_MENU
static void
EWStrcpy(WCHAR_T *buf, char *xxxx)
{
  WCHAR_T x[1024];
  int len;

  len = MBstowcs(x, xxxx, 1024);
  WStrncpy(buf, x, len);
  buf[len] = (WCHAR_T)0;
}

static int
EWStrcmp(WCHAR_T *buf, char *xxxx)
{
  WCHAR_T x[1024];

  MBstowcs(x, xxxx, 1024);
  return(WStrncmp(buf, x, WStrlen(x)));
}

static int
EWStrncmp(WCHAR_T *buf, char *xxxx, int len)
/* ARGSUSED */
{
  WCHAR_T x[1024];

  MBstowcs(x, xxxx, 1024);
  return(WStrncmp(buf, x, WStrlen(x)));
}

int
initGyouTable(void)
{
  gyouA = WString(sgyouA);
  gyouI = WString(sgyouI);
  gyouU = WString(sgyouU);

  if (!gyouA || !gyouI || !gyouU) {
    return NG;
  }
  return 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Ã±¸ìÅÐÏ¿¤ÎÉÊ»ìÁªÂò ¡ÁYes/No ¶¦ÄÌ Quit¡Á                                   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
int uuTHinshiYNQuitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  popCallback(d);
  
  return(dicTourokuHinshi(d));
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Ã±¸ìÅÐÏ¿¤ÎÉÊ»ìÁªÂò ¡ÁYes/No Âè£²ÃÊ³¬ ¶¦ÄÌ¥³¡¼¥ë¥Ð¥Ã¥¯¡Á                   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
int uuTHinshi2YesCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  tourokuContext tc;

  popCallback(d); /* yesNo ¤ò¥Ý¥Ã¥× */

  tourokuYes(d);   /* ÉÊ»ì¤¬·è¤Þ¤ì¤Ð tc->hcode ¤Ë¥»¥Ã¥È¤¹¤ë */

  tc = (tourokuContext)d->modec;

  if (!tc->qbuf[0]) {
    if (tc->hcode[0]) {
      /* ÉÊ»ì¤¬·è¤Þ¤Ã¤¿¤Î¤Ç¡¢ÅÐÏ¿¤¹¤ë¥æ¡¼¥¶¼­½ñ¤Î»ØÄê¤ò¹Ô¤¦ */
      return(dicTourokuDictionary(d, (int(*)(...))uuTDicExitCatch, (int(*)(...))uuTDicQuitCatch));
    }
  }
  return(retval);
}

static
int uuTHinshi2NoCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  tourokuContext tc;

  popCallback(d); /* yesNo ¤ò¥Ý¥Ã¥× */

  tourokuNo(d);   /* ÉÊ»ì¤¬·è¤Þ¤ì¤Ð tc->hcode ¤Ë¥»¥Ã¥È¤¹¤ë */

  tc = (tourokuContext)d->modec;

  if (!tc->qbuf[0]) {
    if (tc->hcode[0]) {
      /* ÉÊ»ì¤¬·è¤Þ¤Ã¤¿¤Î¤Ç¡¢ÅÐÏ¿¤¹¤ë¥æ¡¼¥¶¼­½ñ¤Î»ØÄê¤ò¹Ô¤¦ */
      return(dicTourokuDictionary(d, (int(*)(...))uuTDicExitCatch, (int(*)(...))uuTDicQuitCatch));
    }
  }

  return(retval);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Ã±¸ìÅÐÏ¿¤ÎÉÊ»ìÁªÂò ¡ÁYes/No Âè£±ÃÊ³¬ ¥³¡¼¥ë¥Ð¥Ã¥¯¡Á                       *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
int uuTHinshi1YesCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  tourokuContext tc;
  coreContext ync;
  
  popCallback(d); /* yesNo ¤ò¥Ý¥Ã¥× */

  tourokuYes(d);   /* ÉÊ»ì¤¬·è¤Þ¤ì¤Ð tc->hcode ¤Ë¥»¥Ã¥È¤¹¤ë */

  tc = (tourokuContext)d->modec;

  if(tc->qbuf[0]) {
    /* ¼ÁÌä¤¹¤ë */
    makeGLineMessage(d, tc->qbuf, WStrlen(tc->qbuf));
    if((retval = getYesNoContext(d,
		 NO_CALLBACK, uuTHinshi2YesCatch,
		 uuTHinshiYNQuitCatch, uuTHinshi2NoCatch)) == NG) {
      defineEnd(d);
      return(GLineNGReturnTK(d));
    }
    ync = (coreContext)d->modec;
    ync->majorMode = CANNA_MODE_ExtendMode;
    ync->minorMode = CANNA_MODE_TourokuHinshiMode;
  } else if(tc->hcode[0]) {
    /* ÉÊ»ì¤¬·è¤Þ¤Ã¤¿¤Î¤Ç¡¢ÅÐÏ¿¤¹¤ë¥æ¡¼¥¶¼­½ñ¤Î»ØÄê¤ò¹Ô¤¦ */
    return(dicTourokuDictionary(d, (int(*)(...))uuTDicExitCatch, (int(*)(...))uuTDicQuitCatch));
  }

  return(retval);
}

static
int uuTHinshi1NoCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  tourokuContext tc;
  coreContext ync;

  popCallback(d); /* yesNo ¤ò¥Ý¥Ã¥× */

  tourokuNo(d);   /* ÉÊ»ì¤¬·è¤Þ¤ì¤Ð tc->hcode ¤Ë¥»¥Ã¥È¤¹¤ë */

  tc = (tourokuContext)d->modec;

  if(tc->qbuf[0]) {
    /* ¼ÁÌä¤¹¤ë */
    makeGLineMessage(d, tc->qbuf, WStrlen(tc->qbuf));
    if((retval = getYesNoContext(d,
		 NO_CALLBACK, uuTHinshi2YesCatch,
		 uuTHinshiYNQuitCatch, uuTHinshi2NoCatch)) == NG) {
      defineEnd(d); 
      return(GLineNGReturnTK(d));
    }
    ync = (coreContext)d->modec;
    ync->majorMode = CANNA_MODE_ExtendMode;
    ync->minorMode = CANNA_MODE_TourokuHinshiMode;
  } else if(tc->hcode[0]) {
    /* ÉÊ»ì¤¬·è¤Þ¤Ã¤¿¤Î¤Ç¡¢ÅÐÏ¿¤¹¤ë¥æ¡¼¥¶¼­½ñ¤Î»ØÄê¤ò¹Ô¤¦ */
    return(dicTourokuDictionary(d, (int(*)(...))uuTDicExitCatch, (int(*)(...))uuTDicQuitCatch));
  }

  return(retval);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Ã±¸ìÅÐÏ¿¤ÎÉÊ»ìÊ¬¤±¤¹¤ë¡©                                                  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
int uuTHinshiQYesCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  tourokuContext tc;
  coreContext ync;

  popCallback(d); /* yesNo ¤ò¥Ý¥Ã¥× */

  tc = (tourokuContext)d->modec;

  makeGLineMessage(d, tc->qbuf, WStrlen(tc->qbuf)); /* ¼ÁÌä */
  if((retval = getYesNoContext(d,
	 NO_CALLBACK, uuTHinshi1YesCatch,
	 uuTHinshiYNQuitCatch, uuTHinshi1NoCatch)) == NG) {
    defineEnd(d);
    return(GLineNGReturnTK(d));
  }
  ync = (coreContext)d->modec;
  ync->majorMode = CANNA_MODE_ExtendMode;
  ync->minorMode = CANNA_MODE_TourokuHinshiMode;

  return(retval);
}

static
int uuTHinshiQNoCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  popCallback(d); /* yesNo ¤ò¥Ý¥Ã¥× */

  return(dicTourokuDictionary(d, (int(*)(...))uuTDicExitCatch, (int(*)(...))uuTDicQuitCatch));
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Ã±¸ìÅÐÏ¿¤ÎÉÊ»ìÁªÂò                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static int makeHinshi();

int dicTourokuHinshiDelivery(uiContext d)
{
  tourokuContext tc = (tourokuContext)d->modec;
  coreContext ync;
  int retval = 0;

  makeHinshi(d); /* ÉÊ»ì¡¢¥¨¥é¡¼¥á¥Ã¥»¡¼¥¸¡¢¼ÁÌä¤ò¥»¥Ã¥È¤·¤Æ¤¯¤ë */

#if defined(DEBUG) && !defined(WIN)
  if(iroha_debug) {
    printf("tc->genbuf=%s, tc->qbuf=%s, tc->hcode=%s\n", tc->genbuf, tc->qbuf,
	   tc->hcode);
  }
#endif
  if(tc->genbuf[0]) {
    /* ÆþÎÏ¤µ¤ì¤¿¥Ç¡¼¥¿¤Ë¸í¤ê¤¬¤¢¤Ã¤¿¤Î¤Ç¡¢
       ¥á¥Ã¥»¡¼¥¸¤òÉ½¼¨¤·¤ÆÆÉ¤ßÆþÎÏ¤ËÌá¤ë */
    clearYomi(d);
    return(dicTourokuTango(d, uuTTangoQuitCatch));
  } else if(tc->qbuf[0] && cannaconf.grammaticalQuestion) {
    /* ºÙ¤«¤¤ÉÊ»ìÊ¬¤±¤Î¤¿¤á¤Î¼ÁÌä¤ò¤¹¤ë */
    WStrcpy(d->genbuf, message[0]);
    if((retval = getYesNoContext(d,
		 NO_CALLBACK, uuTHinshiQYesCatch,
		 uuTHinshiYNQuitCatch, uuTHinshiQNoCatch)) == NG) {
      defineEnd(d);
      return(GLineNGReturnTK(d));
    }
    makeGLineMessage(d, d->genbuf, WStrlen(d->genbuf));
    ync = (coreContext)d->modec;
    ync->majorMode = CANNA_MODE_ExtendMode;
    ync->minorMode = CANNA_MODE_TourokuHinshiMode;
    return(retval);
  } else if(tc->hcode[0]) {
    /* ÉÊ»ì¤¬·è¤Þ¤Ã¤¿¤Î¤Ç¡¢ÅÐÏ¿¤¹¤ë¥æ¡¼¥¶¼­½ñ¤Î»ØÄê¤ò¹Ô¤¦ */
    return(dicTourokuDictionary(d, (int(*)(...))uuTDicExitCatch, (int(*)(...))uuTDicQuitCatch));
  }
  return 0;
}

/*
 * ÁªÂò¤µ¤ì¤¿ÉÊ»ì¤«¤é¼¡¤ÎÆ°ºî¤ò¹Ô¤¦
 * 
 * tc->hcode	ÉÊ»ì
 * tc->qbuf	¼ÁÌä
 * tc->genbuf	¥¨¥é¡¼
 */
static int
makeHinshi(uiContext d)
{
  tourokuContext tc = (tourokuContext)d->modec;
  int tlen, ylen, yomi_katsuyou;
  WCHAR_T tmpbuf[256];

  tc->hcode[0] = 0;
  tc->qbuf[0] = 0;
  tc->genbuf[0] = 0;

  tlen = tc->tango_len;
  ylen = tc->yomi_len;

  switch(tc->curHinshi) {
  case MEISHI:
    EWStrcpy(tc->hcode, "#T35");
    tc->katsuyou = 0;
    WSprintf(tc->qbuf, message[5], message[6], tc->tango_buffer);
    break;

  case KOYUMEISHI:
    EWStrcpy(tc->hcode, "#KK");
    WSprintf(tc->qbuf, message[5], message[8], tc->tango_buffer);
    break;
    
  case DOSHI:

    /* ÆþÎÏ¤¬½ª»ß·Á¤«¡© */
    tc->katsuyou = 0;
    while (tc->katsuyou < GOBISUU &&
	   tc->tango_buffer[tlen - 1] != gyouU[tc->katsuyou]) {
      tc->katsuyou++;
    }
    yomi_katsuyou = 0;
    while (yomi_katsuyou < GOBISUU &&
	   tc->yomi_buffer[ylen - 1] != gyouU[yomi_katsuyou]) {
      yomi_katsuyou++;
    }
    if((tc->katsuyou == GOBISUU) || (yomi_katsuyou == GOBISUU)){
      WStrcpy(tc->genbuf, message[1]);
      return(0);
    }
    if(tc->katsuyou != yomi_katsuyou){
      WStrcpy(tc->genbuf, message[2]);
      return(0);
    }

    makeDoushi(d);  /* ¾ÜºÙ¤ÎÉÊ»ì¤òÉ¬Í×¤È¤·¤Ê¤¤¾ì¹ç */
    if (tc->katsuyou == RAGYOU) {
      tc->curHinshi = RAGYODOSHI;
      /* Ì¤Á³·Á¤ò¤Ä¤¯¤ë */
      WStrncpy(tmpbuf, tc->tango_buffer, tlen-1);  
      tmpbuf[tlen - 1] = gyouA[tc->katsuyou];
      tmpbuf[tlen] = (WCHAR_T)0;
      WSprintf(tc->qbuf, message[5], message[10], tmpbuf);
    }
    else {
      tc->curHinshi = GODAN;
      WStrncpy(tmpbuf, tc->tango_buffer, tlen - 1);
      tmpbuf[tlen - 1] = gyouI[tc->katsuyou];
      tmpbuf[tlen] = (WCHAR_T)'\0';
      WSprintf(tc->qbuf, message[5], message[11], tmpbuf);
    }
    break;

  case KEIYOSHI:
    tc->katsuyou = 1;
    if(tlen >= 1 && ylen >= 1 &&
       ((EWStrncmp(tc->tango_buffer+tlen-1, "\244\244", 1) != 0) ||
	(EWStrncmp(tc->yomi_buffer+ylen-1, "\244\244", 1) != 0))) {
                                           /* ¤¤ */
      WStrcpy(tc->genbuf, message[3]);
      return(0);
    }

    EWStrcpy(tc->hcode, "#KY"); /* ¾ÜºÙ¤ÎÉÊ»ì¤òÉ¬Í×¤È¤·¤Ê¤¤¾ì¹ç */
    WStrncpy(tmpbuf, tc->tango_buffer, tlen-1);  
    tmpbuf[tlen-1] = 0;
    WSprintf(tc->qbuf, message[5], message[11], tmpbuf);
    break;

  case KEIYODOSHI:
    tc->katsuyou = 1;
    if(tlen >= 1 && ylen >= 1 &&
       ((EWStrncmp(tc->tango_buffer+tlen-1, "\244\300", 1)) ||
	(EWStrncmp(tc->yomi_buffer+ylen-1, "\244\300", 1)))) {
                                           /* ¤À */
      WStrcpy(tc->genbuf, message[4]);
      return(0);
    }
    EWStrcpy(tc->hcode, "#T05"); /* ¾ÜºÙ¤ÎÉÊ»ì¤òÉ¬Í×¤È¤·¤Ê¤¤¾ì¹ç */
    WStrncpy(tmpbuf, tc->tango_buffer, tlen-1);  
    tmpbuf[tlen-1] = 0;  
    WSprintf(tc->qbuf, message[5], message[6], tmpbuf);
    break;

  case FUKUSHI:
    EWStrcpy(tc->hcode, "#F14"); /* ¾ÜºÙ¤ÎÉÊ»ì¤òÉ¬Í×¤È¤·¤Ê¤¤¾ì¹ç */
    tc->katsuyou = 0;
    WSprintf(tc->qbuf, message[5], message[6], tc->tango_buffer);
    break;

  case TANKANJI:
    EWStrcpy(tc->hcode, "#KJ");
    break;

  case SUSHI:
    EWStrcpy(tc->hcode, "#NN");
    break;

  case RENTAISHI:
    EWStrcpy(tc->hcode, "#RT");
    break;

  case SETSUZOKUSHI:  /* ÀÜÂ³»ì¡¦´¶Æ°»ì */
    EWStrcpy(tc->hcode, "#CJ");
    break;

  case SAHENMEISHI:
  case MEISHIN:
    tc->katsuyou = 0;
    WSprintf(tc->qbuf, message[5], message[7], tc->tango_buffer);
    break;

  case JINMEI:
  case KOYUMEISHIN:
    WSprintf(tc->qbuf, message[5], message[9], tc->tango_buffer);
    break;

  case RAGYOGODAN:
    WStrncpy(tmpbuf, tc->tango_buffer, tlen - 1);
    tmpbuf[tlen - 1] = gyouI[tc->katsuyou];
    tmpbuf[tlen] = (WCHAR_T)'\0';
    WSprintf(tc->qbuf, message[5], message[11], tmpbuf);
    break;

  case KAMISHIMO:
    WStrncpy(tmpbuf, tc->tango_buffer, tlen - 1);
    tmpbuf[tlen - 1] = (WCHAR_T)'\0';
    WSprintf(tc->qbuf, message[5], message[11], tmpbuf);
    break;

  case KEIYODOSHIY:
  case KEIYODOSHIN: 
    WStrncpy(tmpbuf, tc->tango_buffer, tlen - 1);
    tmpbuf[tlen - 1] = 0;
    WSprintf(tc->qbuf, message[5], message[11], tmpbuf);
    break;

  case FUKUSHIY:
  case FUKUSHIN:
    WSprintf(tc->qbuf, message[5], message[13], tc->tango_buffer);
    break;
  }

  return(0);
}

static
int tourokuYes(uiContext d)
{
  tourokuContext tc = (tourokuContext)d->modec;

  tc->hcode[0] = 0;
  tc->qbuf[0] = 0;
  tc->genbuf[0] = 0;

  switch(tc->curHinshi) {
  case MEISHI:
    tc->curHinshi = SAHENMEISHI;
    makeHinshi(d);
    break;

  case KOYUMEISHI:
    tc->curHinshi = JINMEI;
    makeHinshi(d);
    break;

  case GODAN:  /* ¥é¹Ô°Ê³°¤Î¸ÞÃÊ³èÍÑÆ°»ì */
    makeDoushi(d);
    EWStrcat(tc->hcode, "r");              /* ½ñ¤¯¡¢µÞ¤°¡¢°Ü¤¹ */
    break;

  case RAGYODOSHI:
    tc->curHinshi = RAGYOGODAN;
    makeHinshi(d);
    break;

  case KEIYOSHI:
    EWStrcpy(tc->hcode, "#KYT");           /* ¤­¤¤¤í¤¤ */
    break;

  case KEIYODOSHI:
    tc->curHinshi = KEIYODOSHIY;
    makeHinshi(d);
    break;

  case FUKUSHI:
    tc->curHinshi = FUKUSHIY;
    makeHinshi(d);
    break;

  case MEISHIN:
    EWStrcpy(tc->hcode, "#T15");          /* ¿§¡¹¡¢¶¯ÎÏ */
    break;

  case SAHENMEISHI:
    EWStrcpy(tc->hcode, "#T10");          /* °Â¿´¡¢Éâµ¤ */
    break;

  case KOYUMEISHIN:
    EWStrcpy(tc->hcode, "#CN");	          /* Åìµþ */
    break;

  case JINMEI:
    EWStrcpy(tc->hcode, "#JCN");          /* Ê¡Åç */
    break;

  case RAGYOGODAN:
    EWStrcpy(tc->hcode, "#R5r");          /* ¼Õ¤ë */
    break;

  case KAMISHIMO:
    EWStrcpy(tc->hcode, "#KSr");          /* À¸¤­¤ë¡¢ÍÂ¤±¤ë */
    break;

  case KEIYODOSHIY:
    EWStrcpy(tc->hcode, "#T10");          /* ´Ø¿´¤À */
    break;

  case KEIYODOSHIN:
    EWStrcpy(tc->hcode, "#T15");          /* °Õ³°¤À¡¢²ÄÇ½¤À */
    break;

  case FUKUSHIY:
    EWStrcpy(tc->hcode, "#F04");          /* ¤Õ¤Ã¤¯¤é */
    break;

  case FUKUSHIN:
    EWStrcpy(tc->hcode, "#F06");          /* ÆÍÁ³ */
    break;
  }

  return(0);
}

static
int tourokuNo(uiContext d)
{
  tourokuContext tc = (tourokuContext)d->modec;
  int ylen;

  tc->hcode[0] = 0;
  tc->qbuf[0] = 0;
  tc->genbuf[0] = 0;

  switch( tc->curHinshi ) {
  case MEISHI:
    tc->curHinshi = MEISHIN;
    makeHinshi(d);
    break;

  case KOYUMEISHI:
    tc->curHinshi = KOYUMEISHIN;
    makeHinshi(d);
    break;

  case GODAN:  /* ¥é¹Ô°Ê³°¤Î¸ÞÃÊ³èÍÑÆ°»ì */
    makeDoushi(d);
    break;

  case RAGYODOSHI:
    ylen = tc->yomi_len;
    if (ylen >= 2 && !(EWStrcmp(tc->yomi_buffer + ylen - 2, "\244\257\244\353"))) {   /* ¤¯¤ë */
      EWStrcpy(tc->hcode, "#KX");         /* Íè¤ë */
    }
    else if (ylen >=2 && !(EWStrcmp(tc->yomi_buffer + ylen - 2, "\244\271\244\353"))) { /* ¤¹¤ë */
      EWStrcpy(tc->hcode, "#SX");         /* ¤¹¤ë */
    }
    else if (ylen >=2 && !(EWStrcmp(tc->yomi_buffer + ylen - 2, "\244\272\244\353"))) {  /* ¤º¤ë */
      EWStrcpy(tc->hcode, "#ZX");         /* ½à¤º¤ë */
    }
    else {
      tc->curHinshi = KAMISHIMO;
      makeHinshi(d);
    }
    break;

  case KEIYOSHI:
    EWStrcpy(tc->hcode, "#KY");           /* Èþ¤·¤¤¡¢Áá¤¤ */
    break;

  case KEIYODOSHI:
    tc->curHinshi = KEIYODOSHIN;
    makeHinshi(d);
    break;

  case FUKUSHI:
    tc->curHinshi = FUKUSHIN;
    makeHinshi(d);
    break;

  case MEISHIN:
    EWStrcpy(tc->hcode, "#T35");          /* »³¡¢¿å */
    break;

  case SAHENMEISHI:
    EWStrcpy(tc->hcode, "#T30");          /* ÅØÎÏ¡¢¸¡ºº */
    break;

  case KOYUMEISHIN:
    EWStrcpy(tc->hcode, "#KK");           /* ÆüËÜÅÅµ¤ */
    break;

  case JINMEI:
    EWStrcpy(tc->hcode, "#JN");           /* »°´È */
    break;

  case RAGYOGODAN:
    EWStrcpy(tc->hcode, "#R5");           /* °ÒÄ¥¤ë */
    break;

  case KAMISHIMO:
    EWStrcpy(tc->hcode, "#KS");           /* ¹ß¤ê¤ë¡¢Í¿¤¨¤ë */
    break;

  case KEIYODOSHIY:
    EWStrcpy(tc->hcode, "#T13");          /* Â¿¹²¤Æ¤À */
    break;

  case KEIYODOSHIN:
    EWStrcpy(tc->hcode, "#T18");          /* ÊØÍø¤À¡¢ÀÅ¤«¤À */
    break;

  case FUKUSHIY:
    EWStrcpy(tc->hcode, "#F12");          /* ¤½¤Ã¤È */
    break;

  case FUKUSHIN:
    EWStrcpy(tc->hcode, "#F14");          /* Ë°¤¯¤Þ¤Ç */
    break;
  }
  return(0);
}

static void
makeDoushi(uiContext d)
{
  tourokuContext tc = (tourokuContext)d->modec;

    switch(tc->katsuyou){
    case  KAGYOU:
      EWStrcpy( tc->hcode, "#K5" );     /* ÃÖ¤¯ */
      break;
    case  GAGYOU:
      EWStrcpy( tc->hcode, "#G5" );     /* ¶Ä¤° */
      break;
    case  SAGYOU:
      EWStrcpy( tc->hcode, "#S5" );     /* ÊÖ¤¹ */
      break;
    case  TAGYOU:
      EWStrcpy( tc->hcode, "#T5" );     /* Àä¤Ä */
      break;
    case  NAGYOU:
      EWStrcpy( tc->hcode, "#N5" );     /* »à¤Ì */
      break;
    case  BAGYOU:
      EWStrcpy( tc->hcode, "#B5" );     /* Å¾¤Ö */
      break;
    case  MAGYOU:
      EWStrcpy( tc->hcode, "#M5" );     /* ½»¤à */
      break;
    case  RAGYOU:
      EWStrcpy( tc->hcode, "#R5" );     /* °ÒÄ¥¤ë */
      break;
    case  WAGYOU:
      EWStrcpy( tc->hcode, "#W5" );     /* ¸À¤¦ */
      break;
    }
}    

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * ¼­½ñ¤Î°ìÍ÷                                                                *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
int uuTDicExitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  forichiranContext fc;
  int cur;
  tourokuContext tc;

  d->nbytes = 0;

  popCallback(d); /* °ìÍ÷¤ò pop */

  fc = (forichiranContext)d->modec;
  cur = fc->curIkouho;

  popForIchiranMode(d);
  popCallback(d);

  tc = (tourokuContext)d->modec;

  tc->workDic = cur;

  return(tangoTouroku(d));
}

static
int uuTDicQuitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  popCallback(d); /* °ìÍ÷¤ò pop */

  popForIchiranMode(d);
  popCallback(d);

  return(dicTourokuHinshi(d));
}

int dicTourokuDictionary(uiContext d, int (*exitfunc )(...), int (*quitfunc )(...))
{
  tourokuContext tc = (tourokuContext)d->modec;
  forichiranContext fc;
  ichiranContext ic;
  WCHAR_T **work;
  unsigned inhibit = 0;
  int retval, upnelem = 0;

  retval = d->nbytes = 0;
  d->status = 0;

  for(work = tc->udic; *work; work++)
    upnelem++;

  if((retval = getForIchiranContext(d)) == NG) {
    freeDic(tc);
    defineEnd(d);
    return(GLineNGReturnTK(d));
  }
  fc = (forichiranContext)d->modec;

  /* selectOne ¤ò¸Æ¤Ö¤¿¤á¤Î½àÈ÷ */

  fc->allkouho = tc->udic;

  fc->curIkouho = 0;
  if (!cannaconf.HexkeySelect)
    inhibit |= ((unsigned char)NUMBERING | (unsigned char)CHARINSERT); 
  else
    inhibit |= (unsigned char)CHARINSERT;

   if((retval = selectOne(d, fc->allkouho, &fc->curIkouho, upnelem,
		 BANGOMAX, inhibit, 0, WITHOUT_LIST_CALLBACK,
		 NO_CALLBACK,(int(*)(_uiContext*, int, _coreContextRec*))exitfunc, (int(*)(_uiContext*, int, _coreContextRec*))quitfunc, uiUtilIchiranTooSmall)) 
                 == NG) {
    if(fc->allkouho)
      free(fc->allkouho);
    popForIchiranMode(d);
    popCallback(d);
    defineEnd(d);
    return(GLineNGReturnTK(d));
  }

  ic = (ichiranContext)d->modec;
  ic->majorMode = CANNA_MODE_ExtendMode;
  ic->minorMode = CANNA_MODE_TourokuDicMode;
  currentModeInfo(d);

  /* ¸õÊä°ìÍ÷¹Ô¤¬¶¹¤¯¤Æ¸õÊä°ìÍ÷¤¬½Ð¤»¤Ê¤¤ */
  if(ic->tooSmall) {
    d->status = AUX_CALLBACK;
    return(retval);
  }

  makeGlineStatus(d);
  /* d->status = ICHIRAN_EVERYTIME; */

  return(retval);
}

/*
 * Ã±¸ìÅÐÏ¿¤ò¹Ô¤¦
 */
static
int tangoTouroku(uiContext d)
{
  tourokuContext tc = (tourokuContext)d->modec;
  WCHAR_T ktmpbuf[256];
  WCHAR_T ttmpbuf[256];
  WCHAR_T line[ROMEBUFSIZE], line2[ROMEBUFSIZE];
  WCHAR_T xxxx[1024];
  char dicname[1024];
  extern int defaultContext;
  int linecnt;

  defineEnd(d);
  if(tc->katsuyou || (EWStrncmp(tc->hcode, "#K5", 3) == 0)) {
    WStrncpy(ttmpbuf, tc->tango_buffer, tc->tango_len - 1);
    ttmpbuf[tc->tango_len - 1] = (WCHAR_T)0;
    WStrncpy(ktmpbuf, tc->yomi_buffer, tc->yomi_len - 1);
    ktmpbuf[tc->yomi_len - 1] = 0;
  } else {
    WStrcpy(ttmpbuf, tc->tango_buffer);
    WStrcpy(ktmpbuf, tc->yomi_buffer);
  }

  /* ¼­½ñ½ñ¤­¹þ¤ßÍÑ¤Î°ì¹Ô¤òºî¤ë */
  WStraddbcpy(line, ktmpbuf, ROMEBUFSIZE);
  linecnt = WStrlen(line);
  line[linecnt] = (WCHAR_T)' ';
  linecnt++;
  WStrcpy(line + linecnt, tc->hcode);
  linecnt += WStrlen(tc->hcode);
  line[linecnt] = (WCHAR_T)' ';
  linecnt++;
  WStraddbcpy(line + linecnt, ttmpbuf, ROMEBUFSIZE - linecnt);

  if(defaultContext == -1) {
    if((KanjiInit() < 0) || (defaultContext == -1)) {
      jrKanjiError = (char *)e_message[14];
      freeAndPopTouroku(d);
      return(GLineNGReturn(d));
    }
  }
  /* ¼­½ñ¤ËÅÐÏ¿¤¹¤ë */
  WCstombs(dicname, tc->udic[tc->workDic], sizeof(dicname));

  if (RkwDefineDic(defaultContext, dicname, line) != 0) {
    /* ÉÊ»ì¤¬ #JCN ¤Î¤È¤­¤Ï¡¢ÅÐÏ¿¤Ë¼ºÇÔ¤·¤¿¤é¡¢#JN ¤È #CN ¤ÇÅÐÏ¿¤¹¤ë */
    if (EWStrncmp(tc->hcode, "#JCN", 4) == 0) {
      WCHAR_T xxx[3];

      /* ¤Þ¤º #JN ¤ÇÅÐÏ¿¤¹¤ë */
      EWStrcpy(xxx, "#JN");
      WStraddbcpy(line, ktmpbuf, ROMEBUFSIZE);
      EWStrcat(line, " ");
      WStrcat(line, xxx);
      EWStrcat(line, " ");
      linecnt = WStrlen(line);
      WStraddbcpy(line + linecnt, ttmpbuf, ROMEBUFSIZE - linecnt);

      if (RkwDefineDic(defaultContext, dicname, line) == 0) {
        /* #JN ¤ÇÅÐÏ¿¤Ç¤­¤¿¤È¤­¡¢¼¡¤Ë #CN ¤ÇÅÐÏ¿¤¹¤ë */
        EWStrcpy(xxx, "#CN");
        WStraddbcpy(line2, ktmpbuf, ROMEBUFSIZE);
        EWStrcat(line2, " ");
        WStrcat(line2, xxx);
        EWStrcat(line2, " ");
        linecnt = WStrlen(line2);
        WStraddbcpy(line2 + linecnt, ttmpbuf, ROMEBUFSIZE - linecnt);

        if (RkwDefineDic(defaultContext, dicname, line2) == 0) {
          goto success;
        }

        /* #CN ¤ÇÅÐÏ¿¤Ç¤­¤Ê¤«¤Ã¤¿¤È¤­¡¢#JN ¤òºï½ü¤¹¤ë */
        if (RkwDeleteDic(defaultContext, dicname, line) == NG) {
          /* #JN ¤¬ºï½ü¤Ç¤­¤Ê¤«¤Ã¤¿¤é¡¢"¼ºÇÔ¤·¤Þ¤·¤¿" */
          if (errno == EPIPE)
            jrKanjiPipeError();
          WStrcpy(d->genbuf, message[20]);
          goto close;
        }
      }
    }
    /* #JCN °Ê³°¤Î¤È¤­
       #JN ¤¬ÅÐÏ¿¤Ç¤­¤Ê¤«¤Ã¤¿¤È¤­
       #CN ¤¬ÅÐÏ¿¤Ç¤­¤º¡¢#JN ¤¬ºï½ü¤Ç¤­¤¿¤È¤­ */
    if (errno == EPIPE)
      jrKanjiPipeError();
    WStrcpy(d->genbuf, message[15]);
    goto close;
  }

 success:
  if (cannaconf.auto_sync) {
    RkwSync(defaultContext, dicname);
  }
  /* ÅÐÏ¿¤Î´°Î»¤òÉ½¼¨¤¹¤ë */
  WSprintf(d->genbuf, message[16], message[17], tc->tango_buffer);
  WSprintf(xxxx, message[18], message[19], tc->yomi_buffer);
  WStrcat(d->genbuf, xxxx);

 close:
  makeGLineMessage(d, d->genbuf, WStrlen(d->genbuf));

  freeAndPopTouroku(d);
  currentModeInfo(d);

  return(0); /* Ã±¸ìÅÐÏ¿´°Î» */
}
#endif /* NO_EXTEND_MENU */
