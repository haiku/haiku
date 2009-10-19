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
static char rcs_id[] = "@(#) 102.1 $Id: ulmount.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif

#ifndef NO_EXTEND_MENU
#include	<errno.h>
#include 	"canna.h"
#include "RK.h"
#include "RKintern.h"

static mountContext newMountContext(void);
static void freeMountContext(mountContext mc);
static struct dicname *findDic(char *s);
static int uuMountExitCatch(uiContext d, int retval, mode_context env);
static int uuMountQuitCatch(uiContext d, int retval, mode_context env);
static int getDicList(uiContext d);

/* cfunc mountContext
 *
 * mountContext
 *
 */
static mountContext
newMountContext(void)
{
  mountContext mcxt;

  if ((mcxt = (mountContext)calloc(1, sizeof(mountContextRec)))
                                           == (mountContext)NULL) {
#ifndef WIN
    jrKanjiError = "malloc (newMountContext) ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
#else
    jrKanjiError = "malloc (newMountContext) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
#endif
    return (mountContext)NULL;
  }
  mcxt->id = MOUNT_CONTEXT;

  return mcxt;
}

static void
freeMountContext(mountContext mc)
{
  if (mc) {
    if (mc->mountList) {
      if (*(mc->mountList)) {
	free(*(mc->mountList));
      }
      free(mc->mountList);
    }
    if (mc->mountOldStatus) {
      free(mc->mountOldStatus);
    }
    if (mc->mountNewStatus) {
      free(mc->mountNewStatus);
    }
    free(mc);
  }
}

/*
 * ¸õÊä°ìÍ÷¹Ô¤òºî¤ë
 */
int getMountContext(uiContext d)
{
  mountContext mc;
  int retval = 0;

  if (pushCallback(d, d->modec,
                   NO_CALLBACK, NO_CALLBACK,
                   NO_CALLBACK, NO_CALLBACK) == 0) {
#ifndef WIN
    jrKanjiError = "malloc (pushCallback) ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
#else
    jrKanjiError = "malloc (pushCallback) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
#endif
    return(NG);
  }
  
  if((mc = newMountContext()) == (mountContext)NULL) {
    popCallback(d);
    return(NG);
  }
  mc->majorMode = d->majorMode;
  mc->next = d->modec;
  d->modec = (mode_context)mc;

  mc->prevMode = d->current_mode;

  return(retval);
}

void
popMountMode(uiContext d)
{
  mountContext mc = (mountContext)d->modec;

  d->modec = mc->next;
  d->current_mode = mc->prevMode;
  freeMountContext(mc);
}

static struct dicname *
findDic(char *s)
{
  extern struct dicname *kanjidicnames;
  struct dicname *dp;

  for (dp = kanjidicnames ; dp ; dp = dp->next) {
    if (!strcmp(s, dp->name)) {
      return dp;
    }
  }
  return (struct dicname *)0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * ¼­½ñ¤Î¥Þ¥¦¥ó¥È¡¿¥¢¥ó¥Þ¥¦¥ó¥È                                              *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
int uuMountExitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  mountContext mc;
  int i, nmount = 0;
  extern int defaultContext;
  struct dicname *dp;

  killmenu(d);
  popCallback(d); /* OnOff ¤ò¥Ý¥Ã¥× */

  if(defaultContext == -1) {
    if((KanjiInit() != 0) || (defaultContext == -1)) {
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
      popMountMode(d);
      popCallback(d);
      return(GLineNGReturn(d));
    }
  }

  mc = (mountContext)d->modec;
  for(i=0; mc->mountList[i]; i++) {
    if(mc->mountOldStatus[i] != mc->mountNewStatus[i]) {
      if(mc->mountNewStatus[i]) {
	/* ¥Þ¥¦¥ó¥È¤¹¤ë */
	nmount++;
	if((retval = RkwMountDic(defaultContext, (char *)mc->mountList[i],
			    cannaconf.kojin ? PL_ALLOW : PL_INHIBIT)) == NG) {
	  if (errno == EPIPE) {
	    jrKanjiPipeError();
	  }
	  MBstowcs(d->genbuf, "\274\255\275\361\244\316\245\336\245\246"
		"\245\363\245\310\244\313\274\272\307\324\244\267\244\336"
		"\244\267\244\277", 512);
                       /* ¼­½ñ¤Î¥Þ¥¦¥ó¥È¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
	}
	else if(d->contextCache != -1 &&
	  (retval = RkwMountDic(d->contextCache, (char *)mc->mountList[i],
			    cannaconf.kojin ? PL_ALLOW : PL_INHIBIT)) == NG) {
	  if (errno == EPIPE) {
	    jrKanjiPipeError();
	  }
	  MBstowcs(d->genbuf, "\274\255\275\361\244\316\245\336\245\246"
		"\245\363\245\310\244\313\274\272\307\324\244\267\244\336"
		"\244\267\244\277", 512);
                              /* ¼­½ñ¤Î¥Þ¥¦¥ó¥È¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
	}
	else { /* À®¸ù */
	  dp = findDic((char *)mc->mountList[i]);
	  if (!dp) {
	    dp = (struct dicname *)malloc(sizeof(struct dicname));
	    if (dp) {
	      dp->name = (char *)malloc(strlen((char *)mc->mountList[i]) + 1);
	      if (dp->name) {
		/* ¥Þ¥¦¥ó¥È¤·¤¿¤ä¤Ä¤Ï¥ê¥¹¥È¤Ë¤Ä¤Ê¤° */
		strcpy(dp->name, (char *)mc->mountList[i]);
		dp->dictype = DIC_PLAIN;
		/* dp->dicflag = DIC_NOT_MOUNTED; will be rewritten below */
		dp->next = kanjidicnames;
		kanjidicnames = dp;
	      }
	      else { /* (char *)malloc failed */
		free(dp);
		dp = (struct dicname *)0;
	      }
	    }
	  }
	  if (dp) {
	    dp->dicflag = DIC_MOUNTED;
	  }
	}
      } else {
	/* ¥¢¥ó¥Þ¥¦¥ó¥È¤¹¤ë */
	nmount++;
	if((retval = RkwUnmountDic(defaultContext, (char *)mc->mountList[i]))
	   == NG) {
	  if (errno == EPIPE) {
	    jrKanjiPipeError();
	  }
	  MBstowcs(d->genbuf, "\274\255\275\361\244\316\245\242\245\363"
		"\245\336\245\246\245\363\245\310\244\313\274\272\307\324"
		"\244\267\244\336\244\267\244\277", 512);
                             /* ¼­½ñ¤Î¥¢¥ó¥Þ¥¦¥ó¥È¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
	}
	else if(d->contextCache != -1 &&
	  (retval = RkwUnmountDic(d->contextCache, (char *)mc->mountList[i]))
		== NG) {
	  if (errno == EPIPE) {
	    jrKanjiPipeError();
	  }
	  MBstowcs(d->genbuf, "\274\255\275\361\244\316\245\242\245\363"
		"\245\336\245\246\245\363\245\310\244\313\274\272\307\324"
		"\244\267\244\336\244\267\244\277", 512);
                             /* ¼­½ñ¤Î¥¢¥ó¥Þ¥¦¥ó¥È¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
	}
	else {
	  dp = findDic((char *)mc->mountList[i]);
	  if (dp) { /* ¤«¤Ê¤é¤º°Ê²¼¤òÄÌ¤ë¤Ï¤º */
	    dp->dicflag = DIC_NOT_MOUNTED;
	  }
	}
      }
    }
  }

  if(nmount)
    makeAllContextToBeClosed(1);

  if(retval != NG)
    MBstowcs(d->genbuf, "\274\255\275\361\244\316\245\336\245\246\245\363"
	"\245\310\241\277\245\242\245\363\245\336\245\246\245\363\245\310"
	"\244\362\271\324\244\244\244\336\244\267\244\277", 512);
           /* ¼­½ñ¤Î¥Þ¥¦¥ó¥È¡¿¥¢¥ó¥Þ¥¦¥ó¥È¤ò¹Ô¤¤¤Þ¤·¤¿ */ 
  else
    MBstowcs(d->genbuf, "\274\255\275\361\244\316\245\336\245\246\245\363"
	"\245\310\241\277\245\242\245\363\245\336\245\246\245\363\245\310"
	"\244\313\274\272\307\324\244\267\244\336\244\267\244\277", 512);
           /* ¼­½ñ¤Î¥Þ¥¦¥ó¥È¡¿¥¢¥ó¥Þ¥¦¥ó¥È¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
  makeGLineMessage(d, d->genbuf, WStrlen(d->genbuf));

  popMountMode(d);
  popCallback(d);
  currentModeInfo(d);

  return(0);
}

static
int uuMountQuitCatch(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  popCallback(d); /* OnOff ¤ò¥Ý¥Ã¥× */

  popMountMode(d);
  popCallback(d);
  currentModeInfo(d);

  return prevMenuIfExist(d);
}

/*
 * dicLbuf                dicLp       soldp   snewp
 * ¨£¨¡¨¡¨¡¨¡¨¡¨¡¨¡¨¡¨¡¨¤ ¨£¨¡¨¡¨¡¨¤  ¨£¨¡¨¤  ¨£¨¡¨¤
 * ¨¢iroha\@fuzokugo\@k¨¢ ¨¢*iroha¨¢  ¨¢1 ¨¢  ¨¢1 ¨¢
 * ¨¢atakana\@satoko\@s¨¢ ¨¢*fuzo ¨¢  ¨¢1 ¨¢  ¨¢1 ¨¢
 * ¨¢oft\@\@...        ¨¢ ¨¢*kata ¨¢  ¨¢0 ¨¢  ¨¢0 ¨¢
 * ¨¢                  ¨¢ ¨¢  :   ¨¢  ¨¢: ¨¢  ¨¢: ¨¢
 * ¨¦¨¡¨¡¨¡¨¡¨¡¨¡¨¡¨¡¨¡¨¥ ¨¦¨¡¨¡¨¡¨¥  ¨¦¨¡¨¥  ¨¦¨¡¨¥
 * dicMbuf                dicMp
 * ¨£¨¡¨¡¨¡¨¡¨¡¨¡¨¡¨¡¨¡¨¤ ¨£¨¡¨¡¨¡¨¤
 * ¨¢iroha\@fuzokugo\@s¨¢ ¨¢*iroha¨¢
 * ¨¢atoko\@\@...      ¨¢ ¨¢*fuzo ¨¢
 * ¨¢                  ¨¢ ¨¢*sato ¨¢
 * ¨¢                  ¨¢ ¨¢  :   ¨¢
 * ¨¦¨¡¨¡¨¡¨¡¨¡¨¡¨¡¨¡¨¡¨¥ ¨¦¨¡¨¡¨¡¨¥
 */
static
int getDicList(uiContext d)
{
  mountContext mc = (mountContext)d->modec;
  char *dicLbuf, dicMbuf[ROMEBUFSIZE];
  char **dicLp, *dicMp[ROMEBUFSIZE/2];
  char *wptr, **Lp, **Mp;
  BYTE *sop, *snp, *soldp, *snewp;
  int dicLc, dicMc, i;
  extern int defaultContext;

  if((dicLbuf = (char *)malloc(ROMEBUFSIZE)) == (char *)NULL) {
#ifndef WIN
    jrKanjiError = "malloc (getDicList) ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
#else
    jrKanjiError = "malloc (getDicList) \244\307\244\255\244\336\244\273";
#endif
    return(NG);
  }
  if(defaultContext == -1) {
    if((KanjiInit() != 0) || (defaultContext == -1)) {
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
      jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271"
	"\245\265\241\274\245\320\244\310\304\314\277\256\244\307\244\255"
	"\244\336\244\273\244\363";
#endif
#endif
      free(dicLbuf);
      return(NG);
    }
  }
  if((dicLc = RkwGetDicList(defaultContext, (char *)dicLbuf, ROMEBUFSIZE))
     < 0) {
    if(errno == EPIPE)
      jrKanjiPipeError();
    jrKanjiError = "\245\336\245\246\245\363\245\310\262\304\307\275\244\312"
	"\274\255\275\361\244\316\274\350\244\352\275\320\244\267\244\313"
	"\274\272\307\324\244\267\244\336\244\267\244\277";
                   /* ¥Þ¥¦¥ó¥È²ÄÇ½¤Ê¼­½ñ¤Î¼è¤ê½Ð¤·¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    free(dicLbuf);
    return(NG);
  }
  if (dicLc == 0) {
    jrKanjiError = "\245\336\245\246\245\363\245\310\262\304\307\275\244\312"
	"\274\255\275\361\244\254\302\270\272\337\244\267\244\336\244\273"
	"\244\363";
                   /* ¥Þ¥¦¥ó¥È²ÄÇ½¤Ê¼­½ñ¤¬Â¸ºß¤·¤Þ¤»¤ó */
    free(dicLbuf);
    return NG;
  }
  if((dicLp = (char **)calloc(dicLc + 1, sizeof(char *))) == (char **)NULL) {
#ifndef WIN
    jrKanjiError = "malloc (getDicList) ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
#else
    jrKanjiError = "malloc (getDicList) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
#endif
    free(dicLbuf);
    return(NG);
  }
  if((soldp = (BYTE *)malloc(dicLc + 1)) == (BYTE *)NULL) {
#ifndef WIN
    jrKanjiError = "malloc (getDicList) ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
#else
    jrKanjiError = "malloc (getDicList) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
#endif
    free(dicLbuf);
    free(dicLp);
    return(NG);
  }
  if((snewp = (BYTE *)malloc(dicLc + 1)) == (BYTE *)NULL) {
#ifndef WIN
    jrKanjiError = "malloc (getDicList) ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
#else
    jrKanjiError = "malloc (getDicList) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
#endif
    free(dicLbuf);
    free(dicLp);
    free(soldp);
    return(NG);
  }
  for(i = 0, wptr = dicLbuf; i < dicLc; i++) { /* buf ¤òºî¤ë */
    dicLp[i] = wptr;
    while(*wptr++)
      /* EMPTY */
      ; /* NULL ¤Þ¤Ç¥¹¥­¥Ã¥×¤·¡¢NULL ¤Î¼¡¤Þ¤Ç¥Ý¥¤¥ó¥¿¤ò¿Ê¤á¤ë */
  }
  dicLp[i] = (char *)NULL;

  if(defaultContext == -1) {
    if((KanjiInit() != 0) || (defaultContext == -1)) {
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
      jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271"
	"\245\265\241\274\245\320\244\310\304\314\277\256\244\307\244\255"
	"\244\336\244\273\244\363";
#endif
#endif
      free(dicLbuf);
      free(dicLp);
      free(soldp);
      return(NG);
    }
  }
  if((dicMc = RkwGetMountList(defaultContext, (char *)dicMbuf, ROMEBUFSIZE)) <
     0) {
    if(errno == EPIPE)
      jrKanjiPipeError();
    jrKanjiError = "\245\336\245\246\245\363\245\310\244\267\244\306\244\244"
	"\244\353\274\255\275\361\244\316\274\350\244\352\275\320\244\267"
	"\244\313\274\272\307\324\244\267\244\336\244\267\244\277";
                   /* ¥Þ¥¦¥ó¥È¤·¤Æ¤¤¤ë¼­½ñ¤Î¼è¤ê½Ð¤·¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    free(dicLbuf);
    free(dicLp);
    free(soldp);
    free(snewp);
    return(NG);
  }

  for(i = 0, wptr = dicMbuf ; i < dicMc ; i++) { /* buf ¤òºî¤ë */
    dicMp[i] = wptr;
    while (*wptr++)
      /* EMPTY */
      ; /* NULL ¤Þ¤Ç¥¹¥­¥Ã¥×¤·¡¢NULL ¤Î¼¡¤Þ¤Ç¥Ý¥¤¥ó¥¿¤ò¿Ê¤á¤ë */
  }
  dicMp[i] = (char *)NULL;

  for(i=0, sop=soldp, snp=snewp; i<dicLc; i++, sop++, snp++) {
    *sop = 0;
    *snp = 0;
  }
  for(Lp=dicLp, sop=soldp, snp=snewp; *Lp; Lp++, sop++, snp++) {
    for(Mp=dicMp; *Mp; Mp++) {
      if(!strcmp((char *)*Lp, (char *)*Mp)) {
	*sop = *snp = 1;
	break;
      }
    }
  }
  mc->mountList = dicLp;
  mc->mountOldStatus = soldp;
  mc->mountNewStatus = snewp;

  return(dicLc);
}

int dicMount(uiContext d)
{
  ichiranContext oc;
  mountContext mc;
  int retval = 0, currentkouho = 0, nelem;
  WCHAR_T *xxxx[100];
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }    
  d->status = 0;

  if((retval = getMountContext(d)) == NG) {
    killmenu(d);
    return(GLineNGReturn(d));
  }

  /* ¼­½ñ¥ê¥¹¥È¤È¥Þ¥¦¥ó¥È¡¿¥¢¥ó¥Þ¥¦¥ó¥È¤Î¾õÂÖ¤ò montContext ¤Ë¤È¤Ã¤Æ¤¯¤ë */
  if((nelem = getDicList(d)) == NG) {
    popMountMode(d);
    popCallback(d);
    killmenu(d);
    return(GLineNGReturn(d));
  }

  mc = (mountContext)d->modec;
#if defined(DEBUG) && !defined(WIN)
  if(iroha_debug) {
    int i;

    printf("<¡úmount>\n");
    for(i= 0; mc->mountList[i]; i++)
      printf("[%s][%x][%x]\n", mc->mountList[i],
	     mc->mountOldStatus[i], mc->mountNewStatus[i]);
    printf("\n");
  }
#endif

  /* selectOnOff ¤ò¸Æ¤Ö¤¿¤á¤Î½àÈ÷ */
  mc->curIkouho = currentkouho = 0;

  retval = setWStrings(xxxx, mc->mountList, 0);
  if (retval == NG) {
    popMountMode(d);
    popCallback(d);
    killmenu(d);
    return GLineNGReturn(d);
  }
  if((retval = selectOnOff(d, xxxx, &mc->curIkouho, nelem,
		 BANGOMAX, currentkouho, mc->mountOldStatus,
		 (int(*)(...))NO_CALLBACK, (int(*)(...))uuMountExitCatch,
		 (int(*)(...))uuMountQuitCatch, (int(*)(...))uiUtilIchiranTooSmall)) == NG) {
    popMountMode(d);
    popCallback(d);
    killmenu(d);
    return GLineNGReturn(d);
  }

  oc = (ichiranContext)d->modec;
  oc->majorMode = CANNA_MODE_ExtendMode;
  oc->minorMode = CANNA_MODE_MountDicMode;
  currentModeInfo(d);

  /* ¸õÊä°ìÍ÷¹Ô¤¬¶¹¤¯¤Æ¸õÊä°ìÍ÷¤¬½Ð¤»¤Ê¤¤ */
  if(oc->tooSmall) {
#ifndef WIN
    WCHAR_T p[512];
#else
    WCHAR_T p[64];
#endif

    ichiranFin(d);
    popCallback(d); /* OnOff ¤ò¥Ý¥Ã¥× */
    popMountMode(d);
    popCallback(d);
    currentModeInfo(d);
    MBstowcs(p ,"\274\255\275\361\260\354\315\367\315\321\244\316\311\375"
		"\244\254\266\271\244\244\244\316\244\307\274\255\275\361"
		"\245\336\245\246\245\363\245\310\241\277\245\242\245\363"
		"\245\336\245\246\245\363\245\310\244\307\244\255\244\336"
		"\244\273\244\363",64);
         /* ¼­½ñ°ìÍ÷ÍÑ¤ÎÉý¤¬¶¹¤¤¤Î¤Ç¼­½ñ¥Þ¥¦¥ó¥È¡¿¥¢¥ó¥Þ¥¦¥ó¥È¤Ç¤­¤Þ¤»¤ó */
    makeGLineMessage(d, p, WStrlen(p));
    killmenu(d);
    return(0);
  }

  makeGlineStatus(d);
  /* d->status = ICHIRAN_EVERYTIME; */

  return(retval);
}
#endif /* NO_EXTEND_MENU */
