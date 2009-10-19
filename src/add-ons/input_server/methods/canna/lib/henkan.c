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
static	char	rcs_id[] = "@(#) 102.1 $Id: henkan.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif /* lint */

#include	"canna.h"
#include "RK.h"
#include "RKintern.h"

#include	<errno.h>
#include	<fcntl.h>
#ifdef MEASURE_TIME
#include <sys/types.h>
#ifdef WIN
#include <sys/timeb.h>
#else
/* If you compile with Visual C++ on WIN, then please comment out next line. */
#include <sys/times.h>
#endif
#endif

extern struct RkRxDic *romajidic, *englishdic;

extern int defaultBushuContext;
extern int yomiInfoLevel;
extern int ckverbose;
extern int defaultContext;
extern struct dicname *RengoGakushu, *KatakanaGakushu, *HiraganaGakushu;
extern KanjiModeRec cy_mode, cb_mode, yomi_mode, tankouho_mode, empty_mode;
extern char saveapname[];
extern int RkwGetServerVersion (int *, int *);

#define DICERRORMESGLEN 78


static int doYomiHenkan(uiContext d, int len, WCHAR_T *kanji);

static char dictmp[DICERRORMESGLEN];

inline int
kanakanError(uiContext d)
{
  return makeRkError(d, "Kana-kanji conversion feiled");
                        /* ¤«¤Ê´Á»úÊÑ´¹¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
}

static void
dicMesg(char *s, char *d)
{
#ifndef WIN
  if (ckverbose == CANNA_FULL_VERBOSE) {
    char buf[128];
    sprintf(buf, "\"%s\"", d);
    printf("%14s %-20s ¤ò»ØÄê¤·¤Æ¤¤¤Þ¤¹¡£\n", s, buf);
  }
#endif
}

static void
RkwInitError(void)
{
  if (errno == EPIPE) {
    jrKanjiError = KanjiInitError();
  }else {
    jrKanjiError = "Error in initailzaton of Kana-kanji conversion dictionaly.";
                   /* ¤«¤Ê´Á»úÊÑ´¹¼­½ñ¤Î½é´ü²½¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
  }
  addWarningMesg(jrKanjiError);
  RkwFinalize();
}

static void
mountError(char *dic)
{
static char		*mountErrorMessage = " can't mount. "; /* ¤ò¥Þ¥¦¥ó¥È¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
int				mnterrlen;

  if (DICERRORMESGLEN < 
      (unsigned)(strlen(dic) + (mnterrlen = strlen(mountErrorMessage)) + 1)) {
    (void)strncpy(dictmp, dic, DICERRORMESGLEN - mnterrlen - 3/* ... */ - 1);
    (void)strcpy(dictmp + DICERRORMESGLEN - mnterrlen - 3 - 1, "...");
    strcpy(dictmp + DICERRORMESGLEN - mnterrlen - 1, mountErrorMessage);
  }
  else {
    sprintf(dictmp, "%s%s", dic, mountErrorMessage);
  }
  jrKanjiError = dictmp;
  addWarningMesg(dictmp);
}

inline void
autodicError(void)
{
  jrKanjiError = "¼«Æ°ÅÐÏ¿ÍÑ¼­½ñ¤¬Â¸ºß¤·¤Þ¤»¤ó";
  addWarningMesg(jrKanjiError);
}

/*
 * ¤«¤Ê´Á»úÊÑ´¹¤Î¤¿¤á¤Î½é´ü½èÍý
 *
 * ¡¦RkwInitialize¤ò¸Æ¤ó¤Ç¡¢defaultContext ¤òºîÀ®¤¹¤ë
 * ¡¦defaultBushuContext ¤òºîÀ®¤¹¤ë
 * ¡¦¼­½ñ¤Î¥µ¡¼¥Á¥Ñ¥¹¤òÀßÄê¤¹¤ë
 * ¡¦¥·¥¹¥Æ¥à¼­½ñ¡¢Éô¼óÍÑ¼­½ñ¡¢¥æ¡¼¥¶¼­½ñ¤ò¥Þ¥¦¥ó¥È¤¹¤ë
 *
 * °ú¤­¿ô	¤Ê¤·
 * Ìá¤êÃÍ	0:¤Þ¤¢Àµ¾ï¡¢ -1:¤È¤³¤È¤óÉÔÎÉ
 */
int KanjiInit()
{
char			*ptr, *getenv(), *kodmesg = ""/* ¼­½ñ¤Î¼ïÊÌËè¤Î¥á¥Ã¥»¡¼¥¸ */;
int				con;
static int		mountnottry = 1; /* ¥Þ¥¦¥ó¥È½èÍý¤ò¹Ô¤Ã¤Æ¤¤¤ë¤«¤É¤¦¤« */
struct			dicname *stp;
extern struct	dicname *kanjidicnames;
extern			int FirstTime;
extern			jrUserInfoStruct *uinfo;
extern char		*RkGetServerHost(void);
extern char		basepath[];
int 			ret = -1;
char 			dichomedir[256];

#if defined(DEBUG) && !defined(WIN)
  if (iroha_debug) {
    fprintf(stderr,"\n¥µ¡¼¥Ð¤ËÀÜÂ³¤·¤¿ strokelimit = %d (default:%d)\n",
              cannaconf.strokelimit, STROKE_LIMIT);
  }
#endif
  /* Ï¢Ê¸Àá¥é¥¤¥Ö¥é¥ê¤ò½é´ü²½¤¹¤ë */
  if (uinfo) {
    RkwSetUserInfo(uinfo->uname, uinfo->gname, uinfo->topdir);
  }

    sprintf(dichomedir, "%s%s", basepath, "dic");
//    puts(dichomedir);
	fputs( dichomedir, stderr );
  if ((defaultContext = RkwInitialize(dichomedir)) == -1) {
    RkwInitError();
    ret = -1;
    goto return_ret;
  }

  if (defaultContext != -1) {
    if((defaultBushuContext = RkwCreateContext()) == -1) {
      jrKanjiError = "\311\364\274\363\315\321\244\316\245\263\245\363\245\306"
	"\245\257\245\271\245\310\244\362\272\356\300\256\244\307\244\255"
	"\244\336\244\273\244\363\244\307\244\267\244\277";
                     /* Éô¼óÍÑ¤Î¥³¥ó¥Æ¥¯¥¹¥È¤òºîÀ®¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
      addWarningMesg(jrKanjiError);
      defaultContext = -1;
      RkwFinalize();
      ret = -1;
      goto return_ret;
    }
  } else {
    defaultBushuContext = -1;
  }

  debug_message("\245\307\245\325\245\251\245\353\245\310\245\263\245\363"
	"\245\306\245\255\245\271\245\310(%d), \311\364\274\363\245\263"
	"\245\363\245\306\245\255\245\271\245\310(%d)\n",
		defaultContext, defaultBushuContext, 0);
               /* ¥Ç¥Õ¥©¥ë¥È¥³¥ó¥Æ¥­¥¹¥È(%d), Éô¼ó¥³¥ó¥Æ¥­¥¹¥È(%d)\n */

      if((RkwSetDicPath(defaultContext, "canna")) == -1) {
	jrKanjiError = "¼­½ñ¤Î¥Ç¥£¥ì¥¯¥È¥ê¤òÀßÄê¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
	RkwFinalize();
	return(NG);
      }
      if((RkwSetDicPath(defaultBushuContext, "canna")) == -1) {
	jrKanjiError = "¼­½ñ¤Î¥Ç¥£¥ì¥¯¥È¥ê¤òÀßÄê¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿";
	RkwFinalize();
	return(NG);
      }


  if (defaultContext != -1) {

    if (saveapname[0]) {
      RkwSetAppName(defaultContext, saveapname);
    }

    if (!FirstTime && !mountnottry) { /* KC_INITIALIZE ¤Ç¸Æ¤Ó½Ð¤µ¤ì¤Æ¤¤¤Ê¤¯¤Æ¡¢
					 ´û¤Ë¥Þ¥¦¥ó¥È½èÍý¤ò¹Ô¤Ã¤Æ¤¤¤ë¾ì¹ç */
      /* Ê¸Ë¡¼­½ñ¤Î¥Þ¥¦¥ó¥È */
      for (stp = kanjidicnames; stp ; stp = stp->next) {
	if (stp->dictype == DIC_GRAMMAR) {
	  if (stp->dicflag == DIC_MOUNTED) {
	    if (RkwMountDic(defaultContext, stp->name,
			    cannaconf.kojin ? PL_ALLOW : PL_INHIBIT) == -1) {
	      stp->dicflag = DIC_MOUNT_FAILED;
	      mountError(stp->name);
	    }
	    else {
	      stp->dicflag = DIC_MOUNTED;
	      dicMesg("\312\270\313\241\274\255\275\361", stp->name);
                      /* Ê¸Ë¡¼­½ñ */
	    }
	  }
	}
      }
      /* ¥·¥¹¥Æ¥à¼­½ñ¤Î¥Þ¥¦¥ó¥È */
      for (stp = kanjidicnames ; stp ; stp = stp->next) {
        if (stp->dictype != DIC_GRAMMAR) {
          if (stp->dicflag == DIC_MOUNTED) {
            if (stp->dictype == DIC_BUSHU) {
              con = defaultBushuContext;
            }
            else {
              con = defaultContext;
            }
            if (RkwMountDic(con, stp->name,
			    cannaconf.kojin ? PL_ALLOW : PL_INHIBIT)
              == -1) {
#if defined(DEBUG) && !defined(WIN)
            if (iroha_debug) {
              fprintf(stderr, "saveddicname = %s\n", stp->name);
            }
#endif
	      stp->dicflag = DIC_MOUNT_FAILED;
	      mountError(stp->name);
	    }
	    dicMesg("saveddicname\244\316\274\255\275\361", stp->name);
                    /* saveddicname¤Î¼­½ñ */
	  }
	}
      }
    }
    else { /* KC_INITIALIZE ¤«¤é¸Æ¤Ó½Ð¤µ¤ì¤Æ¤¤¤ë¾ì¹ç¡£
              ¤Þ¤¿¤Ï¡¢¥Þ¥¦¥ó¥È½èÍý¤ò¹Ô¤Ã¤Æ¤¤¤Ê¤¤¾ì¹ç */
#if defined(DEBUG) && !defined(WIN)
      if (iroha_debug) {
        fprintf(stderr, "¼­½ñ¤Ï.canna¤ÎÄÌ¤ê¤Ë¥Þ¥¦¥ó¥È¤¹¤ë\n");
      }
#endif

      mountnottry = 0; /* ¥Þ¥¦¥ó¥È½èÍý¤ò¹Ô¤¦¤Î¤Ç mountnottry = 0 ¤Ë¤¹¤ë */
      /* Ê¸Ë¡¼­½ñ¤Î¥Þ¥¦¥ó¥È */
      for (stp = kanjidicnames; stp ; stp = stp->next) {
	if (stp->dictype == DIC_GRAMMAR) {
	  if (RkwMountDic(defaultContext, stp->name,
			  cannaconf.kojin ? PL_ALLOW : PL_INHIBIT) == -1) {
	    stp->dicflag = DIC_MOUNT_FAILED;
	    mountError(stp->name);
	  }
	  else {
	    stp->dicflag = DIC_MOUNTED;
	    dicMesg("\312\270\313\241\274\255\275\361", stp->name);
                    /* Ê¸Ë¡¼­½ñ */
	  }
	}
      }

      /* ¥·¥¹¥Æ¥à¼­½ñ¤Î¥Þ¥¦¥ó¥È */
      for (stp = kanjidicnames ; stp ; stp = stp->next) {
        if (stp->dictype != DIC_GRAMMAR) {
          con = defaultContext;
          if (stp->dictype == DIC_PLAIN) {
            kodmesg = "\245\267\245\271\245\306\245\340\274\255\275\361";
                      /* "¥·¥¹¥Æ¥à¼­½ñ"; */
          }
          else if (stp->dictype == DIC_USER) {
            /* ¥æ¡¼¥¶¼­½ñ¤Î¥Þ¥¦¥ó¥È */    
           kodmesg = "\303\261\270\354\305\320\317\277\315\321\274\255\275\361";
                     /* "Ã±¸ìÅÐÏ¿ÍÑ¼­½ñ"; */
          }
          else if (stp->dictype == DIC_RENGO) {
            /* Ï¢¸ì¼­½ñ¤Î¥Þ¥¦¥ó¥È */
            RengoGakushu = stp;
            kodmesg = "\317\242\270\354\274\255\275\361";
                      /* "Ï¢¸ì¼­½ñ"; */
          }
          else if (stp->dictype == DIC_KATAKANA) {
            KatakanaGakushu = stp;
            kodmesg = "\274\253\306\260\305\320\317\277\315\321\274\255\275\361";
                      /* "¼«Æ°ÅÐÏ¿ÍÑ¼­½ñ"; */
          }
          else if (stp->dictype == DIC_HIRAGANA) {
            HiraganaGakushu = stp;
#ifdef HIRAGANAAUTO
            kodmesg = "\274\253\306\260\305\320\317\277\315\321\274\255\275\361";
                      /* "¼«Æ°ÅÐÏ¿ÍÑ¼­½ñ"; */
#else
            kodmesg = "\317\242\270\354\274\255\275\361";
                      /* "Ï¢¸ì¼­½ñ"; */
#endif
          }
          else if (stp->dictype == DIC_BUSHU) {
            kodmesg = "\311\364\274\363\274\255\275\361";
                      /* "Éô¼ó¼­½ñ"; */
            con = defaultBushuContext;
          }
          if (RkwMountDic(con, stp->name,
			  cannaconf.kojin ? PL_ALLOW : PL_INHIBIT) == -1) {
            extern int auto_define;

            stp->dicflag = DIC_MOUNT_FAILED;
            if (stp->dictype == DIC_KATAKANA
#ifdef HIRAGANAAUTO
                || stp->dictype == DIC_HIRAGANA
#endif
               ) {
              /* ¼«Æ°ÅÐÏ¿¼­½ñ¤À¤Ã¤¿¤é¡¢¼«Æ°ÅÐÏ¿¤·¤Ê¤¤ */
              auto_define = 0;
            }
            if (stp->dictype != DIC_USER || strcmp(stp->name, "user")) {
              /* ¥æ¡¼¥¶¼­½ñ¤Ç user ¤È¤¤¤¦Ì¾Á°¤Î¾ì¹ç¤Ï¥¨¥é¡¼É½¼¨ *
               * ¤·¤Ê¤¤¤è¤¦¤Ë¤¹¤ë¤¿¤á                           */
              int majv, minv;

              RkwGetServerVersion(&majv, &minv);
              if (!(canna_version(majv, minv) < canna_version(3, 4))
                  || ((stp->dictype != DIC_KATAKANA ||
                         strcmp(stp->name, "katakana"))
#ifdef HIRAGANAAUTO
                     && (stp->dictype != DIC_HIRAGANA ||
                           strcmp(stp->name, "hiragana"))
#endif
                  )) {
                /* V3.3 °ÊÁ°¤Ç¡¢¥«¥¿¥«¥Ê¼­½ñ¤¬ katakana¡¢¤Ò¤é¤¬¤Ê¼­½ñ¤¬
                   hiragana ¤Î¾ì¹ç¤Ï¥¨¥é¡¼¤Ë¤·¤Ê¤¤¤¿¤á                  */
                extern char *kataautodic;
#ifdef HIRAGANAAUTO
                extern char *hiraautodic;
#endif

                if (!auto_define ||
                    ((kataautodic && strcmp(stp->name, kataautodic))
#ifdef HIRAGANAAUTO
                    && (hiraautodic && strcmp(stp->name, hiraautodic))
#endif
                   )) {
                  if (stp->dictype == DIC_KATAKANA
#ifdef HIRAGANAAUTO
                      || stp->dictype == DIC_HIRAGANA
#endif
                     ) {
                    autodicError();
                  }
                  else {
                    mountError(stp->name);
                  }
                }
              }
            }
          }
          else {
            stp->dicflag = DIC_MOUNTED;
            dicMesg(kodmesg, stp->name);
          }
        }
      }
    }
    ret = 0;
    goto return_ret;
  }
  ret = -1;
 return_ret:
#ifdef WIN
  free(buf);
#endif
  return ret;
}
/*
 * ¤«¤Ê´Á»úÊÑ´¹¤Î¤¿¤á¤Î¸å½èÍý
 *
 * ¡¦¥·¥¹¥Æ¥à¼­½ñ¡¢Éô¼óÍÑ¼­½ñ¡¢¥æ¡¼¥¶¼­½ñ¤ò¥¢¥ó¥Þ¥¦¥ó¥È¤¹¤ë
 * ¡¦RkwFinalize¤ò¸Æ¤Ö
 *
 * °ú¤­¿ô	¤Ê¤·
 * Ìá¤êÃÍ	¤Ê¤·
 */
int KanjiFin(void)
{
  struct dicname *dp, *np;
  int con;

  for (dp = kanjidicnames ; dp ;) {
    if (dp->dictype == DIC_BUSHU) {
      con = defaultBushuContext;
    }
    else {
      con = defaultContext;
    }
    if (dp->dicflag == DIC_MOUNTED) {
      if (RkwUnmountDic(con, dp->name) == -1) {
#ifdef WIN
	char *buf = malloc(128);
	if (buf) {
	  sprintf(buf, "%s \244\362\245\242\245\363\245\336\245\246\245\363"
	  "\245\310\244\307\244\255\244\336\244\273\244\363\244\307\244\267"
	  "\244\277", dp->name);
	  addWarningMesg(buf);
	  free(buf);
	}
#else
        char buf[256];
	sprintf(buf, "%s ¤ò¥¢¥ó¥Þ¥¦¥ó¥È¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿¡£", dp->name);
	addWarningMesg(buf);
#endif
      }
    }
    np = dp->next;
    free(dp->name);
    free(dp);
    dp = np;
  }
  kanjidicnames = (struct dicname *)0;
	  
  /* Ï¢Ê¸Àá¥é¥¤¥Ö¥é¥ê¤ò½ªÎ»¤µ¤»¤ë */
  RkwFinalize();

  return(0);
}

static tanContext
newTanContext(int majo, int mino)
{
  tanContext tan;

  tan = (tanContext)malloc(sizeof(tanContextRec));
  if (tan) {
    bzero(tan, sizeof(tanContextRec));
    tan->id = TAN_CONTEXT;
    tan->majorMode = majo;
    tan->minorMode = mino;
    tan->left = tan->right = (tanContext)0;
    tan->next = (mode_context)0;
    tan->curMode = &tankouho_mode;
  }
  return tan;
}

void
freeTanContext(tanContext tan)
{
  if (tan->kanji) free(tan->kanji);
  if (tan->yomi) free(tan->yomi);
  if (tan->roma) free(tan->roma);
  if (tan->kAttr) free(tan->kAttr);
  if (tan->rAttr) free(tan->rAttr);
  free(tan);
}

static WCHAR_T *
DUpwstr(WCHAR_T *w, int l)
{
  WCHAR_T *res;

  res = (WCHAR_T *)malloc((l + 1) * sizeof(WCHAR_T));
  if (res) {
    WStrncpy(res, w, l);
    res[l] = (WCHAR_T)0;
  }
  return res;
}

static BYTE *
DUpattr(BYTE *a, int l)
{
  BYTE *res;

  res = (BYTE *)malloc((l + 1) * sizeof(BYTE));
  if (res) {
    bcopy(a, res, (l + 1) * sizeof(BYTE));
  }
  return res;
}

static void
copyYomiinfo2Tan(yomiContext yc, tanContext tan)
{
  tan->next = yc->next;
  tan->prevMode = yc->prevMode;
  tan->generalFlags = yc->generalFlags;
  tan->savedFlags = yc->savedFlags;

  tan->romdic = yc->romdic;
  tan->myMinorMode = yc->myMinorMode;
  tan->myEmptyMode = yc->myEmptyMode;
  tan->savedMinorMode = yc->savedMinorMode;
  tan->allowedChars = yc->allowedChars;
  tan->henkanInhibition = yc->henkanInhibition;
}

static void
copyTaninfo2Yomi(tanContext tan, yomiContext yc)
{
  /* next ¤È prevMode ¤Ï´û¤ËÀßÄêºÑ¤ß */
  yc->generalFlags = tan->generalFlags;
  yc->savedFlags = tan->savedFlags;

  yc->romdic = tan->romdic;
  yc->myMinorMode = tan->myMinorMode;
  yc->myEmptyMode = tan->myEmptyMode;
  yc->savedMinorMode = tan->savedMinorMode;
  yc->allowedChars = tan->allowedChars;
  yc->henkanInhibition = tan->henkanInhibition;
}

extern yomiContext dupYomiContext (yomiContext);
extern void setMode (uiContext, tanContext, int);

extern void trimYomi (uiContext, int, int, int, int);

/*
  Á´Ê¸Àá¤ò tanContext ¤ËÊÑ´¹¤¹¤ë
 */

int
doTanConvertTb(uiContext d, yomiContext yc)
{
  int cur = yc->curbun, i, len, ylen = 0, rlen = 0, ret = 0;
  int scuryomi, ecuryomi, scurroma, ecurroma;
  tanContext tan, prevLeft = yc->left, curtan = (tanContext)0;
  BYTE *p, *q, *r;
#ifndef WIN
  WCHAR_T xxx[ROMEBUFSIZE];
#else
  WCHAR_T *xxx = (WCHAR_T *)malloc(sizeof(WCHAR_T) * ROMEBUFSIZE);
  if (!xxx) {
    return ret;
  }
#endif

  yc->kouhoCount = 0;
  scuryomi = ecuryomi = scurroma = ecurroma = 0;

/*  jrKanjiError = "¥á¥â¥ê¤¬Â­¤ê¤Þ¤»¤ó"; */
  jrKanjiError = "malloc (doTanBubunMuhenkan) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277\241\243";
                 /* malloc (doTanBubunMuhenkan) ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */ 
  for (i = 0 ; i < yc->nbunsetsu ; i++) {
    tan = newTanContext(yc->majorMode, CANNA_MODE_TankouhoMode);
    if (tan) {
      copyYomiinfo2Tan(yc, tan);
      RkwGoTo(yc->context, i);
      len = RkwGetKanji(yc->context, xxx, ROMEBUFSIZE);
      if (len >= 0) {
	tan->kanji = DUpwstr(xxx, len);
	if (tan->kanji) {
	  len = RkwGetYomi(yc->context, xxx, ROMEBUFSIZE);
	  if (len >= 0) {
	    tan->yomi = DUpwstr(xxx, len);
	    if (tan->yomi) {
	      tan->kAttr = DUpattr(yc->kAttr + ylen, len);
	      if (tan->kAttr) {
		r = yc->rAttr + rlen;
		for (p = yc->kAttr + ylen, q = p + len ; p < q ; p++) {
		  if (*p & SENTOU) {
		    r++;
		    while (!(*r & SENTOU)) {
		      r++;
		    }
		  }
		}
		ylen += len;
		len = r - yc->rAttr - rlen; /* ¥í¡¼¥Þ»ú¤ÎÄ¹¤µ */
		tan->roma = DUpwstr(yc->romaji_buffer + rlen, len);
		if (tan->roma) {
		  tan->rAttr = DUpattr(yc->rAttr + rlen, len);
		  if (tan->rAttr) {
		    rlen += len;
		    /* ¤È¤ê¤¢¤¨¤ºº¸¤Ë¤Ä¤Ê¤²¤ë */
		    tan->right = (tanContext)yc;
		    tan->left = yc->left;
		    if (yc->left) {
		      yc->left->right = tan;
		    }
		    yc->left = tan;
		    if (i == cur) {
		      curtan = tan;
		    }
		    continue;
		  }
		  free(tan->roma);
		}
		free(tan->kAttr);
	      }
	      free(tan->yomi);
	    }
	  }
	  else {
	    makeRkError(d, KanjiInitError());
	  }
	  free(tan->kanji);
	}
      }else{
	makeRkError(d, KanjiInitError());
      }
      freeTanContext(tan);
    }
    /* ¥¨¥é¡¼½èÍý¤ò¤¹¤ë */
  procerror:
    while ((tan = yc->left) != prevLeft) {
      yc->left = tan->left;
      freeTanContext(tan);
    }
    ret = -1;
    goto return_ret;
  }

  if (chikujip(yc) && chikujiyomiremain(yc)) {
    int rpos;
    yomiContext lyc = dupYomiContext(yc);

    if (!lyc) { /* ¥¨¥é¡¼½èÍý¤ò¤¹¤ë */
      goto procerror;
    }

    if (yc->right) { /* Ãà¼¡¤Î¾ì¹ç¤Ê¤¤¤Ï¤º¤À¤¬Ç°¤Î¤¿¤á */
      yc->right->left = (tanContext)lyc;
    }
    lyc->right = yc->right;
    yc->right = (tanContext)lyc;
    lyc->left = (tanContext)yc;

    kPos2rPos(lyc, 0, yc->cStartp, (int *)0, &rpos);
    d->modec = (mode_context)lyc;
    moveToChikujiYomiMode(d);
    trimYomi(d, yc->cStartp, yc->kEndp, rpos, yc->rEndp);
    d->modec = (mode_context)yc;
    yc->status = lyc->status;
    lyc->cStartp = lyc->cRStartp = lyc->ys = lyc->ye = 0;
  }

  RkwGoTo(yc->context, cur);
  if (RkwEndBun(yc->context, cannaconf.Gakushu ? 1 : 0) == -1) {
    jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\316"
	"\275\252\316\273\244\313\274\272\307\324\244\267\244\336\244\267"
	"\244\277";
                   /* ¤«¤Ê´Á»úÊÑ´¹¤Î½ªÎ»¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
  }

  d->modec = (mode_context)curtan;
  setMode(d, curtan, 1);
  makeKanjiStatusReturn(d, (yomiContext)curtan);

  /* yc ¤ò¥ê¥ó¥¯¤«¤éÈ´¤¯ */
  if (yc->left) {
    yc->left->right = yc->right;
  }
  if (yc->right) {
    yc->right->left = yc->left;
  }
  abandonContext(d, yc);
  freeYomiContext(yc);

 return_ret:
#ifdef WIN
  free(xxx);
#endif

  return ret;
}

static int
doTanBubunMuhenkan(uiContext d, yomiContext yc)
{
  int cur = yc->curbun, i, len, ylen = 0, rlen = 0, ret = 0;
  int scuryomi, ecuryomi, scurroma, ecurroma;
  tanContext tan, prevLeft = yc->left;
  BYTE *p, *q, *r;
#ifndef WIN
  WCHAR_T xxx[ROMEBUFSIZE];
#else
  WCHAR_T *xxx = (WCHAR_T *)malloc(sizeof(WCHAR_T) * ROMEBUFSIZE);
  if (!xxx) {
    return ret;
  }
#endif

  yc->kouhoCount = 0;
  scuryomi = ecuryomi = scurroma = ecurroma = 0;

/*  jrKanjiError = "¥á¥â¥ê¤¬Â­¤ê¤Þ¤»¤ó"; */
  jrKanjiError = "malloc (doTanBubunMuhenkan) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277\241\243";
                 /* malloc (doTanBubunMuhenkan) ¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */ 
  for (i = 0 ; i < yc->nbunsetsu ; i++) {
    tan = (tanContext)0;
    if (i == cur ||
	(tan = newTanContext(yc->majorMode, CANNA_MODE_TankouhoMode))) {
      if (tan) {
	copyYomiinfo2Tan(yc, tan);
      }
      RkwGoTo(yc->context, i);
      len = RkwGetKanji(yc->context, xxx, ROMEBUFSIZE);
      if (len >= 0) {
	if (!tan || (tan->kanji = DUpwstr(xxx, len))) {
	  len = RkwGetYomi(yc->context, xxx, ROMEBUFSIZE);
	  if (len >= 0) {
	    if (!tan || (tan->yomi = DUpwstr(xxx, len))) {
	      if (!tan || (tan->kAttr = DUpattr(yc->kAttr + ylen, len))) {
		r = yc->rAttr + rlen;
		for (p = yc->kAttr + ylen, q = p + len ; p < q ; p++) {
		  if (*p & SENTOU) {
		    r++;
		    while (!(*r & SENTOU)) {
		      r++;
		    }
		  }
		}
		if (i == cur) {
		  scuryomi = ylen;
		  ecuryomi = ylen + len;
		}
		ylen += len;
		len = r - yc->rAttr - rlen; /* ¥í¡¼¥Þ»ú¤ÎÄ¹¤µ */
		if (!tan ||
		    (tan->roma = DUpwstr(yc->romaji_buffer + rlen, len))) {
		  if (!tan || (tan->rAttr = DUpattr(yc->rAttr + rlen, len))) {
		    if (i == cur) {
		      scurroma = rlen;
		      ecurroma = rlen + len;
		    }
		    rlen += len;
		    if (tan) {
		      if (i != cur) {
			/* ¤È¤ê¤¢¤¨¤ºº¸¤Ë¤Ä¤Ê¤²¤ë */
			tan->right = (tanContext)yc;
			tan->left = yc->left;
			if (yc->left) {
			  yc->left->right = tan;
			}
			yc->left = tan;
		      }
#if defined(DEBUG) && !defined(WIN)
		      {
			char yyy[ROMEBUFSIZE];
			WCstombs(yyy, tan->kanji, ROMEBUFSIZE);
			printf("%s/", yyy);
			WCstombs(yyy, tan->yomi, ROMEBUFSIZE);
			printf("%s/", yyy);
			WCstombs(yyy, tan->roma, ROMEBUFSIZE);
			printf("%s\n", yyy);
		      }
#endif
		    }
		    continue;
		  }
		  if (tan) free(tan->roma);
		}
		if (tan) free(tan->kAttr);
	      }
	      if (tan) free(tan->yomi);
	    }
	  }
	  else {
	    makeRkError(d, KanjiInitError());
	  }
	  if (tan) free(tan->kanji);
	}
      }else{
	makeRkError(d, KanjiInitError());
      }
      if (tan) freeTanContext(tan);
    }
    /* ¥¨¥é¡¼½èÍý¤ò¤¹¤ë */
    while ((tan = yc->left) != prevLeft) {
      yc->left = tan->left;
      freeTanContext(tan);
    }
    ret = -1;
    goto return_ret;
  }

  if (chikujip(yc) && chikujiyomiremain(yc)) {
    int rpos;
    yomiContext lyc = dupYomiContext(yc);

    if (!lyc) { /* ¥¨¥é¡¼½èÍý¤ò¤¹¤ë */
      while ((tan = yc->left) != prevLeft) {
	yc->left = tan->left;
	freeTanContext(tan);
      }
      ret = -1;
      goto return_ret;
    }

    if (yc->right) { /* ¤Ê¤¤¤Ï¤º */
      yc->right->left = (tanContext)lyc;
    }
    lyc->right = yc->right;
    yc->right = (tanContext)lyc;
    lyc->left = (tanContext)yc;

    kPos2rPos(lyc, 0, yc->cStartp, (int *)0, &rpos);
    d->modec = (mode_context)lyc;
    moveToChikujiYomiMode(d);
    trimYomi(d, yc->cStartp, yc->kEndp, rpos, yc->rEndp);
    d->modec = (mode_context)yc;
    yc->status = lyc->status;
    lyc->cStartp = lyc->cRStartp = lyc->ys = lyc->ye = 0;
  }

  if (cur + 1 < yc->nbunsetsu) { /* yc ¤¬ºÇ¸å¤¸¤ã¤Ê¤¤¾ì¹ç */
    int n = yc->nbunsetsu - cur - 1;
    tan = yc->left;
    tan->right = yc->right;
    if (yc->right) {
      yc->right->left = tan;
    }
    for (i = 1 ; i < n ; i++) { /* yomi ¤Î right ¤ËÍè¤ë¤Ù¤­ tan ¤òÆÀ¤¿¤¤ */
      tan = tan->left;
    }
    if (tan->left) {
      tan->left->right = (tanContext)yc;
    }
    yc->left = tan->left;
    tan->left = (tanContext)yc;
    yc->right = tan;
  }
  RkwGoTo(yc->context, cur);
  if (RkwEndBun(yc->context, cannaconf.Gakushu ? 1 : 0) == -1) {
    jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\316"
	"\275\252\316\273\244\313\274\272\307\324\244\267\244\336\244\267"
	"\244\277";
                   /* ¤«¤Ê´Á»úÊÑ´¹¤Î½ªÎ»¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
  }

  trimYomi(d, scuryomi, ecuryomi, scurroma, ecurroma);

  yc->cRStartp = yc->rCurs = yc->rStartp = 0;
  yc->cStartp = yc->kCurs = yc->kRStartp =
    yc->ys = yc->ye = 0;
  yc->status &= CHIKUJI_NULL_STATUS;
  /* ¤Ê¤ó¤ÈÃà¼¡¤Ç¤Ê¤¯¤Ê¤ë */
  if (chikujip(yc)) {
    yc->generalFlags &= ~CANNA_YOMI_CHIKUJI_MODE;
    yc->generalFlags |= CANNA_YOMI_BASE_CHIKUJI;
  }

  d->current_mode = yc->curMode = &yomi_mode;
  yc->minorMode = getBaseMode(yc);

  /* Á´ÉôÌµÊÑ´¹¤Ë¤¹¤ë */
  yc->nbunsetsu = 0;

  /* Ã±¸õÊä¾õÂÖ¤«¤éÆÉ¤ß¤ËÌá¤ë¤È¤­¤Ë¤ÏÌµ¾ò·ï¤Ëmark¤òÀèÆ¬¤ËÌá¤¹ */
  yc->cmark = yc->pmark = 0;

  abandonContext(d, yc);
  ret = 0;

 return_ret:
#ifdef WIN
  free(xxx);
#endif

  return ret;
}

extern void restoreChikujiIfBaseChikuji (yomiContext);
extern void ReCheckStartp (yomiContext);
extern void fitmarks (yomiContext);

int YomiBubunKakutei (uiContext);

int
YomiBubunKakutei(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  tanContext tan;
  int len;

  if (yc->id != YOMI_CONTEXT) {
    /* ¤¢¤êÆÀ¤Ê¤¤¤Î¤Ç¤Ï? */
  }
  else /* if (yc->left) */ {
    /* yomiContext ¤Çºï½ü¤¹¤ëÉôÊ¬¤ò¤Þ¤º tanContext ¤ËÀÚ¤ê½Ð¤·¡¢yc ¤Îº¸
       Â¦¤ËÁÞÆþ¤¹¤ë¡£¼¡¤Ë yc ¤Îº¸¤ò¤Ð¤Ã¤µ¤ê¤È³ÎÄê¤¹¤ë¡£
       Á´¤Æ¤³¤Î¥í¥¸¥Ã¥¯¤Ç¤ä¤í¤¦¤«¤·¤é¡£
       */
    tan = newTanContext(yc->majorMode, CANNA_MODE_TankouhoMode);
    if (tan) {
      copyYomiinfo2Tan(yc, tan);
      /* ¤«¤Ê¤ò¥³¥Ô¡¼¤¹¤ë */
      tan->kanji = DUpwstr(yc->kana_buffer, yc->kCurs);
      if (tan->kanji) {
	/* ¤³¤³¤âÆ±¤¸¤«¤Ê¤ò¥³¥Ô¡¼¤¹¤ë */
	tan->yomi = DUpwstr(yc->kana_buffer, yc->kCurs);
	if (tan->yomi) {
	  tan->kAttr = DUpattr(yc->kAttr, yc->kCurs);
	  if (tan->kAttr) {
	    tan->roma = DUpwstr(yc->romaji_buffer, yc->rCurs);
	    if (tan->roma) {
	      tan->rAttr = DUpattr(yc->rAttr, yc->rCurs);
	      if (tan->rAttr) {
		WCHAR_T *sb = d->buffer_return, *eb = sb + d->n_buffer;

		tan->left = yc->left;
		tan->right = (tanContext)yc;
		if (yc->left) {
		  yc->left->right = tan;
		}
		yc->left = tan;
		while (tan->left) {
		  tan = tan->left;
		}

		trimYomi(d, yc->kCurs, yc->kEndp, yc->rCurs, yc->rEndp);

		len = doKakutei(d, tan, (tanContext)yc, sb, eb,
				(yomiContext *)0);
		d->modec = (mode_context)yc;
		yc->left = (tanContext)0;
		goto done;
	      }
	      free(tan->roma);
	    }
	    free(tan->kAttr);
	  }
	  free(tan->yomi);
	}
	free(tan->kanji);
      }
      free(tan); /* not freeTanContext(tan); */
    }
  }
#if 0
  /* ËÜÍè¤³¤³¤Î½èÍý¤ò¤¤¤ì¤¿Êý¤¬¸úÎ¨¤¬ÎÉ¤¤¤È»×¤ï¤ì¤ë¤¬¡¢ÆÉ¤ß¤Î°ìÉô¤ò³Î
  Äê¤µ¤»¤Æ¡¢¤·¤«¤â¥í¡¼¥Þ»ú¾ðÊó¤Ê¤É¤â¤¤¤ì¤ë¤Î¤ÏÌÌÅÝ¤Ê¤Î¤Ç¤¢¤È¤Þ¤ï¤·¤È¤¹¤ë */
  else {
    
    /* ³ÎÄê¤µ¤»¤ë¡£
       ¼¡¤Ë trim ¤¹¤ë*/
  }
#endif

 done:
  if (!yc->kEndp) {
    if (yc->savedFlags & CANNA_YOMI_MODE_SAVED) {
      restoreFlags(yc);
    }
    if (yc->right) {
      removeCurrentBunsetsu(d, (tanContext)yc);
      yc = (yomiContext)0;
    }
    else {
      /* Ì¤³ÎÄêÊ¸»úÎó¤¬Á´¤¯¤Ê¤¯¤Ê¤Ã¤¿¤Î¤Ê¤é¡¢¦Õ¥â¡¼¥É¤ËÁ«°Ü¤¹¤ë */
      restoreChikujiIfBaseChikuji(yc);
      d->current_mode = yc->curMode = yc->myEmptyMode;
      d->kanji_status_return->info |= KanjiEmptyInfo;
    }
    currentModeInfo(d);
  }
  else {
    if (yc->kCurs != yc->kRStartp) {
      ReCheckStartp(yc);
    }
  }

  if (yc) {
    fitmarks(yc);
  }

  makeYomiReturnStruct(d);
  return len;
}

yomiContext
newFilledYomiContext(mode_context next, KanjiMode prev)
{
  yomiContext yc;

  yc = newYomiContext((WCHAR_T *)NULL, 0, /* ·ë²Ì¤Ï³ÊÇ¼¤·¤Ê¤¤ */
		      CANNA_NOTHING_RESTRICTED,
		      (int)!CANNA_YOMI_CHGMODE_INHIBITTED,
		      (int)!CANNA_YOMI_END_IF_KAKUTEI,
		      CANNA_YOMI_INHIBIT_NONE);
  if (yc) {
    yc->majorMode = yc->minorMode = CANNA_MODE_HenkanMode;
    yc->curMode = &yomi_mode;
    yc->myEmptyMode = &empty_mode;
    yc->romdic = romajidic;
    yc->next = next;
    yc->prevMode = prev;
  }
  return yc;
}

#ifdef DO_MERGE
static
yomiContext
mergeYomiContext(yomiContext yc)
{
  yomiContext res, a, b;

  res = yc;
  while (res->left && res->left->id == YOMI_CONTEXT) {
    res = (yomiContext)res->left;
  }
  for (a = (yomiContext)res->right ; a && a->id == YOMI_CONTEXT ; a = b) {
    b = (yomiContext)a->right;
    appendYomi2Yomi(a, res);
    if (yc == a) {
      res->kCurs = res->kRStartp = res->kEndp;
      res->rCurs = res->rStartp = res->rEndp;
      res->cmark = res->kCurs;
    }
    res->right = a->right;
    if (res->right) {
      res->right->left = (tanContext)res;
    }
    /* yc->context ¤Î close ¤Ï¤¤¤é¤Ê¤¤¤Î¤«¤Ê¤¢¡£1996.10.30 º£ */
    freeYomiContext(a);
  }
  return res;
}
#endif

/*
  tanContext ¤ò yomiContext ¤Ë¤·¤Æ¡¢ÆÉ¤ßÆþÎÏ¾õÂÖ¤Ë¤¹¤ë

   0          ¼ºÇÔ
   otherwise  ¤¢¤¿¤é¤·¤¤ÆÉ¤ß¥³¥ó¥Æ¥­¥¹¥È¤¬ÊÖ¤ë

 */

static yomiContext
tanbunUnconvert(uiContext d, tanContext tan)
{
  yomiContext yc;

  yc = newFilledYomiContext(tan->next, tan->prevMode);
  if (yc) {
    extern KanjiModeRec yomi_mode, empty_mode;

    appendTan2Yomi(tan, yc);
    copyTaninfo2Yomi(tan, yc);
    yc->right = tan->right;
    yc->left = tan->left;
    if (yc->myMinorMode) {
      yc->minorMode = yc->myMinorMode;
    }

    if (chikujip(yc)) { /* Ãà¼¡¤Ë¤Ï¤·¤Ê¤¤ */
      yc->generalFlags &= ~CANNA_YOMI_CHIKUJI_MODE;
      yc->generalFlags |= CANNA_YOMI_BASE_CHIKUJI;
    }

    if (yc->left) {
      yc->left->right = (tanContext)yc;
    }
    if (yc->right) {
      yc->right->left = (tanContext)yc;
    }
    freeTanContext(tan);
#ifdef DO_MERGE /* ÄêµÁ¤·¤Æ¤¤¤Ê¤¤ */
    yc = mergeYomiContext(yc);
#endif
    d->current_mode = yc->curMode;
    d->modec = (mode_context)yc;
    return yc;
  }
  jrKanjiError = "\245\341\245\342\245\352\244\254\302\255\244\352\244\336"
	"\244\273\244\363";
                 /* ¥á¥â¥ê¤¬Â­¤ê¤Þ¤»¤ó */
  return (yomiContext)0;
}

static int
TbBubunMuhenkan(uiContext d)
{
  tanContext tan = (tanContext)d->modec;
  yomiContext yc;

  yc = tanbunUnconvert(d, tan);
  if (yc) {
    currentModeInfo(d);
    makeKanjiStatusReturn(d, yc);
    return 0;
  }
  makeGLineMessageFromString(d, jrKanjiError);
  return NothingChangedWithBeep(d);
}

/*
  TanBubunMuhenkan -- ÊÑ´¹Ãæ¤ÎÊ¸»úÎó¤òÊ¸ÀáËè¤ËÊ¬³ä¤¹¤ë¡£

    ¤½¤ÎºÝ¡¢ÆÉ¤ß¤ä¥í¡¼¥Þ»ú¤âÊ¬³ä¤¹¤ë
 */

int
TanBubunMuhenkan(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT) {
    return TbBubunMuhenkan(d);
  }

  if (!yc->right && !yc->left && yc->nbunsetsu == 1) {
    return TanMuhenkan(d);
  }

  if (doTanBubunMuhenkan(d, yc) < 0) {
    makeGLineMessageFromString(d, jrKanjiError);
    return TanMuhenkan(d);
  }
  makeYomiReturnStruct(d);
  currentModeInfo(d);
  return 0;
}

int
prepareHenkanMode(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (confirmContext(d, yc) < 0) {
    return 0;
  }
  d->current_mode = yc->curMode = &tankouho_mode;

  return 1;
}

int doHenkan(uiContext d, int len, WCHAR_T *kanji)
{
  /* ¤è¤ß¤ò´Á»ú¤ËÊÑ´¹¤¹¤ë */
  if(doYomiHenkan(d, len, kanji) == NG) {
    return -1;
  }

  /* kanji_status_return¤òºî¤ë */
  makeKanjiStatusReturn(d, (yomiContext)d->modec);
  return 0;
}


/*
 * ¤«¤Ê´Á»úÊÑ´¹¤ò¹Ô¤¦
 * ¡¦d->yomi_buffer¤Ë¤è¤ß¤ò¼è¤ê½Ð¤·¡¢RkwBgnBun¤ò¸Æ¤ó¤Ç¤«¤Ê´Á»úÊÑ´¹¤ò³«»Ï¤¹¤ë
 * ¡¦¥«¥ì¥ó¥ÈÊ¸Àá¤òÀèÆ¬Ê¸Àá¤Ë¤·¤Æ¡¢¥¨¥³¡¼Ê¸»úÎó¤òºî¤ë
 *
 * °ú¤­¿ô	uiContext
 *		len       len ¤¬»ØÄê¤µ¤ì¤Æ¤¤¤¿¤éÊ¸ÀáÄ¹¤ò¤½¤ÎÄ¹¤µ¤Ë¤¹¤ë¡£
 *		kanji	  kanji ¤¬»ØÄê¤µ¤ì¤Æ¤¤¤¿¤éÃ±Ê¸ÀáÊÑ´¹¤·¤Æ¡¢
 *			  ¥«¥ì¥ó¥È¸õÊä¤ò kanji ¤Ç¼¨¤µ¤ì¤¿¸õÊä¤Ë¹ç¤ï¤»¤ë¡£
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
static int
doYomiHenkan(uiContext d, int len, WCHAR_T *kanji)
{
  unsigned int mode;
  yomiContext yc = (yomiContext)d->modec;
  extern int defaultContext;

#if defined(DEBUG) && !defined(WIN)
  if (iroha_debug) {
/*    printf("yomi     => "); Wprintf(hc->yomi_buffer); putchar('\n');*/
    printf("yomi len => %d\n", hc->yomilen);
  }
#endif

  /* Ï¢Ê¸ÀáÊÑ´¹¤ò³«»Ï¤¹¤ë *//* ¼­½ñ¤Ë¤Ê¤¤ ¥«¥¿¥«¥Ê ¤Ò¤é¤¬¤Ê ¤òÉÕ²Ã¤¹¤ë */
  mode = 0;
  mode = (RK_XFER<<RK_XFERBITS) | RK_KFER;
  if (kanji) {
    mode |= RK_HENKANMODE(RK_TANBUN |
			  RK_MAKE_WORD |
			  RK_MAKE_EISUUJI |
			  RK_MAKE_KANSUUJI) << (2 * RK_XFERBITS);
  }
  
  if (confirmContext(d, yc) < 0) {
    return NG;
  }

#ifdef MEASURE_TIME
  {
    struct tms timebuf;
    long RkTime, times();

    RkTime = times(&timebuf);
#endif /* MEASURE_TIME */

    if ((yc->nbunsetsu =
	 RkwBgnBun(yc->context, yc->kana_buffer, yc->kEndp, mode)) == -1) {
      yc->nbunsetsu = 0;
      return kanakanError(d);
    }
    
    if (len > 0 && (yc->nbunsetsu = RkwResize(yc->context, len)) == -1) {
      RkwEndBun(yc->context, 0);
      yc->nbunsetsu = 0;
      return kanakanError(d);
    }

    if (kanji) {
      /* kanji ¤¬»ØÄê¤µ¤ì¤Æ¤¤¤¿¤é¡¢Æ±¤¸¸õÊä¤¬¤Ç¤ë¤Þ¤Ç RkwNext ¤ò¤¹¤ë */
      int i, n;

      n = RkwGetKanjiList(yc->context, d->genbuf, ROMEBUFSIZE);
      if (n < 0) {
	return kanakanError(d);
      }
      for (i = 0 ; i < n ; i++) {
	RkwXfer(yc->context, i);
	len = RkwGetKanji(yc->context, d->genbuf, ROMEBUFSIZE);
	if (len < 0) {
	  return kanakanError(d);
	}
	d->genbuf[len] = (WCHAR_T)'\0';
	if (!WStrcmp(kanji, d->genbuf)) {
	  break;
	}
      }
      if (i == n) {
	RkwXfer(yc->context, 0);
      }
    }

#ifdef MEASURE_TIME
    yc->rktime = times(&timebuf);
    yc->rktime -= RkTime;
  }
#endif /* MEASURE_TIME */

  /* ¥«¥ì¥ó¥ÈÊ¸Àá¤ÏÀèÆ¬Ê¸Àá */
  yc->curbun = 0;

  return(0);
}

int
TanNop(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  /* currentModeInfo ¤Ç¥â¡¼¥É¾ðÊó¤¬É¬¤ºÊÖ¤ë¤è¤¦¤Ë¥À¥ß¡¼¤Î¥â¡¼¥É¤òÆþ¤ì¤Æ¤ª¤¯ */
  d->majorMode = d->minorMode = CANNA_MODE_AlphaMode;
  currentModeInfo(d);

  makeKanjiStatusReturn(d, yc);
  return 0;
}

static int
doGoTo(uiContext d, yomiContext yc)
{
  if (RkwGoTo(yc->context, yc->curbun) == -1) {
    return makeRkError(d, "\312\270\300\341\244\316\260\334\306\260\244\313"
	"\274\272\307\324\244\267\244\336\244\267\244\277");
                          /* Ê¸Àá¤Î°ÜÆ°¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
  }
  yc->status |= CHIKUJI_OVERWRAP;

  /* kanji_status_return¤òºî¤ë */
  makeKanjiStatusReturn(d, yc);
  return 0;
}

/*
 * ¼¡Ê¸Àá¤Ë°ÜÆ°¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */

int
TanForwardBunsetsu(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT) {
    return TbForward(d);
  }

  yc->kouhoCount = 0;
  if (yc->curbun + 1 < yc->nbunsetsu) {
    yc->curbun++;
  }
  else if (yc->cStartp && yc->cStartp < yc->kEndp) { /* Ãà¼¡¤ÎÆÉ¤ß¤¬±¦¤Ë¤¢¤ë */
    yc->kRStartp = yc->kCurs = yc->cStartp;
    yc->rStartp = yc->rCurs = yc->cRStartp;
    moveToChikujiYomiMode(d);
  }
  else if (yc->right) {
    return TbForward(d);
  }
  else if (cannaconf.kakuteiIfEndOfBunsetsu) {
    d->nbytes = TanKakutei(d);
    d->kanji_status_return->length 
     = d->kanji_status_return->revPos
       = d->kanji_status_return->revLen = 0;
    return d->nbytes;
  }
  else if (!cannaconf.CursorWrap) {
    return NothingForGLine(d);
  }
  else if (yc->left) {
    return TbBeginningOfLine(d);
  }
  else {
    yc->curbun = 0;
  }

  /* ¥«¥ì¥ó¥ÈÊ¸Àá¤ò£±¤Ä±¦¤Ë°Ü¤¹ */
  /* ¥«¥ì¥ó¥ÈÊ¸Àá¤¬ºÇ±¦¤À¤Ã¤¿¤é¡¢
     ºÇº¸¤ò¥«¥ì¥ó¥ÈÊ¸Àá¤Ë¤¹¤ë   */
  return doGoTo(d, yc);
}

/*
 * Á°Ê¸Àá¤Ë°ÜÆ°¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
int
TanBackwardBunsetsu(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT) {
    return TbBackward(d);
  }

  yc->kouhoCount = 0;
  if (yc->curbun) {
    yc->curbun--;
  }
  else if (yc->left) {
    return TbBackward(d);
  }
  else if (!cannaconf.CursorWrap) {
    return NothingForGLine(d);
  }
  else if (yc->right) {
    return TbEndOfLine(d);
  }
  else if (yc->cStartp && yc->cStartp < yc->kEndp) { /* Ãà¼¡¤ÎÆÉ¤ß¤¬±¦¤Ë¤¢¤ë */
    yc->kCurs = yc->kRStartp = yc->kEndp;
    yc->rCurs = yc->rStartp = yc->rEndp;
    moveToChikujiYomiMode(d);
  }
  else {
    yc->curbun = yc->nbunsetsu - 1;
  }

  return doGoTo(d, yc);
}

/*
 * ¼¡¸õÊä¤ò¥«¥ì¥ó¥È¸õÊä¤Ë¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */

static int
tanNextKouho(uiContext d, yomiContext yc)
{
#ifdef MEASURE_TIME
  struct tms timebuf;
  long time, times(;

  time = times(&timebuf;
#endif /* MEASURE_TIME */

  /* ¼¡¤Î¸õÊä¤ò¥«¥ì¥ó¥È¸õÊä¤È¤¹¤ë */
  if (RkwNext(yc->context) == -1) {
    makeRkError(d, "\245\253\245\354\245\363\245\310\270\365\312\344\244\362"
	"\274\350\244\352\275\320\244\273\244\336\244\273\244\363\244\307"
	"\244\267\244\277");
                   /* ¥«¥ì¥ó¥È¸õÊä¤ò¼è¤ê½Ð¤»¤Þ¤»¤ó¤Ç¤·¤¿ */
    return TanMuhenkan(d);
  }

#ifdef MEASURE_TIME
  yc->rktime = times(&timebuf);
  yc->rktime -= proctime;
#endif /* MEASURE_TIME */

  /* kanji_status_return¤òºî¤ë */
  makeKanjiStatusReturn(d, yc);

#ifdef MEASURE_TIME
  yc->time = times(&timebuf;
  yc->proctime -= proctime;
#endif /* MEASURE_TIME */

  return(0);
}

/*
  tanbunHenkan -- ÊÑ´¹¤¹¤ë¡£

    ¤ß¤½¤Ï¡¢kanji ¤Ç¼¨¤µ¤ì¤¿¸õÊä¤ÈÆ±¤¸¸õÊä¤¬½Ð¤ë¤Þ¤Ç RkwNext ¤ò¤¹¤ë¤³¤È
    ¤Ç¤¢¤ë¡£°ì¼þ¤·¤¿¤³¤È¤â¸¡½Ð¤·¤Ê¤±¤ì¤Ð¤Ê¤ë¤Þ¤¤¡£
 */

static int
tanbunHenkan(uiContext d, yomiContext yc, WCHAR_T *kanji)
{
  if (!prepareHenkanMode(d)) {
    makeGLineMessageFromString(d, jrKanjiError);
    makeYomiReturnStruct(d);
    return 0;
  }
  yc->minorMode = CANNA_MODE_TankouhoMode;
  yc->kouhoCount = 1;
  if (doHenkan(d, 0, kanji) < 0) {
    makeGLineMessageFromString(d, jrKanjiError);
    makeYomiReturnStruct(d);
    return 0;
  }
  if (cannaconf.kouho_threshold > 0 &&
      yc->kouhoCount >= cannaconf.kouho_threshold) {
    return tanKouhoIchiran(d, 0);
  }
  
  currentModeInfo(d);
  makeKanjiStatusReturn(d, yc);
  return 0;
}

/*
  enterTanHenkanMode -- tanContext ¤ò yomiContext ¤Ë¤·¤ÆÊÑ´¹¤Î½àÈ÷¤ò¤¹¤ë

 */

static int
enterTanHenkanMode(uiContext d, int fnum)
{
  tanContext tan = (tanContext)d->modec;
  yomiContext yc;
  WCHAR_T *prevkanji;

  prevkanji = tan->kanji;
  tan->kanji = (WCHAR_T *)0;

  yc = tanbunUnconvert(d, tan);
  if (yc) {
    tanbunHenkan(d, yc, prevkanji);
    free(prevkanji);

    /*¤³¤³¤Ç
      Ã±¸õÊä¥â¡¼¥É¤Î·Á¤Ë¤¹¤ë
      */

    d->more.todo = 1;
    d->more.ch = d->ch;
    d->more.fnum = fnum;
    return 0;
  }
  else { /* Æó½Å¥Õ¥ê¡¼¤ò¤·¤Ê¤¤¤¿¤á¶¯Ä´Åª¤Ë else ¤ò½ñ¤¯ */
    free(prevkanji);
  }
  makeGLineMessageFromString(d, jrKanjiError);
  return NothingChangedWithBeep(d);
}

/*
 * ¸õÊä°ìÍ÷¹Ô¤òÉ½¼¨¤¹¤ë
 *
 * ¡¦¸õÊä°ìÍ÷É½¼¨¤Î¤¿¤á¤Î¥Ç¡¼¥¿¤ò¥Æ¡¼¥Ö¥ë¤ËºîÀ®¤¹¤ë
 * ¡¦¸õÊä°ìÍ÷É½¼¨¹Ô¤¬¶¹¤¤¤È¤­¤Ï¡¢°ìÍ÷¤òÉ½¼¨¤·¤Ê¤¤¤Ç¼¡¸õÊä¤ò¤½¤Î¾ì¤ËÉ½¼¨¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */

int TanKouhoIchiran(uiContext d)
{
  if (d->modec->id != YOMI_CONTEXT) {
    return enterTanHenkanMode(d, CANNA_FN_KouhoIchiran);
  }
  return tanKouhoIchiran(d, 1);
}

int TanNextKouho(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT) {
    return enterTanHenkanMode(d, CANNA_FN_Next);
  }
  yc->status |= CHIKUJI_OVERWRAP;
  yc->kouhoCount = 0;
  return tanNextKouho(d, yc);
}

/*

  TanHenkan -- ²ó¿ô¤ò¥Á¥§¥Ã¥¯¤¹¤ë°Ê³°¤Ï TanNextKouho ¤È¤Û¤ÜÆ±¤¸

 */
static int TanHenkan (uiContext);

static int
TanHenkan(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT) {
    return enterTanHenkanMode(d, CANNA_FN_Henkan);
  }

  if (cannaconf.kouho_threshold &&
      ++yc->kouhoCount >= cannaconf.kouho_threshold) {
    return TanKouhoIchiran(d);
  }
  else {
    return tanNextKouho(d, yc);
  }
}

/*
 * Á°¸õÊä¤ò¥«¥ì¥ó¥È¸õÊä¤Ë¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
int TanPreviousKouho(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT) {
    return enterTanHenkanMode(d, CANNA_FN_Prev);
  }

  yc->status |= CHIKUJI_OVERWRAP;
  yc->kouhoCount = 0;
  /* Á°¤Î¸õÊä¤ò¥«¥ì¥ó¥È¸õÊä¤È¤¹¤ë */
  if (RkwPrev(yc->context) == -1) {
    makeRkError(d, "\245\253\245\354\245\363\245\310\270\365\312\344\244\362"
	"\274\350\244\352\275\320\244\273\244\336\244\273\244\363\244\307"
	"\244\267\244\277");
                   /* ¥«¥ì¥ó¥È¸õÊä¤ò¼è¤ê½Ð¤»¤Þ¤»¤ó¤Ç¤·¤¿ */
    return TanMuhenkan(d);
  }

  /* kanji_status_return¤òºî¤ë */
  makeKanjiStatusReturn(d, yc);

  return 0;
}

/*
  tanJishuHenkan -- ÆÃÄê¤ÎÊ¸Àá¤À¤±»ú¼ïÊÑ´¹¤¹¤ë
 */

static int tanJishuHenkan (uiContext, int);

static int
tanJishuHenkan(uiContext d, int fn)
{
  d->nbytes = TanBubunMuhenkan(d);
  d->more.todo = 1;
  d->more.ch = d->ch;
  d->more.fnum = fn;
  return d->nbytes;
}

int TanHiragana(uiContext d)
{
  return tanJishuHenkan(d, CANNA_FN_Hiragana);
}

int TanKatakana(uiContext d)
{
  return tanJishuHenkan(d, CANNA_FN_Katakana);
}

int TanRomaji(uiContext d)
{
  return tanJishuHenkan(d, CANNA_FN_Romaji);
}

int TanUpper(uiContext d)
{
  return tanJishuHenkan(d, CANNA_FN_ToUpper);
}

int TanCapitalize(uiContext d)
{
  return tanJishuHenkan(d, CANNA_FN_Capitalize);
}

int TanZenkaku(uiContext d)
{
  return tanJishuHenkan(d, CANNA_FN_Zenkaku);
}

int TanHankaku(uiContext d)
{
  return tanJishuHenkan(d, CANNA_FN_Hankaku);
}

int TanKanaRotate (uiContext);

int TanKanaRotate(uiContext d)
{
  return tanJishuHenkan(d, CANNA_FN_KanaRotate);
}

int TanRomajiRotate (uiContext);

int TanRomajiRotate(uiContext d)
{
  return tanJishuHenkan(d, CANNA_FN_RomajiRotate);
}

int TanCaseRotateForward (uiContext);

int TanCaseRotateForward(uiContext d)
{
  return tanJishuHenkan(d, CANNA_FN_CaseRotate);
}

static int
gotoBunsetsu(yomiContext yc, int n)
{
  /* ¥«¥ì¥ó¥ÈÊ¸Àá¤ò°ÜÆ°¤¹¤ë */
  if (RkwGoTo(yc->context, n) == -1) {
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
    jrKanjiError = "\312\270\300\341\244\316\260\334\306\260\244\313\274\272"
	"\307\324\244\267\244\336\244\267\244\277";
                   /* Ê¸Àá¤Î°ÜÆ°¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    return NG;
  }
  yc->curbun = n;
  return 0;
}

/*
 * ºÇº¸Ê¸Àá¤Ë°ÜÆ°¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
int
TanBeginningOfBunsetsu(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT || yc->left) {
    return TbBeginningOfLine(d);
  }
  yc->kouhoCount = 0;
  if (gotoBunsetsu(yc, 0) < 0) {
    return NG;
  }
  makeKanjiStatusReturn(d, yc);
  return 0;
}

/*
 * ºÇ±¦Ê¸Àá¤Ë°ÜÆ°¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
int
TanEndOfBunsetsu(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT || yc->right) {
    return TbEndOfLine(d);
  }

  yc->kouhoCount = 0;
  if (yc->cStartp && yc->cStartp < yc->kEndp) {
    yc->kRStartp = yc->kCurs = yc->kEndp;
    yc->rStartp = yc->rCurs = yc->rEndp;
    moveToChikujiYomiMode(d);
  }
  if (gotoBunsetsu(yc, yc->nbunsetsu - 1) < 0) {
    return NG;
  }
  yc->status |= CHIKUJI_OVERWRAP;
  makeKanjiStatusReturn(d, yc);
  return 0;
}

int
tanMuhenkan(uiContext d, int kCurs)
{
  extern KanjiModeRec yomi_mode;
  yomiContext yc = (yomiContext)d->modec;
  int autoconvert = (yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE);

  if (RkwEndBun(yc->context, 0) == -1) {
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
  }

  if (autoconvert) {
    yc->status &= CHIKUJI_NULL_STATUS;
    d->current_mode = yc->curMode = &cy_mode;
    yc->ys = yc->ye = yc->cStartp = yc->cRStartp = 0;
    yc->rCurs = yc->rStartp = yc->rEndp;
    yc->kCurs = yc->kRStartp = yc->kEndp;
    clearHenkanContext(yc);
  }
  else {
    d->current_mode = yc->curMode = &yomi_mode;
  }
  yc->minorMode = getBaseMode(yc);

  if (kCurs >= 0) {
    int rpos;

    kPos2rPos(yc, 0, kCurs, (int *)0, &rpos);
    yc->kCurs = yc->kRStartp = kCurs;
    yc->rCurs = yc->rStartp = rpos;
  }

  /* Á´ÉôÌµÊÑ´¹¤Ë¤¹¤ë */
  yc->nbunsetsu = 0;

  /* Ã±¸õÊä¾õÂÖ¤«¤éÆÉ¤ß¤ËÌá¤ë¤È¤­¤Ë¤ÏÌµ¾ò·ï¤Ëmark¤òÀèÆ¬¤ËÌá¤¹ */
  yc->cmark = yc->pmark = 0;

  abandonContext(d, yc);

  return 0;
}

/*
 * Á´¤Æ¤ÎÊ¸Àá¤òÆÉ¤ß¤ËÌá¤·¡¢YomiInputMode ¤ËÌá¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */

int TanMuhenkan(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec, newyc;
  tanContext tan;

  if (yc->id != YOMI_CONTEXT || yc->left || yc->right) {
    tan = (tanContext)yc;
    while (tan->left) {
      tan = tan->left;
    }
    if (tan->id == YOMI_CONTEXT) {
      newyc = (yomiContext)tan;
    }
    else {
      newyc = newFilledYomiContext(yc->next, yc->prevMode);
      if (newyc) {
	tan->left = (tanContext)newyc;
	newyc->right = tan;
	newyc->generalFlags = tan->generalFlags;
	newyc->savedFlags = tan->savedFlags;
	if (chikujip(newyc)) {
	  newyc->curMode = &cy_mode;
	}
	newyc->minorMode = getBaseMode(newyc);
      }else{
	jrKanjiError = "\245\341\245\342\245\352\244\254\302\255\244\352"
	"\244\336\244\273\244\363";
                       /* ¥á¥â¥ê¤¬Â­¤ê¤Þ¤»¤ó */
	makeGLineMessageFromString(d, jrKanjiError);
	return NothingChangedWithBeep(d);
      }
    }
    d->modec = (mode_context)newyc;
    d->current_mode = newyc->curMode;

    doMuhenkan(d, newyc);

    if (newyc->generalFlags &
	(CANNA_YOMI_CHIKUJI_MODE | CANNA_YOMI_BASE_CHIKUJI)) {
      /* ¡Ö¿´¤ÏÃà¼¡¤À¤Ã¤¿¡×¤Î¤Ç¤¢¤ì¤Ð¡¢Ãà¼¡¥â¡¼¥É¤ËÌá¤¹ */
      newyc->generalFlags |= CANNA_YOMI_CHIKUJI_MODE;
      newyc->generalFlags &= ~CANNA_YOMI_BASE_CHIKUJI;
      newyc->minorMode = getBaseMode(newyc);
      d->current_mode = newyc->curMode = &cy_mode;
    }

    makeYomiReturnStruct(d);
    currentModeInfo(d);
    return 0;
  }

  if (yc->generalFlags & 
      (CANNA_YOMI_CHIKUJI_MODE | CANNA_YOMI_BASE_CHIKUJI)) {
    /* ¡Ö¿´¤ÏÃà¼¡¤À¤Ã¤¿¡×¤Î¤Ç¤¢¤ì¤Ð¡¢Ãà¼¡¥â¡¼¥É¤ËÌá¤¹ */
    yc->generalFlags |= CANNA_YOMI_CHIKUJI_MODE;
    yc->generalFlags &= ~CANNA_YOMI_BASE_CHIKUJI;
    /* ¥Ì¥ë¥¹¥Æ¡¼¥¿¥¹¤ËÌá¤¹ */
    yc->status &= CHIKUJI_NULL_STATUS;
  }

  tanMuhenkan(d, -1);
  makeYomiReturnStruct(d);
  currentModeInfo(d);
  return 0;
}

int
TanDeletePrevious(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  int i, j, l = -1, ret = 0;
#ifndef WIN
  WCHAR_T tmpbuf[ROMEBUFSIZE];
#else
  WCHAR_T *tmpbuf = (WCHAR_T *)malloc(sizeof(WCHAR_T) * ROMEBUFSIZE);
  if (!tmpbuf) {
    return ret;
  }
#endif

  if (yc->id != YOMI_CONTEXT) {
    ret = TanMuhenkan(d);
    goto return_ret;
  }

  if ((yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) &&
      !cannaconf.BackspaceBehavesAsQuit) {
    ret = ChikujiTanDeletePrevious(d);
    goto return_ret;
  }
  else {
    if (cannaconf.keepCursorPosition) {
      for (i = 0, l = 0; i <= yc->curbun; i++) {
	if (RkwGoTo(yc->context, i) == -1
	    || (j = RkwGetYomi(yc->context, tmpbuf, ROMEBUFSIZE)) == -1) {
	  l = -1;
	  break;
	}
	l += j;
      }
    }
    yc->status &= CHIKUJI_NULL_STATUS;
    tanMuhenkan(d, l);
    makeYomiReturnStruct(d);
    currentModeInfo(d);
    ret = 0;
  }
 return_ret:
#ifdef WIN
  free(tmpbuf);
#endif
  return ret;
}

#if 0
/*
  doTanKakutei -- ³ÎÄê¤µ¤»¤ëÆ°ºî¤ò¤¹¤ë

  retval 0 -- ÌäÂêÌµ¤¯³ÎÄê¤·¤¿¡£
         1 -- ³ÎÄê¤·¤¿¤é¤Ê¤¯¤Ê¤Ã¤¿¡£
        -1 -- ¥¨¥é¡¼¡©
 */

static
doTanKakutei(uiContext d, yomiContext yc)
{
  if ((yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) &&
      (yc->cStartp < yc->kEndp)) {
    (void)RomajiFlushYomi(d, (WCHAR_T *)0, 0);
  }

  return 0;
}
#endif /* 0 */

void
finishTanKakutei(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  int autoconvert = yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE;

#ifdef DO_RENGO_LEARNING
#define RENGOBUFSIZE 256

  /* This will not be defined when WIN is defined.  So I don't care
     about the local array located below.  1996.6.5 kon */

  if (RengoGakushu && hc->nbunsetsu > 1) { /* Ï¢¸ì³Ø½¬¤ò¤·¤è¤¦¤«¤Ê¤¡ */
    RkLex  lex[2][RENGOBUFSIZE];
    WCHAR_T yomi[2][RENGOBUFSIZE];
    WCHAR_T kanji[2][RENGOBUFSIZE];
    WCHAR_T word[1024], *w;
    unsigned char xxxx[ROMEBUFSIZE];
    int    nword[2], wlen;

    *(w = word) = (WCHAR_T) '\0';
    wlen = 1024;

    RkwGoTo(hc->context, 0);
    nword[0] = RkwGetLex(hc->context, lex[0], RENGOBUFSIZE);
    yomi[0][0] =
      (WCHAR_T) '\0'; /* yomi[current][0]¤Î¿¿ÍýÃÍ ¢á RkwGetYomi¤·¤¿¤« */

    for (i = 1 ; i < hc->nbunsetsu ; i++) {
      int current, previous, mighter;

      current = i % 2;
      previous = 1 - current;

      nword[current] = 0;
      if ( !nword[previous] ) {
	nword[previous] = RkwGetLex(hc->context, lex[previous], RENGOBUFSIZE);
      }
      RkwRight(hc->context);

      if (nword[previous] == 1) {
	nword[current] = RkwGetLex(hc->context, lex[current], RENGOBUFSIZE);
	yomi[current][0] = (WCHAR_T) '\0';
	if (((lex[previous][0].ylen <= 3 && lex[previous][0].klen == 1) ||
	     (lex[current][0].ylen <= 3 && lex[current][0].klen == 1)) &&
	    (lex[current][0].rownum < R_K5 ||
	     R_NZX < lex[current][0].rownum)) {
	  if ( !yomi[previous][0] ) {
	    RkwLeft(hc->context);
	    RkwGetYomi(hc->context, yomi[previous], RENGOBUFSIZE);
	    RkwGetKanji(hc->context, kanji[previous], RENGOBUFSIZE);
	    RkwRight(hc->context);
	  }
	  RkwGetYomi(hc->context, yomi[current], RENGOBUFSIZE);
	  RkwGetKanji(hc->context, kanji[current], RENGOBUFSIZE);

	  WStrncpy(yomi[previous] + lex[previous][0].ylen,
		   yomi[current], lex[current][0].ylen);
	  yomi[previous][lex[previous][0].ylen + lex[current][0].ylen] =
	    (WCHAR_T) '\0';

	  WStrncpy(kanji[previous] + lex[previous][0].klen,
		   kanji[current], lex[current][0].klen);
	  kanji[previous][lex[previous][0].klen + lex[current][0].klen] =
	    (WCHAR_T) '\0';

#ifdef NAGASADEBUNPOUWOKIMEYOU
	  if (lex[previous][0].klen >= lex[current][0].klen) {
	    /* Á°¤Î´Á»ú¤ÎÄ¹¤µ       >=    ¸å¤í¤Î´Á»ú¤ÎÄ¹¤µ */
	    mighter = previous;
	  }
	  else {
	    mighter = current;
	  }
#else /* !NAGASADEBUNPOUWOKIMEYOU */
	  mighter = current;
#endif /* !NAGASADEBUNPOUWOKIMEYOU */
	  WStrcpy(w, yomi[previous]);
	  printf(xxxx, " #%d#%d ", lex[mighter][0].rownum,
		 lex[mighter][0].colnum);
	  MBstowcs(w + WStrlen(w), xxxx, wlen - WStrlen(w));
	  WStrcat(w, kanji[previous]);
	  wlen -= (WStrlen(w) + 1); w += WStrlen(w) + 1; *w = (WCHAR_T) '\0';
	}
      }
    }
  }
#endif /* DO_RENGO_LEARNING */

  if (RkwEndBun(yc->context, cannaconf.Gakushu ? 1 : 0) == -1) {
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
  }

#ifdef DO_RENGO_LEARNING
  if (RengoGakushu && yc->nbunsetsu > 1) { /* Ï¢¸ì³Ø½¬¤ò¤·¤è¤¦¤«¤Ê¤¡ */
    for (w = word ; *w ; w += WStrlen(w) + 1) {
      RkwDefineDic(yc->context, RengoGakushu, w);
    }
  }
#endif /* DO_RENGO_LEARNING */

  if (autoconvert) {
    yc->status &= CHIKUJI_NULL_STATUS;
    yc->ys = yc->ye = yc->cStartp = yc->cRStartp = 0;
    clearHenkanContext(yc);
    yc->kEndp = yc->rEndp = yc->kCurs = yc->rCurs =
      yc->cStartp = yc->cRStartp = 
	yc->rStartp = yc->kRStartp = 0;
    yc->kAttr[0] = yc->rAttr[0] = SENTOU;
    yc->kana_buffer[0] = yc->romaji_buffer[0] = 0;
/*    d->kanji_status_return->info |= KanjiEmptyInfo; Â¿Ê¬Í×¤é¤Ê¤¤¤Î¤Ç.. */
    d->current_mode = yc->curMode = yc->myEmptyMode;
  }
  yc->minorMode = getBaseMode(yc);
  
  /* Ã±¸õÊä¾õÂÖ¤«¤éÆÉ¤ß¤ËÌá¤ë¤È¤­¤Ë¤ÏÌµ¾ò·ï¤Ëmark¤òÀèÆ¬¤ËÌá¤¹ */
  yc->nbunsetsu = 0;
  yc->cmark = yc->pmark = 0;

  abandonContext(d, yc);

  if (yc->savedFlags & CANNA_YOMI_MODE_SAVED) {
    restoreFlags(yc);
  }
}

int TanKakutei(uiContext d)
{
  return YomiKakutei(d);
}

/*
 * ´Á»ú¸õÊä¤ò³ÎÄê¤µ¤»¡¢¥í¡¼¥Þ»ú¤ò¥¤¥ó¥µ¡¼¥È¤¹¤ë
 *
 * renbun-continue ¤¬ t ¤Î¤È¤­¤Ï¡¢¼ÂºÝ¤Ë¤Ï³ÎÄê¤·¤Ê¤¤¤Î¤Ç½èÍý¤¬
 * ÌÌÅÝ¤À¤Ã¤¿¤ê¤¹¤ë¡£
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */

static int TanKakuteiYomiInsert (uiContext);

static int
TanKakuteiYomiInsert(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  tanContext tan;

  if (cannaconf.RenbunContinue || cannaconf.ChikujiContinue) {
    d->nbytes = 0;
    for (tan = (tanContext)yc ; tan->right ; tan = tan->right)
      /* bodyless 'for' */;
    yc = (yomiContext)0; /* Ç°¤Î¤¿¤á */
    d->modec = (mode_context)tan;
    setMode(d, tan, 1);

    if (tan->id == YOMI_CONTEXT) {
      yc = (yomiContext)tan;

      if (yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) {
	/* Ãà¼¡¤Ê¤éÉáÄÌ¤ËÂ³¤±¤ë¤À¤±¤À¤«¤é¤Ê¤¢ */
	yc->minorMode = CANNA_MODE_ChikujiTanMode;
	d->current_mode = yc->curMode = &cb_mode;
	currentModeInfo(d);
	yc->status &= ~CHIKUJI_OVERWRAP;
	if (yc->kCurs != yc->kEndp) {
	  yc->rStartp = yc->rCurs = yc->rEndp;
	  yc->kRStartp = yc->kCurs = yc->kEndp;
	}
	yc->ys = yc->ye = yc->cStartp;
	return YomiInsert(d);
      }else{ /* Ãà¼¡¤¸¤ã¤Ê¤¤¾ì¹ç */
	extern int nKouhoBunsetsu;
    
	yc->curbun = yc->nbunsetsu;
	if (doTanBubunMuhenkan(d, yc) < 0) {
	  makeGLineMessageFromString(d, jrKanjiError);
	  return NothingChangedWithBeep(d);
	}
	if (nKouhoBunsetsu) {
	  (void)cutOffLeftSide(d, yc, nKouhoBunsetsu);
	}
      }
    }
    else {
      yc = newFilledYomiContext(tan->next, tan->prevMode);
      /* ¤¢¤êÆÀ¤Ê¤¤ if (tan->right) yc->right = tan->right;
	 yc->right->left = yc; */
      tan->right = (tanContext)yc;
      yc->left = tan;
      d->modec = (mode_context)yc;
      /* d->current_mode = yc->curMode = yc->myEmptyMode; */
    }
  }
  else {
    d->nbytes = YomiKakutei(d);
  }
  /* YomiKakutei(d) ¤Ç d->modec ¤¬ÊÑ¹¹¤µ¤ì¤¿²ÄÇ½À­¤¬¤¢¤ë¤Î¤ÇºÆÆÉ¤ß¹þ¤ß¤¹¤ë */
  yc = (yomiContext)d->modec;

  if (yc->id == YOMI_CONTEXT) {
    yc->minorMode = getBaseMode(yc);
  }
  currentModeInfo(d);
  d->more.todo = 1;
  d->more.ch = d->ch;
  d->more.fnum = 0;    /* ¾å¤Î ch ¤Ç¼¨¤µ¤ì¤ë½èÍý¤ò¤»¤è */
  return d->nbytes;
}


/* cfuncdef

  pos ¤Ç»ØÄê¤µ¤ì¤¿Ê¸Àá¤ª¤è¤Ó¤½¤ì¤è¤ê¸å¤ÎÊ¸Àá¤Î»ú¼ïÊÑ´¹¾ðÊó¤ò
  ¥¯¥ê¥¢¤¹¤ë¡£
*/

static int
doTbResize(uiContext d, yomiContext yc, int n)
{
  int len;

  if (doTanBubunMuhenkan(d, yc) < 0) {
    makeGLineMessageFromString(d, jrKanjiError);
    return NothingChangedWithBeep(d);
  }
  len = yc->kEndp;
  doMuhenkan(d, yc); /* yc ¤«¤é±¦¤ò¤ß¤ó¤ÊÌµÊÑ´¹¤Ë¤·¤Æ yc ¤Ë¤Ä¤Ê¤²¤ë */
  if (!prepareHenkanMode(d)) {
    makeGLineMessageFromString(d, jrKanjiError);
    makeYomiReturnStruct(d);
    currentModeInfo(d);
    return 0;
  }
  yc->minorMode = CANNA_MODE_TankouhoMode;
  yc->kouhoCount = 0;
  if (doHenkan(d, len + n, (WCHAR_T *)0) < 0) {
    makeGLineMessageFromString(d, jrKanjiError);
    makeYomiReturnStruct(d);
    currentModeInfo(d);
    return 0;
  }
  currentModeInfo(d);
  makeKanjiStatusReturn(d, yc);
  return 0;
}

/*
 * Ê¸Àá¤ò¿­¤Ð¤¹
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
static int TanExtendBunsetsu (uiContext);

static int
TanExtendBunsetsu(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT) {
    return enterTanHenkanMode(d, CANNA_FN_Extend);
  }

  d->nbytes = 0;
  yc->kouhoCount = 0;
  if (yc->right) {
    return doTbResize(d, yc, 1);
  }
  if ((yc->nbunsetsu = RkwEnlarge(yc->context)) <= 0) {
    makeRkError(d, "\312\270\300\341\244\316\263\310\302\347\244\313\274\272"
	"\307\324\244\267\244\336\244\267\244\277");
                   /* Ê¸Àá¤Î³ÈÂç¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    return TanMuhenkan(d);
  }
  makeKanjiStatusReturn(d, yc);
  return(d->nbytes);
}

/*
 * Ê¸Àá¤ò½Ì¤á¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
static int TanShrinkBunsetsu (uiContext);

static int
TanShrinkBunsetsu(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT) {
    return enterTanHenkanMode(d, CANNA_FN_Shrink);
  }

  d->nbytes = 0;
  yc->kouhoCount = 0;

  if (yc->right) {
    return doTbResize(d, yc, -1);
  }

  /* Ê¸Àá¤ò½Ì¤á¤ë */
  if ((yc->nbunsetsu = RkwShorten(yc->context)) <= 0) {
    makeRkError(d, "\312\270\300\341\244\316\275\314\276\256\244\313\274\272"
	"\307\324\244\267\244\336\244\267\244\277");
                   /* Ê¸Àá¤Î½Ì¾®¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    return TanMuhenkan(d);
  }
  makeKanjiStatusReturn(d, yc);
  
  return(d->nbytes);
}

#define BUNPOU_DISPLAY

#ifdef BUNPOU_DISPLAY
/*
 * Ê¸Ë¡¾ðÊó¤ò¥×¥ê¥ó¥È¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
int TanPrintBunpou(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  static WCHAR_T mesg[512]; /* static! */

  if (yc->id != YOMI_CONTEXT) {
    return enterTanHenkanMode(d, CANNA_FN_ConvertAsHex);
  }

#ifdef notdef
#ifdef DO_GETYOMI
  if (RkwGetYomi(yc->context, buf, 256) == -1) {
    if (errno == EPIPE) {
      jrKanjiPipeError();
      TanMuhenkan(d);
    }
    fprintf(stderr, "¥«¥ì¥ó¥È¸õÊä¤ÎÆÉ¤ß¤ò¼è¤ê½Ð¤»¤Þ¤»¤ó¤Ç¤·¤¿¡£\n");
  }
  Wfprintf(stderr, "%s\n", buf);
#endif /* DO_GETYOMI */

  if(RkwGetKanji(yc->context, buf, 256) == -1) {
    if(errno == EPIPE) {
      jrKanjiPipeError();
    }
    jrKanjiError = "\245\253\245\354\245\363\245\310\270\365\312\344\244\362"
	"\274\350\244\352\275\320\244\273\244\336\244\273\244\363\244\307"
	"\244\267\244\277";
                   /* ¥«¥ì¥ó¥È¸õÊä¤ò¼è¤ê½Ð¤»¤Þ¤»¤ó¤Ç¤·¤¿ */
    return NG;
  }
#endif

  if (RkwGetHinshi(yc->context, mesg, sizeof(mesg) / sizeof(WCHAR_T)) < 0) {
    jrKanjiError = "\311\312\273\354\276\360\312\363\244\362\274\350\244\352"
	"\275\320\244\273\244\336\244\273\244\363\244\307\244\267\244\277";
                   /* ÉÊ»ì¾ðÊó¤ò¼è¤ê½Ð¤»¤Þ¤»¤ó¤Ç¤·¤¿ */
    makeGLineMessageFromString(d, jrKanjiError);
    makeKanjiStatusReturn(d, yc);
    return 0;
  }

  makeKanjiStatusReturn(d, yc);
  d->kanji_status_return->info |= KanjiGLineInfo;
  d->kanji_status_return->gline.line = mesg;
  d->kanji_status_return->gline.length = WStrlen(mesg);
  d->kanji_status_return->gline.revPos = 0;
  d->kanji_status_return->gline.revLen = 0;
  d->flags |= PLEASE_CLEAR_GLINE;
  return 0;
}
#endif /* BUNPOU_DISPLAY */

#ifdef MEASURE_TIME
static
TanPrintTime(uiContext d)
{
  /* MEASURE_TIME will not be defined when WIN is defined.  So I will not
     care about arrays located on stack below.  1996.6.5 kon */
  unsgined char tmpbuf[1024];
  static WCHAR_T buf[256];
  yomiContext yc = (yomiContext)d->modec;

  ycc->kouhoCount = 0;
  sprintf(tmpbuf, "\312\321\264\271\273\376\264\326 %d [ms]¡¢\244\246\244\301"
	" UI \311\364\244\317 %d [ms]",
	   (yc->time * 50 / 3,
	   (yc->time - yc->rktime) * 50 / 3;
               /* ÊÑ´¹»þ´Ö %d [ms]¡¢¤¦¤Á UI Éô¤Ï %d [ms] */
  MBstowcs(buf, tmpbuf, 1024);
  d->kanji_status_return->info |= KanjiGLineInfo;
  d->kanji_status_return->gline.line = buf;
  d->kanji_status_return->gline.length = WStrlen(buf);
  d->kanji_status_return->gline.revPos = 0;
  d->kanji_status_return->gline.revLen = 0;
  d->kanji_status_return->length = -1;
  d->flags |= PLEASE_CLEAR_GLINE;
  return 0;
}
#endif /* MEASURE_TIME */

void
jrKanjiPipeError(void)
{
  extern int defaultContext, defaultBushuContext;

  defaultContext = -1;
  defaultBushuContext = -1;

  makeAllContextToBeClosed(0);

  RkwFinalize();
#if defined(DEBUG) && !defined(WIN)
  if (iroha_debug) {
    fprintf(stderr, "\300\334\302\263\244\254\300\332\244\354\244\277\n");
                    /* ÀÜÂ³¤¬ÀÚ¤ì¤¿ */
  }
#endif
}

/* cfuncdef

  TanBunsetsuMode -- Ã±¸õÊä¥â¡¼¥É¤«¤éÊ¸Àá¿­¤Ð¤·½Ì¤á¥â¡¼¥É¤Ø°Ü¹Ô¤¹¤ë

 */

static int TanBunsetsuMode (uiContext);

static int
TanBunsetsuMode(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT) {
    return enterTanHenkanMode(d, CANNA_FN_AdjustBunsetsu);
  }
  if (yc->right) {
    doTbResize(d, yc, 0);
    yc = (yomiContext)d->modec;
  }
  if (enterAdjustMode(d, yc) < 0) {
    return TanMuhenkan(d);
  }
  makeKanjiStatusReturn(d, yc);
  currentModeInfo(d);
  return 0;
}

static void
chikujiSetCursor(uiContext d, int forw)
{
  yomiContext yc = (yomiContext)d->modec;

  if (forw) { /* °ìÈÖº¸¤Ø¹Ô¤¯ */
    if (yc->nbunsetsu) { /* Ê¸Àá¤¬¤¢¤ë¡© */
      gotoBunsetsu(yc, 0);
      moveToChikujiTanMode(d);
    }
    else {
      yc->kRStartp = yc->kCurs = yc->cStartp;
      yc->rStartp = yc->rCurs = yc->cRStartp;
      moveToChikujiYomiMode(d);
    }
  }
  else { /* °ìÈÖ±¦¤Ø¹Ô¤¯ */
    if (yc->cStartp < yc->kEndp) { /* ÆÉ¤ß¤¬¤¢¤ë¡© */
      yc->kRStartp = yc->kCurs = yc->kEndp;
      yc->rStartp = yc->rCurs = yc->rEndp;
      moveToChikujiYomiMode(d);
    }
    else {
      gotoBunsetsu(yc, yc->nbunsetsu - 1);
      moveToChikujiTanMode(d);
    }
  }
}


void
setMode(uiContext d, tanContext tan, int forw)
{
  yomiContext yc = (yomiContext)tan;

  d->current_mode = yc->curMode;
  currentModeInfo(d);
  if (tan->id == YOMI_CONTEXT) {
    if (yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) {
      chikujiSetCursor(d, forw);
    }
    else if (yc->nbunsetsu) {
      if (forw) {
	gotoBunsetsu(yc, 0);
      }else{
	gotoBunsetsu(yc, yc->nbunsetsu - 1);
      }
    }
    else /* ÆÉ¤ß¥â¡¼¥É */ if (forw) {
      yc->kCurs = yc->kRStartp = yc->cStartp;
      yc->rCurs = yc->rStartp = yc->cRStartp;
    }
    else {
      yc->kCurs = yc->kRStartp = yc->kEndp;
      yc->rCurs = yc->rStartp = yc->rEndp;
    }
  }
}

int
TbForward(uiContext d)
{
  tanContext tan = (tanContext)d->modec;

  if (tan->right) {
    d->modec = (mode_context)tan->right;
    tan = (tanContext)d->modec;
  }
  else if (cannaconf.CursorWrap && tan->left) {
    while (tan->left) {
      tan = tan->left;
    }
    d->modec = (mode_context)tan;
  }
  else {
    return NothingChanged(d);
  }
  setMode(d, tan, 1);
  makeKanjiStatusReturn(d, (yomiContext)d->modec);
  return 0;
}

int
TbBackward(uiContext d)
{
  tanContext tan = (tanContext)d->modec;

  if (tan->left) {
    d->modec = (mode_context)tan->left;
    tan = (tanContext)d->modec;
  }
  else if (cannaconf.CursorWrap && tan->right) {
    while (tan->right) {
      tan = tan->right;
    }
    d->modec = (mode_context)tan;
  }
  else {
    return NothingChanged(d);
  }
  setMode(d, tan, 0);
  makeKanjiStatusReturn(d, (yomiContext)d->modec);
  return 0;
}

int
TbBeginningOfLine(uiContext d)
{
  tanContext tan = (tanContext)d->modec;

  while (tan->left) {
    tan = tan->left;
  }
  d->modec = (mode_context)tan;
  setMode(d, tan, 1);
  makeKanjiStatusReturn(d, (yomiContext)d->modec);
  return 0;
}

int
TbEndOfLine(uiContext d)
{
  tanContext tan = (tanContext)d->modec;

  while (tan->right) {
    tan = tan->right;
  }
  d->modec = (mode_context)tan;
  setMode(d, tan, 0);
  makeKanjiStatusReturn(d, (yomiContext)d->modec);
  return 0;
}


inline int
TbChooseChar(uiContext d, int head)
{
  tanContext tan = (tanContext)d->modec;

  if (!head) {
    int len = WStrlen(tan->kanji);
    tan->kanji[0] = tan->kanji[len - 1];
  }
  tan->yomi[0] = tan->roma[0] = tan->kanji[0];
  tan->yomi[1] = tan->roma[1] = tan->kanji[1] = (WCHAR_T)0;
  tan->rAttr[0] = SENTOU;
  tan->kAttr[0] = SENTOU | HENKANSUMI;
  tan->rAttr[1] = tan->kAttr[1] = 0;

  makeKanjiStatusReturn(d, (yomiContext)tan);
  return 0;
}

static int
TanChooseChar(uiContext d, int head)
{
  int retval, len;
  yomiContext yc = (yomiContext)d->modec;
#ifndef WIN
  WCHAR_T xxx[ROMEBUFSIZE];
#else
  WCHAR_T *xxx;
#endif

  if (yc->id != YOMI_CONTEXT) {
    return TbChooseChar(d, head);
  }
#ifdef WIN
  xxx = (WCHAR_T *)malloc(sizeof(WCHAR_T) * ROMEBUFSIZE);
  if (!xxx) {
    return 0;
  }
#endif
  RkwGoTo(yc->context, yc->curbun);
  len = RkwGetKanji(yc->context, xxx, ROMEBUFSIZE);
  if (len >= 0) {
    retval = TanBubunMuhenkan(d);
    if (retval >= 0) {
      tanContext tan;
      yc = (yomiContext)d->modec;
      tan = newTanContext(yc->majorMode, CANNA_MODE_TankouhoMode);
      if (tan) {
	copyYomiinfo2Tan(yc, tan);
	tan->kanji = DUpwstr(xxx + (head ? 0 : len - 1), 1);
	tan->yomi = DUpwstr(yc->kana_buffer, yc->kEndp);
	tan->roma = DUpwstr(yc->romaji_buffer, yc->rEndp);
	tan->kAttr = DUpattr(yc->kAttr, yc->kEndp);
	tan->rAttr = DUpattr(yc->rAttr, yc->rEndp);
	tan->right = yc->right;
	if (tan->right) tan->right->left = tan;
	yc->right = tan;
	tan->left = (tanContext)yc;
	removeCurrentBunsetsu(d, (tanContext)yc);
	makeKanjiStatusReturn(d, (yomiContext)tan);
	goto done;
      }
    }
  }
  retval = NothingChangedWithBeep(d);
 done:
#ifdef WIN
  free(xxx);
#endif
  return retval;
}

static int TanChooseHeadChar (uiContext);
static int TanChooseTailChar (uiContext);

static int
TanChooseHeadChar(uiContext d)
{
  return TanChooseChar(d, 1);
}

static int
TanChooseTailChar(uiContext d)
{
  return TanChooseChar(d, 0);
}

#include	"tanmap.h"
