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
static	char	rcs_id[] = "@(#) 102.1 $Id: onoff.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif /* lint */

#include	<errno.h>
#include	"canna.h"

#ifndef NO_EXTEND_MENU
#define ICHISIZE 9

static void popOnOffMode(uiContext d);
static int makeOnOffIchiran(uiContext d, int nelem, int bangomax, int currentkouho, unsigned char *status);
static int OnOffSelect(uiContext d);
static int OnOffKakutei(uiContext d);

static int makeOnOffIchiran();

static WCHAR_T *black;
static WCHAR_T *white;
static WCHAR_T *space;

int
initOnoffTable(void)
{
  black = WString("\241\375");
                  /* ¡ý */
  white = WString("\241\373");
                  /* ¡û */
  space = WString("\241\241");
                  /* ¡¡ */

  if (!black || !white || !space) {
    return NG;
  }
  return 0;
}

static void
popOnOffMode(uiContext d)
{
  ichiranContext oc = (ichiranContext)d->modec;

  d->modec = oc->next;
  d->current_mode = oc->prevMode;
  freeIchiranContext(oc);
}

/*
 * ¸õÊä°ìÍ÷¹Ô¤òºî¤ë
 */
int selectOnOff(uiContext d, WCHAR_T **buf, int *ck, int nelem, int bangomax, int currentkouho, unsigned char *status, int (*everyTimeCallback )(...), int (*exitCallback )(...), int (*quitCallback )(...), int (*auxCallback )(...))
{
  extern KanjiModeRec onoff_mode;
  ichiranContext oc;
  int retval = 0;
  ichiranContext newIchiranContext();

  if(pushCallback(d, d->modec,
	(int(*)(_uiContext*, int, _coreContextRec*))everyTimeCallback,
	(int(*)(_uiContext*, int, _coreContextRec*))exitCallback,
	(int(*)(_uiContext*, int, _coreContextRec*))quitCallback,
	(int(*)(_uiContext*, int, _coreContextRec*))auxCallback) == 0) {
    jrKanjiError = "malloc (pushCallback) \244\307\244\255\244\336\244\273\244\363\244\307\244\267\244\277";
                                       /* ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
    return(NG);
  }
  
  if ((oc = (ichiranContext)newIchiranContext()) == (ichiranContext)NULL) {
    popCallback(d);
    return(NG);
  }
  oc->next = d->modec;
  d->modec = (mode_context)oc;

  oc->prevMode = d->current_mode;
  d->current_mode = &onoff_mode;

  oc->allkouho = buf;
  oc->curIkouho = ck;

  if((retval = makeOnOffIchiran(d, nelem, bangomax,
			currentkouho, status))   == NG) {
    popOnOffMode(d);
    popCallback(d);
    return(NG);
  }
  return(retval);
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
makeOnOffIchiran(uiContext d, int nelem, int bangomax, int currentkouho, unsigned char *status)
{
  ichiranContext oc = (ichiranContext)d->modec;
  WCHAR_T **kkptr, *kptr, *gptr, *svgptr;
  int ko, lnko, cn = 0, svcn, line = 0, dn = 0, svdn;

  oc->nIkouho = nelem;	/* ¸õÊä¤Î¿ô */

  /* ¥«¥ì¥ó¥È¸õÊä¤ò¥»¥Ã¥È¤¹¤ë */
  oc->svIkouho = *(oc->curIkouho);
  *(oc->curIkouho) += currentkouho;
  if(*(oc->curIkouho) >= oc->nIkouho)
    oc->svIkouho = *(oc->curIkouho) = 0;

  if(allocIchiranBuf(d) == NG)
    return(NG);

  if(d->ncolumns < 1) {
    oc->tooSmall = 1;
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

  kkptr = oc->allkouho;
  kptr = *(oc->allkouho);
  gptr = oc->glinebufp;

  /* line -- ²¿ÎóÌÜ¤«
     ko   -- Á´ÂÎ¤ÎÀèÆ¬¤«¤é²¿ÈÖÌÜ¤Î¸õÊä¤«
     lnko -- Îó¤ÎÀèÆ¬¤«¤é²¿ÈÖÌÜ¤Î¸õÊä¤«
     cn   -- Îó¤ÎÀèÆ¬¤«¤é²¿¥Ð¥¤¥ÈÌÜ¤« */

  for(line=0, ko=0; ko<oc->nIkouho; line++) {
    oc->glineifp[line].gldata = gptr; /* ¸õÊä¹Ô¤òÉ½¼¨¤¹¤ë¤¿¤á¤ÎÊ¸»úÎó */
    oc->glineifp[line].glhead = ko;   /* ¤³¤Î¹Ô¤ÎÀèÆ¬¸õÊä¤Ï¡¢Á´ÂÎ¤Ç¤ÎkoÈÖÌÜ */

    oc->tooSmall = 1;
    for(lnko = cn = dn = 0;
	dn<d->ncolumns - (cannaconf.kCount ? ICHISIZE + 1: 0) &&
	lnko<bangomax && ko<oc->nIkouho ; lnko++, ko++) {
      oc->tooSmall = 0;
      kptr = kkptr[ko];
      oc->kouhoifp[ko].khretsu = line; /* ²¿¹ÔÌÜ¤ËÂ¸ºß¤¹¤ë¤«¤òµ­Ï¿ */
      oc->kouhoifp[ko].khpoint = cn + (lnko ? 1 : 0);
      oc->kouhoifp[ko].khdata = kptr;  /* ¤½¤ÎÊ¸»úÎó¤Ø¤Î¥Ý¥¤¥ó¥¿ */
      svgptr = gptr;
      svcn = cn;
      svdn = dn;

      /* ¡ý¤«¡û¤ò¥³¥Ô¡¼¤¹¤ë */
      if(lnko) {
	WStrncpy(gptr++, space, WStrlen(space));
	cn++; dn += 2;
      }
      if(status[ko] == 1)
	WStrncpy(gptr, black, WStrlen(black));
      else
	WStrncpy(gptr, white, WStrlen(white));	 
      cn ++; gptr++; dn +=2;
      /* ¸õÊä¤ò¥³¥Ô¡¼¤¹¤ë */
      for(; *kptr && dn<d->ncolumns - (cannaconf.kCount ? ICHISIZE + 1: 0);
	  gptr++, kptr++, cn++) {
	if (((*gptr = *kptr) & 0x8080) == 0x8080) dn++;
        dn++;
      }

      /* ¥«¥é¥à¿ô¤è¤ê¤Ï¤ß¤À¤·¤Æ¤·¤Þ¤¤¤½¤¦¤Ë¤Ê¤Ã¤¿¤Î¤Ç£±¤ÄÌá¤¹ */
      if ((dn >= d->ncolumns - (cannaconf.kCount ? ICHISIZE + 1: 0))
	  && *kptr) {
	if (lnko) {
	  gptr = svgptr;
	  cn = svcn;
	  dn = svdn;
	}
	else {
	  oc->tooSmall = 1;
	}
	break;
      }
    }
    if (oc->tooSmall) {
      return 0;
    }
    if (cannaconf.kCount) {
      for (;dn < d->ncolumns - 1; dn++) {
	*gptr++ = ' ';
      }
    }
    /* £±¹Ô½ª¤ï¤ê */
    *gptr++ = (WCHAR_T)0;
    oc->glineifp[line].glkosu = lnko;
    oc->glineifp[line].gllen = WStrlen(oc->glineifp[line].gldata);
  }
  /* ºÇ¸å¤ËNULL¤òÆþ¤ì¤ë */
  oc->kouhoifp[ko].khretsu = 0;
  oc->kouhoifp[ko].khpoint = 0;
  oc->kouhoifp[ko].khdata  = (WCHAR_T *)NULL;
  oc->glineifp[line].glkosu  = 0;
  oc->glineifp[line].glhead  = 0;
  oc->glineifp[line].gllen   = 0;
  oc->glineifp[line].gldata  = (WCHAR_T *)NULL;

#if defined(DEBUG) && !defined(WIN)
  if (iroha_debug) {
    int i;
    for(i=0; oc->glineifp[i].glkosu; i++)
      printf("%d: %s\n", i, oc->glineifp[i].gldata);
  }
#endif

  return(0);
}

/*
 * ¥«¥ì¥ó¥È¸õÊä¤ò¸½ºß¤ÈÈ¿ÂÐ¤Ë¤¹¤ë(ON¢ªOFF, OFF¢ªON)
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
static
int OnOffSelect(uiContext d)
{
  ichiranContext oc = (ichiranContext)d->modec;
  mountContext mc = (mountContext)oc->next;
  int point, retval = 0;
  WCHAR_T *gline;

  /* mountNewStatus ¤òÊÑ¹¹¤¹¤ë (1¢ª0, 0¢ª1) */
  if(mc->mountNewStatus[*(oc->curIkouho)])
    mc->mountNewStatus[*(oc->curIkouho)] = 0;
  else
    mc->mountNewStatus[*(oc->curIkouho)] = 1;

  /* glineÍÑ¤Î¥Ç¡¼¥¿¤ò½ñ¤­´¹¤¨¤ë (¡ý¢ª¡û, ¡û¢ª¡ý) */
  gline = oc->glineifp[oc->kouhoifp[*(oc->curIkouho)].khretsu].gldata;
  point = oc->kouhoifp[*(oc->curIkouho)].khpoint;

    *(gline+point) = ((mc->mountNewStatus[*(oc->curIkouho)]) ? *black : *white);
  makeGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return(retval);
}

/*
 * status ¤ò¤½¤Î¤Þ¤ÞÊÖ¤·¡¢OnOff¥â¡¼¥É¤òPOP¤¹¤ë (EXIT_CALLBACK)
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
static
int OnOffKakutei(uiContext d)
{
  ichiranContext oc = (ichiranContext)d->modec;
  int retval = 0;
/* ¤¤¤é¤Ê¤¤¤Î¤Ç¤Ï unsigned char *kakuteiStrings;*/

  /* ¸õÊä°ìÍ÷É½¼¨¹ÔÍÑ¤Î¥¨¥ê¥¢¤ò¥Õ¥ê¡¼¤¹¤ë */
  freeIchiranBuf(oc);

  popOnOffMode(d);

#if defined(DEBUG) && !defined(WIN)
  if(iroha_debug) {
    mountContext mc = (mountContext)d->modec;
    int i;

    printf("<¡úmount>\n");
    for(i= 0; mc->mountList[i]; i++)
      printf("[%s][%x][%x]\n", mc->mountList[i],
	     mc->mountOldStatus[i], mc->mountNewStatus[i]);
    printf("\n");
  }
#endif

  /* gline ¤ò¥¯¥ê¥¢¤¹¤ë */
  GlineClear(d);

  d->status = EXIT_CALLBACK;

  return(retval);
}
#endif /* NO_EXTEND_MENU */

#include	"onoffmap.h"
