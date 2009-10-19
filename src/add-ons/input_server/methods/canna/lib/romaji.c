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
static char rcs_id[] = "@(#) 102.1 $Id: romaji.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif /* lint */

#include "canna.h"
#include "RK.h"
#include "RKintern.h"

#include <ctype.h>
#include <errno.h>
#ifdef MEASURE_TIME
#include <sys/types.h>
#include <sys/times.h>
#endif

#define DEFAULT_ROMKANA_TABLE "/dic/default.cbp"
extern struct RkRxDic *romajidic, *englishdic;

static struct RkRxDic *OpenRoma(char *table);
static int makePhonoOnBuffer(uiContext d, yomiContext yc, unsigned char key, int flag, int english);
static int growDakuonP(WCHAR_T ch);
static int KanaYomiInsert(uiContext d);
static int howFarToGoBackward(yomiContext yc);
static int howFarToGoForward(yomiContext yc);
static int YomiBackward(uiContext d);
static int YomiNop(uiContext d);
static int YomiForward(uiContext d);
static int YomiBeginningOfLine(uiContext d);
static int YomiEndOfLine(uiContext d);
static int appendYomi2Yomi(yomiContext yom, yomiContext yc);
static int YomiDeletePrevious(uiContext d);
static int YomiDeleteNext(uiContext d);
static int YomiKillToEndOfLine(uiContext d);
static int YomiQuit(uiContext d);
static int simplePopCallback(uiContext d, int retval, mode_context env);
static int exitYomiQuotedInsert(uiContext d, int retval, mode_context env);
static int YomiInsertQuoted(uiContext d);
static int yomiquotedfunc(uiContext d, KanjiMode mode, int whattodo, int key, int fnum);
static int ConvertAsHex(uiContext d);
static int everySupkey(uiContext d, int retval, mode_context env);
static int exitSupkey(uiContext d, int retval, mode_context env);
static int quitSupkey(uiContext d, int retval, mode_context env);
static int YomiHenkan(uiContext d);
static int YomiHenkanNaive(uiContext d);
static int YomiHenkanOrNothing(uiContext d);
static int YomiBaseHira(uiContext d);
static int YomiBaseKata(uiContext d);
static int YomiBaseEisu(uiContext d);
static int YomiBaseZen(uiContext d);
static int YomiBaseHan(uiContext d);
static int YomiBaseKana(uiContext d);
static int YomiBaseKakutei(uiContext d);
static int YomiBaseHenkan(uiContext d);
static int YomiJishu(uiContext d, int fn);
static int chikujiEndBun(uiContext d);
static void replaceEnglish(uiContext d, yomiContext yc, int start, int end, int RKflag, int engflag);
static int YomiNextJishu(uiContext d);
static int YomiPreviousJishu(uiContext d);
static int YomiKanaRotate(uiContext d);
static int YomiRomajiRotate(uiContext d);
static int YomiCaseRotateForward(uiContext d);
static int YomiZenkaku(uiContext d);
static int YomiHankaku(uiContext d);
static int YomiHiraganaJishu(uiContext d);
static int YomiKatakanaJishu(uiContext d);
static int YomiRomajiJishu(uiContext d);
static int YomiToLower(uiContext d);
static int YomiToUpper(uiContext d);
static int YomiCapitalize(uiContext d);

int forceRomajiFlushYomi (uiContext);
static int chikujiEndBun (uiContext);
extern void EWStrcat (WCHAR_T *, char *);

extern int yomiInfoLevel;

extern struct RkRxDic *englishdic;

/*
 * int d->rStartp;     ro shu c|h    shi f   ¥í¡¼¥Þ»ú ¥¹¥¿¡¼¥È ¥¤¥ó¥Ç¥Ã¥¯¥¹
 * int d->rEndp;       ro shu ch     shi f|  ¥í¡¼¥Þ»ú ¥Ð¥Ã¥Õ¥¡ ¥¤¥ó¥Ç¥Ã¥¯¥¹
 * int d->rCurs;       ro shu ch|    shi f   ¥í¡¼¥Þ»ú ¥¨¥ó¥É   ¥¤¥ó¥Ç¥Ã¥¯¥¹
 * int d->rAttr[1024]; 10 100 11     100 1   ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹ÀèÆ¬¥Õ¥é¥°¥Ð¥Ã¥Õ¥¡
 * int d->kEndp;       ¤í ¤·  ch  ¤å ¤·  f|  ¤«¤Ê     ¥Ð¥Ã¥Õ¥¡ ¥¤¥ó¥Ç¥Ã¥¯¥¹
 * int d->kRStartp;    ¤í ¤·  c|h ¤å ¤·  f   ¥«¡¼¥½¥ë ¥¹¥¿¡¼¥È ¥¤¥ó¥Ç¥Ã¥¯¥¹
 * int d->kCurs;      ¤í ¤·  ch| ¤å ¤·  f   ¥«¡¼¥½¥ë ¥¨¥ó¥É   ¥¤¥ó¥Ç¥Ã¥¯¥¹
 * int d->kAttr[1024]; 11 11  00  11 11  0   ¥«¥ÊÊÑ´¹¤·¤¿¥Õ¥é¥°¥Ð¥Ã¥Õ¥¡
 * int d->nrf;                1              ¥í¡¼¥Þ»úÊÑ´¹¤·¤Þ¤»¤ó¥Õ¥é¥°
 */

/*
 * ¥Õ¥é¥°¤ä¥Ý¥¤¥ó¥¿¤ÎÆ°¤­
 *
 *           ¤Ò¤ã¤¯           hyaku
 *  Àè       100010           10010
 *  ÊÑ       111111
 *  ¶Ø       000000
 * rStartp                         1
 * rCurs                           1
 * rEndp                           1
 * kRstartp        1
 * kCurs           1
 * kEndp           1
 *
 * ¢«
 *           ¤Ò¤ã¤¯           hyaku
 *  Àè       100010           10010
 *  ÊÑ       111111
 *  ¶Ø       000000
 * rStartp                       1
 * rCurs                         1
 * rEndp                           1
 * kRstartp      1
 * kCurs         1
 * kEndp           1
 *
 * ¢«
 *           ¤Ò¤ã¤¯           hyaku
 *  Àè       100010           10010
 *  ÊÑ       111111
 *  ¶Ø       000000
 * rStartp                       1
 * rCurs                         1
 * rEndp                           1
 * kRstartp    1
 * kCurs       1
 * kEndp           1
 *
 * ¢«
 *           ¤Ò¤ã¤¯           hyaku
 *  Àè       100010           10010
 *  ÊÑ       111111
 *  ¶Ø       000000
 * rStartp                    1
 * rCurs                      1
 * rEndp                           1
 * kRstartp  1
 * kCurs     1
 * kEndp           1
 *
 * ¢ª
 *           ¤Ò¤ã¤¯           hyaku
 *  Àè       100010           10010
 *  ÊÑ       111111
 *  ¶Ø       000000
 * rStartp                       1
 * rCurs                         1
 * rEndp                           1
 * kRstartp    1
 * kCurs       1
 * kEndp           1
 *
 * 'k'
 *           ¤Òk¤ã¤¯           hyakku
 *  Àè       1010010           100110
 *  ÊÑ       1101111
 *  ¶Ø       0010000
 * rStartp                        1
 * rCurs                           1
 * rEndp                             1
 * kRstartp    1
 * kCurs        1
 * kEndp            1
 *
 * 'i'
 *           ¤Ò¤­¤ã¤¯           hyakiku
 *  Àè       10100010           1001010
 *  ÊÑ       11111111
 *  ¶Ø       00110000
 * rStartp                           1
 * rCurs                             1
 * rEndp                               1
 * kRstartp      1
 * kCurs         1
 * kEndp             1
 */

#ifdef WIN
#define KANALIMIT 127 /* This is caused by the Canna IME coding */
#endif

#ifndef KANALIMIT
#define KANALIMIT 255
#endif
#define ROMAJILIMIT 255

#define  doubleByteP(x) ((x) & 0x80)

#ifdef DEBUG

void debug_yomi(yomiContext x)
{
  char foo[1024];
  int len, i;

#ifndef WIN
  if (iroha_debug) {
    len = WCstombs(foo, x->romaji_buffer, 1024);
    foo[len] = '\0';
    printf("    %s\nÀè: ", foo);
    for (i = 0 ; i <= x->rEndp ; i++) {
      printf("%s", (x->rAttr[i] & SENTOU) ? "1" : " ");
    }
    printf("\n¥Ý: ");
    for (i = 0 ; i < x->rStartp ; i++) {
      printf(" ");
    }
    printf("^\n");

    len = WCstombs(foo, x->kana_buffer, 1024);
    foo[len] = '\0';
    printf("    %s\nÀè: ", foo);
    for (i = 0 ; i <= x->kEndp ; i++) {
      printf("%s ", (x->kAttr[i] & SENTOU) ? "1" : " ");
    }
    printf("\nºÑ: ");
    for (i = 0 ; i <= x->kEndp ; i++) {
      printf("%s", (x->kAttr[i] & HENKANSUMI) ? "ºÑ" : "Ì¤");
    }
    printf("\n¥Ý: ");
    for (i = 0 ; i < x->kRStartp ; i++) {
      printf("  ");
    }
    printf("¢¬\n");

  }
#endif
}
#else /* !DEBUG */
# define debug_yomi(x)
#endif /* !DEBUG */

#ifndef CALLBACK
#define kanaReplace(where, insert, insertlen, mask) \
kanaRepl(d, where, insert, insertlen, mask)

inline void
kanaRepl(uiContext d, int where, WCHAR_T *insert, int insertlen, int mask)
{
  yomiContext yc = (yomiContext)d->modec;

  generalReplace(yc->kana_buffer, yc->kAttr, &yc->kRStartp,
		 &yc->kCurs, &yc->kEndp,
		 where, insert, insertlen, mask);
}
#else /* CALLBACK */
#define kanaReplace(where, insert, insertlen, mask) \
kanaRepl(d, where, insert, insertlen, mask)

static void
kanaRepl(uiContext d, int where, WCHAR_T *insert, int insertlen, int mask)
{
  yomiContext yc = (yomiContext)d->modec;
#ifndef WIN
  WCHAR_T buf[256];
#else
  WCHAR_T *buf = (WCHAR_T *)malloc(sizeof(WCHAR_T) * 256);
  if (!buf) {
    return;
  }
#endif

  WStrncpy(buf, insert, insertlen);
  buf[insertlen] = '\0';

  generalReplace(yc->kana_buffer, yc->kAttr, &yc->kRStartp, 
		 &yc->kCurs, &yc->kEndp,
		 where, insert, insertlen, mask);
#ifdef WIN
  free(buf);
#endif
}
#endif /* CALLBACK */

#define  romajiReplace(where, insert, insertlen, mask) \
romajiRepl(d, where, insert, insertlen, mask)

inline void
romajiRepl(uiContext d, int where, WCHAR_T *insert, int insertlen, int mask)
{
  yomiContext yc = (yomiContext)d->modec;

  generalReplace(yc->romaji_buffer, yc->rAttr,
		 &yc->rStartp, &yc->rCurs, &yc->rEndp,
		 where, insert, insertlen, mask);
}

/* cfuncdef

   kPos2rPos -- ¤«¤Ê¥Ð¥Ã¥Õ¥¡¤Î¥ê¡¼¥¸¥ç¥ó¤«¤é¥í¡¼¥Þ»ú¥Ð¥Ã¥Õ¥¡¤Î¥ê¡¼¥¸¥ç¥ó¤òÆÀ¤ë

   yc : ÆÉ¤ß¥³¥ó¥Æ¥¯¥¹¥È
   s  : ¤«¤Ê¥Ð¥Ã¥Õ¥¡¤Î¥ê¡¼¥¸¥ç¥ó¤Î³«»Ï°ÌÃÖ
   e  : ¤«¤Ê¥Ð¥Ã¥Õ¥¡¤Î¥ê¡¼¥¸¥ç¥ó¤Î½ªÎ»°ÌÃÖ
   rs : ¥í¡¼¥Þ»ú¥Ð¥Ã¥Õ¥¡¤ÎÂÐ±þ¤¹¤ë³«»Ï°ÌÃÖ¤ò³ÊÇ¼¤¹¤ëÊÑ¿ô¤Ø¤Î¥Ý¥¤¥ó¥¿
   rs : ¥í¡¼¥Þ»ú¥Ð¥Ã¥Õ¥¡¤ÎÂÐ±þ¤¹¤ë½ªÎ»°ÌÃÖ¤ò³ÊÇ¼¤¹¤ëÊÑ¿ô¤Ø¤Î¥Ý¥¤¥ó¥¿
 */

void
kPos2rPos(yomiContext yc, int s, int e, int *rs, int *re)
{
  int i, j, k;

  for (i = 0, j = 0 ; i < s ; i++) {
    if (yc->kAttr[i] & SENTOU) {
      do {
	j++;
      } while (!(yc->rAttr[j] & SENTOU));
    }
  }
  for (i = s, k = j ; i < e ; i++) {
    if (yc->kAttr[i] & SENTOU) {
      do {
	k++;
      } while (!(yc->rAttr[k] & SENTOU));
    }
  }
  if (rs) *rs = j;
  if (re) *re = k;
}

/*
  makeYomiReturnStruct-- ÆÉ¤ß¤ò¥¢¥×¥ê¥±¡¼¥·¥ç¥ó¤ËÊÖ¤¹»þ¤Î¹½Â¤ÂÎ¤òºî¤ë´Ø¿ô

  makeYomiReturnStruct ¤Ï kana_buffer ¤òÄ´¤Ù¤ÆÅ¬Åö¤ÊÃÍ¤òÁÈ¤ßÎ©¤Æ¤ë¡£¤½
  ¤Î»þ¤Ë¥ê¥Ð¡¼¥¹¤ÎÎÎ°è¤âÀßÄê¤¹¤ë¤¬¡¢¥ê¥Ð¡¼¥¹¤ò¤É¤Î¤¯¤é¤¤¤¹¤ë¤«¤Ï¡¢
  ReverseWidely ¤È¤¤¤¦ÊÑ¿ô¤ò¸«¤Æ·èÄê¤¹¤ë¡£

  */

void
makeYomiReturnStruct(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  makeKanjiStatusReturn(d, yc);
}

extern int ckverbose;

static struct RkRxDic *
OpenRoma(char *table)
{
struct RkRxDic *retval = (struct RkRxDic *)0;
char *p;
char rdic[1024];
extern char basepath[];

  if (table || *table) {
    retval = RkwOpenRoma(table);

    if (ckverbose == CANNA_FULL_VERBOSE) {
      if (retval != (struct RkRxDic *)NULL) { /* ¼­½ñ¤¬¥ª¡¼¥×¥ó¤Ç¤­¤¿ */
        printf("¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë¤Ï \"%s\" ¤òÍÑ¤¤¤Þ¤¹¡£\n", table);
      }
    }

    if (retval == (struct RkRxDic *)NULL) {
      /* ¤â¤·¼­½ñ¤¬¥ª¡¼¥×¥ó¤Ç¤­¤Ê¤±¤ì¤Ð¥¨¥é¡¼ */
      extern jrUserInfoStruct *uinfo;

      rdic[0] = '\0';
      if (uinfo && uinfo->topdir && uinfo->uname) {
	sprintf(rdic, "%sdic/user/%s/%s", uinfo->topdir, uinfo->uname, table);
	retval = RkwOpenRoma(rdic);
      }else{
        p = getenv("HOME");
        if (p) {
          sprintf(rdic, "%s/%s", p, table);
          retval = RkwOpenRoma(rdic);
        }
      }

      if (ckverbose == CANNA_FULL_VERBOSE) {
	if (retval != (struct RkRxDic *)NULL) {
#ifndef WIN
          printf("¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë¤Ï \"%s\" ¤òÍÑ¤¤¤Þ¤¹¡£\n", rdic);
#endif
	}
      }

      if (retval == (struct RkRxDic *)NULL) { /* ¤³¤ì¤â¥ª¡¼¥×¥ó¤Ç¤­¤Ê¤¤ */
        extern jrUserInfoStruct *uinfo;

        rdic[0] = '\0';
        if (uinfo && uinfo->topdir) {
	  strcpy(rdic, uinfo->topdir);
        }
        else {
          strcpy(rdic, basepath);
        }
	strcat(rdic, "dic/");	/*******************/
	strcat(rdic, table);
	retval = RkwOpenRoma(rdic);

	if (ckverbose) {
	  if (retval != (struct RkRxDic *)NULL) {
	    if (ckverbose == CANNA_FULL_VERBOSE) {
              printf("¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë¤Ï \"%s\" ¤òÍÑ¤¤¤Þ¤¹¡£\n", rdic);
	    }
	  }
	}

	if (retval == (struct RkRxDic *)NULL) { /* Á´Éô¥ª¡¼¥×¥ó¤Ç¤­¤Ê¤¤ */
	  sprintf(rdic, 
#ifndef WIN
		  "¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë(%s)¤¬¥ª¡¼¥×¥ó¤Ç¤­¤Þ¤»¤ó¡£",
#else
	"\245\355\241\274\245\336\273\372\244\253\244\312"
	"\312\321\264\271\245\306\241\274\245\326\245\353\50\45\163\51\244\254"
	"\245\252\241\274\245\327\245\363\244\307\244\255\244\336\244\273"
	"\244\363\241\243",
#endif
		  table);
           /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë(%s)¤¬¥ª¡¼¥×¥ó¤Ç¤­¤Þ¤»¤ó¡£ */
	  addWarningMesg(rdic);
	  retval = (struct RkRxDic *)0;
	  goto return_ret;
	}
      }
    }
  }
 return_ret:
  return retval;
}

int RomkanaInit(void)
{
  extern char *RomkanaTable, *EnglishTable;
  extern extraFunc *extrafuncp;
  extern char basepath[];
  
  extraFunc *extrafunc1, *extrafunc2;

  /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë¤Î¥ª¡¼¥×¥ó */
  if (RomkanaTable) {
    romajidic = OpenRoma(RomkanaTable);
  }
  else {
    char buf[1024];

    sprintf(buf, "%sdic/default.kp", basepath);
    romajidic = RkwOpenRoma(buf);

    if (romajidic != NULL) {
      int len = strlen(buf);
      RomkanaTable = (char *)malloc(len + 1);
      if (RomkanaTable) {
	strcpy(RomkanaTable, buf);
      }
      if (ckverbose == CANNA_FULL_VERBOSE) {
	printf("¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë¤Ï \"%s\" ¤òÍÑ¤¤¤Þ¤¹¡£\n", buf);
      }
    }
    else { /* ¥ª¡¼¥×¥ó¤Ç¤­¤Ê¤«¤Ã¤¿ */
      if (ckverbose) {
	printf("¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë \"%s\" ¤¬¥ª¡¼¥×¥ó¤Ç¤­¤Þ¤»¤ó¡£\n",
	       buf);
      }
      sprintf(buf, 
	      "¥·¥¹¥Æ¥à¤Î¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë¤¬¥ª¡¼¥×¥ó¤Ç¤­¤Þ¤»¤ó¡£");
      addWarningMesg(buf);
    }
  }

  if (EnglishTable && (!RomkanaTable || strcmp(RomkanaTable, EnglishTable))) {
    /* RomkanaTable ¤È EnglishTable ¤¬°ì½ï¤À¤Ã¤¿¤é¤À¤á */
    englishdic = OpenRoma(EnglishTable);
  }

  /* ¥æ¡¼¥¶¥â¡¼¥É¤Î½é´ü²½ */
  for (extrafunc1 = extrafuncp ; extrafunc1 ; extrafunc1 = extrafunc1->next) {
    /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë¤Î¥ª¡¼¥×¥ó */
    if (extrafunc1->keyword == EXTRA_FUNC_DEFMODE) {
      if (extrafunc1->u.modeptr->romaji_table) {
        if (RomkanaTable && 
            !strcmp(RomkanaTable,
		    (char *)extrafunc1->u.modeptr->romaji_table)) {
	  extrafunc1->u.modeptr->romdic = romajidic;
	  extrafunc1->u.modeptr->romdic_owner = 0;
        }
        else if (EnglishTable && 
	         !strcmp(EnglishTable,
			 (char *)extrafunc1->u.modeptr->romaji_table)) {
	  extrafunc1->u.modeptr->romdic = englishdic;
	  extrafunc1->u.modeptr->romdic_owner = 0;
        }
        else {
	  for (extrafunc2 = extrafuncp ; extrafunc1 != extrafunc2 ;
					extrafunc2 = extrafunc2->next) {
	    if (extrafunc2->keyword == EXTRA_FUNC_DEFMODE &&
		extrafunc2->u.modeptr->romaji_table) {
	      if (!strcmp((char *)extrafunc1->u.modeptr->romaji_table,
			  (char *)extrafunc2->u.modeptr->romaji_table)) {
	        extrafunc1->u.modeptr->romdic = extrafunc2->u.modeptr->romdic;
	        extrafunc1->u.modeptr->romdic_owner = 0;
	        break;
	      }
	    }
	  }
	  if (extrafunc2 == extrafunc1) {
	    extrafunc1->u.modeptr->romdic = 
              OpenRoma(extrafunc1->u.modeptr->romaji_table);
	    extrafunc1->u.modeptr->romdic_owner = 1;
	  }
        }
      }else{
        extrafunc1->u.modeptr->romdic = (struct RkRxDic *)0; /* nil¤Ç¤¹¤è¡ª */
        extrafunc1->u.modeptr->romdic_owner = 0;
      }
    }
  }

  return 0;
}
#if 0
RomkanaInit(void)
{
  extern char *RomkanaTable, *EnglishTable;
  extern extraFunc *extrafuncp;
  extraFunc *extrafunc1, *extrafunc2;
  extern jrUserInfoStruct *uinfo;

  /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë¤Î¥ª¡¼¥×¥ó */
  if (uinfo) {
    if (uinfo->romkanatable) {
      if (RomkanaTable) {
        free(RomkanaTable);
      }      
      RomkanaTable = (char *)malloc(strlen(uinfo->romkanatable) + 1);
      if (RomkanaTable) {
        strcpy(RomkanaTable, uinfo->romkanatable);
      }
    }
  }
  if (RomkanaTable) {
    romajidic = OpenRoma(RomkanaTable);
  }
  else {
#ifndef WIN
    char buf[1024];
#else
    char *buf = malloc(1024);
    if (!buf) {
      return 0;
    }
#endif

    buf[0] = '\0';
    if (uinfo && uinfo->topdir) {
      strcpy(buf, uinfo->topdir);
    }
    else {
      strcpy(buf, basepath);
    }
    strcat(buf, DEFAULT_ROMKANA_TABLE);
    romajidic = RkwOpenRoma(buf);

    if (romajidic != (struct RkRxDic *)NULL) {
      int len = strlen(buf);
      RomkanaTable = (char *)malloc(len + 1);
      if (RomkanaTable) {
	strcpy(RomkanaTable, buf);
      }
      if (ckverbose == CANNA_FULL_VERBOSE) {
#ifndef WIN
        printf("¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë¤Ï \"%s\" ¤òÍÑ¤¤¤Þ¤¹¡£\n", buf);
#endif
      }
    }
    else { /* ¥ª¡¼¥×¥ó¤Ç¤­¤Ê¤«¤Ã¤¿ */
      if (ckverbose) {
#ifndef WIN
        printf("¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë \"%s\" ¤¬¥ª¡¼¥×¥ó¤Ç¤­¤Þ¤»¤ó¡£\n",
               buf);
#endif
      }
      sprintf(buf, "\245\267\245\271\245\306\245\340\244\316\245\355\241\274"
	"\245\336\273\372\244\253\244\312\312\321\264\271\245\306\241\274"
	"\245\326\245\353\244\254\245\252\241\274\245\327\245\363\244\307"
	"\244\255\244\336\244\273\244\363\241\243");
         /* ¥·¥¹¥Æ¥à¤Î¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë¤¬¥ª¡¼¥×¥ó¤Ç¤­¤Þ¤»¤ó¡£ */
      addWarningMesg(buf);
    }
#ifdef WIN
    free(buf);
#endif
  }

#ifndef NOT_ENGLISH_TABLE
  if (EnglishTable && (!RomkanaTable || strcmp(RomkanaTable, EnglishTable))) {
    /* RomkanaTable ¤È EnglishTable ¤¬°ì½ï¤À¤Ã¤¿¤é¤À¤á */
    englishdic = OpenRoma(EnglishTable);
  }
#endif

  /* ¥æ¡¼¥¶¥â¡¼¥É¤Î½é´ü²½ */
  for (extrafunc1 = extrafuncp ; extrafunc1 ; extrafunc1 = extrafunc1->next) {
    /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë¤Î¥ª¡¼¥×¥ó */
    if (extrafunc1->keyword == EXTRA_FUNC_DEFMODE) {
      if (extrafunc1->u.modeptr->romaji_table) {
        if (RomkanaTable && 
            !strcmp(RomkanaTable,
		    (char *)extrafunc1->u.modeptr->romaji_table)) {
	  extrafunc1->u.modeptr->romdic = romajidic;
	  extrafunc1->u.modeptr->romdic_owner = 0;
        }
#ifndef NOT_ENGLISH_TABLE
        else if (EnglishTable && 
	         !strcmp(EnglishTable,
			 (char *)extrafunc1->u.modeptr->romaji_table)) {
	  extrafunc1->u.modeptr->romdic = englishdic;
	  extrafunc1->u.modeptr->romdic_owner = 0;
        }
#endif
        else {
	  for (extrafunc2 = extrafuncp ; extrafunc1 != extrafunc2 ;
					extrafunc2 = extrafunc2->next) {
	    if (extrafunc2->keyword == EXTRA_FUNC_DEFMODE &&
		extrafunc2->u.modeptr->romaji_table) {
	      if (!strcmp((char *)extrafunc1->u.modeptr->romaji_table,
			  (char *)extrafunc2->u.modeptr->romaji_table)) {
	        extrafunc1->u.modeptr->romdic = extrafunc2->u.modeptr->romdic;
	        extrafunc1->u.modeptr->romdic_owner = 0;
	        break;
	      }
	    }
	  }
	  if (extrafunc2 == extrafunc1) {
	    extrafunc1->u.modeptr->romdic = 
              OpenRoma(extrafunc1->u.modeptr->romaji_table);
	    extrafunc1->u.modeptr->romdic_owner = 1;
	  }
        }
      }else{
        extrafunc1->u.modeptr->romdic = (struct RkRxDic *)0; /* nil¤Ç¤¹¤è¡ª */
        extrafunc1->u.modeptr->romdic_owner = 0;
      }
    }
  }

  return 0;
}
#endif

/* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë¤Î¥¯¥í¡¼¥º */

extern keySupplement keysup[];
extern void RkwCloseRoma (struct RkRxDic *);

void
RomkanaFin(void)
{
  extern char *RomkanaTable, *EnglishTable;
  extern int nkeysup;
  int i;

  /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë¤Î¥¯¥í¡¼¥º */
  if (romajidic != (struct RkRxDic *)NULL) {
    RkwCloseRoma(romajidic);
  }
  if (RomkanaTable) {
    free(RomkanaTable);
    RomkanaTable = (char *)NULL;
  }
#ifndef NOT_ENGLISH_TABLE
  if (englishdic != (struct RkRxDic *)NULL) {
    RkwCloseRoma(englishdic);
  }
  if (EnglishTable) {
    free(EnglishTable);
    EnglishTable = (char *)NULL;
  }
#endif
  /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥ë¡¼¥ë¤ÎÊäÂ­¤Î¤¿¤á¤ÎÎÎ°è¤Î²òÊü */
  for (i = 0 ; i < nkeysup ; i++) {
    if (keysup[i].cand) {
      free(keysup[i].cand);
      keysup[i].cand = (WCHAR_T **)NULL;
    }
    if (keysup[i].fullword) {
      free(keysup[i].fullword);
      keysup[i].fullword = (WCHAR_T *)NULL;
    }
  }
  nkeysup = 0;
}

/* cfunc newYomiContext

  yomiContext ¹½Â¤ÂÎ¤ò°ì¤Äºî¤êÊÖ¤¹¡£

 */

yomiContext
newYomiContext(WCHAR_T *buf, int bufsize, int allowedc, int chmodinhibit, int quitTiming, int hinhibit)
{
  yomiContext ycxt;

  ycxt = (yomiContext)malloc(sizeof(yomiContextRec));
  if (ycxt) {
    bzero(ycxt, sizeof(yomiContextRec));
    ycxt->id = YOMI_CONTEXT;
    ycxt->allowedChars = allowedc;
    ycxt->generalFlags = chmodinhibit ? CANNA_YOMI_CHGMODE_INHIBITTED : 0;
    ycxt->generalFlags |= quitTiming ? CANNA_YOMI_END_IF_KAKUTEI : 0;
    ycxt->savedFlags = (long)0;
    ycxt->henkanInhibition = hinhibit;
    ycxt->n_susp_chars = 0;
    ycxt->retbufp = ycxt->retbuf = buf;
    ycxt->romdic = (struct RkRxDic *)0;
    ycxt->myEmptyMode = (KanjiMode)0;
    ycxt->last_rule = 0;
    if ((ycxt->retbufsize = bufsize) == 0) {
      ycxt->retbufp = 0;
    }
    ycxt->right = ycxt->left = (tanContext)0;
    ycxt->next = (mode_context)0;
    ycxt->prevMode = 0;

    /* ÊÑ´¹¤ÎÊ¬ */
    ycxt->nbunsetsu = 0;  /* Ê¸Àá¤Î¿ô¡¢¤³¤ì¤ÇÆÉ¤ß¥â¡¼¥É¤«¤É¤¦¤«¤ÎÈ½Äê¤â¤¹¤ë */
    ycxt->context = -1;
    ycxt->kouhoCount = 0;
    ycxt->allkouho = (WCHAR_T **)0;
    ycxt->curbun = 0;
    ycxt->curIkouho = 0;  /* ¥«¥ì¥ó¥È¸õÊä */
    ycxt->proctime = ycxt->rktime = 0;

    /* Ãà¼¡¤ÎÊ¬ */
    ycxt->ys = ycxt->ye = ycxt->cStartp = ycxt->cRStartp = ycxt->status = 0;
  }
  return ycxt;
}

/*

  GetKanjiString ¤Ï´Á»ú¤«¤Êº®¤¸¤êÊ¸¤ò¼è¤Ã¤Æ¤¯¤ë´Ø¿ô¤Ç¤¢¤ë¡£¼ÂºÝ¤Ë¤Ï 
  empty ¥â¡¼¥É¤òÀßÄê¤¹¤ë¤À¤±¤Ç¥ê¥¿¡¼¥ó¤¹¤ë¡£ºÇ½ªÅª¤Ê·ë²Ì¤¬ buf ¤Ç»ØÄê
  ¤µ¤ì¤¿¥Ð¥Ã¥Õ¥¡¤Ë³ÊÇ¼¤µ¤ì exitCallback ¤¬¸Æ¤Ó½Ð¤µ¤ì¤ë¤³¤È¤Ë¤è¤Ã¤Æ¸Æ¤Ó
  ½Ð¤·Â¦¤Ï´Á»ú¤«¤Êº®¤¸¤êÊ¸»ú¤òÆÀ¤ë¤³¤È¤¬¤Ç¤­¤ë¡£

  Âè£²°ú¿ô¤Î ycxt ¤ÏÄÌ¾ï¤Ï£°¤ò»ØÄê¤¹¤ë¡£¥¢¥ë¥Õ¥¡¥Ù¥Ã¥È¥â¡¼¥É¤«¤éÆüËÜ¸ì
  ¥â¡¼¥É¤Ø¤ÎÀÚ¤êÂØ¤¨¤ËºÝ¤·¤Æ¤Î¤ß¤Ï uiContext ¤ÎÄì¤ËÊÝÂ¸¤·¤Æ¤¢¤ë¥³¥ó¥Æ
  ¥­¥¹¥È¤òÍÑ¤¤¤ë¡£¥¢¥ë¥Õ¥¡¥Ù¥Ã¥È¥â¡¼¥É¤ÈÆüËÜ¸ì¥â¡¼¥É¤È¤ÎÀÚ¤êÂØ¤¨¤Ï¥¹¥¿¥Ã
  ¥¯¾å¤ËÀÑ¤ß¹þ¤Þ¤ì¤¿¥â¡¼¥É¤Î push/pop Áàºî¤Ç¤Ï¤Ê¤¯¡¢¥¹¥ï¥Ã¥×¾å¤Î¥â¡¼¥É
  ¤Î°ìÈÖ¾å¤ÎÍ×ÁÇ¤ÎÆþ¤ìÂØ¤¨¤Ë¤Ê¤ë¡£

  £³¤Ä¤Î Callback ¤Î¤¦¤Á¡¢exitCallback ¤Ï¤Ò¤ç¤Ã¤È¤·¤¿¤é»È¤ï¤ì¤Ê¤¤¤Ç¡¢
  everyTimeCallback ¤È quitCallback ¤·¤«ÍÑ¤¤¤Ê¤¤¤«¤âÃÎ¤ì¤Ê¤¤¡£

 */

yomiContext
GetKanjiString(uiContext d, WCHAR_T *buf, int bufsize, int allowedc, int chmodinhibit, int quitTiming, int hinhibit, canna_callback_t everyTimeCallback, canna_callback_t exitCallback, canna_callback_t quitCallback)
{
  extern KanjiModeRec empty_mode;
  yomiContext yc;

  if ((pushCallback(d, d->modec, everyTimeCallback, exitCallback, quitCallback,
		    NO_CALLBACK)) == (struct callback *)0) {
    return (yomiContext)0;
  }

  yc = newYomiContext(buf, bufsize, allowedc, chmodinhibit,
		      quitTiming, hinhibit);
  if (yc == (yomiContext)0) {
    popCallback(d);
    return (yomiContext)0;
  }
  yc->romdic = romajidic;
  yc->majorMode = d->majorMode;
  yc->minorMode = CANNA_MODE_HenkanMode;
  yc->next = d->modec;
  d->modec = (mode_context)yc;
  /* Á°¤Î¥â¡¼¥É¤ÎÊÝÂ¸ */
  yc->prevMode = d->current_mode;
  /* ¥â¡¼¥ÉÊÑ¹¹ */
  d->current_mode = yc->curMode = yc->myEmptyMode = &empty_mode;
  return yc;
}

/* cfuncdef

   popYomiMode -- ÆÉ¤ß¥â¡¼¥É¤ò¥Ý¥Ã¥×¥¢¥Ã¥×¤¹¤ë¡£

 */

void
popYomiMode(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  d->modec = yc->next;
  d->current_mode = yc->prevMode;

  if (yc->context >= 0) {
    RkwCloseContext(yc->context);
    yc->context = -1;
  }

  freeYomiContext(yc);
}

/* cfuncdef

  checkIfYomiExit -- ÆÉ¤ß¥â¡¼¥É¤¬½ªÎ»¤«¤É¤¦¤«¤òÄ´¤Ù¤ÆÃÍ¤òÊÖ¤¹¥Õ¥£¥ë¥¿

  ¤³¤Î¥Õ¥£¥ë¥¿¤ÏÆÉ¤ß¥â¡¼¥É¤Î³Æ´Ø¿ô¤ÇÃÍ¤òÊÖ¤½¤¦¤È¤¹¤ë»þ¤Ë¸Æ¤Ö¡£ÆÉ¤ß¥â¡¼
  ¥É¤Ç¤Î½èÍý¤¬½ªÎ»¤¹¤ë¤È¤³¤í¤Ç¤¢¤ì¤Ð¡¢ÆÉ¤ß¥â¡¼¥É¤ò½ªÎ»¤·¡¢uiContext ¤Ë
  ¥×¥Ã¥·¥å¤µ¤ì¤Æ¤¤¤¿¥í¡¼¥«¥ë¥Ç¡¼¥¿¤ä¥â¡¼¥É¹½Â¤ÂÎ¤¬¥Ý¥Ã¥×¤µ¤ì¤ë¡£

  ¥í¡¼¥«¥ë¥Ç¡¼¥¿¤Ë exitCallback ¤¬ÄêµÁ¤µ¤ì¤Æ¤¤¤Ê¤±¤ì¤Ð¤¤¤«¤Ê¤ë¾ì¹ç¤Ë¤â
  ¹½Â¤ÂÎ¤Î¥Ý¥Ã¥×¥¢¥Ã¥×¤Ï¹Ô¤ï¤ì¤Ê¤¤¡£

  º£¤Î¤È¤³¤í¡¢ÆÉ¤ß¥â¡¼¥É¤Î½ªÎ»¤Ï¼¡¤Î¤è¤¦¤Ê¾ì¹ç¤¬¹Í¤¨¤é¤ì¤ë¡£

  (1) C-m ¤¬³ÎÄêÆÉ¤ß¤ÎºÇ¸å¤ÎÊ¸»ú¤È¤·¤ÆÊÖ¤µ¤ì¤¿»þ¡£(ÊÑ´¹µö²Ä¤Î»þ)

  (2) ³ÎÄêÊ¸»úÎó¤¬Â¸ºß¤¹¤ë¾ì¹ç¡£(ÊÑ´¹¶Ø»ß¤Î»þ)

  quit ¤ÇÆÉ¤ß¥â¡¼¥É¤ò½ªÎ»¤¹¤ë»þ¤Ï?Â¾¤Î´Ø¿ô?¤ò¸Æ¤Ö¡£

 */

inline
int checkIfYomiExit(uiContext d, int retval)
{
  yomiContext yc = (yomiContext)d->modec;

  if (retval <= 0) {
    /* ³ÎÄêÊ¸»úÎó¤¬¤Ê¤¤¤«¥¨¥é¡¼¤Î¾ì¹ç ¢á exit ¤Ç¤Ï¤Ê¤¤ */
    return retval;
  }
  if (yc->retbufp && yc->retbufsize - (yc->retbufp - yc->retbuf) > retval) {
    /* Ê¸»úÎó³ÊÇ¼¥Ð¥Ã¥Õ¥¡¤¬¤¢¤Ã¤Æ¡¢³ÎÄê¤·¤¿Ê¸»úÎó¤è¤ê¤â¤¢¤Þ¤Ã¤Æ¤¤¤ëÎÎ
       °è¤¬Ä¹¤¤¤Î¤Ç¤¢¤ì¤Ð³ÊÇ¼¥Ð¥Ã¥Õ¥¡¤Ë³ÎÄê¤·¤¿Ê¸»úÎó¤ò¥³¥Ô¡¼¤¹¤ë */
    WStrncpy(yc->retbufp, d->buffer_return, retval);
    yc->retbufp[retval] = (WCHAR_T)0;
    yc->retbufp += retval;
  }
  if (yc->generalFlags & CANNA_YOMI_END_IF_KAKUTEI
      || d->buffer_return[retval - 1] == '\n') {
    /* ÊÑ´¹¤¬¶Ø»ß¤µ¤ì¤Æ¤¤¤ë¤È¤·¤¿¤é exit */
    /* ¤½¤¦¤Ç¤Ê¤¤¾ì¹ç¤Ï¡¢\n ¤¬Æþ¤Ã¤Æ¤¤¤¿¤é exit */
    d->status = EXIT_CALLBACK;
    if (!(d->cb && d->cb->func[EXIT_CALLBACK] == NO_CALLBACK)) {
      d->status = EXIT_CALLBACK;
      popYomiMode(d);
    }
  }
  return retval;
}

static
int checkIfYomiQuit(uiContext d, int retval)
/* ARGSUSED */
{
#ifdef QUIT_IN_YOMI /* ¥³¥á¥ó¥È¥¢¥¦¥È¤¹¤ëÌÜÅª¤Î ifdef */
  yomiContext yc = (yomiContext)d->modec;

  if (d->cb && d->cb->func[QUIT_CALLBACK] == NO_CALLBACK) {
    /* ¥³¡¼¥ë¥Ð¥Ã¥¯¤¬¤Ê¤¤¾ì¹ç

       ¤³¤ó¤Ê¥Á¥§¥Ã¥¯¤ò¿ÆÀÚ¤Ë¹Ô¤¦¤Î¤Ï¡¢ÆÉ¤ß¥â¡¼¥É¤¬Èó¾ï¤Ë´ðËÜÅª¤Ê¥â¡¼¥É
       ¤Ç¤¢¤ê¡¢´°Á´¤ËÈ´¤±¤ë¤È¤­¤Ë¤ï¤¶¤ï¤¶¥Ý¥Ã¥×¥¢¥Ã¥×¤·¤Æ¤â¤¹¤°¤Ë¥×¥Ã¥·¥å
       ¤¹¤ë¾ì¹ç¤¬Â¿¤¤¤È¹Í¤¨¤é¤ì¤Æ½èÍý¤¬ÌµÂÌ¤À¤«¤é¤Ç¤¢¤ë¡£

     */
  }
  else {
    d->status = QUIT_CALLBACK;
    popYomiMode(d);
  }
#endif /* QUIT_IN_YOMI */
  return retval;
}

void fitmarks(yomiContext);

void
fitmarks(yomiContext yc)
{
  if (yc->kRStartp < yc->pmark) {
    yc->pmark = yc->kRStartp;
  }
  if (yc->kRStartp < yc->cmark) {
    yc->cmark = yc->kRStartp;
  }
}

/* Ä¾Á°¤ËÌ¤ÊÑ´¹Ê¸»úÎó¤¬¤Ê¤¤¤«¤É¤¦¤«³ÎÇ§ */
void
ReCheckStartp(yomiContext yc)
{
  int r = yc->rStartp, k = yc->kRStartp, i;

  do { 
    yc->kRStartp--;
    yc->rStartp--;
  } while ( yc->kRStartp >= 0 
	   && !(yc->kAttr[yc->kRStartp] & HENKANSUMI)
	   );
  yc->kRStartp++;
  yc->rStartp++;

  /* Ì¤ÊÑ´¹Éô¤ËÀèÆ¬¥Þ¡¼¥¯¤¬ÉÕ¤¤¤Æ¤¤¤¿¾ì¹ç¤ÏÀèÆ¬¥Þ¡¼¥¯¤ò¤Ï¤º¤¹¡£

     Ì¤ÊÑ´¹Éô¤ÎÀèÆ¬¤Ë´Ø¤·¤Æ¤ÏÀèÆ¬¥Þ¡¼¥¯¤òÉÕ¤±¤Æ¤ª¤¯¡£
     Ì¤ÊÑ´¹Éô¤¬¤¢¤Ã¤¿¾ì¹ç(kRStartp < k)¡¢¤½¤ì¤¬¡¢kCurs ¤è¤ê¤â
     º¸Â¦¤Ç¤¢¤ì¤ÐÀèÆ¬¥Õ¥é¥°¤òÍî¤È¤¹¡£ */

  if (yc->kRStartp < k && k < yc->kCurs) {
    yc->kAttr[k] &= ~SENTOU;
    yc->rAttr[r] &= ~SENTOU;
  }
  for (i = yc->kRStartp + 1 ; i < k ; i++) {
    yc->kAttr[i] &= ~SENTOU;
  }
  for (i = yc->rStartp + 1 ; i < r ; i++) {
    yc->rAttr[i] &= ~SENTOU;
  }
}

extern void setMode (uiContext d, tanContext tan, int forw);

void
removeCurrentBunsetsu(uiContext d, tanContext tan)
{
  if (tan->left) {
    tan->left->right = tan->right;
    d->modec = (mode_context)tan->left;
    d->current_mode = tan->left->curMode;
    setMode(d, tan->left, 0);
  }
  if (tan->right) {
    tan->right->left = tan->left;
    d->modec = (mode_context)tan->right;
    d->current_mode = tan->right->curMode;
    setMode(d, tan->right, 1);
  }
  switch (tan->id) {
  case YOMI_CONTEXT:
    freeYomiContext((yomiContext)tan);
    break;
  case TAN_CONTEXT:
    freeTanContext(tan);
    break;
  }
}

/* tabledef

 charKind -- ¥­¥ã¥é¥¯¥¿¤Î¼ïÎà¤Î¥Æ¡¼¥Ö¥ë

 0x20 ¤«¤é 0x7f ¤Þ¤Ç¤Î¥­¥ã¥é¥¯¥¿¤Î¼ïÎà¤òÉ½¤¹¥Æ¡¼¥Ö¥ë¤Ç¤¢¤ë¡£

 3: ¿ô»ú
 2: £±£¶¿Ê¿ô¤È¤·¤ÆÍÑ¤¤¤é¤ì¤ë±Ñ»ú
 1: ¤½¤ì°Ê³°¤Î±Ñ»ú
 0: ¤½¤ÎÂ¾

 ¤È¤Ê¤ë¡£

 */

static BYTE charKind[] = {
/*sp !  "  #  $  %  &  '  (  )  *  +  ,  -  .  / */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/*0  1  2  3  4  5  6  7  8  9  :  ;  <  =  >  ? */
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1,
/*@  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O */
  1, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2,
/*P  Q  R  S  T  U  V  W  X  Y  X  [  \  ]  ^  _ */
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1,
/*`  a  b  c  d  e  f  g  h  i  j  k  l  m  n  o */
  1, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2,
/*p  q  r  s  t  u  v  w  x  y  z  {  |  }  ~  DEL */
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1,
};

/*
  YomiInsert -- ¥í¡¼¥Þ»ú¤ò£±Ê¸»úÁÞÆþ¤¹¤ë´Ø¿ô

  */

void
restoreChikujiIfBaseChikuji(yomiContext yc)
{
  if (!chikujip(yc) && (yc->generalFlags & CANNA_YOMI_BASE_CHIKUJI)) {
    yc->generalFlags &= ~CANNA_YOMI_BASE_CHIKUJI;
    yc->generalFlags |= CANNA_YOMI_CHIKUJI_MODE;
    yc->minorMode = getBaseMode(yc);
  }
}

int YomiInsert (uiContext);

int YomiInsert(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  int subst, autoconvert = (yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE);
  int kugiri = 0;
#ifdef USE_ROMKANATABLE_FOR_KANAKEY
  WCHAR_T key = 0;
#endif

  d->nbytes = 0;
  if (autoconvert) {
    if (yc->status & CHIKUJI_ON_BUNSETSU) {
      yc->status &= ~CHIKUJI_OVERWRAP;
      if (yc->kCurs != yc->kEndp) {
	yc->rStartp = yc->rCurs = yc->rEndp;
	yc->kRStartp = yc->kCurs = yc->kEndp;
      }
    }
    else {
      if (yc->rEndp == yc->rCurs) {
	yc->status &= ~CHIKUJI_OVERWRAP;
      }
      if (yc->kCurs < yc->ys) {
	yc->ys = yc->kCurs;
      }
    }
  }

  if (yc->allowedChars == CANNA_NOTHING_ALLOWED)/* ¤É¤Î¥­¡¼¤â¼õÉÕ¤±¤Ê¤¤ */
    return NothingChangedWithBeep(d);
  if  (yc->rEndp >= ROMAJILIMIT
       || yc->kEndp >= KANALIMIT
       /* ½Â¤¤·×»»¤ò¤·¤Æ¤¤¤ë
       || (chc && yc->rEndp + chc->hc->ycx->rEndp > ROMAJILIMIT)*/) {
    return NothingChangedWithBeep(d);
  }

  fitmarks(yc);

  if (0xa0 < d->ch && d->ch < 0xe0) {
#ifdef USE_ROMKANATABLE_FOR_KANAKEY
    key = d->buffer_return[0];
#else
    if (yc->allowedChars == CANNA_NOTHING_RESTRICTED) {
      return KanaYomiInsert(d); /* callback ¤Î¥Á¥§¥Ã¥¯¤Ï KanaYomiInsert ¤Ç! */
    }
    else {
      return NothingChangedWithBeep(d);
    }
#endif
  }

  /*   (d->ch & ~0x1f) == 0x1f < (unsigned char)d->ch */
  if (!(d->ch & ~0x1f) && yc->allowedChars != CANNA_NOTHING_RESTRICTED
      || (d->ch < 0x80 ? charKind[d->ch - 0x20] : 1) < yc->allowedChars) {
    /* Á°¤Î¹Ô¡¢USE_ROMKANATABLE_FOR_KANAKEY ¤Î¤È¤­¤Ë¤Þ¤º¤¤ */
    /* 0x20 ¤Ï¥³¥ó¥È¥í¡¼¥ë¥­¥ã¥é¥¯¥¿¤ÎÊ¬ */
    return NothingChangedWithBeep(d);
  }

  if (yc->allowedChars != CANNA_NOTHING_RESTRICTED) {
    /* allowed all °Ê³°¤Ç¤Ï¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¤ò¹Ô¤ï¤Ê¤¤ */
    WCHAR_T romanBuf[4]; /* £²¥Ð¥¤¥È¤Ç½½Ê¬¤À¤È»×¤¦¤±¤É¤Í */
    int len;
#ifdef USE_ROMKANATABLE_FOR_KANAKEY
    WCHAR_T tempc = key ? key : (WCHAR_T)d->ch;
#else
    WCHAR_T tempc = (WCHAR_T)d->ch;
#endif
    romajiReplace(0, &tempc, 1, SENTOU);

    len = RkwCvtNone(romanBuf, 4, &tempc, 1);

    if (yc->generalFlags & CANNA_YOMI_KAKUTEI) { /* ³ÎÄê¤·¤Á¤ã¤¦ */
      WStrncpy(d->buffer_return + d->nbytes, yc->kana_buffer, yc->kCurs);
      /* ¥í¡¼¥Þ»ú¤ÎÃÇÊÒ¤¬»Ä¤Ã¤Æ¤¤¤ë¤³¤È¤Ï¤Ê¤¤¤Î¤Ç¡¢yc->kRStartp ¤Ç¤Ê¤¯¤Æ¡¢
	 yc->kCurs ¤¬»È¤¨¤ë */
      d->nbytes += yc->kCurs;
      romajiReplace(-yc->rCurs, (WCHAR_T *)0, 0, 0);
      kanaReplace(-yc->kCurs, (WCHAR_T *)0, 0, 0);

      WStrncpy(d->buffer_return + d->nbytes, romanBuf, len);
      d->nbytes += len;
      len = 0;
    }

    kanaReplace(0, romanBuf, len, HENKANSUMI);
    yc->kAttr[yc->kRStartp] |= SENTOU;
    yc->rStartp = yc->rCurs;
    yc->kRStartp = yc->kCurs;
  }
  else { /* ¥í¡¼¥Þ»ú¥«¥ÊÊÑ´¹¤¹¤ë¾ì¹ç */
#ifdef USE_ROMKANATABLE_FOR_KANAKEY
    WCHAR_T tempc = key ? key : (WCHAR_T)d->ch;
#else
    WCHAR_T tempc = (WCHAR_T)d->ch;
#endif
    int ppos;
    if (cannaconf.BreakIntoRoman)
      yc->generalFlags |= CANNA_YOMI_BREAK_ROMAN;

    /* Ä¾Á°¤ËÌ¤ÊÑ´¹Ê¸»úÎó¤¬¤Ê¤¤¤«¤É¤¦¤«³ÎÇ§ */

    if (yc->kCurs == yc->kRStartp) {
      ReCheckStartp(yc);
    }

    /* ¤Þ¤º¥«¡¼¥½¥ëÉôÊ¬¤Ë¥í¡¼¥Þ»ú¤ò£±Ê¸»úÆþ¤ì¤ë */

    romajiReplace(0, &tempc, 1, (yc->rStartp == yc->rCurs) ? SENTOU : 0);

    ppos = yc->kRStartp;
    kanaReplace(0, &tempc, 1, (yc->kRStartp == yc->kCurs) ? SENTOU : 0);

#ifdef USE_ROMKANATABLE_FOR_KANAKEY
    kugiri = makePhonoOnBuffer(d, yc, key ? key : (unsigned char)d->ch, 0, 0);
#else
    kugiri = makePhonoOnBuffer(d, yc, (unsigned char)d->ch, 0, 0);
#endif

    if (kugiri && autoconvert) {
      if (ppos < yc->ys) {
	yc->ys = ppos;
      }
      if ((subst = ChikujiSubstYomi(d)) < 0) {
	makeGLineMessageFromString(d, jrKanjiError);
	if (subst == -2) {
	  TanMuhenkan(d);
	}
	else {
	  makeYomiReturnStruct(d);
	}
	return 0; /* ²¼¤Þ¤Ç¹Ô¤«¤Ê¤¯¤Æ¤¤¤¤¤Î¤«¤Ê¤¢ */
      }
    }
  }

  debug_yomi(yc);
  makeYomiReturnStruct(d);

  if (!yc->kEndp && !(autoconvert && yc->nbunsetsu)) {
    if (yc->left || yc->right) {
      removeCurrentBunsetsu(d, (tanContext)yc);
    }
    else {
      /* Ì¤³ÎÄêÊ¸»úÎó¤¬Á´¤¯¤Ê¤¯¤Ê¤Ã¤¿¤Î¤Ê¤é¡¢¦Õ¥â¡¼¥É¤ËÁ«°Ü¤¹¤ë */
      restoreChikujiIfBaseChikuji(yc);
      d->current_mode = yc->curMode = yc->myEmptyMode;
      d->kanji_status_return->info |= KanjiEmptyInfo;
    }
    currentModeInfo(d);
  }

  return d->nbytes;
}

/* cfuncdef

   findSup -- supkey ¤ÎÃæ¤«¤é¥­¡¼¤Ë°ìÃ×¤¹¤ë¤â¤Î¤òÃµ¤¹¡£

   ÊÖ¤ëÃÍ¤Ï supkey ¤ÎÃæ¤Ç key ¤¬°ìÃ×¤¹¤ë¤â¤Î¤¬²¿ÈÖÌÜ¤ËÆþ¤Ã¤Æ¤¤¤ë¤«¤òÉ½¤¹¡£
   ²¿ÈÖÌÜ¤È¸À¤¦¤Î¤Ï£±¤«¤é»Ï¤Þ¤ëÃÍ¡£

   ¸«¤Ä¤«¤é¤Ê¤¤»þ¤Ï£°¤òÊÖ¤¹¡£
 */

int findSup (WCHAR_T);

int findSup(WCHAR_T key)
{
  int i;
  extern int nkeysup;

  for (i = 0 ; i < nkeysup ; i++) {
    if (key == keysup[i].key) {
      return i + 1;
    }
  }
  return 0;
}

/* cfuncdef

   makePhonoOnBuffer -- yomiContext ¤Î¥Ð¥Ã¥Õ¥¡¾å¤Ç¥­¡¼ÆþÎÏ¢ªÉ½²»Ê¸»úÊÑ´¹¤ò
   ¤¹¤ë

   ÊÑ´¹¤Ë¤Ò¤È¶èÀÚ¤ê¤¬ÉÕ¤¤¤¿»þÅÀ¤Ç 1 ¤òÊÖ¤¹¡£¤½¤ì°Ê³°¤Î¾ì¹ç¤Ë¤Ï 0 ¤òÊÖ¤¹¡£

   ºÇ¸å¤«¤é£²¤Ä¤á¤Î flag ¤Ï RkwMapPhonogram ¤ËÅÏ¤¹¥Õ¥é¥°¤Ç¡¢
   ºÇ¸å¤Î english ¤È¸À¤¦¤Î¤Ï±ÑÃ±¸ì¥«¥ÊÊÑ´¹¤ò¤¹¤ë¤«¤É¤¦¤«¤òÉ½¤¹¥Õ¥é¥°

 */

static
int makePhonoOnBuffer(uiContext d, yomiContext yc, unsigned char key, int flag, int english)
{
  int i, n, m, t, sm, henkanflag, prevflag, cond;
  int retval = 0;
  int sup = 0;
  int engflag = (english && englishdic);
  int engdone = 0;
  WCHAR_T *subp;
#ifndef WIN
  WCHAR_T kana_char[1024], sub_buf[1024];
#else
  WCHAR_T *kana_char, *sub_buf;

  kana_char = (WCHAR_T *)malloc(sizeof(WCHAR_T) * 1024);
  sub_buf = (WCHAR_T *)malloc(sizeof(WCHAR_T) * 1024);
  if (!kana_char || !sub_buf) {
    if (kana_char) {
      free(kana_char);
    }
    if (sub_buf) {
      free(sub_buf);
    }
    return 0;
  }
#endif

  if (cannaconf.ignore_case) flag |= RK_IGNORECASE;

  /* Ì¤ÊÑ´¹¥í¡¼¥ÞÊ¸»úÎó¤Î¤«¤ÊÊÑ´¹ */
  for (;;) {
#ifndef USE_ROMKANATABLE_FOR_KANAKEY
    if ((flag & RK_FLUSH) &&
	yc->kRStartp != yc->kCurs &&
	!WIsG0(yc->kana_buffer[yc->kCurs - 1])) {
      /* ¥¢¥¹¥­¡¼Ê¸»ú¤¬Æþ¤Ã¤Æ¤¤¤ë¤ï¤±¤Ç¤Ê¤«¤Ã¤¿¤é */
      kana_char[0] = yc->kana_buffer[yc->kRStartp];
      n = m = 1; t = 0;
      henkanflag = HENKANSUMI;
    }
    /* Êä½õ¥Þ¥Ã¥Ô¥ó¥°¤ÎÄ´ºº */
    else
#endif
      if ((cond = (!(yc->generalFlags & CANNA_YOMI_ROMAJI) &&
		   !(yc->generalFlags & CANNA_YOMI_IGNORE_USERSYMBOLS) &&
		   (yc->kCurs - yc->kRStartp) == 1 &&
		   (sup = findSup(yc->kana_buffer[yc->kRStartp]))) )
	  && keysup[sup - 1].ncand > 0) {
      n = 1; t = 0;
      WStrcpy(kana_char, keysup[sup - 1].cand[0]);
      m = WStrlen(kana_char);
      /* defsymbol ¤Î¿·¤·¤¤µ¡Ç½¤ËÂÐ±þ¤·¤¿½èÍý */
      yc->romaji_buffer[yc->rStartp] = keysup[sup - 1].xkey;
      henkanflag = HENKANSUMI | SUPKEY;
    }
    else {
      if (cond) { /* && keysup[sup - 1].ncand == 0 */
      /* defsymbol ¤Î¿·¤·¤¤µ¡Ç½¤ËÂÐ±þ¤·¤¿½èÍý¡£ÆþÎÏÊ¸»ú¼«¿È¤òÃÖ¤­´¹¤¨¤ë */
	yc->kana_buffer[yc->kRStartp] = 
	  yc->romaji_buffer[yc->rStartp] = keysup[sup - 1].xkey;
      }
      if (yc->romdic != (struct RkRxDic *)NULL
	  && !(yc->generalFlags & CANNA_YOMI_ROMAJI)) {
	if (engflag &&
	    RkwMapPhonogram(englishdic, kana_char, 1024,
			    yc->kana_buffer + yc->kRStartp,
			    yc->kCurs - yc->kRStartp,
			    (WCHAR_T)key,
			    flag, &n, &m, &t, &yc->last_rule) &&
	    n > 0) {
	  henkanflag = HENKANSUMI | GAIRAIGO;
	  engdone = 1;
	}
	else if (engflag && 0 == n /* ¾å¤Î RkwMapPhonogram ¤ÇÆÀ¤¿ÃÍ */ &&
		 RkwMapPhonogram(englishdic, kana_char, 1024,
				 yc->kana_buffer + yc->kRStartp,
				 yc->kCurs - yc->kRStartp,
				 (WCHAR_T)key,
				 flag | RK_FLUSH,
				 &n, &m, &t, &yc->last_rule) &&
		 n > 0) {
	  henkanflag = HENKANSUMI | GAIRAIGO;
	  engdone = 1;
	}
	else {
	  engflag = 0;
	  if (RkwMapPhonogram(yc->romdic, kana_char, 1024, 
			      yc->kana_buffer + yc->kRStartp,
			      yc->kCurs - yc->kRStartp,
			      (WCHAR_T) key,
			      flag | RK_SOKON, &n, &m, &t, &yc->last_rule)) {
	    /* RK_SOKON ¤òÉÕ¤±¤ë¤Î¤Ïµì¼­½ñÍÑ */
	    henkanflag = HENKANSUMI;
	  }
	  else {
	    henkanflag = 0;
	  }
	  if (n > 0 && !engdone) {
	    engflag = (english && englishdic);
	  }
	}
	if (n == yc->kCurs - yc->kRStartp) {
	  key = (unsigned char)0;
	}
      }else{
	t = 0;
	henkanflag = (yc->generalFlags & CANNA_YOMI_ROMAJI) ?
	  (HENKANSUMI | STAYROMAJI) : 0;
	m = n = (yc->kCurs - yc->kRStartp) ? 1 : 0;
	WStrncpy(kana_char, yc->kana_buffer + yc->kRStartp, n);
      }
    }

    /* ¥í¡¼¥Þ»ú¤Î¤¦¤Á n Ê¸»úÊ¬¥«¥Ê¤ËÊÑ´¹¤µ¤ì¤¿ */

    if (n <= 0) {
      break;
    }
    else {
      int unchanged;

      /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¤Î·ë²Ì¤ò²Ã¹©¤¹¤ë */
      if (cannaconf.abandonIllegalPhono && !henkanflag && !yc->n_susp_chars) {
	/* ÊÑ¤Ê¥í¡¼¥Þ»ú¤Ï¼Î¤Æ¤ë */
	sm = 0; subp = sub_buf;
	/* t ¤¬¤¢¤ë¤Î¤Ë henkanflag ¤¬ 0 ¤Î¤³¤È¤Ã¤Æ¤Ê¤¤¤ó¤À¤±¤É¤Í */
	/* WStrncpy(subp, kana_char + m, t); */
      }else{
	sm = m; subp = kana_char;
	if (yc->generalFlags & (CANNA_YOMI_KATAKANA | CANNA_YOMI_HIRAGANA)) {
	  int tempm;

	  if (yc->generalFlags & CANNA_YOMI_KATAKANA) {
	    tempm = RkwCvtKana(sub_buf, 1024, subp, sm);
	  }
	  else {
	    tempm = RkwCvtHira(sub_buf, 1024, subp, sm);
	  }
	  /* Ä¹¤µ¥Á¥§¥Ã¥¯¤¬ËÜÅö¤Ï¤¤¤ë¤¬¡¢ÂÌÌÜ¤Î¤È¤­¤Î½èÍý¤ò¹Í¤¨¤¿¤¯¤Ê¤¤ */
	  WStrncpy(sub_buf + tempm, subp + sm, t);
	  subp = sub_buf;
	  sm = tempm;
	}
	if (yc->generalFlags & (CANNA_YOMI_ZENKAKU | CANNA_YOMI_HANKAKU)) {
	  int tempm;
	  WCHAR_T *otherp = (subp == sub_buf) ? kana_char : sub_buf;

	  if (yc->generalFlags & CANNA_YOMI_ZENKAKU) {
	    tempm = RkwCvtZen(otherp, 1024, subp, sm);
	  }
	  else {
	    tempm = RkwCvtHan(otherp, 1024, subp, sm);
	  }
	  WStrncpy(otherp + tempm, subp + sm, t);
	  subp = otherp;
	  sm = tempm;
	}

	if (yc->generalFlags & CANNA_YOMI_KAKUTEI) { /* ³ÎÄê¤·¤Á¤ã¤¦ */
	  int off;

	  chikujiEndBun(d);
	  WStrncpy(d->buffer_return + d->nbytes,
		   yc->kana_buffer, yc->kRStartp);
	  d->nbytes += yc->kRStartp;

	  off = yc->kCurs - yc->kRStartp;
	  yc->kRStartp = 0;
	  yc->kCurs -= off;
	  kanaReplace(-yc->kCurs, (WCHAR_T *)0, 0, 0);
	  yc->kCurs += off;

	  WStrncpy(d->buffer_return + d->nbytes, subp, sm);
	  d->nbytes += sm;
	  subp += sm;
	  sm = 0;
	}
      }
      /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¤Î·ë²Ì¤ò¥«¥Ê¥Ð¥Ã¥Õ¥¡¤ËÆþ¤ì¤ë¡£ */

      unchanged = yc->kCurs - yc->kRStartp - n;
      yc->kCurs -= unchanged;
      prevflag = (yc->kAttr[yc->kRStartp] & SENTOU);
      kanaReplace(-n, subp, sm + t, henkanflag);
      if ( prevflag ) {
	yc->kAttr[yc->kRStartp] |= SENTOU;
      }
      yc->kRStartp += sm;
      if (t == 0 && m > 0 && unchanged) {
	yc->kAttr[yc->kRStartp] |= SENTOU;
      }
      for (i = yc->kRStartp ; i < yc->kCurs ; i++) {
	yc->kAttr[i] &= ~HENKANSUMI; /* HENKANSUMI ¥Õ¥é¥°¤ò¼è¤ê½ü¤¯ */
      }
      yc->kCurs += unchanged;

      if (t > 0) {
	/* suspend ¤·¤Æ¤¤¤ëÊ¸»úÄ¹¤Ï¥í¡¼¥Þ»ú¥Ð¥Ã¥Õ¥¡¤È¤«¤Ê¥Ð¥Ã¥Õ¥¡¤È¤Î
           ³ÆÊ¸»ú¤ÎÂÐ±þÉÕ¤±¤Ë±Æ¶Á¤¹¤ë¤¬¡¢¤½¤ÎÄ´À°¤ò¤¹¤ë¤¿¤á¤Î·×»» */

	if (yc->n_susp_chars) {
	  yc->n_susp_chars += t - n;
	}
	else {
	  yc->n_susp_chars = SUSPCHARBIAS + t - n;
	}

	/* ¤Ä¤¤¤Ç¤Ë¼¡¤Î¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹ÍÑ¤Ë key ¤ò¹Í¤¨¤Æ¤ß¤ë¡£ */
	key = (unsigned char)yc->kana_buffer[yc->kRStartp + t];
      }
      else if (m > 0) { /* ¥í¡¼¥Þ»ú¤È¤«¤Ê¤ÎÂÐ±þ¤òÉÕ¤±¤ë¤¿¤á¤Î½èÍý */
	int n_cor_keys = n -
	  (yc->n_susp_chars ? yc->n_susp_chars - SUSPCHARBIAS : 0);

	retval = 1; /* ¤Ò¤È¶èÀÚ¤ê¤¬¤Ä¤¤¤¿ */
	yc->rStartp += n_cor_keys;
	if (cannaconf.abandonIllegalPhono &&
	    !henkanflag && !yc->n_susp_chars) {
	  yc->rStartp -= n;
	  unchanged = yc->rCurs - yc->rStartp - n;
	  yc->rCurs -= unchanged;
	  romajiReplace(-n, (WCHAR_T *)0, 0, 0);
	  yc->rCurs += unchanged;
	  retval = 0; /* ¤ä¤Ã¤Ñ¤ê¶èÀÚ¤ê¤¬¤Ä¤¤¤Æ¤¤¤Ê¤¤ */
	}
	else if (yc->generalFlags & CANNA_YOMI_KAKUTEI) {
	  int offset = yc->rCurs - yc->rStartp;

	  yc->rCurs -= offset;
	  romajiReplace(-yc->rCurs, (WCHAR_T *)0, 0, 0);
	  yc->rCurs += offset;
	  retval = 0; /* ¤ä¤Ã¤Ñ¤ê¶èÀÚ¤ê¤¬¤Ä¤¤¤Æ¤¤¤Ê¤¤ */
	}
	yc->rAttr[yc->rStartp] |= SENTOU;
	yc->n_susp_chars = /* t ? SUSPCHARBIAS + t : (t ¤ÏÉ¬¤º 0)*/ 0;
      }
    }
  }
#ifdef WIN
  free(kana_char);
  free(sub_buf);
#endif
  return retval;
}

#define KANAYOMIINSERT_BUFLEN 10

/* °Ê²¼¤Î¤¤¤¯¤Ä¤«¤Î´Ø¿ô¤ÏÈó¾ï¤ËÆüËÜ¸ì¤Ë°ÍÂ¸¤·¤Æ¤¤¤ë¡£
   ¤«¤ÊÆþÎÏ¤Ë¤Ä¤¤¤Æ¤â¥Æ¡¼¥Ö¥ë¤ò»È¤¦¤è¤¦¤Ë¤·¤Æ¡¢°ÍÂ¸ÉôÊ¬¤ò
   ÇÓ½ü¤¹¤ë¤è¤¦¤Ë¤·¤¿¤¤¤â¤Î¤À */

/*
  dakuonP -- predicate for Japanese voiced sounds (Japanese specific)

  argument:
            ch(WCHAR_T): character to be inspected

  return value:
            0: Not a voiced sound.
	    1: Semi voiced sound.
	    2: Full voiced sound.
 */

#define DAKUON_HV 1
#define DAKUON_FV 2

inline
int dakuonP(WCHAR_T ch)
{
  static int dakuon_first_time = 1;
  static WCHAR_T hv, fv;

  if (dakuon_first_time) { /* ÆüËÜ¸ì¸ÇÍ­¤Î½èÍý */
    WCHAR_T buf[2];

    dakuon_first_time = 0;

    MBstowcs(buf, "\216\336"/* Âù²» */, 2);
    fv = buf[0];
    MBstowcs(buf, "\216\337"/* È¾Âù²» */, 2);
    hv = buf[0];
  }

  if (ch == hv) {
    return DAKUON_HV;
  }
  else if (ch == fv) {
    return DAKUON_FV;
  }
  else {
    return 0;
  }
}

/*
  growDakuonP -- Âù²»¤¬ÉÕ¤¯¤«¤É¤¦¤«

  °ú¿ô:
       ch(WCHAR_T): Ä´¤Ù¤ëÂÐ¾Ý¤ÎÊ¸»ú

  ÊÖ¤êÃÍ:
       0: ÉÕ¤«¤Ê¤¤
       1: ¡Ö¤¦¡×
       2: Âù²»¤À¤±¤¬ÉÕ¤¯
       3: È¾Âù²»¤ÈÂù²»¤¬ÉÕ¤¯
 */

#define GROW_U  1
#define GROW_FV 2
#define GROW_HV 3

static
int growDakuonP(WCHAR_T ch)
{
  /* ÂùÅÀ¤¬Â³¤¯²ÄÇ½À­¤¬¤¢¤ëÊ¸»ú¤Î½èÍý (¤¦¡¢¤«¡Á¤È¡¢¤Ï¡Á¤Û) */
  static int dakuon_first_time = 1;
  static WCHAR_T wu, wka, wto, wha, who;

  if (dakuon_first_time) { /* ÆüËÜ¸ì¸ÇÍ­¤Î½èÍý */
    WCHAR_T buf[2];

    dakuon_first_time = 0;

    MBstowcs(buf, "\216\263"/* ¥¦ */, 2);
    wu = buf[0];
    MBstowcs(buf, "\216\266"/* ¥« */, 2);
    wka = buf[0];
    MBstowcs(buf, "\216\304"/* ¥È */, 2);
    wto = buf[0];
    MBstowcs(buf, "\216\312"/* ¥Ï */, 2);
    wha = buf[0];
    MBstowcs(buf, "\216\316"/* ¥Û */, 2);
    who = buf[0];
  }

  if (ch == wu) {
    return GROW_U;
  }
  else if (wka <= ch && ch <= wto) {
    return GROW_FV;
  }
  else if (wha <= ch && ch <= who) {
    return GROW_HV;
  }
  else {
    return 0;
  }
}

static
int KanaYomiInsert(uiContext d)
{
  static WCHAR_T kana[3], *kanap;
  WCHAR_T buf1[KANAYOMIINSERT_BUFLEN], buf2[KANAYOMIINSERT_BUFLEN];
  /* The array above is not so big (10 WCHAR_T length) 1996.6.5 kon */
  WCHAR_T *bufp, *nextbufp;
  int len, replacelen, spos;
  yomiContext yc = (yomiContext)d->modec;
  int dakuon, grow_dakuon;

  yc->generalFlags &= ~CANNA_YOMI_BREAK_ROMAN;
  kana[0] = (WCHAR_T)0;
  kana[1] = d->buffer_return[0];
  kana[2] = (WCHAR_T)0;
  kanap = kana + 1;
  replacelen = 0; len = 1;
  romajiReplace(0, kanap, 1, SENTOU);
  yc->rStartp = yc->rCurs;
  if ((dakuon = dakuonP(kanap[0])) != 0) { /* ÂùÅÀ¤Î½èÍý */
    if (yc->rCurs > 1) {
      kana[0] = yc->romaji_buffer[yc->rCurs - 2];
      if ((grow_dakuon = growDakuonP(kana[0])) == GROW_HV ||
	  (grow_dakuon && dakuon == DAKUON_FV)) {
	kanap = kana; len = 2; replacelen = -1;
	yc->rAttr[yc->rCurs - 1] &= ~SENTOU;
      }
    }
  }
#ifdef DEBUG
  if (iroha_debug) {
    WCHAR_T aho[200];

    WStrncpy(aho, kana, len);
    aho[len] = 0;
    fprintf(stderr, "\312\321\264\271\301\260(%s)", aho);
                   /* ÊÑ´¹Á° */
  }
#endif
  bufp = kanap; nextbufp = buf1;
  if (yc->generalFlags & CANNA_YOMI_ZENKAKU ||
      !(yc->generalFlags & (CANNA_YOMI_ROMAJI | CANNA_YOMI_HANKAKU))) {
    len = RkwCvtZen(nextbufp, KANAYOMIINSERT_BUFLEN, bufp, len);
    bufp = nextbufp;
    if (bufp == buf1) {
      nextbufp = buf2;
    }
    else {
      nextbufp = buf1;
    }
  }
  if (!(yc->generalFlags & (CANNA_YOMI_ROMAJI | CANNA_YOMI_KATAKANA))) {
    /* ¤Ò¤é¤¬¤Ê¤Ë¤¹¤ë */
    len = RkwCvtHira(nextbufp, KANAYOMIINSERT_BUFLEN, bufp, len);
    bufp = nextbufp;
    if (bufp == buf1) {
      nextbufp = buf2;
    }
    else {
      nextbufp = buf1;
    }
  }

  spos = yc->kCurs + replacelen;
  kanaReplace(replacelen, bufp, len, HENKANSUMI);
  yc->kAttr[spos] |= SENTOU;

  yc->kRStartp = yc->kCurs;
  yc->rStartp = yc->rCurs;
  if (growDakuonP(yc->romaji_buffer[yc->rCurs - 1])) {
    yc->kRStartp--;
    yc->rStartp--;
  }

  if (yc->generalFlags & CANNA_YOMI_KAKUTEI) { /* ³ÎÄê¥â¡¼¥É¤Ê¤é */
    int off, i;

    for (i = len = 0 ; i < yc->kRStartp ; i++) {
      if (yc->kAttr[i] & SENTOU) {
	do {
	  len++;
	} while (!(yc->rAttr[len] & SENTOU));
      }
    }

    if (yc->kRStartp < d->n_buffer) {
      WStrncpy(d->buffer_return, yc->kana_buffer, yc->kRStartp);
      d->nbytes = yc->kRStartp;
    }
    else {
      d->nbytes = 0;
    }
    off = yc->kCurs - yc->kRStartp;
    yc->kCurs -= off;
    kanaReplace(-yc->kCurs, (WCHAR_T *)0, 0, 0);
    yc->kCurs += off;
    off = yc->rCurs - len;
    yc->rCurs -= off;
    romajiReplace(-yc->rCurs, (WCHAR_T *)0, 0, 0);
    yc->rCurs += off;
  }
  else {
    d->nbytes = 0;
  }

  if (yc->rStartp == yc->rCurs && yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE &&
      ChikujiSubstYomi(d) == -1) {
    makeRkError(d, "\303\340\274\241\312\321\264\271\244\313\274\272\307\324"
	"\244\267\244\336\244\267\244\277");
                   /* Ãà¼¡ÊÑ´¹¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    return 0;
  }

  makeYomiReturnStruct(d);

  if (yc->kEndp <= yc->cStartp &&
      !((yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) && yc->nbunsetsu)) {
    if (yc->left || yc->right) {
      removeCurrentBunsetsu(d, (tanContext)yc);
    }
    else {
      /* Ì¤³ÎÄêÊ¸»úÎó¤¬Á´¤¯¤Ê¤¯¤Ê¤Ã¤¿¤Î¤Ê¤é¡¢¦Õ¥â¡¼¥É¤ËÁ«°Ü¤¹¤ë */
      restoreChikujiIfBaseChikuji(yc);
      d->current_mode = yc->curMode = yc->myEmptyMode;
      d->kanji_status_return->info |= KanjiEmptyInfo;
    }
    currentModeInfo(d);
  }

  return d->nbytes;
}

#undef KANAYOMIINSERT_BUFLEN

void
moveStrings(WCHAR_T *str, BYTE *attr, int start, int end, int distance)
{     
  int i;

  if (distance > 0) { /* ¸å¤í¤Ë¤º¤ì¤ì¤Ð */
    for (i = end ; start <= i ; i--) { /* ¸å¤í¤«¤é¤º¤é¤¹ */
      str[i + distance]  = str[i];
      attr[i + distance] = attr[i];
    }
  }
  else if (distance < 0) { /* Á°¤Ë¤º¤ì¤ì¤Ð */
    for (i = start ; i <= end ; i++) {     /* Á°¤«¤é¤º¤é¤¹ */
      str[i + distance]  = str[i];
      attr[i + distance] = attr[i];
    }
  }
  /* else { ¤Ê¤Ë¤â¤·¤Ê¤¤ } */
}

static
int howFarToGoBackward(yomiContext yc)
{
  if (yc->kCurs <= yc->cStartp) {
    return 0;
  }
  if (!cannaconf.ChBasedMove) {
    BYTE *st = yc->kAttr;
    BYTE *cur = yc->kAttr + yc->kCurs;
    BYTE *p = cur;
    
    for (--p ; p > st && !(*p & SENTOU) ;) {
      --p;
    }
    if (yc->kAttr + yc->cStartp > p) {
      p = yc->kAttr + yc->cStartp;
    }
    return cur - p;
  }
  return 1;
}

static
int howFarToGoForward(yomiContext yc)
{
  if (yc->kCurs == yc->kEndp) {
    return 0;
  }
  if (!cannaconf.ChBasedMove) {
    BYTE *end = yc->kAttr + yc->kEndp;
    BYTE *cur = yc->kAttr + yc->kCurs;
    BYTE *p = cur;

    for (++p ; p < end && !(*p & SENTOU) ;) {
      p++;
    }
    return p - cur;
  }
  return 1;
}


static int
YomiBackward(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  int howManyMove;

  d->nbytes = 0;
  if (forceRomajiFlushYomi(d))
    return(d->nbytes);

  if ((yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) &&
      !(yc->status & CHIKUJI_OVERWRAP) && yc->nbunsetsu) {
    /* ¥ª¡¼¥Ð¥é¥Ã¥×¤¸¤ã¤Ê¤¤¤Ê¤é */
    yc->status |= CHIKUJI_OVERWRAP;
    moveToChikujiTanMode(d);
    return TanBackwardBunsetsu(d);
  }

  howManyMove = howFarToGoBackward(yc);
  if (howManyMove) {
    yc->kCurs -= howManyMove;

    if (yc->kCurs < yc->kRStartp)
      yc->kRStartp = yc->kCurs;   /* Ì¤³ÎÄê¥í¡¼¥Þ»ú¥«¡¼¥½¥ë¤â¤º¤é¤¹ */

    /* ¤«¤Ê¤Î¥Ý¥¤¥ó¥¿¤¬ÊÑ´¹¤µ¤ì¤¿¤È¤­¤ÎÅÓÃæ¤Î¥Ç¡¼¥¿¤Ç¤Ê¤¤¾ì¹ç
       (¤Ä¤Þ¤êÊÑ´¹¤Î»þ¤ËÀèÆ¬¤Î¥Ç¡¼¥¿¤À¤Ã¤¿¾ì¹ç)¤Ë¤Ï¥í¡¼¥Þ»ú¤Î
       ¥«¡¼¥½¥ë¤â¤º¤é¤¹ */

    if (yc->kAttr[yc->kCurs] & SENTOU) {
      while ( yc->rCurs > 0 && !(yc->rAttr[--yc->rCurs] & SENTOU) )
	/* EMPTY */
	;
      if (yc->rCurs < yc->rStartp)
	yc->rStartp = yc->rCurs;
    }
  }
  else if (yc->nbunsetsu) { /* Ê¸Àá¤¬¤¢¤ë¤Ê¤é(Ãà¼¡) */
    yc->curbun = yc->nbunsetsu - 1;
    if (RkwGoTo(yc->context, yc->nbunsetsu - 1) == -1) { /* ºÇ¸åÈøÊ¸Àá¤Ø */
      return makeRkError(d, "\312\270\300\341\244\316\260\334\306\260\244\313"
	"\274\272\307\324\244\267\244\336\244\267\244\277");
                            /* Ê¸Àá¤Î°ÜÆ°¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    }
    yc->kouhoCount = 0;
    moveToChikujiTanMode(d);
  }
  else if (yc->left) {
    return TbBackward(d);
  }
  else if (!cannaconf.CursorWrap) {
    return NothingChanged(d);
  }
  else if (yc->right) {
    return TbEndOfLine(d);
  }
  else {
    yc->kCurs = yc->kRStartp = yc->kEndp;
    yc->rCurs = yc->rStartp = yc->rEndp;
  }
  yc->status |= CHIKUJI_OVERWRAP;
  makeYomiReturnStruct(d);

  return 0;
}

static
int YomiNop(uiContext d)
{
  /* currentModeInfo ¤Ç¥â¡¼¥É¾ðÊó¤¬É¬¤ºÊÖ¤ë¤è¤¦¤Ë¥À¥ß¡¼¤Î¥â¡¼¥É¤òÆþ¤ì¤Æ¤ª¤¯ */
  d->majorMode = d->minorMode = CANNA_MODE_AlphaMode;
  currentModeInfo(d);
  makeYomiReturnStruct(d);
  return 0;
}

static
int YomiForward(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  int howManyMove;

  d->nbytes = 0;
  if (forceRomajiFlushYomi(d))
    return(d->nbytes);

  if ((yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) &&
      !(yc->status & CHIKUJI_OVERWRAP) && yc->nbunsetsu) {
    yc->status |= CHIKUJI_OVERWRAP;
    moveToChikujiTanMode(d);
    return TanForwardBunsetsu(d);
  }

  howManyMove = howFarToGoForward(yc);
  if (howManyMove) {
    if (yc->kAttr[yc->kCurs] & SENTOU) { /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹»þÀèÆ¬¤À¤Ã¤¿ */
      while ( !yc->rAttr[++yc->rCurs] )
	/* EMPTY */
	; /* ¼¡¤ÎÀèÆ¬¤Þ¤Ç¤º¤é¤¹ */
      yc->rStartp = yc->rCurs;
    }

    yc->kCurs += howManyMove;   /* ²èÌÌ¤ÎÆþÎÏ°ÌÃÖ ¥«¡¼¥½¥ë¤ò±¦¤Ë¤º¤é¤¹ */
    yc->kRStartp = yc->kCurs;
    yc->status &= ~CHIKUJI_ON_BUNSETSU;
  }
  else if (yc->right) {
    return TbForward(d);
  }
  else if (!cannaconf.CursorWrap) {
    return NothingChanged(d);
  }
  else if (yc->left) {
    return TbBeginningOfLine(d);
  }
  else if (yc->nbunsetsu) { /* Ê¸Àá¤¬¤¢¤ë(Ãà¼¡) */
    yc->kouhoCount = 0;
    yc->curbun = 0;
    if (RkwGoTo(yc->context, 0) == -1) {
      return makeRkError(d, "\312\270\300\341\244\316\260\334\306\260\244\313"
	"\274\272\307\324\244\267\244\336\244\267\244\277");
                            /* Ê¸Àá¤Î°ÜÆ°¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    }
    moveToChikujiTanMode(d);
  }
  else {
    yc->kRStartp = yc->kCurs = yc->rStartp = yc->rCurs = 0;
  }

  yc->status |= CHIKUJI_OVERWRAP;
  makeYomiReturnStruct(d);
  return 0;
}

int static YomiBeginningOfLine (uiContext);

static
int YomiBeginningOfLine(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  d->nbytes = 0;
  if (forceRomajiFlushYomi(d))
    return(d->nbytes);

  if (yc->left) {
    return TbBeginningOfLine(d);
  }
  else if (yc->nbunsetsu) { /* Ãà¼¡¤Çº¸Â¦¤ËÊ¸Àá¤¬¤¢¤ë¤Ê¤é */
    yc->kouhoCount = 0;
    if (RkwGoTo(yc->context, 0) < 0) {
      return makeRkError(d, "\312\270\300\341\244\316\260\334\306\260\244\313"
	"\274\272\307\324\244\267\244\336\244\267\244\277");
                            /* Ê¸Àá¤Î°ÜÆ°¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    }
    yc->curbun = 0;
    moveToChikujiTanMode(d);
  }
  else {
    yc->kRStartp = yc->kCurs = yc->cStartp;
    yc->rStartp = yc->rCurs = yc->cRStartp;
  }
  yc->status |= CHIKUJI_OVERWRAP;
  makeYomiReturnStruct(d);
  return(0);
}


static
int YomiEndOfLine(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  d->nbytes = 0;
  if (forceRomajiFlushYomi(d))
    return(d->nbytes);

  if (yc->right) {
    return TbEndOfLine(d);
  }
  else {
    yc->kRStartp = yc-> kCurs = yc->kEndp;
    yc->rStartp = yc-> rCurs = yc->rEndp;
    yc->status &= ~CHIKUJI_ON_BUNSETSU;
    yc->status |= CHIKUJI_OVERWRAP;
  }
  makeYomiReturnStruct(d);
  return 0;
}

int
forceRomajiFlushYomi(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->kCurs != yc->kRStartp) {
    d->nbytes = 0;
    if (RomajiFlushYomi(d, (WCHAR_T *)NULL, 0) == 0) { /* empty mode */
      d->more.todo = 1;
      d->more.ch = d->ch;
      d->more.fnum = 0;    /* ¾å¤Î ch ¤Ç¼¨¤µ¤ì¤ë½èÍý¤ò¤»¤è */
      return(1);
    }
  }
  return(0);
}

/* RomajiFlushYomi(d, buffer, bufsize) ¥æ¡¼¥Æ¥£¥ê¥Æ¥£´Ø¿ô
 *
 * ¤³¤Î´Ø¿ô¤Ï¡¢(uiContext)d ¤ËÃß¤¨¤é¤ì¤Æ¤¤¤ëÆÉ¤ß¤Î¾ðÊó 
 * (yc->romaji_buffer ¤È yc->kana_buffer)¤òÍÑ¤¤¤Æ¡¢buffer ¤Ë¤½¤ÎÆÉ¤ß¤ò¥Õ
 * ¥é¥Ã¥·¥å¤·¤¿·ë²Ì¤òÊÖ¤¹´Ø¿ô¤Ç¤¢¤ë¡£¥Õ¥é¥Ã¥·¥å¤·¤¿·ë²Ì¤ÎÊ¸»úÎó¤ÎÄ¹¤µ
 * ¤Ï¤³¤Î´Ø¿ô¤ÎÊÖ¤êÃÍ¤È¤·¤ÆÊÖ¤µ¤ì¤ë¡£
 *
 * buffer ¤È¤·¤Æ NULL ¤¬»ØÄê¤µ¤ì¤¿»þ¤Ï¡¢¥Ð¥Ã¥Õ¥¡¤ËÂÐ¤¹¤ë³ÊÇ¼¤Ï¹Ô¤ï¤Ê¤¤
 *
 * ¡ÚºîÍÑ¡Û   
 *
 *    ÆÉ¤ß¤ò³ÎÄê¤¹¤ë
 *
 * ¡Ú°ú¿ô¡Û
 *
 *    d  (uiContext)  ¥«¥Ê´Á»úÊÑ´¹¹½Â¤ÂÎ
 *    buffer (char *)    ÆÉ¤ß¤òÊÖ¤¹¤¿¤á¤Î¥Ð¥Ã¥Õ¥¡ (NULL ²Ä)
 *
 * ¡ÚÌá¤êÃÍ¡Û
 *
 *    buffer ¤Ë³ÊÇ¼¤·¤¿Ê¸»úÎó¤ÎÄ¹¤µ(¥Ð¥¤¥ÈÄ¹)
 *
 * ¡ÚÉûºîÍÑ¡Û
 *
 */

int RomajiFlushYomi(uiContext d, WCHAR_T *b, int bsize)
{
  int ret;
  yomiContext yc = (yomiContext)d->modec;

  yc->generalFlags &= ~CANNA_YOMI_BREAK_ROMAN;

  makePhonoOnBuffer(d, yc, (unsigned char)0, RK_FLUSH, 0);
  yc->n_susp_chars = 0; /* ¾å¤Î¹Ô¤ÇÊÝ¾Ú¤µ¤ì¤ë¤«¤âÃÎ¤ì¤Ê¤¤ */
  yc->last_rule = 0;

  ret = yc->kEndp - yc->cStartp; /* ¤½¤Î·ë²Ì¤¬¤³¤Î´Ø¿ô¤ÎÊÖ¤êÃÍ¤Ë¤Ê¤ë */
  if (b) {
    if (bsize > ret) {
      WStrncpy(b, yc->kana_buffer + yc->cStartp, ret);
      b[ret] = '\0';
    }
    else {
      WStrncpy(b, yc->kana_buffer + yc->cStartp, bsize);
      ret = bsize;
    }
  }
  if (ret == 0) { /* ÆÉ¤ß¤¬Ìµ¤¯¤Ê¤Ã¤¿¤Î¤Ê¤é¥¨¥ó¥×¥Æ¥£¥â¡¼¥É¤Ø */
    d->current_mode = yc->curMode = yc->myEmptyMode;
    /* ¤â¤Ã¤È¤¤¤í¤¤¤í¥¯¥ê¥¢¤·¤¿Êý¤¬ÎÉ¤¤¤ó¤¸¤ã¤Ê¤¤¤Î */
  }
  return ret;
}

inline int
saveFlags(yomiContext yc)
{
  if (!(yc->savedFlags & CANNA_YOMI_MODE_SAVED)) {
    yc->savedFlags = (yc->generalFlags &
		      (CANNA_YOMI_ATTRFUNCS | CANNA_YOMI_BASE_HANKAKU)) |
			CANNA_YOMI_MODE_SAVED;
    yc->savedMinorMode = yc->minorMode;
    return 1;
  }
  else {
    return 0;
  }
}

void
restoreFlags(yomiContext yc)
{
  yc->generalFlags &= ~(CANNA_YOMI_ATTRFUNCS | CANNA_YOMI_BASE_HANKAKU);
  yc->generalFlags |= yc->savedFlags
    & (CANNA_YOMI_ATTRFUNCS | CANNA_YOMI_BASE_HANKAKU);
  yc->savedFlags = (long)0;
  yc->minorMode = yc->savedMinorMode;
}

/*
 doYomiKakutei -- ÆÉ¤ß¤ò³ÎÄê¤µ¤»¤ëÆ°ºî¤ò¤¹¤ë¡£

  retval 0 -- ÌäÂêÌµ¤¯³ÎÄê¤·¤¿¡£
         1 -- ³ÎÄê¤·¤¿¤é¤Ê¤¯¤Ê¤Ã¤¿¡£
        -1 -- ¥¨¥é¡¼¡©
 */

inline int
doYomiKakutei(uiContext d)
{
  int len;

  len = RomajiFlushYomi(d, (WCHAR_T *)0, 0);
  if (len == 0) {
    return 1;
  }

  return 0;
}

int
xString(WCHAR_T *str, int len, WCHAR_T *s, WCHAR_T *e)
{
  if (e < s + len) {
    len = e - s;
  }
  WStrncpy(s, str, len);
  return len;
}

inline int
xYomiKakuteiString(yomiContext yc, WCHAR_T *s, WCHAR_T *e)
{
  return xString(yc->kana_buffer + yc->cStartp, yc->kEndp - yc->cStartp, s, e);
}

inline int
xYomiYomi(yomiContext yc, WCHAR_T *s, WCHAR_T *e)
{
  return xString(yc->kana_buffer, yc->kEndp, s, e);
}

inline int
xYomiRomaji(yomiContext yc, WCHAR_T *s, WCHAR_T *e)
{
  return xString(yc->romaji_buffer, yc->rEndp, s, e);
}

inline void
finishYomiKakutei(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->savedFlags & CANNA_YOMI_MODE_SAVED) {
    restoreFlags(yc);
  }
}

int
appendTan2Yomi(tanContext tan, yomiContext yc)
{
  int klen, rlen;

  klen = WStrlen(tan->yomi);
  rlen = WStrlen(tan->roma);

  if (yc->kEndp + klen < ROMEBUFSIZE && yc->rEndp + rlen < ROMEBUFSIZE) {
    WStrcpy(yc->kana_buffer + yc->kEndp, tan->yomi);
    WStrcpy(yc->romaji_buffer + yc->rEndp, tan->roma);
    bcopy(tan->kAttr, yc->kAttr + yc->kEndp, (klen + 1) * sizeof(BYTE));
    bcopy(tan->rAttr, yc->rAttr + yc->rEndp, (rlen + 1) * sizeof(BYTE));
    yc->rEndp += rlen;
    yc->kEndp += klen;
    return 1;
  }
  return 0;
}

static
int appendYomi2Yomi(yomiContext yom, yomiContext yc)
{
  int rlen, klen;

  rlen = yom->rEndp;
  klen = yom->kEndp;
  if (yc->kEndp + klen < ROMEBUFSIZE && yc->rEndp + rlen < ROMEBUFSIZE) {
    yom->romaji_buffer[rlen] = (WCHAR_T)'\0';
    yom->kana_buffer[klen] = (WCHAR_T)'\0';
    WStrcpy(yc->romaji_buffer + yc->rEndp, yom->romaji_buffer);
    WStrcpy(yc->kana_buffer + yc->kEndp, yom->kana_buffer);
    bcopy(yom->kAttr, yc->kAttr + yc->kEndp, (klen + 1) * sizeof(BYTE));
    bcopy(yom->rAttr, yc->rAttr + yc->rEndp, (rlen + 1) * sizeof(BYTE));
    yc->rEndp += rlen;
    yc->kEndp += klen;
    return 1;
  }
  return 0;
}

yomiContext
dupYomiContext(yomiContext yc)
{
  yomiContext res;

  res = newYomiContext((WCHAR_T *)NULL, 0, /* ·ë²Ì¤Ï³ÊÇ¼¤·¤Ê¤¤ */
		       CANNA_NOTHING_RESTRICTED,
		       (int)!CANNA_YOMI_CHGMODE_INHIBITTED,
		       (int)!CANNA_YOMI_END_IF_KAKUTEI,
		       CANNA_YOMI_INHIBIT_NONE);
  if (res) {
    res->generalFlags = yc->generalFlags;
    res->status = yc->status;
    res->majorMode = yc->majorMode;
    res->minorMode = yc->minorMode;
    res->myMinorMode = yc->myMinorMode;
    res->curMode = yc->curMode;
    res->myEmptyMode = yc->myEmptyMode;
    res->romdic = yc->romdic;
    res->next = yc->next;
    res->prevMode = yc->prevMode;
    appendYomi2Yomi(yc, res);
  }
  return res;
}


/*
  doMuhenkan -- ÌµÊÑ´¹½èÍý¤ò¤¹¤ë¡£

  yc ¤«¤é±¦¤Î tanContext/yomiContext ¤ò¥Ü¥Ä¤Ë¤·¤Æ¡¢¤½¤Î¤Ê¤«¤Ë³ÊÇ¼¤µ¤ì¤Æ¤¤¤¿
  ÆÉ¤ß¤ò yc ¤Ë¤¯¤Ã¤Ä¤±¤ë¡£
 */

void
doMuhenkan(uiContext d, yomiContext yc)
{
  tanContext tan, netan, st = (tanContext)yc;
  yomiContext yom;

  /* ¤Þ¤ºÌµÊÑ´¹½àÈ÷½èÍý¤ò¤¹¤ë */
  for (tan = st ; tan ; tan = tan->right) {
    if (tan->id == YOMI_CONTEXT) {
      yom = (yomiContext)tan;
      d->modec = (mode_context)yom;
      if (yom->nbunsetsu || (yom->generalFlags & CANNA_YOMI_CHIKUJI_MODE)) {
	tanMuhenkan(d, -1);
      }
      if (yom->jishu_kEndp) {
	leaveJishuMode(d, yom);
      }
      /* else ÆÉ¤ß¥â¡¼¥É¤Ç¤Ï¤Ê¤Ë¤â¤¹¤ëÉ¬Í×¤¬¤Ê¤¤¡£ */
    }
  }

  /* ¼¡¤ËÆÉ¤ß¤Ê¤É¤ÎÊ¸»ú¤ò¼è¤ê½Ð¤¹ */
  for (tan = st ; tan ; tan = netan) {
    netan = tan->right;
    if (tan->id == TAN_CONTEXT) {
      appendTan2Yomi(tan, yc);
      freeTanContext(tan);
    }
    else if (tan->id == YOMI_CONTEXT) {
      if ((yomiContext)tan != yc) {
	appendYomi2Yomi((yomiContext)tan, yc);
	freeYomiContext((yomiContext)tan);
      }
    }
  }
  yc->rCurs = yc->rStartp = yc->rEndp;
  yc->kCurs = yc->kRStartp = yc->kEndp;
  yc->right = (tanContext)0;
  d->modec = (mode_context)yc;
}

inline int
xTanKakuteiString(yomiContext yc, WCHAR_T *s, WCHAR_T *e)
{
  WCHAR_T *ss = s;
  int i, len, nbun;

  nbun = yc->bunlen ? yc->curbun : yc->nbunsetsu;

  for (i = 0 ; i < nbun ; i++) {
    RkwGoTo(yc->context, i);
    len = RkwGetKanji(yc->context, s, (int)(e - s));
    if (len < 0) {
      if (errno == EPIPE) {
	jrKanjiPipeError();
      }
      jrKanjiError = "\245\253\245\354\245\363\245\310\270\365\312\344\244\362"
	"\274\350\244\352\275\320\244\273\244\336\244\273\244\363\244\307"
	"\244\267\244\277";
                    /* ¥«¥ì¥ó¥È¸õÊä¤ò¼è¤ê½Ð¤»¤Þ¤»¤ó¤Ç¤·¤¿ */
    }
    else {
      s += len;
    }
  }
  RkwGoTo(yc->context, yc->curbun);

  if (yc->bunlen) {
    len = yc->kEndp - yc->kanjilen;
    if (((int)(e - s)) < len) {
      len = (int)(e - s);
    }
    WStrncpy(s, yc->kana_buffer + yc->kanjilen, len);
    s += len;
  }

  if ((yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) &&
      yc->cStartp < yc->kEndp) {
    len = xYomiKakuteiString(yc, s, e);
    s += len;
  }
  return (int)(s - ss);
}

inline int
doJishuKakutei(uiContext d, yomiContext yc)
{
  exitJishu(d);
  yc->jishu_kEndp = 0;
  return 0;
}


/*
  doKakutei -- ³ÎÄê½èÍý¤ò¤¹¤ë¡£

    st ¤«¤é et ¤ÎÄ¾Á°¤Þ¤Ç¤Î tanContext/yomiContext ¤ò³ÎÄê¤µ¤»¤ë
    s ¤«¤é e ¤ÎÈÏ°Ï¤Ë³ÎÄê·ë²Ì¤¬³ÊÇ¼¤µ¤ì¤ë¡£
    yc_return ¤Ï yomiContext ¤ò°ì¤Ä»Ä¤·¤ÆÍß¤·¤¤¾ì¹ç¤Ë¡¢»Ä¤Ã¤¿ yomiContext
    ¤ò³ÊÇ¼¤·¤ÆÊÖ¤¹¤¿¤á¤Î¥¢¥É¥ì¥¹¡£yc_return ¤¬¥Ì¥ë¤Ê¤é¡¢²¿¤â»Ä¤µ¤º free 
    ¤¹¤ë¡£
    et->left ¤Ï¸Æ¤Ó½Ð¤·¤¿¤È¤³¤í¤Ç 0 ¤Ë¤¹¤ë¤³¤È¡£

    ³ÎÄê¤·¤¿Ê¸»ú¤ÎÄ¹¤µ¤¬ÊÖ¤µ¤ì¤ë¡£

    ¤³¤Î´Ø¿ô¤ò¸Æ¤ó¤À¤é d->modec ¤¬²õ¤ì¤Æ¤¤¤ë¤Î¤ÇÆþ¤ìÄ¾¤µ¤Ê¤±¤ì¤Ð¤Ê¤é¤Ê¤¤ 

 */

int
doKakutei(uiContext d, tanContext st, tanContext et, WCHAR_T *s, WCHAR_T *e, yomiContext *yc_return)
{
  tanContext tan, netan;
  yomiContext yc;
  int len, res;
  WCHAR_T *ss = s;
  int katakanadef = 0, hiraganadef = 0;
  extern int auto_define;
#ifndef WIN
  WCHAR_T ytmpbuf[256];
#else
  WCHAR_T *ytmpbuf = (WCHAR_T *)malloc(sizeof(WCHAR_T) * 256);
  if (!ytmpbuf) {
    return 0;
  }
#endif

  /* ¤Þ¤º³ÎÄê½àÈ÷½èÍý¤ò¤¹¤ë */
  for (tan = st ; tan != et ; tan = tan->right) {
    if (tan->id == YOMI_CONTEXT) {
      yc = (yomiContext)tan;
      d->modec = (mode_context)yc;
      if (yc->jishu_kEndp) {
        if (auto_define) {
          if (yc->jishu_kc == JISHU_ZEN_KATA)
            katakanadef = 1;
#ifdef HIRAGANAAUTO
          if (yc->jishu_kc == JISHU_HIRA)
            hiraganadef = 1;
#endif
          WStrcpy(ytmpbuf, yc->kana_buffer);
        }
	doJishuKakutei(d, yc);
      }
      else if (!yc->bunlen && /* Ê¸Àá¿­¤Ð¤·½Ì¤áÃæ */
	       (!yc->nbunsetsu || /* ´Á»ú¤¬¤Ê¤¤¤«... */
		(yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE &&
		 yc->cStartp < yc->kEndp))) { /* ÆÉ¤ß¤¬¤Þ¤À¤¢¤ë .. */
	long savedFlag = yc->generalFlags;
	yc->generalFlags &= ~CANNA_YOMI_KAKUTEI;
	/* base-kakutei ¤À¤È doYomiKakutei() ¤¬¸Æ¤Ó½Ð¤·¤Æ¤¤¤ë
	   RomajiFlushYomi() ¤ÎÃæ¤Ç³ÎÄêÊ¸»úÎó¤¬È¯À¸¤·¡¢
	   ½èÍý¤¬ÌÌÅÝ¤Ë¤Ê¤ë¤Î¤Ç¤È¤ê¤¢¤¨¤º base-kakutei ¤ò¿²¤»¤ë */
	doYomiKakutei(d);
	yc->generalFlags = savedFlag;
      }
    }
  }

  /* ¼¡¤Ë³ÎÄêÊ¸»ú¤ò¼è¤ê½Ð¤¹ */
  for (tan = st ; tan != et ; tan = tan->right) {
    if (tan->id == TAN_CONTEXT) {
      len = extractTanString(tan, s, e);
    }
    else if (tan->id == YOMI_CONTEXT) {
      yc = (yomiContext)tan;
      d->modec = (mode_context)yc;
      if (yc->nbunsetsu || (yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE)) {
	len = xTanKakuteiString(yc, s, e);
      }else{ /* else ¤Ã¤Æ¤³¤È¤Ï¡¢ÆÉ¤ß¾õÂÖ¤·¤«¤Ê¤¤ */
	len = xYomiKakuteiString(yc, s, e);
      }
    }
    s += len;
  }
  res = (int)(s - ss);
  if (s < e) {
    *s++ = (WCHAR_T)'\0';
  }

  /* yomiInfo ¤Î½èÍý¤ò¤¹¤ë */
  if (yomiInfoLevel > 0) {
    d->kanji_status_return->info |= KanjiYomiInfo;
    for (tan = st ; tan != et ; tan = tan->right) {
      if (tan->id == TAN_CONTEXT) {
	len = extractTanYomi(tan, s, e);
      }
      else if (tan->id == YOMI_CONTEXT) {
	len = xYomiYomi((yomiContext)tan, s, e);
      }
      s += len;
    }
    if (s < e) {
      *s++ = (WCHAR_T)'\0';
    }
    
    if (yomiInfoLevel > 1) {
      for (tan = st ; tan != et ; tan = tan->right) {
	if (tan->id == TAN_CONTEXT) {
	  len = extractTanRomaji(tan, s, e);
	}
	else if (tan->id == YOMI_CONTEXT) {
	  len = xYomiRomaji((yomiContext)tan, s, e);
	}
	s += len;
      }
    }
    if (s < e) {
      *s++ = (WCHAR_T)'\0';
    }
  }

  /* ³ÎÄê¤Î»Ä½èÍý¤ò¹Ô¤¦ */
  if (yc_return) {
    *yc_return = (yomiContext)0;
  }
  for (tan = st ; tan != et ; tan = netan) {
    netan = tan->right;
    if (tan->id == TAN_CONTEXT) {
      freeTanContext(tan);
    }
    else { /* tan->id == YOMI_CONTEXT */
      yc = (yomiContext)tan;
      d->modec = (mode_context)yc;

      if (yc->nbunsetsu || (yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE)) {
	if (yc->bunlen) {
	  leaveAdjustMode(d, yc);
	}
	finishTanKakutei(d);
      }else{ /* ¤Ã¤Æ¤³¤È¤Ï¡¢ÆÉ¤ß¾õÂÖ¤·¤«¤Ê¤¤ */
	finishYomiKakutei(d);
      }
      if (yc_return && !*yc_return) {
	*yc_return = yc;
      }else{
	/* ¤È¤Ã¤Æ¤ª¤¯¤ä¤Ä¤¬¤â¤¦¤¢¤ë¤«¡¢¤¤¤é¤Ê¤¤¤Ê¤é¡¢º£¤Î¤Ï¼Î¤Æ¤ë */
	/* yc->context ¤Î close ¤Ï¤¤¤é¤Ê¤¤¤Î¤«¤Ê¤¢¡£1996.10.30 º£ */
	freeYomiContext(yc);
      }
    }
  }

  if (yc_return) {
    yc = *yc_return;
    if (yc) {
      yc->left = yc->right = (tanContext)0;
    }
  }
  d->modec = (mode_context)0;
  /* ²õ¤ì¤Æ¤¤¤ë¤«¤âÃÎ¤ì¤Ê¤¤¤Î¤Ç»È¤¤´Ö°ã¤ï¤Ê¤¤¤è¤¦¤Ë²õ¤·¿Ô¤¯¤·¤Æ¤ª¤¯ */

  /* »ú¼ïÊÑ´¹¤ÇÁ´³Ñ¥«¥¿¥«¥Ê¤ò³ÎÄê¤·¤¿¤é¡¢¼«Æ°ÅÐÏ¿¤¹¤ë */
  if (katakanadef || hiraganadef) {
    WCHAR_T line[ROMEBUFSIZE];
    int cnt;
    extern int defaultContext;
    extern char *kataautodic;
#ifdef HIRAGANAAUTO
    extern char *hiraautodic;
#endif

    WStraddbcpy(line, ytmpbuf, ROMEBUFSIZE);
    EWStrcat(line, " ");
    EWStrcat(line, "#T30");
    EWStrcat(line, " ");
    cnt = WStrlen(line);
    WStraddbcpy(line + cnt, ss, ROMEBUFSIZE - cnt);

    if (defaultContext == -1) {
      if ((KanjiInit() < 0) || (defaultContext == -1)) {
	jrKanjiError = KanjiInitError();
        makeGLineMessageFromString(d, jrKanjiError);
        goto return_res;
      }
    }

    if (katakanadef) {
      if (RkwDefineDic(defaultContext, kataautodic, line) != 0) {
        jrKanjiError = "\274\253\306\260\305\320\317\277\244\307\244\255"
                       "\244\336\244\273\244\363\244\307\244\267\244\277";
                         /* ¼«Æ°ÅÐÏ¿¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
        makeGLineMessageFromString(d, jrKanjiError);
      }else{
        if (cannaconf.auto_sync) {
          (void)RkwSync(defaultContext, kataautodic);
        }
      }
    }
    if (hiraganadef) {
#ifdef HIRAGANAAUTO
      if (RkwDefineDic(defaultContext, hiraautodic, line) != 0) {
        jrKanjiError = "\274\253\306\260\305\320\317\277\244\307\244\255"
                       "\244\336\244\273\244\363\244\307\244\267\244\277";
                         /* ¼«Æ°ÅÐÏ¿¤Ç¤­¤Þ¤»¤ó¤Ç¤·¤¿ */
        makeGLineMessageFromString(d, jrKanjiError);
      }else{
        if (cannaconf.auto_sync) {
          (void)RkwSync(defaultContext, hiraautodic);
        }
      }
#endif
    }
  }
 return_res:
#ifdef WIN
  free(ytmpbuf);
#endif
  return res;
}

/*
  cutOffLeftSide -- º¸¤ÎÊý¤Î tanContext ¤ò³ÎÄê¤µ¤»¤ë¡£

  n -- º¸¤Ë n ¸Ä»Ä¤·¤Æ³ÎÄê¤¹¤ë¡£

 */

int
cutOffLeftSide(uiContext d, yomiContext yc, int n)
{
  int i;
  tanContext tan = (tanContext)yc, st;

  for (i = 0 ; i < n && tan ; i++) {
    tan = tan->left;
  }
  if (tan && tan->left) {
    st = tan->left;
    while (st->left) {
      st = st->left;
    }
    d->nbytes = doKakutei(d, st, tan, d->buffer_return,
			  d->buffer_return + d->n_buffer, (yomiContext *)0);
    d->modec = (mode_context)yc;
    tan->left = (tanContext)0;
    return 1;
  }
  return 0;
}

extern KanjiModeRec cy_mode;

int YomiKakutei (uiContext);

int
YomiKakutei(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  tanContext leftmost;
  int len, res;
  WCHAR_T *s = d->buffer_return, *e = s + d->n_buffer;
  mode_context next = yc->next;
  KanjiMode prev = yc->prevMode;
  long prevflags;

  prevflags = (yc->id == YOMI_CONTEXT) ?
    yc->generalFlags : ((tanContext)yc)->generalFlags;

  d->kanji_status_return->length = 0;
  d->nbytes = 0;

  leftmost = (tanContext)yc;
  while (leftmost->left) {
    leftmost = leftmost->left;
  }

  len = doKakutei(d, leftmost, (tanContext)0, s, e, &yc);

  if (!yc) {
    yc = newFilledYomiContext(next, prev);
    yc->generalFlags = prevflags;
    yc->minorMode = getBaseMode(yc);
  }
  d->modec = (mode_context)yc;
  if (!yc) {
    freeRomeStruct(d);
    return -1; /* ËÜÅö¤Ë¤³¤ì¤Ç¤¤¤¤¤Î¤«¡©¢ª¤¤¤¤ 1994.2.23 kon */
  }
  d->current_mode = yc->curMode;
  d->nbytes = len;

  res = YomiExit(d, d->nbytes);
  currentModeInfo(d);
  return res;
}

/* Á´¤¯ 0 ¤Ë¤¹¤ë¤ï¤±¤Ç¤Ï¤Ê¤¤¤Î¤ÇÃí°Õ */

void
clearYomiContext(yomiContext yc)
{
  yc->rStartp = 0;
  yc->rCurs = 0;
  yc->rEndp = 0;
  yc->romaji_buffer[0] = (WCHAR_T)0;
  yc->rAttr[0] = SENTOU;
  yc->kRStartp = 0;
  yc->kCurs = 0;
  yc->kEndp = 0;
  yc->kana_buffer[0] = (WCHAR_T)0;
  yc->kAttr[0] = SENTOU;
  yc->pmark = yc->cmark = 0;
  yc->englishtype = CANNA_ENG_KANA;
  yc->cStartp = yc->cRStartp = 0;
  yc->jishu_kEndp = 0;
}

inline int
clearChikujiContext(yomiContext yc)
{
  clearYomiContext(yc);
  yc->status &= CHIKUJI_NULL_STATUS;
  yc->ys = yc->ye = yc->cStartp;
  clearHenkanContext(yc);
  return 0;
}


/* RomajiClearYomi(d) ¥æ¡¼¥Æ¥£¥ê¥Æ¥£´Ø¿ô
 *
 * ¤³¤Î´Ø¿ô¤Ï¡¢(uiContext)d ¤ËÃß¤¨¤é¤ì¤Æ¤¤¤ëÆÉ¤ß¤Î¾ðÊó 
 * ¤ò¥¯¥ê¥¢¤¹¤ë¡£
 *
 * ¡ÚºîÍÑ¡Û   
 *
 *    ÆÉ¤ß¤ò¥¯¥ê¥¢¤¹¤ë¡£
 *
 * ¡Ú°ú¿ô¡Û
 *
 *    d  (uiContext)  ¥«¥Ê´Á»úÊÑ´¹¹½Â¤ÂÎ
 *
 * ¡ÚÌá¤êÃÍ¡Û
 *
 *    ¤Ê¤·¡£
 *
 * ¡ÚÉûºîÍÑ¡Û
 *
 *    yc->rEndp = 0;
 *    yc->kEndp = 0; Åù
 */

void
RomajiClearYomi(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) {
    if (yc->context >= 0) {
      RkwEndBun(yc->context, 0);
      abandonContext(d, yc);
    }
    clearChikujiContext(yc);
  }
  else {
    clearYomiContext(yc);
  }
}

int YomiExit(uiContext d, int retval)
{
  yomiContext yc = (yomiContext)d->modec;

  RomajiClearYomi(d);

  /* ³ÎÄê¤·¤Æ¤·¤Þ¤Ã¤¿¤é¡¢ÆÉ¤ß¤¬¤Ê¤¯¤Ê¤ë¤Î¤Ç¦Õ¥â¡¼¥É¤ËÁ«°Ü¤¹¤ë¡£ */
  restoreChikujiIfBaseChikuji(yc);
  d->current_mode = yc->curMode = yc->myEmptyMode;
  d->kanji_status_return->info |= KanjiEmptyInfo;

  return checkIfYomiExit(d, retval);
}

/* RomajiStoreYomi(d, kana) ¥æ¡¼¥Æ¥£¥ê¥Æ¥£´Ø¿ô
 *
 * ¤³¤Î´Ø¿ô¤Ï¡¢(uiContext)d ¤ËÆÉ¤ß¤Î¾ðÊó¤ò¥¹¥È¥¢¤¹¤ë¡£
 *
 * ¡ÚºîÍÑ¡Û   
 *
 *    ÆÉ¤ß¤ò³ÊÇ¼¤¹¤ë¡£
 *
 * ¡Ú°ú¿ô¡Û
 *
 *    d    (uiContext)  ¥«¥Ê´Á»úÊÑ´¹¹½Â¤ÂÎ
 *    kana (WCHAR_T *) ¤«¤ÊÊ¸»úÎó
 *    roma (WCHAR_T *) ¥í¡¼¥Þ»úÊ¸»úÎó
 * ¡ÚÌá¤êÃÍ¡Û
 *
 *    ¤Ê¤·¡£
 *
 * ¡ÚÉûºîÍÑ¡Û
 *
 *    yc->rEndp = WStrlen(kana);
 *    yc->kEndp = WStrlen(kana); Åù
 */

void
RomajiStoreYomi(uiContext d, WCHAR_T *kana, WCHAR_T *roma)
{
  int i, ylen, rlen, additionalflag;
  yomiContext yc = (yomiContext)d->modec;

  rlen = ylen = WStrlen(kana);
  if (roma) {
    rlen = WStrlen(roma);
    additionalflag = 0;
  }
  else {
    additionalflag = SENTOU;
  }
  WStrcpy(yc->romaji_buffer, (roma ? roma : kana));
  yc->rStartp = rlen;
  yc->rCurs = rlen;
  yc->rEndp = rlen;
  WStrcpy(yc->kana_buffer, kana);
  yc->kRStartp = ylen;
  yc->kCurs = ylen;
  yc->kEndp = ylen;
  for (i = 0 ; i < rlen ; i++) {
    yc->rAttr[i] = additionalflag;
  }
  yc->rAttr[0] |= SENTOU;
  yc->rAttr[i] = SENTOU;
  for (i = 0 ; i < ylen ; i++) {
    yc->kAttr[i] = HENKANSUMI | additionalflag;
  }
  yc->kAttr[0] |= SENTOU;
  yc->kAttr[i] = SENTOU;
}

/*
  KanaDeletePrevious -- ¿§¡¹¤Ê¤È¤³¤í¤«¤é»È¤ï¤ì¤ë¡£

*/

int KanaDeletePrevious(uiContext d)
{
  int howManyDelete;
  int prevflag;
  yomiContext yc = (yomiContext)d->modec;

  /* ¥«¡¼¥½¥ë¤Îº¸Â¦¤òºï½ü¤¹¤ë¤Î¤À¤¬¡¢¥«¡¼¥½¥ë¤Îº¸Â¦¤¬

    (1) ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¤ÎÅÓÃæ¤Î¾õÂÖ¤Ç¤¢¤ê¡¢¥¢¥ë¥Õ¥¡¥Ù¥Ã¥È¤Ë¤Ê¤Ã¤Æ¤¤¤ë»þ¡¢
    (2) ÀèÆ¬¤Ç¤¢¤ë¤È¤­

    ¤Ê¤É¤¬¹Í¤¨¤é¤ì¤ë¡£(Í×¤ÏÀ°Íý¤µ¤ì¤Æ¤¤¤Ê¤¤¤Î¤Ç¤â¤Ã¤È¤¢¤ê¤½¤¦)
   */

  if (!yc->kCurs) { /* º¸Ã¼¤Î¤È¤­ */
    d->kanji_status_return->length = -1;
    return 0;
  }
  yc->last_rule = 0;
  howManyDelete = howFarToGoBackward(yc);
  if (howManyDelete > 0 && (yc->generalFlags & CANNA_YOMI_BREAK_ROMAN)) {
    yc->generalFlags &= ~CANNA_YOMI_BREAK_ROMAN;
    yc->rStartp = yc->rCurs - 1;
    while ( yc->rStartp > 0 && !(yc->rAttr[yc->rStartp] & SENTOU) ) {
      yc->rStartp--;
    }
    romajiReplace (-1, (WCHAR_T *)NULL, 0, 0);
    yc->kRStartp = yc->kCurs - 1;
    while ( yc->kRStartp > 0 && !(yc->kAttr[yc->kRStartp] & SENTOU) )
      yc->kRStartp--;
    prevflag = (yc->kAttr[yc->kRStartp] & SENTOU);
    kanaReplace(yc->kRStartp - yc->kCurs, 
		yc->romaji_buffer + yc->rStartp,
		yc->rCurs - yc->rStartp,
		0);
    yc->kAttr[yc->kRStartp] |= prevflag;
    yc->n_susp_chars = 0; /* ¤È¤ê¤¢¤¨¤º¥¯¥ê¥¢¤·¤Æ¤ª¤¯ */
    makePhonoOnBuffer(d, yc, (unsigned char)0, 0, 0);
  }
  else {
    if ( yc->kAttr[yc->kCurs - howManyDelete] & HENKANSUMI ) {
      if (yc->kAttr[yc->kCurs - howManyDelete] & SENTOU) { 
	/* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¤ÎÀèÆ¬¤À¤Ã¤¿¤é */
	if (yc->kAttr[yc->kCurs] & SENTOU) {
	  int n;
	
	  /* ÀèÆ¬¤À¤Ã¤¿¤é¥í¡¼¥Þ»ú¤âÀèÆ¬¥Þ¡¼¥¯¤¬Î©¤Ã¤Æ¤¤¤ë¤È¤³¤í¤Þ¤ÇÌá¤¹ */
	
	  for (n = 1 ; yc->rCurs > 0 && !(yc->rAttr[--yc->rCurs] & SENTOU) ;) {
	    n++;
	  }
	  moveStrings(yc->romaji_buffer, yc->rAttr,
		      yc->rCurs + n, yc->rEndp,-n);
	  if (yc->rCurs < yc->rStartp) {
	    yc->rStartp = yc->rCurs;
	  }
	  yc->rEndp -= n;
	}
	else {
	  yc->kAttr[yc->kCurs] |= SENTOU;
	}
      }
    }
    else {
      romajiReplace(-howManyDelete, (WCHAR_T *)NULL, 0, 0);
    }
    kanaReplace(-howManyDelete, (WCHAR_T *)NULL, 0, 0);
  }
  debug_yomi(yc);
  return(0);
}


static int
YomiDeletePrevious(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  KanaDeletePrevious(d);
  if (!yc->kEndp) {
    if (yc->savedFlags & CANNA_YOMI_MODE_SAVED) {
      restoreFlags(yc);
    }
    if (yc->left || yc->right) {
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
  return 0;
}

static int YomiDeleteNext (uiContext);

static int
YomiDeleteNext(uiContext d)
{
  int howManyDelete;
  yomiContext yc = (yomiContext)d->modec;

  if (chikujip(yc) && (yc->status & CHIKUJI_ON_BUNSETSU)) {
    return NothingChangedWithBeep(d);
  }

  if (yc->kCurs == yc->kEndp) {
    /* ±¦Ã¼¤À¤«¤é¤Ê¤Ë¤â¤·¤Ê¤¤¤Î¤Ç¤·¤ç¤¦¤Í¤§ */
    d->kanji_status_return->length = -1;
    return 0;
  }

  fitmarks(yc);

  yc->last_rule = 0;
  howManyDelete = howFarToGoForward(yc);

  if (yc->kAttr[yc->kCurs] & SENTOU) {
    if (yc->kAttr[yc->kCurs + howManyDelete] & SENTOU) {
      int n = 1;
      while ( !(yc->rAttr[++yc->rCurs] & SENTOU) )
	n++;
      moveStrings(yc->romaji_buffer, yc->rAttr, yc->rCurs, yc->rEndp, -n);
      yc->rCurs -= n;
      yc->rEndp -= n;
    }
    else {
      yc->kAttr[yc->kCurs + howManyDelete] |= SENTOU;
    }
  }
  kanaReplace(howManyDelete, (WCHAR_T *)NULL, 0, 0);
  /* ¤³¤³¤Þ¤Çºï½ü½èÍý */

  if (yc->cStartp < yc->kEndp) { /* ÆÉ¤ß¤¬¤Þ¤À¤¢¤ë */
    if (yc->kCurs < yc->ys) {
      yc->ys = yc->kCurs; /* ¤³¤ó¤Ê¤â¤ó¤Ç¤¤¤¤¤Î¤Ç¤·¤ç¤¦¤«¡© */
    }
  }
  else if (yc->nbunsetsu) { /* ÆÉ¤ß¤Ï¤Ê¤¤¤¬Ê¸Àá¤Ï¤¢¤ë */
    if (RkwGoTo(yc->context, yc->nbunsetsu - 1) == -1) {
      return makeRkError(d, "\312\270\300\341\244\316\260\334\306\260\244\313"
	"\274\272\307\324\244\267\244\336\244\267\244\277");
                            /* Ê¸Àá¤Î°ÜÆ°¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    }
    yc->kouhoCount = 0;
    yc->curbun = yc->nbunsetsu - 1;
    moveToChikujiTanMode(d);
  }
  else { /* ÆÉ¤ß¤âÊ¸Àá¤â¤Ê¤¤ */
    if (yc->savedFlags & CANNA_YOMI_MODE_SAVED) {
      restoreFlags(yc);
    }
    if (yc->left || yc->right) {
      removeCurrentBunsetsu(d, (tanContext)yc);
    }
    else {
      /* Ì¤³ÎÄêÊ¸»úÎó¤¬Á´¤¯¤Ê¤¯¤Ê¤Ã¤¿¤Î¤Ê¤é¡¢¦Õ¥â¡¼¥É¤ËÁ«°Ü¤¹¤ë */
      restoreChikujiIfBaseChikuji(yc);
      d->current_mode = yc->curMode = yc->myEmptyMode;
      d->kanji_status_return->info |= KanjiEmptyInfo;
    }
    currentModeInfo(d);
  }
  makeYomiReturnStruct(d);
  return 0;
}

static int YomiKillToEndOfLine (uiContext);

static int
YomiKillToEndOfLine(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  romajiReplace (yc->rEndp - yc->rCurs, (WCHAR_T *)NULL, 0, 0);
  kanaReplace   (yc->kEndp - yc->kCurs, (WCHAR_T *)NULL, 0, 0);

  fitmarks(yc);

  if (!yc->kEndp) {
    if (yc->savedFlags & CANNA_YOMI_MODE_SAVED) {
      restoreFlags(yc);
    }
    if (yc->left || yc->right) {
      removeCurrentBunsetsu(d, (tanContext)yc);
    }
    else {
      /* Ì¤³ÎÄêÊ¸»úÎó¤¬Á´¤¯¤Ê¤¯¤Ê¤Ã¤¿¤Î¤Ê¤é¡¢¦Õ¥â¡¼¥É¤ËÁ«°Ü¤¹¤ë */
      restoreChikujiIfBaseChikuji(yc);
      d->current_mode = yc->curMode = yc->myEmptyMode;
      d->kanji_status_return->info |= KanjiEmptyInfo;
    }
    currentModeInfo(d);
  }
  makeYomiReturnStruct(d);
  return 0;
}

static int YomiQuit (uiContext);

static int
YomiQuit(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  /* Ì¤³ÎÄêÊ¸»úÎó¤òºï½ü¤¹¤ë */
  RomajiClearYomi(d);

  if (yc->left || yc->right) {
    removeCurrentBunsetsu(d, (tanContext)yc);
  }
  else {
    /* Ì¤³ÎÄêÊ¸»úÎó¤¬Á´¤¯¤Ê¤¯¤Ê¤Ã¤¿¤Î¤Ç¡¢¦Õ¥â¡¼¥É¤ËÁ«°Ü¤¹¤ë */
    restoreChikujiIfBaseChikuji(yc);
    d->current_mode = yc->curMode = yc->myEmptyMode;
    d->kanji_status_return->info |= KanjiEmptyInfo;
  }
  makeYomiReturnStruct(d);
  currentModeInfo(d);
  return checkIfYomiQuit(d, 0);
}

coreContext
newCoreContext(void)
{
  coreContext cc;

  cc = (coreContext)malloc(sizeof(coreContextRec));
  if (cc) {
    cc->id = CORE_CONTEXT;
  }
  return cc;
}

static
int simplePopCallback(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  popCallback(d);
  currentModeInfo(d);
  return retval;
}

int alphaMode(uiContext d)
{
  extern KanjiModeRec alpha_mode;
  coreContext cc;
  char *bad = "\245\341\245\342\245\352\244\254\302\255\244\352\244\336"
	"\244\273\244\363";
              /* ¥á¥â¥ê¤¬Â­¤ê¤Þ¤»¤ó */

  cc = newCoreContext();
  if (cc == (coreContext)0) {
    makeGLineMessageFromString(d, bad);
    return 0;
  }
  if (pushCallback(d, d->modec,
		   NO_CALLBACK, simplePopCallback,
                   simplePopCallback, NO_CALLBACK) == 0) {
    freeCoreContext(cc);
    makeGLineMessageFromString(d, bad);
    return 0;
  }
  cc->prevMode = d->current_mode;
  cc->next = d->modec;
  cc->majorMode =
    cc->minorMode = CANNA_MODE_AlphaMode;
  d->current_mode = &alpha_mode;
  d->modec = (mode_context)cc;
  return 0;
}

/* Quoted Insert Mode -- °úÍÑÆþÎÏ¥â¡¼¥É¡£

   ¤³¤Î¥â¡¼¥É¤Ç¤Ï¼¡¤Î°ìÊ¸»ú¤ÏÈÝ±þÌµ¤·¤Ë¤½¤Î¤Þ¤ÞÆþÎÏ¤µ¤ì¤ë¡£

 */

static int exitYomiQuotedInsert (uiContext, int, mode_context);

static
int exitYomiQuotedInsert(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  popCallback(d);
  return retval;
}

static
int YomiInsertQuoted(uiContext d)
{
  unsigned char ch;
  coreContext cc = (coreContext)d->modec;
  yomiContext yc;

  ch = (unsigned char)*(d->buffer_return);

  if (IrohaFunctionKey(ch)) {
    d->kanji_status_return->length = -1;
    d->kanji_status_return->info = 0;
    return 0;
  } else {
    d->current_mode = cc->prevMode;
    d->modec = cc->next;
    free(cc);

    yc = (yomiContext)d->modec;

    romajiReplace (0, d->buffer_return, d->nbytes, 0);
    kanaReplace   (0, d->buffer_return, d->nbytes, HENKANSUMI);
    yc->rStartp = yc->rCurs;
    yc->kRStartp = yc->kCurs;
    makeYomiReturnStruct(d);
    currentModeInfo(d);
    d->status = EXIT_CALLBACK;
    return 0;
  }
}

static int yomiquotedfunc (uiContext, KanjiMode, int, int, int);

static
int yomiquotedfunc(uiContext d, KanjiMode mode, int whattodo, int key, int fnum)
     /* ARGSUSED */
{
  switch (whattodo) {
  case KEY_CALL:
    return YomiInsertQuoted(d);
  case KEY_CHECK:
    return 1;
  case KEY_SET:
    return 0;
  }
  /* NOTREACHED */
}

static KanjiModeRec yomi_quoted_insert_mode = {
  yomiquotedfunc,
  0, 0, 0,
};

inline void
yomiQuotedInsertMode(uiContext d)
{
  coreContext cc;

  cc = newCoreContext();
  if (cc == 0) {
    NothingChangedWithBeep(d);
    return;
  }
  cc->prevMode = d->current_mode;
  cc->next = d->modec;
  cc->majorMode = d->majorMode;
  cc->minorMode = CANNA_MODE_QuotedInsertMode;
  if (pushCallback(d, d->modec,
                   NO_CALLBACK, exitYomiQuotedInsert,
                   NO_CALLBACK, NO_CALLBACK) == (struct callback *)0) {
    freeCoreContext(cc);
    NothingChangedWithBeep(d);
    return;
  }
  d->modec = (mode_context)cc;
  d->current_mode = &yomi_quoted_insert_mode;
  currentModeInfo(d);
  return;
}

int YomiQuotedInsert(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  d->nbytes = 0;

  if (yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) {
    if (yc->status & CHIKUJI_ON_BUNSETSU) {
      if (yc->kEndp != yc->kCurs) {
	yc->rStartp = yc->rCurs = yc->rEndp;
	yc->kRStartp = yc->kCurs = yc->kEndp;
      }
      yc->status &= ~CHIKUJI_ON_BUNSETSU;
      yc->status |= CHIKUJI_OVERWRAP;
    }
    else if (yc->rEndp == yc->rCurs) {
      yc->status &= ~CHIKUJI_OVERWRAP;
    }
  }

  if (forceRomajiFlushYomi(d))
    return(d->nbytes);

  fitmarks(yc);

  yomiQuotedInsertMode(d);
  d->kanji_status_return->length = -1;
  return 0;
}

inline int
mapAsKuten(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  int i, j, ch, len, clen, kanalen, pos;
  char tmpbuf[4];
  WCHAR_T *hexbuf;
  WCHAR_T buf[2];
  static int allowTwoByte = 1;

  tmpbuf[0] = tmpbuf[1] = tmpbuf[2] = tmpbuf[3] = '\0';

  if (yc->kCurs < yc->cmark) {
    int tmp = yc->kCurs;
    yc->kCurs = yc->cmark;
    yc->cmark = tmp;
    kPos2rPos(yc, 0, yc->kCurs, (int *) 0, &tmp);
    yc->rCurs = tmp;
  }
  else if (yc->kCurs == yc->cmark) {
    yc->kCurs = yc->kRStartp = yc->kEndp;
    yc->rCurs = yc->rStartp = yc->rEndp;
  }

  if (*yc->romaji_buffer == 'x' || *yc->romaji_buffer == 'X')
    len = yc->rCurs - 1;
  else
    len = yc->rCurs;
  if (len > 6) {
    return 0;
  }
  hexbuf = yc->romaji_buffer + yc->rCurs - len;

  kPos2rPos(yc, 0, yc->cmark, (int *) 0, &pos);

  if (hexbuf < yc->romaji_buffer + pos) {
    if (hexbuf + 6 < yc->romaji_buffer + pos) {
      return 0;
    }
  }
  for (i = 0, j = 1; i < len; i++) {
    ch = *(hexbuf + i);
    if ('0' <= ch && ch <= '9')
      tmpbuf[j] = tmpbuf[j] * 10 + (ch - '0');
    else if (ch == '-' && j == 1)
      j++;
    else
      return 0;
  }
  tmpbuf[2] = (char)((0x80 | tmpbuf[2]) + 0x20);
  if (tmpbuf[1] < 0x5f) {
    tmpbuf[1] = (char)((0x80 | tmpbuf[1]) + 0x20);
  }
  else {
    tmpbuf[1] = (char)((0x80 | tmpbuf[1]) - 0x5e + 0x20);
    tmpbuf[0] = (char)0x8f; /* SS3 */
  }
  if ((unsigned char)tmpbuf[1] < 0xa1 ||
      0xfe < (unsigned char)tmpbuf[1] ||
      (len > 2 && ((unsigned char)tmpbuf[2] < 0xa1 ||
                   0xfe < (unsigned char)tmpbuf[2]))) {
    return 0;
  }
  if (hexbuf[-1] == 'x' || hexbuf[-1] == 'X') {
    tmpbuf[0] = (char)0x8f;/*SS3*/
    len++;
  }
  if (tmpbuf[0]) {
    clen = MBstowcs(buf, tmpbuf, 2);
  }
  else {
    clen = MBstowcs(buf, tmpbuf + 1, 2);
  }
  for (i = 0, kanalen = 0 ; i < len ; i++) {
    if (yc->rAttr[yc->rCurs - len + i] & SENTOU) {
      do {
	kanalen++;
      } while (!(yc->kAttr[yc->kCurs - kanalen] & SENTOU));
      yc->rAttr[yc->rCurs - len + i] &= ~SENTOU;
    }
  }
  yc->rAttr[yc->rCurs - len] |= SENTOU;
  kanaReplace(-kanalen, buf, clen, HENKANSUMI);
  yc->kAttr[yc->kCurs - clen] |= SENTOU;
  yc->kRStartp = yc->kCurs;
  yc->rStartp = yc->rCurs;
  yc->pmark = yc->cmark;
  yc->cmark = yc->kCurs;
  yc->n_susp_chars = 0; /* ¥µ¥¹¥Ú¥ó¥É¤·¤Æ¤¤¤ëÊ¸»ú¤¬¤¢¤ë¾ì¹ç¤¬¤¢¤ë¤Î¤Ç¥¯¥ê¥¢ */
  return 1;
}

inline int
mapAsHex(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  int i, ch, len = 4, clen, kanalen, pos;
  char tmpbuf[8], *a;
  WCHAR_T *hexbuf;
  WCHAR_T buf[2];
  static int allowTwoByte = 1;
  extern struct CannaConfig cannaconf;

  if (yc->kCurs < yc->cmark) {
    int tmp = yc->kCurs;
    yc->kCurs = yc->cmark;
    yc->cmark = tmp;
    kPos2rPos(yc, 0, yc->kCurs, (int *)0, &tmp);
    yc->rCurs = tmp;
  }
  else if (yc->kCurs == yc->cmark) {
    yc->kCurs = yc->kRStartp = yc->kEndp;
    yc->rCurs = yc->rStartp = yc->rEndp;
  }

  hexbuf = yc->romaji_buffer + yc->rCurs - 4;

  kPos2rPos(yc, 0, yc->cmark, (int *)0, &pos);

  if (hexbuf < yc->romaji_buffer + pos) {
    if (!allowTwoByte || hexbuf + 2 < yc->romaji_buffer + pos) {
      return 0;
    }
    hexbuf += 2;
    len = 2;
  }
 retry:
  for (i = 0, a = tmpbuf + 1; i < len ; i++) {
    ch = *(hexbuf + i);
    if ('0' <= ch && ch <= '9')
      ch -= '0';
    else if ('A' <= ch && ch <= 'F')
      ch -= 'A' - 10;
    else if ('a' <= ch && ch <= 'f')
      ch -= 'a' - 10;
    else if (allowTwoByte && i < 2 && 2 < len) {
      hexbuf += 2;
      len = 2;
      goto retry;
    }
    else {
      return 0;
    }
    *a++ = ch;
  }
  if (cannaconf.code_input == CANNA_CODE_SJIS) { /* sjis ¥³¡¼¥É¤À¤Ã¤¿¤é */
    char eucbuf[4];  /* SS3 ¤Î¤³¤È¤¬¤¢¤ë¤¿¤á */

    tmpbuf[1] = tmpbuf[1] * 16 + tmpbuf[2];
    if (len > 2) {
      tmpbuf[2] = tmpbuf[3] * 16 + tmpbuf[4];
      tmpbuf[3] = '\0';
    }
    else {
      tmpbuf[2] = '\0';
    }
    if ((unsigned char)tmpbuf[1] < 0x81 ||
        (0x9f < (unsigned char)tmpbuf[1] && (unsigned char)tmpbuf[1] < 0xe0) ||
        0xfc < (unsigned char)tmpbuf[1] ||
        (len > 2 && ((unsigned char)tmpbuf[2] < 0x40 ||
                     0xfc < (unsigned char)tmpbuf[2] ||
                     (unsigned char)tmpbuf[2] == 0x7f))) {
      return 0;
    }
    RkCvtEuc((unsigned char *)eucbuf, sizeof(eucbuf),
                        (unsigned char *)tmpbuf + 1, 2);
    clen = MBstowcs(buf, eucbuf, 2);
  }
  else {
    tmpbuf[1] = 0x80 | (tmpbuf[1] * 16 + tmpbuf[2]);
    if (len > 2) {
      tmpbuf[2] = 0x80 | (tmpbuf[3] * 16 + tmpbuf[4]);
      tmpbuf[3] = '\0';
    }
    else {
      tmpbuf[2] = '\0';
    }
    if ((unsigned char)tmpbuf[1] < 0xa1 ||
        0xfe < (unsigned char)tmpbuf[1] ||
        (len > 2 && ((unsigned char)tmpbuf[2] < 0xa1 ||
                      0xfe < (unsigned char)tmpbuf[2]))) {
      return 0;
    }
    if (len == 2) {
      tmpbuf[1] &= 0x7f;
    }
    if (hexbuf > yc->romaji_buffer
        && len > 2 && (hexbuf[-1] == 'x' || hexbuf[-1] == 'X')) {
      tmpbuf[0] = (char)0x8f;/*SS3*/
      len++;
      clen = MBstowcs(buf, tmpbuf, 2);
    }
    else {
      clen = MBstowcs(buf, tmpbuf + 1, 2);
    }
  }
  for (i = 0, kanalen = 0 ; i < len ; i++) {
    if (yc->rAttr[yc->rCurs - len + i] & SENTOU) {
      do {
	kanalen++;
      } while (!(yc->kAttr[yc->kCurs - kanalen] & SENTOU));
      yc->rAttr[yc->rCurs - len + i] &= ~SENTOU;
    }
  }
  yc->rAttr[yc->rCurs - len] |= SENTOU;
  kanaReplace(-kanalen, buf, clen, HENKANSUMI);
  yc->kAttr[yc->kCurs - clen] |= SENTOU;
  yc->kRStartp = yc->kCurs;
  yc->rStartp = yc->rCurs;
  yc->pmark = yc->cmark;
  yc->cmark = yc->kCurs;
  yc->n_susp_chars = 0; /* ¥µ¥¹¥Ú¥ó¥É¤·¤Æ¤¤¤ëÊ¸»ú¤¬¤¢¤ë¾ì¹ç¤¬¤¢¤ë¤Î¤Ç¥¯¥ê¥¢ */
  return 1;
}

/* ConvertAsHex -- £±£¶¿Ê¤È¤ß¤Ê¤·¤Æ¤ÎÊÑ´¹ 

  ¥í¡¼¥Þ»úÆþÎÏ¤µ¤ì¤ÆÈ¿Å¾É½¼¨¤µ¤ì¤Æ¤¤¤ëÊ¸»úÎó¤ò£±£¶¿Ê¤ÇÉ½¼¨¤µ¤ì¤Æ¤¤¤ë¥³¡¼¥É¤È
  ¤ß¤Ê¤·¤ÆÊÑ´¹¤¹¤ë¡£

  (MSB¤Ï£°¤Ç¤â£±¤Ç¤âÎÉ¤¤)

  */

static int ConvertAsHex (uiContext);

static
int ConvertAsHex(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  extern struct CannaConfig cannaconf;

  if (yc->henkanInhibition & CANNA_YOMI_INHIBIT_ASHEX) {
    return NothingChangedWithBeep(d);
  }
  if (yc->savedFlags & CANNA_YOMI_MODE_SAVED) {
    restoreFlags(yc);
    currentModeInfo(d);
  }
  if (cannaconf.code_input != CANNA_CODE_KUTEN) {
    if (!mapAsHex(d)) {
      return NothingChangedWithBeep(d);
    }
  }
  else {
    if (!mapAsKuten(d)) {
      return NothingChangedWithBeep(d);
    }
  }
  if (yc->kCurs - 1 < yc->ys) {
    yc->ys = yc->kCurs - 1;
  }
  makeYomiReturnStruct(d);
  return 0;
}

/*
  convertAsHex  £±£¶¿Ê¤Î¿ô»ú¤ò´Á»úÊ¸»ú¤ËÊÑ´¹

  ¤³¤ì¤ÏÆâÉôÅª¤Ë»ÈÍÑ¤¹¤ë¤¿¤á¤Î¥ë¡¼¥Á¥ó¤Ç¤¢¤ë¡£d->romaji_buffer ¤Ë´Þ¤Þ
  ¤ì¤ëÊ¸»úÎó¤ò£±£¶¿Ê¤ÇÉ½¤µ¤ì¤¿´Á»ú¥³¡¼¥É¤Ç¤¢¤ë¤È¤ß¤Ê¤·¤Æ¡¢¤½¤Î¥³¡¼¥É¤Ë
  ¤è¤Ã¤ÆÉ½¸½¤µ¤ì¤ë´Á»úÊ¸»ú¤ËÊÑ´¹¤¹¤ë¡£ÊÑ´¹¤·¤¿Ê¸»úÎó¤Ï buffer_return 
  ¤Ë³ÊÇ¼¤¹¤ë¡£¥ê¥¿¡¼¥óÃÍ¤Ï¥¨¥é¡¼¤¬¤Ê¤±¤ì¤Ð buffer_return ¤Ë³ÊÇ¼¤·¤¿Ê¸
  »úÎó¤ÎÄ¹¤µ¤Ç¤¢¤ë(ÄÌ¾ï¤Ï£²¤Ç¤¢¤ë)¡£¥¨¥é¡¼¤¬È¯À¸¤·¤Æ¤¤¤ë»þ¤Ï¡Ý£±¤¬³ÊÇ¼
  ¤µ¤ì¤ë¡£

  ¥â¡¼¥É¤ÎÊÑ¹¹Åù¤Î½èÍý¤Ï¤³¤Î´Ø¿ô¤Ç¤Ï¹Ô¤ï¤ì¤Ê¤¤¡£

  ¤Þ¤¿¥Ð¥Ã¥Õ¥¡¤Î¥¯¥ê¥¢¤Ê¤É¤â¹Ô¤ï¤Ê¤¤¤Î¤ÇÃí°Õ¤¹¤ë¤Ù¤­¤Ç¤¢¤ë¡£

  <Ìá¤êÃÍ>
    Àµ¤·¤¯£±£¶¿Ê¤ËÊÑ´¹¤Ç¤­¤¿¾ì¹ç¤Ï£±¤½¤¦¤Ç¤Ê¤¤»þ¤Ï£°¤¬ÊÖ¤ë¡£
*/

#ifdef WIN
static 
#endif
int cvtAsHex(uiContext d, WCHAR_T *buf, WCHAR_T *hexbuf, int hexlen)
{
  int i;
  char tmpbuf[5], *a, *b;
  WCHAR_T rch;
  
  if (hexlen != 4) { /* ÆþÎÏ¤µ¤ì¤¿Ê¸»úÎó¤ÎÄ¹¤µ¤¬£´Ê¸»ú¤Ç¤Ê¤¤¤Î¤Ç¤¢¤ì¤ÐÊÑ´¹
			¤·¤Æ¤¢¤²¤Ê¤¤ */
    d->kanji_status_return->length = -1;
    return 0;
  }
  for (i = 0, a = tmpbuf; i < 4 ; i++) {
    rch = hexbuf[i]; /* ¤Þ¤º°ìÊ¸»ú¼è¤ê½Ð¤·¡¢£±£¶¿Ê¤Î¿ô»ú¤Ë¤¹¤ë¡£ */
    if ('0' <= rch && rch <= '9') {
      rch -= '0';
    }
    else if ('A' <= rch && rch <= 'F') {
      rch -= 'A' - 10;
    }
    else if ('a' <= rch && rch <= 'f') {
      rch -= 'a' - 10;
    }
    else {
      d->kanji_status_return->length = -1;
      return 0;
    }
    *a++ = (char)rch; /* ¼è¤ê´º¤¨¤ºÊÝÂ¸¤·¤Æ¤ª¤¯ */
  }
  b = (a = tmpbuf) + 1;
  *a = (char)(0x80 | (*a * 16 + *b));
  *(tmpbuf+1) = 0x80 | (*(a += 2) * 16 + *(b += 2));
  *a = '\0';
  if ((unsigned char)*tmpbuf < 0xa1 ||
      0xfe < (unsigned char)*tmpbuf ||
      (unsigned char)*--a < 0xa1 ||
      0xfe < (unsigned char)*a) {
    return 0;
  } else {
    MBstowcs(buf, tmpbuf, 2);
    return 1;
  }
}

int convertAsHex(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  return cvtAsHex(d, d->buffer_return, yc->romaji_buffer, yc->rEndp);
}

/*
   ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹ÊäÂ­¥ë¡¼¥Á¥ó´ØÏ¢
 */

inline void
replaceSup2(int ind, int n)
{
  int i;
  WCHAR_T *temp, **p;

  if (ind < 0)
    return;

  temp = (p = keysup[ind].cand)[n];
  for (i = n ; i > 0 ; i--) {
    p[i] = p[i - 1];
  }
  p[0] = temp;
}

inline void
replaceSup(int ind, int n)
{
  int i, group;
  extern int nkeysup;

  group = keysup[ind].groupid;
  for (i = 0 ; i < nkeysup ; i++) {
    if (keysup[i].groupid == group) {
      replaceSup2(i, n);
    }
  }
}

static int everySupkey (uiContext, int, mode_context);

static
int everySupkey(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  ichiranContext ic = (ichiranContext)d->modec;
  WCHAR_T *cur;

  cur = ic->allkouho[*(ic->curIkouho)];
  d->kanji_status_return->revPos = 0;
  d->kanji_status_return->revLen = 0;
  d->kanji_status_return->echoStr = cur;
  d->kanji_status_return->length = WStrlen(cur);

  return retval;
}

static int exitSupkey (uiContext, int, mode_context);

static
int exitSupkey(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  yomiContext yc;

  popCallback(d); /* °ìÍ÷¤ò¥Ý¥Ã¥× */

  yc = (yomiContext)d->modec;

  replaceSup(findSup(yc->romaji_buffer[0]) - 1, yc->cursup);

#ifdef NOT_KAKUTEI
  yc->rCurs = yc->rStartp = yc->rEndp;
  yc->kCurs = yc->kEndp;
  kanaReplace(-yc->kEndp, d->buffer_return, retval, HENKANSUMI | SUPKEY);
  yc->kRStartp = yc->kCurs;
  yc->kAttr[0] |= SENTOU;
  yc->rAttr[0] |= SENTOU | HENKANSUMI;
  for (i = 1 ; i < retval ; i++) {
    yc->kAttr[i] &= ~SENTOU;
  }
  currentModeInfo(d);
  makeYomiReturnStruct(d);
  return 0;
#else
  /* Ì¤³ÎÄêÊ¸»úÎó¤òºï½ü¤¹¤ë */
  RomajiClearYomi(d);

  /* Ì¤³ÎÄêÊ¸»úÎó¤¬Á´¤¯¤Ê¤¯¤Ê¤Ã¤¿¤Î¤Ç¡¢¦Õ¥â¡¼¥É¤ËÁ«°Ü¤¹¤ë */
  restoreChikujiIfBaseChikuji(yc);
  d->current_mode = yc->curMode = yc->myEmptyMode;
  d->kanji_status_return->info |= KanjiEmptyInfo;
  currentModeInfo(d);
  makeYomiReturnStruct(d);
  return checkIfYomiQuit(d, retval);
#endif
}

static int quitSupkey (uiContext, int, mode_context);

static
int quitSupkey(uiContext d, int retval, mode_context env)
/* ARGSUSED */
{
  popCallback(d); /* °ìÍ÷¤ò¥Ý¥Ã¥× */
  makeYomiReturnStruct(d);
  currentModeInfo(d);
  return retval;
}

int selectKeysup(uiContext d, yomiContext yc, int ind)
{
  int retval;
  ichiranContext ic;
  extern int nkeysup;

  yc->cursup = 0;
  retval = selectOne(d, keysup[ind].cand, &(yc->cursup), keysup[ind].ncand,
		     BANGOMAX,
		     (unsigned)(!cannaconf.HexkeySelect ? NUMBERING : 0),
		     0, WITH_LIST_CALLBACK,
		     everySupkey, exitSupkey, quitSupkey, NO_CALLBACK);

  ic = (ichiranContext)d->modec;
  ic->majorMode = CANNA_MODE_IchiranMode;
  ic->minorMode = CANNA_MODE_IchiranMode;

  currentModeInfo(d);
  *(ic->curIkouho) = 0;

  /* ¸õÊä°ìÍ÷¹Ô¤¬¶¹¤¯¤Æ¸õÊä°ìÍ÷¤¬½Ð¤»¤Ê¤¤ */
  if(ic->tooSmall) {
    d->status = AUX_CALLBACK;
    return(retval);
  }

  if ( !(ic->flags & ICHIRAN_ALLOW_CALLBACK) ) {
    makeGlineStatus(d);
  }

  return retval;
}

/*
  ³°Íè¸ìÊÑ´¹¤ò¤¹¤ë¤è¤¦¤Ê¥ê¡¼¥¸¥ç¥ó¤Ë¤Ê¤Ã¤Æ¤¤¤ë¤«¡©

  ¤É¤ó¤Ê¤³¤È¤òÄ´¤Ù¤ë¤«¤È¸À¤¦¤È¡¢¤Þ¤º¡¢¥ê¡¼¥¸¥ç¥óÆâ¤¬³°Íè¸ìÊÑ´¹¤µ¤ì¤Æ¤¤
  ¤ë¤«¤É¤¦¤«¤òÄ´¤Ù¤ë¡£¼¡¤Ë¡¢¥ê¡¼¥¸¥ç¥ó¤ÎÎ¾Ã¼¤¬ÀèÆ¬Ê¸»ú¤Ë¤Ê¤Ã¤Æ¤¤¤ë¤³¤È
  ¤òÄ´¤Ù¤¿¤¤¤È¤³¤í¤À¤¬¡¢¤³¤ì¤Ï¤ä¤Ã¤Ñ¤ê¤Ï¤º¤·¤¿¡£

  ³°Íè¸ì¤ÎÅÓÃæ¤«¤é¤È¤«ÅÓÃæ¤Þ¤Ç¤È¤«¤Ç mark ¤ò¹Ô¤Ã¤¿»þ¤Ë¤µ¤é¤Ë³°Íè¸ìÊÑ´¹
  ¤ò¹Ô¤¦¤³¤È¤òÍÞÀ©¤¹¤ë¡£

 */

inline
int regionGairaigo(yomiContext yc, int s, int e)
{
  if ((yc->kAttr[s] & SENTOU) && (yc->kAttr[e] & SENTOU)) {
    return 1;
  }
  else {
    return 0;
  }
}


/*
  ³°Íè¸ìÊÑ´¹ºÑ¤Î»ú¤¬Æþ¤Ã¤Æ¤¤¤ë¤«¡©
 */

inline int
containGairaigo(yomiContext yc)
{
  int i;

  for (i = 0 ; i < yc->kEndp ; i++) {
    if (yc->kAttr[i] & GAIRAIGO) {
      return 1;
    }
  }
  return 0;
}

int containUnconvertedKey(yomiContext yc)
{
  int i, s, e;

  if (containGairaigo(yc)) {
    return 0;
  }

  if ((s = yc->cmark) > yc->kCurs) {
    e = s;
    s = yc->kCurs;
  }
  else {
    e = yc->kCurs;
  }

  for (i = s ; i < e ; i++) {
    if ( !(yc->kAttr[i] & HENKANSUMI) ) {
      return 1;
    }
  }
  return 0;
}

/*
 * ¤«¤Ê´Á»úÊÑ´¹¤ò¹Ô¤¤(ÊÑ´¹¥­¡¼¤¬½é¤á¤Æ²¡¤µ¤ì¤¿)¡¢TanKouhoMode¤Ë°Ü¹Ô¤¹¤ë
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */

static int YomiHenkan (uiContext);

static int
YomiHenkan(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  int len, idx;

#ifdef MEASURE_TIME
  struct tms timebuf;
  long   currenttime, times();

  currenttime = times(&timebuf);
#endif

  if (yc->henkanInhibition & CANNA_YOMI_INHIBIT_HENKAN) {
    return NothingChangedWithBeep(d);
  }

  d->nbytes = 0;
  len = RomajiFlushYomi(d, d->genbuf, ROMEBUFSIZE);

  if (containUnconvertedKey(yc)) {
    YomiMark(d);
    len = RomajiFlushYomi(d, d->genbuf, ROMEBUFSIZE);
  }

  yc->kRStartp = yc->kCurs = yc->kEndp;
  yc->rStartp  = yc->rCurs = yc->rEndp;

  if (len == 0) { /* empty ¥â¡¼¥É¤Ë¹Ô¤Ã¤Æ¤·¤Þ¤Ã¤¿ */
    d->more.todo = 1;
    d->more.ch = d->ch;
    d->more.fnum = 0;    /* ¾å¤Î ch ¤Ç¼¨¤µ¤ì¤ë½èÍý¤ò¤»¤è */
    return d->nbytes;
  }

  if (yc->rEndp == 1 && (yc->kAttr[0] & SUPKEY) &&
      !yc->left && !yc->right &&
      (idx = findSup(yc->romaji_buffer[0])) &&
      keysup[idx - 1].ncand > 1) {
    return selectKeysup(d, yc, idx - 1);
  }

  if (!prepareHenkanMode(d)) {
    makeGLineMessageFromString(d, jrKanjiError);
    makeYomiReturnStruct(d);
    return 0;
  }
  yc->minorMode = CANNA_MODE_TankouhoMode;
  yc->kouhoCount = 1;
  if (doHenkan(d, 0, (WCHAR_T *)0) < 0) {
    makeGLineMessageFromString(d, jrKanjiError);
    return TanMuhenkan(d);
  }
  if (cannaconf.kouho_threshold > 0 &&
      yc->kouhoCount >= cannaconf.kouho_threshold) {
    return tanKouhoIchiran(d, 0);
  }
  currentModeInfo(d);

#ifdef MEASURE_TIME
  hc->time = times(&timebuf;
  hc->proctime -= currenttime;
#endif

  return 0;
}

static int YomiHenkanNaive (uiContext);

static int
YomiHenkanNaive(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags &
      (CANNA_YOMI_HANKAKU | CANNA_YOMI_ROMAJI | CANNA_YOMI_BASE_HANKAKU)) {
    return YomiInsert(d);
  }
  else {
    return YomiHenkan(d);
  }
}

static int YomiHenkanOrNothing (uiContext);

static int
YomiHenkanOrNothing(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags &
      (CANNA_YOMI_HANKAKU | CANNA_YOMI_ROMAJI | CANNA_YOMI_BASE_HANKAKU)) {
    return NothingChanged(d);
  }
  else {
    return YomiHenkan(d);
  }
}

/* ¥Ù¡¼¥¹Ê¸»ú¤ÎÀÚ¤êÂØ¤¨ */

extern int EmptyBaseHira (uiContext);
extern int EmptyBaseKata (uiContext); 
extern int EmptyBaseEisu (uiContext);
extern int EmptyBaseZen (uiContext);
extern int EmptyBaseHan (uiContext);

static int YomiBaseHira (uiContext);

static
int YomiBaseHira(uiContext d)
{
  (void)RomajiFlushYomi(d, d->genbuf, ROMEBUFSIZE);
  (void)EmptyBaseHira(d);
  makeYomiReturnStruct(d);
  return 0;
}

static int YomiBaseKata (uiContext);

static
int YomiBaseKata(uiContext d)
{
  (void)RomajiFlushYomi(d, d->genbuf, ROMEBUFSIZE);
  (void)EmptyBaseKata(d);
  makeYomiReturnStruct(d);
  return 0;
}

static int YomiBaseEisu (uiContext);

static
int YomiBaseEisu(uiContext d)
{
  (void)RomajiFlushYomi(d, d->genbuf, ROMEBUFSIZE);
  (void)EmptyBaseEisu(d);
  makeYomiReturnStruct(d);
  return 0;
}

static int YomiBaseZen (uiContext);

static
int YomiBaseZen(uiContext d)
{
  (void)RomajiFlushYomi(d, d->genbuf, ROMEBUFSIZE);
  (void)EmptyBaseZen(d);
  makeYomiReturnStruct(d);
  return 0;
}

static int YomiBaseHan (uiContext);

static
int YomiBaseHan(uiContext d)
{
  (void)RomajiFlushYomi(d, d->genbuf, ROMEBUFSIZE);
  (void)EmptyBaseHan(d);
  makeYomiReturnStruct(d);
  return 0;
}

static int YomiBaseKana (uiContext);

static
int YomiBaseKana(uiContext d)
{
  (void)RomajiFlushYomi(d, d->genbuf, ROMEBUFSIZE);
  (void)EmptyBaseKana(d);
  makeYomiReturnStruct(d);
  return 0;
}

static int YomiBaseKakutei (uiContext);

static
int YomiBaseKakutei(uiContext d)
{
  (void)RomajiFlushYomi(d, d->genbuf, ROMEBUFSIZE);
  (void)EmptyBaseKakutei(d);
  makeYomiReturnStruct(d);
  return 0;
}

static int YomiBaseHenkan (uiContext);

static
int YomiBaseHenkan(uiContext d)
{
  (void)RomajiFlushYomi(d, d->genbuf, ROMEBUFSIZE);
  (void)EmptyBaseHenkan(d);
  makeYomiReturnStruct(d);
  return 0;
}

int YomiBaseHiraKataToggle (uiContext);

int YomiBaseHiraKataToggle(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  (void)RomajiFlushYomi(d, d->genbuf, ROMEBUFSIZE);

  if (yc->generalFlags & CANNA_YOMI_KATAKANA) {
    (void)EmptyBaseHira(d);
  }
  else {
    (void)EmptyBaseKata(d);
  }
  makeYomiReturnStruct(d);
  return 0;
}

int YomiBaseZenHanToggle (uiContext);

int YomiBaseZenHanToggle(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  (void)RomajiFlushYomi(d, d->genbuf, ROMEBUFSIZE);

  if (yc->generalFlags & CANNA_YOMI_BASE_HANKAKU) {
    (void)EmptyBaseZen(d);
  }
  else {
    (void)EmptyBaseHan(d);
  }
  makeYomiReturnStruct(d);
  return 0;
}

int YomiBaseRotateForw (uiContext);

int YomiBaseRotateForw(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  (void)RomajiFlushYomi(d, d->genbuf, ROMEBUFSIZE);

  if (!(yc->generalFlags & CANNA_YOMI_BASE_HANKAKU) &&
      ((yc->generalFlags & CANNA_YOMI_ROMAJI) ||
       ((yc->generalFlags & CANNA_YOMI_KATAKANA) &&
	!cannaconf.InhibitHankakuKana) )) {
    (void)EmptyBaseHan(d);
  }
  else {
    yc->generalFlags &= ~CANNA_YOMI_BASE_HANKAKU;
    if (yc->generalFlags & CANNA_YOMI_ROMAJI) {
      (void)EmptyBaseHira(d);
    }
    else if (yc->generalFlags & CANNA_YOMI_KATAKANA) {
      (void)EmptyBaseEisu(d);
    }
    else {
      (void)EmptyBaseKata(d);
    }
  }
  makeYomiReturnStruct(d);
  return 0;
}

int YomiBaseRotateBack (uiContext);

int YomiBaseRotateBack(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  (void)RomajiFlushYomi(d, d->genbuf, ROMEBUFSIZE);

  if (yc->generalFlags & CANNA_YOMI_BASE_HANKAKU) {
    (void)EmptyBaseZen(d);
  }
  else if (yc->generalFlags & CANNA_YOMI_KATAKANA) {
    (void)EmptyBaseHira(d);
  }
  else if (yc->generalFlags & CANNA_YOMI_ROMAJI) {
    if (!cannaconf.InhibitHankakuKana) {
      yc->generalFlags |= CANNA_YOMI_BASE_HANKAKU;
    }
    (void)EmptyBaseKata(d);
  }
  else {
    yc->generalFlags &= ~CANNA_YOMI_ZENKAKU;
    yc->generalFlags |= CANNA_YOMI_BASE_HANKAKU;
    (void)EmptyBaseEisu(d);
  }
  makeYomiReturnStruct(d);
  return 0;
}

int YomiBaseKanaEisuToggle (uiContext);

int YomiBaseKanaEisuToggle(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  (void)RomajiFlushYomi(d, d->genbuf, ROMEBUFSIZE);

  if (yc->generalFlags & CANNA_YOMI_ROMAJI) {
    (void)EmptyBaseKana(d);
  }
  else {
    (void)EmptyBaseEisu(d);
  }
  makeYomiReturnStruct(d);
  return 0;
}

int YomiBaseKakuteiHenkanToggle (uiContext);

int YomiBaseKakuteiHenkanToggle(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  (void)RomajiFlushYomi(d, d->genbuf, ROMEBUFSIZE);

  if (yc->generalFlags & CANNA_YOMI_KAKUTEI) {
    (void)EmptyBaseHenkan(d);
  }
  else { /* ËÜÅö¤Ï°ì¶ÚÆì¤Ç¤Ï¹Ô¤«¤Ê¤¤ */
    (void)EmptyBaseKakutei(d);
  }
  makeYomiReturnStruct(d);
  return 0;
}

int YomiModeBackup (uiContext);

int YomiModeBackup(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;

  (void)saveFlags(yc);
  return NothingChanged(d);
}

/* »ú¼ïÊÑ´¹´ØÏ¢ */

/* cfuncdef

   exitJishu -- »ú¼ïÊÑ´¹¤ò³ÎÄê¤µ¤»¤ë

   ¤³¤Î´Ø¿ô¤Ï»ú¼ïÊÑ´¹¤ò³ÎÄê¤µ¤»¤ÆÆÉ¤ß¥â¡¼¥É¤ËÌá¤Ã¤¿¤È¤³¤í¤Ç¼Â¹Ô¤µ¤ì¤ë
   ´Ø¿ô¤Ç¤¢¤ë¡£

   ¡Ú»ú¼ïÊÑ´¹¤È¤Î¤ªÌóÂ«»ö¡Û

   ¤³¤Î´Ø¿ô¤Ï jishu.c ¤Ë½ñ¤¤¤Æ¤¢¤ë JishuKakutei ¤¬¸Æ¤Ó½Ð¤µ¤ì¤¿¤È
   ¤­¤Ê¤É¤Ë¸Æ¤Ó½Ð¤µ¤ì¤ë´Ø¿ô¤Ç¤¢¤ë¡£JishuKakutei ¤Ç¤ÏºÇ½ªÅª¤Ê»ú¼ï
   ¤Î»ØÄê¤ä¤½¤ÎÈÏ°Ï¤Î»ØÄê¤ò¤·¤ÆÍè¤ë¤À¤±¤Ç¼ÂºÝ¤ÎÌÜÅª»ú¼ï¤Ø¤ÎÊÑ´¹¤Ï
   ¤³¤Î´Ø¿ô¤Ç¹Ô¤ï¤Ê¤±¤ì¤Ð¤Ê¤é¤Ê¤¤¡£¤Ê¤¼¤«¤È¸À¤¦¤È¥í¡¼¥Þ»ú¤È¤ÎÂÐ±þ
   ¤Å¤±¤ò¤­¤Á¤ó¤ÈÊÝ»ý¤·¤Æ¤ª¤­¤¿¤¤¤«¤é¤Ç¤¢¤ë JishuKakutei ¤È¤Î´Ö¤Î
   ¤ªÌóÂ«¤Ï°Ê²¼¤ÎÄÌ¤ê

   (1) ºÇ½ªÅª¤Ê»ú¼ï¤Ï yc ¤Î»ú¼ï´ØÏ¢¤Î¥á¥ó¥Ð¤ËÊÖ¤µ¤ì¤ë
   (2) ¶ñÂÎÅª¤Ë¤Ï°Ê²¼¤ËÆþ¤ë¡£
       jishu_kc    ºÇ½ªÅª¤Ê»ú¼ï¤Î¼ïÎà (JISHU_ZEN_KATA ¤Ê¤É)
       jishu_case  ºÇ½ªÅª¤Ê»ú¼ï¤Î¥±¡¼¥¹ (CANNA_JISHU_UPPER ¤Ê¤É)
       jishu_kEndp »ú¼ïÊÑ´¹¤ÎÂÐ¾ÝÈÏ°Ï
       jishu_rEndp »ú¼ïÊÑ´¹¤ÎÂÐ¾ÝÈÏ°Ï¤Î¥í¡¼¥Þ»ú¥Ð¥Ã¥Õ¥¡¤Ç¤Î°ÌÃÖ
   (3) yc->cmark ¤Þ¤Ç¤Ï»ú¼ï¤¬ÃÖ¤­ÊÑ¤ï¤é¤Ê¤¤¤Î¤ËÃí°Õ¤¹¤ë¡£
   (4) yc->kana_buffer ¤ÎÃÖ¤­´¹¤¨¤Ï exitJishu ¤¬¹Ô¤¦¡£
   (5) yc->kana_buffer ¤Ç»ú¼ïÊÑ´¹ÈÏ°Ï°Ê³°¤Î¤â¤Î¤Ï yc->romaji_buffer
       ¤ò¤â¤¦°ìÅÙ¥³¥Ô¡¼¤·¤Æ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¤µ¤ì¤ë¤³¤È¤ÇÉÕ¤±²Ã¤¨¤é¤ì¤ë¡£
   (6) ¤¿¤À¤·¡¢yc->kRStartp == yc->jishu_kEndp ¤Ê¤é¤Ð¾åµ­¤Î½èÍý¤Ï¹Ô¤ï¤Ê
       ¤¤¡£
   (7) ¾åµ­¤ÇÊÖ¤µ¤ì¤Ê¤¤ÉôÊ¬¤Î¥í¡¼¥Þ»ú¤Ï yc->jishu_rEndp °Ê¹ß¤Ç¤¢¤ë¡£
   (8) exitJishu ¤Ï¤½¤ÎÉôÊ¬¤ò yc->kana_buffer ¤Ë°ÜÆ°¤·¤â¤¦°ìÅÙ¥í¡¼¥Þ»ú
       ¤«¤ÊÊÑ´¹¤ò¹Ô¤¦¡£
 */

int exitJishu(uiContext d)
{
  yomiContext yc;
  int len, srclen, i, pos;
  BYTE jishu, jishu_case, head = 1;
  int jishu_kEndp, jishu_rEndp;
  int (*func1)(...), (*func2)(...);
  long savedgf;
  WCHAR_T *buf, *p;
#ifndef WIN
  WCHAR_T xxxx[1024];
#else
  WCHAR_T *xxxx = (WCHAR_T *)malloc(sizeof(WCHAR_T) * 1024);
  if (!xxxx) {
    return 0;
  }
#endif

  /* ¤³¤³¤«¤é²¼¤Ï´°Á´¤Ê¡ØÆÉ¤ß¡Ù¥â¡¼¥É */

  yc = (yomiContext)d->modec;

  jishu = yc->jishu_kc;
  jishu_case = yc->jishu_case;
  jishu_kEndp = yc->jishu_kEndp;
  jishu_rEndp = yc->jishu_rEndp;

  leaveJishuMode(d, yc);

  /* ¥Æ¥ó¥Ý¥é¥ê¥â¡¼¥É¤À¤Ã¤¿¤é¸µ¤ËÌá¤¹ */
  if (yc->savedFlags & CANNA_YOMI_MODE_SAVED) {
    restoreFlags(yc);
  }
  /* Ãà¼¡¤ÎÆÉ¤ß¥Ý¥¤¥ó¥¿¤ò¥¯¥ê¥¢ */
  yc->ys = yc->cStartp;

  /* ¤Þ¤º¡¢»ú¼ïÊÑ´¹¤µ¤ì¤¿ÉôÊ¬¤òÊÑ´¹ */
  buf = d->genbuf;
  switch (jishu) {
  case JISHU_ZEN_KATA: /* Á´³Ñ¥«¥¿¥«¥Ê¤ËÊÑ´¹¤¹¤ë */
    func1 = (int (*)(...))RkwCvtZen;
    func2 = (int (*)(...))RkwCvtKana;
    goto jishuKakuteiKana;

  case JISHU_HAN_KATA: /* È¾³Ñ¥«¥¿¥«¥Ê¤ËÊÑ´¹¤¹¤ë */
    func1 = (int (*)(...))RkwCvtKana;
    func2 = (int (*)(...))RkwCvtHan;
    goto jishuKakuteiKana;

  case JISHU_HIRA: /* ¤Ò¤é¤¬¤Ê¤ËÊÑ´¹¤¹¤ë */
    func1 = (int (*)(...))RkwCvtZen;
    func2 = (int (*)(...))RkwCvtHira;

  jishuKakuteiKana:
    /* ¤Þ¤º¡¢¥Ù¡¼¥¹¤¬¥í¡¼¥Þ»ú¤Î¤È¤­¤ËÆþÎÏ¤µ¤ì¤¿¤â¤Î¤¬¤¢¤ì¤Ð¤«¤Ê¤ËÊÑ´¹¤¹¤ë */
    savedgf = yc->generalFlags;
    yc->generalFlags = savedgf & CANNA_YOMI_IGNORE_USERSYMBOLS;
    for (i = yc->cmark ; i < jishu_kEndp ;) {
      int j = i;
      while (i < jishu_kEndp && yc->kAttr[i] & STAYROMAJI) {
	yc->kAttr[i++] &= ~(HENKANSUMI | STAYROMAJI);
      }
      if (j < i) {
	kPos2rPos(yc, j, i, &yc->rStartp, &yc->rCurs);
	yc->kRStartp = j;
	yc->kCurs = i;
	makePhonoOnBuffer(d, yc, (unsigned char)0, RK_FLUSH, 0);
	jishu_kEndp += yc->kCurs - i;
	i = yc->kCurs;
      }else{
	i++;
      }
    }
    yc->generalFlags = savedgf;

    /* ¤³¤³¤Ç¡¢¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹Ã±°Ì¤Ç»ú¼ïÊÑ´¹¤¹¤ë */
    for (i = yc->cmark ; i < jishu_kEndp ; i = yc->kCurs) {
      int j;

      for (j = i + 1 ; !(yc->kAttr[j] & SENTOU) ;) {
	j++;
      }
      if(j > jishu_kEndp) {
	j = jishu_kEndp;
      }
      srclen = j - i;

      len = (*func1)(xxxx, 1024, yc->kana_buffer + i, srclen);
      len = (*func2)(buf, ROMEBUFSIZE, xxxx, len);
      yc->kCurs = j;
      kanaReplace(-srclen, buf, len, 0);
      jishu_kEndp += len - srclen; /* yc->kCurs - j ¤ÈÆ±¤¸ÃÍ */

      for (j = yc->kCurs - len ; j < yc->kCurs ; j++) {
	yc->kAttr[j] = HENKANSUMI;
      }
      yc->kAttr[yc->kCurs - len] |= SENTOU;
    }
    break;

  case JISHU_ZEN_ALPHA: /* Á´³Ñ±Ñ¿ô¤ËÊÑ´¹¤¹¤ë */
  case JISHU_HAN_ALPHA: /* È¾³Ñ±Ñ¿ô¤ËÊÑ´¹¤¹¤ë */
    p = yc->romaji_buffer;
    kPos2rPos(yc, 0, yc->cmark, (int *)0, &pos);

    for (i = pos ; i < jishu_rEndp ; i++) {
      xxxx[i - pos] =
	(jishu_case == CANNA_JISHU_UPPER) ? WToupper(p[i]) :
        (jishu_case == CANNA_JISHU_LOWER) ? WTolower(p[i]) : p[i];
      if (jishu_case == CANNA_JISHU_CAPITALIZE) {
	if (p[i] <= ' ') {
	  head = 1;
	}
	else if (head) {
	  head = 0;
	  xxxx[i - pos] = WToupper(p[i]);
	}
      }
    }
    xxxx[i - pos] = (WCHAR_T)0;
#if 0
    if (jishu_case == CANNA_JISHU_CAPITALIZE) {
      xxxx[0] = WToupper(xxxx[0]);
    }
#endif
    if (jishu == JISHU_ZEN_ALPHA) {
      len = RkwCvtZen(buf, ROMEBUFSIZE, xxxx, jishu_rEndp - pos);
    }
    else {
      len = RkwCvtNone(buf, ROMEBUFSIZE, xxxx, jishu_rEndp - pos);
    }
    yc->rCurs = jishu_rEndp;
    yc->kCurs = jishu_kEndp;
    kanaReplace(yc->cmark - yc->kCurs, buf, len, 0);
    jishu_kEndp = yc->kCurs;

    /* ¤³¤³¤ÇÀèÆ¬¥Ó¥Ã¥È¤òÎ©¤Æ¤ë */
    for (i = pos ; i < yc->rCurs ; i++) {
      yc->rAttr[i] = SENTOU;
    }

    len = yc->kCurs;
    for (i = yc->cmark ; i < len ; i++) {
      yc->kAttr[i] = HENKANSUMI | SENTOU;
    }

    /* ¸å¤í¤ÎÉôÊ¬ */
    for (i = jishu_rEndp ; i < yc->rEndp ; i++) {
      yc->rAttr[i] = 0;
    }
    yc->rAttr[jishu_rEndp] = SENTOU;

    kanaReplace(yc->kEndp - jishu_kEndp, yc->romaji_buffer + jishu_rEndp,
		yc->rEndp - jishu_rEndp, 0);
    yc->rAttr[jishu_rEndp] |= SENTOU;
    yc->kAttr[jishu_kEndp] |= SENTOU;
    yc->rStartp = jishu_rEndp;
    yc->kRStartp = jishu_kEndp;

    for (yc->kCurs = jishu_kEndp, yc->rCurs = jishu_rEndp ;
	 yc->kCurs < yc->kEndp ;) {
      yc->kCurs++; yc->rCurs++;
      if (yc->kRStartp == yc->kCurs - 1) {
	yc->kAttr[yc->kRStartp] |= SENTOU;
      }
      makePhonoOnBuffer(d, yc,
                          (unsigned char)yc->kana_buffer[yc->kCurs - 1], 0, 0);
    }
    if (yc->kRStartp != yc->kEndp) {
      if (yc->kRStartp == yc->kCurs - 1) {
	yc->kAttr[yc->kRStartp] |= SENTOU;
      }
      makePhonoOnBuffer(d, yc, (unsigned char)0, RK_FLUSH, 0);
    }
    break;

  default:/* ¤É¤ì¤Ç¤â¤Ê¤«¤Ã¤¿¤éÊÑ´¹½ÐÍè¤Ê¤¤¤Î¤Ç²¿¤â¤·¤Ê¤¤ */
    jishu_rEndp = jishu_kEndp = 0;
    break;
  }
  yc->kCurs = yc->kRStartp = yc->kEndp;
  yc->rCurs = yc->rStartp = yc->rEndp;
  yc->pmark = yc->cmark;
  yc->cmark = yc->kCurs;
  yc->jishu_kEndp = 0;
#ifdef WIN
  free(xxxx);
#endif
  return 0;
}

static
int YomiJishu(uiContext d, int fn)
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->henkanInhibition & CANNA_YOMI_INHIBIT_JISHU) {
    return NothingChangedWithBeep(d);
  }
  d->nbytes = 0;
  if ((yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) &&
      !(yc->status & CHIKUJI_OVERWRAP) && yc->nbunsetsu) {
    yc->status |= CHIKUJI_OVERWRAP;
    moveToChikujiTanMode(d);
  }
  else if (! RomajiFlushYomi(d, (WCHAR_T *)NULL, 0)) {
    d->more.todo = 1;
    d->more.ch = d->ch;
    d->more.fnum = 0;    /* ¾å¤Î ch ¤Ç¼¨¤µ¤ì¤ë½èÍý¤ò¤»¤è */
    return d->nbytes;
  }
  else {
    enterJishuMode(d, yc);
    yc->minorMode = CANNA_MODE_JishuMode;
  }
  currentModeInfo(d);
  d->more.todo = 1;
  d->more.ch = d->ch;
  d->more.fnum = fn;
  return 0;
}

static int
chikujiEndBun(uiContext d)
{
  yomiContext yc = (yomiContext)d->modec;
  int ret = 0;

  if ((yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) && yc->nbunsetsu) {
    KanjiMode mdsv;
#ifndef WIN
    yomiContextRec ycsv;
#else
    yomiContext ycsv;

    ycsv = (yomiContext)malloc(sizeof(yomiContextRec));
    if (ycsv) {
#endif

    /* µ¿Ìä¤¬»Ä¤ë½èÍý */
#ifdef WIN
    * /* This is a little bit tricky source code */
#endif
    ycsv = *yc;
    yc->kEndp = yc->rEndp = 0;
    mdsv = d->current_mode;
    ret = TanKakutei(d);
    d->current_mode = mdsv;
    *yc =
#ifdef WIN
      * /* this is also a little bit trick source code */
#endif
      ycsv;
  }
#ifdef WIN
    free(ycsv);
  }
#endif 
  return(ret);
}

/* cfuncdef

   replaceEnglish -- ¤«¤Ê¥Ð¥Ã¥Õ¥¡¤ò¥í¡¼¥Þ»ú¤ËÌá¤·¤ÆºÆ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¤¹¤ë

   d, yc      : ¥³¥ó¥Æ¥¯¥¹¥È
   start, end : ¥í¡¼¥Þ»ú¤ËÌá¤¹ÈÏ°Ï
   RKflag     : RkwMapPhonogram ¤ËÍ¿¤¨¤ë¥Õ¥é¥°
   engflag    : ±ÑÃ±¸ì¥«¥¿¥«¥ÊÊÑ´¹¤ò¤¹¤ë¤«¤É¤¦¤«¤Î¥Õ¥é¥°

 */

inline void
replaceEnglish(uiContext d, yomiContext yc, int start, int end, int RKflag, int engflag)
{
  int i;

  kanaReplace(yc->pmark - yc->cmark,
	      yc->romaji_buffer + start, end - start, 0);
  yc->kRStartp = yc->pmark;
  yc->rStartp = start;
  for (i = start ; i < end ; i++) {
    yc->rAttr[i] &= ~SENTOU;
  }
  yc->rAttr[start] |= SENTOU;
  for (i = yc->pmark ; i < yc->kCurs ; i++) {
    yc->kAttr[i] &= ~(SENTOU | HENKANSUMI);
  }
  yc->kAttr[yc->pmark] |= SENTOU;

  yc->n_susp_chars = 0; /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¤ä¤êÄ¾¤·¤Ê¤Î¤Ç¥¯¥ê¥¢¤¹¤ë */
  makePhonoOnBuffer(d, yc, 0, (unsigned char)RKflag, engflag);
  yc->kRStartp = yc->kCurs;
  yc->rStartp = yc->rCurs;
}

int YomiMark (uiContext);

int
YomiMark(uiContext d)
{
#ifndef NOT_ENGLISH_TABLE
  int rc, rp, i;
#endif
  yomiContext yc = (yomiContext)d->modec;
  
#if defined(DEBUG) && !defined(WIN)
  if (iroha_debug) {
    fprintf(stderr,"yc->kCurs=%d yc->cmark=%d\n", yc->kCurs,yc->cmark);
  }
#endif /* DEBUG */

  if (yc->kCurs != yc->cmark) { /* ¤ª½é */

    if (yc->cmark < yc->kCurs) {
      yc->pmark = yc->cmark;
      yc->cmark = yc->kCurs;
    }
    else {
      /* °Ê²¼¡¢pmark < cmark ¤ò²¾Äê¤·¤Æ¤¤¤ë½èÍý¤¬¤¢¤ë¤Î¤Ç¡¢
	 cmark < pmark ¤Î¾ì¹ç¤Ï pmark ¤â cmark ¤ÈÆ±¤¸ÃÍ¤Ë¤·¤Æ¤·¤Þ¤¦¡£
	 ¤Á¤ç¤Ã¤ÈÁ°¤Þ¤Ç¤Ï pmark ¤È cmark ¤ÎÆþ¤ì´¹¤¨¤ò¤ä¤Ã¤Æ¤¤¤¿¤¬¡¢
	 ¤½¤¦¤·¤Æ¤·¤Þ¤¦¤È¡¢¸½ºß¤Î¥Þ¡¼¥¯¤è¤ê¤âº¸¤Ø¤Ï¥Þ¡¼¥¯¤¬ÉÕ¤±¤é¤ì¤Ê¤¤
	 ¤È¸À¤¦¤³¤È¤Ë¤Ê¤Ã¤Æ¤·¤Þ¤¦¡£ */
      yc->pmark = yc->cmark = yc->kCurs;
    }
    yc->englishtype = CANNA_ENG_NO;
  }
#ifndef NOT_ENGLISH_TABLE
  if (englishdic) {
    if (regionGairaigo(yc, yc->pmark, yc->cmark)) {
      yc->englishtype++;
      yc->englishtype = (BYTE)((int)yc->englishtype % (int)(CANNA_ENG_NO + 1));
      if (yc->englishtype == CANNA_ENG_KANA) {
	kPos2rPos(yc, yc->pmark, yc->cmark, &rp, &rc);
	replaceEnglish(d, yc, rp, rc, RK_FLUSH, 1);
	yc->cmark = yc->kCurs;
      }
    }
    else {
      makeYomiReturnStruct(d);
      return 0;
    }

    /* ¤Þ¤º¤Ï¡¢¥«¥Ê¤Ë¤Ç¤­¤ë±ÑÃ±¸ì¤¬¤¢¤Ã¤¿¤«¤É¤¦¤«¤ò¥Á¥§¥Ã¥¯ */
    rp = rc = 0;
    for (i = yc->pmark ; i < yc->cmark ; i++) {
      if (yc->kAttr[i] & GAIRAIGO) {
	rp = i;
	do {
	  i++;
	} while (!(yc->kAttr[i] & SENTOU));
	rc = i;
	break;
      }
    }
    if (rp || rc) {
      int rs, re, offset;
      WCHAR_T space2[2];

      kPos2rPos(yc, rp, rc, &rs, &re);
      switch (yc->englishtype) {
      case CANNA_ENG_KANA:
	break;
      case CANNA_ENG_ENG1:
	offset = yc->kCurs - rc;
	yc->kCurs -= offset;
	kanaReplace(rp - rc, yc->romaji_buffer + rs, re - rs,
		    HENKANSUMI | GAIRAIGO);
	yc->kAttr[yc->kCurs - re + rs] |= SENTOU;
	yc->kCurs += offset;
	yc->cmark = yc->kRStartp = yc->kCurs;
	break;
      case CANNA_ENG_ENG2:
	offset = yc->kCurs - rc;
	yc->kCurs -= offset;
	space2[0] = (WCHAR_T)' ';
	space2[1] = (WCHAR_T)' ';
	kanaReplace(rp - rc, space2, 2, HENKANSUMI | GAIRAIGO);
	yc->kAttr[yc->kCurs - 2] |= SENTOU;
	yc->kCurs--;
	kanaReplace(0, yc->romaji_buffer + rs, re - rs, HENKANSUMI | GAIRAIGO);
	yc->kAttr[yc->kCurs - re + rs] &= ~SENTOU;
	yc->kCurs += offset + 1;
	yc->cmark = yc->kRStartp = yc->kCurs;
	break;
      case CANNA_ENG_NO:
	kPos2rPos(yc, yc->pmark, yc->cmark, &rs, &re);
	replaceEnglish(d, yc, rs, re, 0, 0);
	yc->cmark = yc->kRStartp = yc->kCurs;
	break;
      }
    }
  }
#endif
  makeYomiReturnStruct(d);
  debug_yomi(yc);
  return 0;
}

int
Yomisearchfunc(uiContext d, KanjiMode mode, int whattodo, int key, int fnum)
{
  yomiContext yc = (yomiContext)0;
  int len;
  extern KanjiModeRec yomi_mode;

  if (d) {
    yc = (yomiContext)d->modec;
  }

  if (yc && yc->id != YOMI_CONTEXT) {
    /* ËÜÍè¤¢¤ê¤¨¤Ê¤¤¤¬¡¢¥Ð¥°¤Ã¤Æ¤¤¤Æ¡¢¤³¤¦¤Ê¤Ã¤Æ¤Æ¤â core ¤òÅÇ¤­¤µ¤¨
       ¤·¤Ê¤±¤ì¤Ð¤½¤Î¤¦¤ÁÀµ¤·¤¤¾õÂÖ¤ËÌá¤ë¤Î¤ÇÇ°¤Î°Ù¤¤¤ì¤Æ¤ª¤¯ */
    yc = (yomiContext)0;
  }

  if (cannaconf.romaji_yuusen && yc) { /* ¤â¤·¡¢Í¥Àè¤Ê¤é */
    len = yc->kCurs - yc->kRStartp;
    if (fnum == 0) {
      fnum = mode->keytbl[key];
    }
    if (fnum != CANNA_FN_FunctionalInsert && len > 0) {
      int n, m, t, flag, prevrule;
      WCHAR_T kana[128], roma[128];
      flag = cannaconf.ignore_case ? RK_IGNORECASE : 0;

      WStrncpy(roma, yc->kana_buffer + yc->kRStartp, len);
      roma[len++] = (WCHAR_T)key;
    
      prevrule = yc->last_rule;
      if ((RkwMapPhonogram(yc->romdic, kana, 128, roma, len, (WCHAR_T)key,
			   flag | RK_SOKON, &n, &m, &t, &prevrule) &&
	   n == len) || n == 0) {
	/* RK_SOKON ¤òÉÕ¤±¤ë¤Î¤Ïµì¼­½ñÍÑ */
	fnum = CANNA_FN_FunctionalInsert;
      }
    }
  }
  return searchfunc(d, mode, whattodo, key, fnum);
}

/*
  trimYomi -- ÆÉ¤ß¥Ð¥Ã¥Õ¥¡¤Î¤¢¤ëÎÎ°è°Ê³°¤òºï¤ë

   sy ey  ¤«¤Ê¤ÎÉôÊ¬¤Ç»Ä¤¹ÎÎ°è¡¢¤³¤Î³°Â¦¤Ïºï¤é¤ì¤ë¡£
   sr er  ¥í¡¼¥Þ»ú             ¡·
 */

void
trimYomi(uiContext d, int sy, int ey, int sr, int er)
{
  yomiContext yc = (yomiContext)d->modec;

  yc->kCurs = ey;
  yc->rCurs = er;

  romajiReplace (yc->rEndp - er, (WCHAR_T *)NULL, 0, 0);
  kanaReplace   (yc->kEndp - ey, (WCHAR_T *)NULL, 0, 0);

  yc->kCurs = sy;
  yc->rCurs = sr;

  romajiReplace (-sr, (WCHAR_T *)NULL, 0, 0);
  kanaReplace   (-sy, (WCHAR_T *)NULL, 0, 0);
}

inline int
TbBubunKakutei(uiContext d)
{
  tanContext tan, tc = (tanContext)d->modec;
  WCHAR_T *s = d->buffer_return, *e = s + d->n_buffer;
  int len;

  tan = tc;
  while (tan->left) {
    tan = tan->left;
  }

  len = doKakutei(d, tan, tc, s, e, (yomiContext *)0);
  d->modec = (mode_context)tc;
  tc->left = (tanContext)0;
  s += len;
  (void)TanMuhenkan(d);
  return len;
}

int doTanConvertTb (uiContext, yomiContext);

int TanBubunKakutei (uiContext);

int
TanBubunKakutei(uiContext d)
{
  int len;
  tanContext tan;
  yomiContext yc = (yomiContext)d->modec;
  WCHAR_T *s = d->buffer_return, *e = s + d->n_buffer;

  if (yc->id == YOMI_CONTEXT) {
    doTanConvertTb(d, yc);
    yc = (yomiContext)d->modec;
  }
  tan = (tanContext)yc;
  while (tan->left) {
    tan = tan->left;
  }
  len = doKakutei(d, tan, (tanContext)yc, s, e, (yomiContext *)0);
  d->modec = (mode_context)yc;
  yc->left = (tanContext)0;

  makeYomiReturnStruct(d);
  currentModeInfo(d);
  return len;
}

#if 0
/*
 * ¥«¥ì¥ó¥ÈÊ¸Àá¤ÎÁ°¤Þ¤Ç³ÎÄê¤·¡¢¥«¥ì¥ó¥È°Ê¹ß¤ÎÊ¸Àá¤òÆÉ¤ß¤ËÌá¤¹
 *
 * °ú¤­¿ô	uiContext
 * Ìá¤êÃÍ	Àµ¾ï½ªÎ»»þ 0	°Û¾ï½ªÎ»»þ -1
 */
TanBubunKakutei(uiContext d)
{
  extern KanjiModeRec cy_mode, yomi_mode;
  WCHAR_T *ptr = d->buffer_return, *eptr = ptr + d->n_buffer;
  yomiContext yc = (yomiContext)d->modec;
  tanContext tan;
  int i, j, n, l = 0, len, con, ret = 0;
#ifndef WIN
  WCHAR_T tmpbuf[ROMEBUFSIZE];
#else
  WCHAR_T *tmpbuf = (WCHAR_T *)malloc(sizeof(WCHAR_T) * ROMEBUFSIZE);
  if (!tmpbuf) {
    return 0;
  }
#endif

  if (yc->id != YOMI_CONTEXT) {
    ret = TbBubunKakutei(d);
    goto return_ret;
  }

  tan = (tanContext)yc;
  while (tan->left) {
    tan = tan->left;
  }

  len = doKakutei(d, tan, (tanContext)yc, ptr, eptr, (yomiContext *)0);
  d->modec = (mode_context)yc;
  yc->left = (tanContext)0;
  ptr += len;

  if (yomiInfoLevel > 0) {  /* ÌÌÅÝ¤Ê¤Î¤Ç yomiInfo ¤ò¼Î¤Æ¤ë */
    d->kanji_status_return->info &= ~KanjiYomiInfo;
  }

  con = yc->context;

  /* ³ÎÄêÊ¸»úÎó ¤òºî¤ë */
  for (i = 0, n = yc->curbun ; i < n ; i++) {
    if (RkwGoTo(con, i) < 0) {
      ret = makeRkError(d, "\312\270\300\341\244\316\260\334\306\260\244\313"
	"\274\272\307\324\244\267\244\336\244\267\244\277");
                            /* Ê¸Àá¤Î°ÜÆ°¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
      goto return_ret;
    }
    len = RkwGetKanji(con, ptr, (int)(eptr - ptr));
    if (len < 0) {
      (void)makeRkError(d, "\264\301\273\372\244\316\274\350\244\352\275\320"
	"\244\267\244\313\274\272\307\324\244\267\244\336\244\267\244\277");
                           /* ´Á»ú¤Î¼è¤ê½Ð¤·¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
      ret = TanMuhenkan(d);
      goto return_ret;
    }
    ptr += len;
    j = RkwGetYomi(yc->context, tmpbuf, ROMEBUFSIZE);
    if (j < 0) {
      (void)makeRkError(d, "\245\271\245\306\245\244\245\277\245\271\244\362"
	"\274\350\244\352\275\320\244\273\244\336\244\273\244\363\244\307"
	"\244\267\244\277");
                           /* ¥¹¥Æ¥¤¥¿¥¹¤ò¼è¤ê½Ð¤»¤Þ¤»¤ó¤Ç¤·¤¿ */
      ret = TanMuhenkan(d);
      goto return_ret;
    }
    l += j;
  }
  d->nbytes = ptr - d->buffer_return;

  for (i = j = 0 ; i < l ; i++) {
    if (yc->kAttr[i] & SENTOU) {
      do {
	++j;
      } while (!(yc->rAttr[j] & SENTOU));
    }
  }
  yc->rStartp = yc->rCurs = j;
  romajiReplace(-j, (WCHAR_T *)NULL, 0, 0);
  yc->kRStartp = yc->kCurs = i;
  kanaReplace(-i, (WCHAR_T *)NULL, 0, 0);

  if (RkwEndBun(yc->context, cannaconf.Gakushu ? 1 : 0) == -1) {
    jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\316"
	"\275\252\316\273\244\313\274\272\307\324\244\267\244\336\244\267"
	"\244\277";
                   /* ¤«¤Ê´Á»úÊÑ´¹¤Î½ªÎ»¤Ë¼ºÇÔ¤·¤Þ¤·¤¿ */
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
  }

  if (yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) {
    yc->status &= CHIKUJI_NULL_STATUS;
    yc->cStartp = yc->cRStartp = 0;
    yc->kCurs = yc->kRStartp = yc->kEndp;
    yc->rCurs = yc->rStartp = yc->rEndp;
    yc->ys = yc->ye = yc->cStartp;
    clearHenkanContext(yc);
    d->current_mode = yc->curMode = yc->rEndp ? &cy_mode : yc->myEmptyMode;
  }
  else {
    d->current_mode = yc->curMode = &yomi_mode;
  }
  yc->minorMode = getBaseMode(yc);

  yc->nbunsetsu = 0;

  /* Ã±¸õÊä¾õÂÖ¤«¤éÆÉ¤ß¤ËÌá¤ë¤È¤­¤Ë¤ÏÌµ¾ò·ï¤Ëmark¤òÀèÆ¬¤ËÌá¤¹ */
  yc->cmark = yc->pmark = 0;

  abandonContext(d, yc);

  doMuhenkan(d, yc);

  makeYomiReturnStruct(d);
  currentModeInfo(d);

  ret = d->nbytes;

 return_ret:
#ifdef WIN
  free(tmpbuf);
#endif
  return ret;
}
#endif /* 0 */

/*
  removeKana -- yomiContext ¤ÎÀèÆ¬¤«¤é»ú¤òºï¤ë(Ãà¼¡¤Ç»È¤¦)

  k -- ¤«¤Ê¤Îºï¤ë¿ô
  r -- ¥í¡¼¥Þ»ú¤Îºï¤ë¿ô
  d ¤Ï¤¤¤é¤Ê¤¤¤è¤¦¤Ë¸«¤¨¤ë¤¬¥Þ¥¯¥í¤Ç¼Â¤Ï»È¤Ã¤Æ¤¤¤ë¤Î¤ÇÉ¬Í×¡£

 */

void
removeKana(uiContext d, yomiContext yc, int k, int r)
{
  int offs;

  offs = yc->kCurs - k;
  yc->kCurs = k;
  kanaReplace(-k, (WCHAR_T *)NULL, 0, 0);
  if (offs > 0) {
    yc->kCurs = offs;
  }
  yc->cmark = yc->kRStartp = yc->kCurs;
  offs = yc->rCurs - r;
  yc->rCurs = r;
  romajiReplace(-r, (WCHAR_T *)NULL, 0, 0);
  if (offs > 0) {
    yc->rCurs = offs;
  }
  yc->rStartp = yc->rCurs;
}

static int YomiNextJishu (uiContext);

static
int YomiNextJishu(uiContext d)
{
  return YomiJishu(d, CANNA_FN_Next);
}

static int YomiPreviousJishu (uiContext);

static
int YomiPreviousJishu(uiContext d)
{
  return YomiJishu(d, CANNA_FN_Prev);
}

static int YomiKanaRotate (uiContext);

static
int YomiKanaRotate(uiContext d)
{
  return YomiJishu(d, CANNA_FN_KanaRotate);
}

static int YomiRomajiRotate (uiContext);

static
int YomiRomajiRotate(uiContext d)
{
  return YomiJishu(d, CANNA_FN_RomajiRotate);
}

static int YomiCaseRotateForward (uiContext);

static
int YomiCaseRotateForward(uiContext d)
{
  return YomiJishu(d, CANNA_FN_CaseRotate);
}

static int YomiZenkaku (uiContext);

static
int YomiZenkaku(uiContext d)
{
  return YomiJishu(d, CANNA_FN_Zenkaku);
}

static int YomiHankaku (uiContext);

static
int YomiHankaku(uiContext d)
{
  if (cannaconf.InhibitHankakuKana)
    return NothingChangedWithBeep(d);
  else
    return YomiJishu(d, CANNA_FN_Hankaku);
}

static int YomiHiraganaJishu (uiContext);

static
int YomiHiraganaJishu(uiContext d)
{
  return YomiJishu(d, CANNA_FN_Hiragana);
}

static int YomiKatakanaJishu (uiContext);

static
int YomiKatakanaJishu(uiContext d)
{
  return YomiJishu(d, CANNA_FN_Katakana);
}

static int YomiRomajiJishu (uiContext);

static
int YomiRomajiJishu(uiContext d)
{
  return YomiJishu(d, CANNA_FN_Romaji);
}

static int YomiToLower (uiContext);
static
int YomiToLower(uiContext d)
{
  return YomiJishu(d, CANNA_FN_ToLower);
}

static int YomiToUpper (uiContext);

static
int YomiToUpper(uiContext d)
{
  return YomiJishu(d, CANNA_FN_ToUpper);
}

static int YomiCapitalize (uiContext);

static
int YomiCapitalize(uiContext d)
{
  return YomiJishu(d, CANNA_FN_Capitalize);
}

/* ±Ñ¸ì¥«¥¿¥«¥ÊÊÑ´¹¤Î¤ä¤ê»Ä¤·

 ¡¦³°Íè¸ìÊÑ´¹¤Ï»ú¼ïÊÑ´¹¤Ë¼è¤ê¹þ¤ß¤¿¤¤

   ¤Ä¤¤¤Ç¤Ê¤Î¤Ç¥¨¥ó¥¸¥óÀÚ¤êÂØ¤¨¤Î¤ä¤ê»Ä¤·

 ¡¦£Ä£Ó£Ï¤¬¤Ê¤¤¾ì¹ç¤Ë¡Ö¤½¤Î¥¨¥ó¥¸¥ó¤ËÀÚ¤êÂØ¤¨¤é¤ì¤Ê¤¤¡×¤È¸À¤¤¤¿¤¤
 ¡¦¤½¤ÎÂ¾¥¨¥é¡¼¥Á¥§¥Ã¥¯

 */

#include "yomimap.h"
