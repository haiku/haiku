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

/*
 * @(#) 102.1 $Id: canna.h 14875 2005-11-12 21:25:31Z bonefish $
 */

/************************************************************************/
/* THIS SOURCE CODE IS MODIFIED FOR TKO BY T.MURAI 1997 */
/************************************************************************/


#ifndef _CANNA_H_
#define _CANNA_H_

#undef DEBUG

#include "cannaconf.h"
#include "widedef.h"
#include <stdio.h>
#include <canna/cannabuild.h>
#include <stdlib.h>
#include <bsd_mem.h>

#include <string.h>
# ifndef index
# define index strchr
# endif

#include <canna/RK.h>
#include <canna/jrkanji.h>

#ifdef BIGPOINTER
#define POINTERINT long long
#else
#define POINTERINT long
#endif

#define	WCHARSIZE	(sizeof(WCHAR_T))

#ifdef HAVE_WCHAR_OPERATION

#ifndef JAPANESE_LOCALE
#define JAPANESE_LOCALE "japan"
#endif

#define MBstowcs mbstowcs
#define WCstombs wcstombs

#else

#define MBstowcs CANNA_mbstowcs
#define WCstombs CANNA_wcstombs

extern int CANNA_wcstombs (char *, WCHAR_T *, int);

#endif

#define STROKE_LIMIT 500 /* ¥¹¥È¥í¡¼¥¯¤ÇÀÜÂ³¤òÀÚ¤ë */

typedef unsigned char BYTE;

/*
 * CANNALIBDIR  -- ¥·¥¹¥Æ¥à¤Î¥«¥¹¥¿¥Þ¥¤¥º¥Õ¥¡¥¤¥ë¤ä¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹
 *                 ¥Æ¡¼¥Ö¥ë¤¬ÃÖ¤«¤ì¤ë¥Ç¥£¥ì¥¯¥È¥ê¡£
 */

//#ifndef CANNALIBDIR
//#define CANNALIBDIR "/usr/lib/canna"
//#endif

/* flags ¤Î¾ðÊó */
#define CANNA_KANJIMODE_TABLE_SHARED	01
#define CANNA_KANJIMODE_EMPTY_MODE	02

/* func ¤ÎÂè»°°ú¿ô */
#define KEY_CALL  0
#define KEY_CHECK 1
#define KEY_SET   2

extern BYTE default_kmap[];

/* menuitem -- ¥á¥Ë¥å¡¼É½¼¨¤Î¹àÌÜ¤òÄêµÁ¤¹¤ë¥Æ¡¼¥Ö¥ë */

typedef struct _menuitem {
  int flag; /* ²¼¤ò¸«¤è */
  union {
    struct _menustruct *menu_next; /* ¥á¥Ë¥å¡¼¤Ø¤Î¥Ý¥¤¥ó¥¿ */
    int fnum;    /* µ¡Ç½ÈÖ¹æ */
    char *misc;  /* ¤½¤ÎÂ¾(lisp ¤Î¥·¥ó¥Ü¥ë¤Ê¤É) */
  } u;
} menuitem;

#define MENU_SUSPEND 0 /* ¤Þ¤À·è¤Þ¤Ã¤Æ¤¤¤Ê¤¤(lisp ¤Î¥·¥ó¥Ü¥ë) */
#define MENU_MENU    1 /* ¥á¥Ë¥å¡¼ */
#define MENU_FUNC    2 /* µ¡Ç½ÈÖ¹æ */

/* menustruct -- ¥á¥Ë¥å¡¼¤ò¤·¤­¤ë¹½Â¤ÂÎ */

typedef struct _menustruct {
  int     nentries; /* ¥á¥Ë¥å¡¼¤Î¹àÌÜ¤Î¿ô */
  WCHAR_T **titles; /* ¥á¥Ë¥å¡¼¤Î¸«½Ð¤·¥ê¥¹¥È */
  WCHAR_T *titledata; /* ¾å¤Î¥ê¥¹¥È¤Î¼ÂÂÖÊ¸»úÎó */
  menuitem *body;   /* ¥á¥Ë¥å¡¼¤ÎÃæ¿È(ÇÛÎó) */
  int     modeid;   /* ¥á¥Ë¥å¡¼¤Î¥â¡¼¥ÉÈÖ¹æ */
  struct _menustruct *prev; /* °ì¤ÄÁ°¤Î¥á¥Ë¥å¡¼¤Ø¤Î¥Ý¥¤¥ó¥¿ */
} menustruct;

typedef struct _menuinfo {
  menustruct *mstruct; /* ¤É¤Î¥á¥Ë¥å¡¼¤Î */
  int        curnum;   /* ¤³¤Ê¤¤¤ÀÁªÂò¤µ¤ì¤¿ÈÖ¹æ¤Ï¤³¤ì¤Ç¤¹¤è */
  struct _menuinfo *next;
} menuinfo;

/* defselection ¤ÇÄêµÁ¤µ¤ì¤¿µ­¹æ´Ø·¸¤Î°ìÍ÷¤ò¤È¤Ã¤Æ¤ª¤¯¹½Â¤ÂÎ */

typedef struct {
  WCHAR_T	**kigo_data;	/* °ìÍ÷É½¼¨¤Î³ÆÍ×ÁÇ¤ÎÇÛÎó */
  WCHAR_T	*kigo_str;	/* °ìÍ÷É½¼¨¤ÎÁ´Í×ÁÇ¤òÆþ¤ì¤ëÇÛÎó */
  int		kigo_size;	/* Í×ÁÇ¤Î¿ô */
  int		kigo_mode;	/* ¤½¤Î¤È¤­¤Î¥â¡¼¥É */
} kigoIchiran;

typedef struct _selectinfo {
  kigoIchiran	*ichiran;	/* ¤É¤Î°ìÍ÷¤Î */
  int		curnum;		/* Á°²óÁªÂò¤µ¤ì¤¿ÈÖ¹æ */
  struct _selectinfo *next;
} selectinfo;

/* deldicinfo -- Ã±¸ìºï½ü¤ÎºÝ¤ËÉ¬Í×¤Ê¼­½ñ¤Î¾ðÊó¤ò¤¤¤ì¤Æ¤ª¤¯¹½Â¤ÂÎ */

#define INDPHLENGTH 16 /* ¼«Î©¸ì¤Ç°ìÈÖÄ¹¤¤ÉÊ»ì¤ÎÄ¹¤µ */

typedef struct _deldicinfo {
  WCHAR_T *name;
  WCHAR_T hcode[INDPHLENGTH];
} deldicinfo;
  
/*
 * glineinfo -- ¸õÊä°ìÍ÷É½¼¨¤Î¤¿¤á¤ÎÆâÉô¾ðÊó¤ò³ÊÇ¼¤·¤Æ¤ª¤¯¤¿¤á¤Î¹½Â¤ÂÎ¡£
 * ¤½¤ì¤¾¤ì¤Î¥á¥ó¥Ð¤Ï°Ê²¼¤Î°ÕÌ£¤ò»ý¤Ä¡£
 *
 * glkosu -- ¤½¤Î¹Ô¤Ë¤¢¤ë¸õÊä¤Î¿ô
 * glhead -- ¤½¤Î¹Ô¤ÎÀèÆ¬¸õÊä¤¬¡¢kouhoinfo¤Î²¿ÈÖÌÜ¤«(0¤«¤é¿ô¤¨¤ë)
 * gllen  -- ¤½¤Î¹Ô¤òÉ½¼¨¤¹¤ë¤¿¤á¤ÎÊ¸»úÎó¤ÎÄ¹¤µ
 * gldata -- ¤½¤Î¹Ô¤òÉ½¼¨¤¹¤ë¤¿¤á¤ÎÊ¸»úÎó¤Ø¤Î¥Ý¥¤¥ó¥¿
 */

typedef struct {
  int glkosu;
  int glhead;
  int gllen;
  WCHAR_T *gldata;
} glineinfo;

/*
 * kouhoinfo -- ¸õÊä°ìÍ÷¤Î¤¿¤á¤ÎÆâÉô¾ðÊó¤ò³ÊÇ¼¤·¤Æ¤ª¤¯¤¿¤á¤Î¹½Â¤ÂÎ
 * ¤½¤ì¤¾¤ì¤Î¥á¥ó¥Ð¤Ï°Ê²¼¤Î°ÕÌ£¤ò»ý¤Ä¡£
 *
 * khretsu -- ¤½¤Î¸õÊä¤¬¤¢¤ë¹Ô
 * khpoint -- ¤½¤Î¸õÊä¤Î¹Ô¤Î¤Ê¤«¤Ç¤Î°ÌÃÖ
 * khdata -- ¤½¤Î¸õÊä¤ÎÊ¸»úÎó¤Ø¤Î¥Ý¥¤¥ó¥¿
 */

typedef struct {
  int khretsu;
  int khpoint;
  WCHAR_T *khdata;
} kouhoinfo;

#define ROMEBUFSIZE 	1024
#define	BANGOSIZE	2	/* ¸õÊä¹ÔÃæ¤Î³Æ¸õÊä¤ÎÈÖ¹æ¤Î¥³¥é¥à¿ô */
#define	BANGOMAX   	9	/* £±¸õÊä¹ÔÃæ¤ÎºÇÂç¸õÊä¿ô */

#define	KIGOBANGOMAX   	16	/* £±¸õÊä¹ÔÃæ¤ÎºÇÂç¸õÊä¿ô */
#define GOBISUU		9

#define	ON		1
#define	OFF		0

#define	NG		-1

#define NO_CALLBACK     (canna_callback_t)0
#define NCALLBACK	4

#define	JISHU_HIRA	0
#define JISHU_ZEN_KATA	1
#define JISHU_HAN_KATA	2
#define JISHU_ZEN_ALPHA	3
#define JISHU_HAN_ALPHA	4
#define MAX_JISHU	5

#define  SENTOU        0x01
#define  HENKANSUMI    0x02
#define  SUPKEY        0x04
#define  GAIRAIGO      0x08
#define  STAYROMAJI    0x10

/* Ã±¸ìÅÐÏ¿¤ÎÉÊ»ì */
#define MEISHI       0
#define KOYUMEISHI   1
#define DOSHI        2
#define KEIYOSHI     3
#define KEIYODOSHI   4
#define FUKUSHI      5
#define TANKANJI     6
#define SUSHI        7
#define RENTAISHI    8
#define SETSUZOKUSHI 9
#define SAHENMEISHI 10
#define MEISHIN     11
#define JINMEI      12
#define KOYUMEISHIN 13
#define GODAN       14
#define RAGYODOSHI  15
#define RAGYOGODAN  16
#define KAMISHIMO   17
#define KEIYOSHIY   18
#define KEIYOSHIN   19
#define KEIYODOSHIY 20
#define KEIYODOSHIN 21
#define FUKUSHIY    22
#define FUKUSHIN    23

/* identifier for each context structures */
#define CORE_CONTEXT       ((BYTE)0)
#define YOMI_CONTEXT       ((BYTE)1)
#define ICHIRAN_CONTEXT    ((BYTE)2)
#define FORICHIRAN_CONTEXT ((BYTE)3)
#define MOUNT_CONTEXT      ((BYTE)4)
#define TOUROKU_CONTEXT    ((BYTE)5)
#define TAN_CONTEXT	   ((BYTE)6)

typedef struct _coreContextRec {
  BYTE id;
  BYTE majorMode, minorMode;
  struct _kanjiMode *prevMode; /* £±¤ÄÁ°¤Î¥â¡¼¥É */
  struct _coreContextRec *next;
} coreContextRec, *coreContext;

typedef coreContext mode_context;

typedef struct  _yomiContextRec {
  /* core ¾ðÊó¤ÈÆ±¤¸¾ðÊó */
  BYTE id;
  BYTE majorMode, minorMode;
  struct _kanjiMode *prevMode;	/* £±¤ÄÁ°¤Î¥â¡¼¥É */
  mode_context    next;

  struct _kanjiMode *curMode;
  struct _tanContextRec	 *left, *right;

  /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹´Ø·¸ */
  struct RkRxDic *romdic;	/* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë */
  WCHAR_T   romaji_buffer[ROMEBUFSIZE];
  /* ¥í¡¼¥Þ»ú¥Ð¥Ã¥Õ¥¡¤Ï rStartp, rEndp ¤Î£²¤Ä¤Î¥¤¥ó¥Ç¥Ã¥¯¥¹¤Ë¤è¤Ã¤Æ´ÉÍý¤µ¤ì
   * ¤ë¡£rStartp ¤Ï¥«¥Ê¤ËÊÑ´¹¤Ç¤­¤Ê¤«¤Ã¤¿¥í¡¼¥Þ»ú¤ÎºÇ½é¤ÎÊ¸»ú¤Ø¤Î¥¤¥ó¥Ç¥Ã
   * ¥¯¥¹¤Ç¤¢¤ê¡¢rEndp ¤Ï¿·¤¿¤Ë¥í¡¼¥Þ»ú¤òÆþÎÏ¤¹¤ë»þ¤Ë¡¢³ÊÇ¼¤¹¤Ù¤­ 
   * romaji_buffer Æâ¤Î¥¤¥ó¥Ç¥Ã¥¯¥¹¤Ç¤¢¤ë¡£¿·¤¿¤ËÆþÎÏ¤µ¤ì¤ë¥í¡¼¥Þ»ú¤Ï¡¢
   * romaji_buffer + rEndp ¤è¤êÀè¤Ë³ÊÇ¼¤µ¤ì¡¢¤½¤Î¥í¡¼¥Þ»ú¤ò¥«¥Ê¤ËÊÑ´¹¤¹
   * ¤ë»þ¤Ï¡¢romaji_buffer + rStartp ¤«¤é rEndp - rStartp ¥Ð¥¤¥È¤ÎÊ¸»ú¤¬
   * ÂÐ¾Ý¤È¤Ê¤ë¡£ */
  int		  rEndp, rStartp, rCurs; /* ¥í¡¼¥Þ»ú¥Ð¥Ã¥Õ¥¡¤Î¥Ý¥¤¥ó¥¿ */
  WCHAR_T         kana_buffer[ROMEBUFSIZE];
  BYTE            rAttr[ROMEBUFSIZE], kAttr[ROMEBUFSIZE];
  int		  kEndp; /* ¤«¤Ê¥Ð¥Ã¥Õ¥¡¤ÎºÇ¸å¤ò²¡¤¨¤ë¥Ý¥¤¥ó¥¿ */
  int             kRStartp, kCurs;

  /* ¤½¤ÎÂ¾¤Î¥ª¥×¥·¥ç¥ó */
  BYTE            myMinorMode;  /* yomiContext ¸ÇÍ­¤Î¥Þ¥¤¥Ê¥â¡¼¥É */
  struct _kanjiMode *myEmptyMode;		/* empty ¥â¡¼¥É¤Ï¤É¤ì¤« */
  long		  generalFlags;		/* see below */
  long		  savedFlags;		/* ¾å¤Î¥Õ¥é¥°¤Î°ìÉô¤Î¥»¡¼¥Ö */
  BYTE		  savedMinorMode;	/* ¥Þ¥¤¥Ê¥â¡¼¥É¤Î¥»¡¼¥Ö */
  BYTE		  allowedChars;		/* see jrkanji.h */
  BYTE		  henkanInhibition;	/* see below */
  int             cursup;		/* ¥í¤«¤Ê¤ÎÊäÄÉ¤Î»þ¤Ë»È¤¦ */
#define SUSPCHARBIAS 100
  int             n_susp_chars;

/* from henkanContext */
  /* ¥«¥Ê´Á»úÊÑ´¹´Ø·¸ */
  int            context;
  int		 kouhoCount;	/* ²¿²ó henkanNext ¤¬Ï¢Â³¤·¤Æ²¡¤µ¤ì¤¿¤« */
  WCHAR_T        echo_buffer[ROMEBUFSIZE];
  WCHAR_T        **allkouho; /* RkGetKanjiList¤ÇÆÀ¤é¤ì¤ëÊ¸»úÎó¤òÇÛÎó¤Ë¤·¤Æ
				¤È¤Ã¤Æ¤ª¤¯¤È¤³¤í */
  int            curbun;     /* ¥«¥ì¥ó¥ÈÊ¸Àá */
  int		 curIkouho;  /* ¥«¥ì¥ó¥È¸õÊä */
  int            nbunsetsu;  /* Ê¸Àá¤Î¿ô */

/* ifdef MEASURE_TIME */
  long		 proctime;   /* ½èÍý»þ´Ö(ÊÑ´¹¤Ç·×Â¬¤¹¤ë */
  long		 rktime;     /* ½èÍý»þ´Ö(RK¤Ë¤«¤«¤ë»þ´Ö) */
/* endif MEASURE_TIME */
/* end of from henkanContext */

/* Ãà¼¡¥³¥ó¥Æ¥­¥¹¥È¤«¤é */
  int		 ye, ys, status;
/* Ãà¼¡¥³¥ó¥Æ¥­¥¹¥È¤«¤é(¤³¤³¤Þ¤Ç) */
  int		 cStartp, cRStartp; /* Ãà¼¡¤ÇÆÉ¤ß¤È¤·¤Æ»Ä¤Ã¤Æ¤¤¤ëÉôÊ¬ */

/* »ú¼ï¥³¥ó¥Æ¥­¥¹¥È¤«¤é */
  BYTE           inhibition;
  BYTE           jishu_kc, jishu_case;
  int            jishu_kEndp, jishu_rEndp;
  short          rmark;
/* »ú¼ï¥³¥ó¥Æ¥­¥¹¥È¤«¤é(¤³¤³¤Þ¤Ç) */

/* adjustContext ¤«¤é */
  int kanjilen, bunlen;           /* ´Á»úÉôÊ¬¡¢Ê¸Àá¤ÎÄ¹¤µ */
/* adjustContext ¤«¤é(¤³¤³¤Þ¤Ç) */
  struct _kanjiMode *tanMode; /* Ã±¸õÊä¤Î¤È¤­¤Î¥â¡¼¥É */
  int tanMinorMode;     /*        ¡·            */

  /* ºî¶ÈÍÑÊÑ¿ô */
  int		  last_rule;		/* Á°²ó¤Î¥í¤«¤ÊÊÑ´¹¤Ë»È¤ï¤ì¤¿¥ë¡¼¥ë */
  WCHAR_T	  *retbuf, *retbufp;
  int		  retbufsize;
  short           pmark, cmark; /* £±¤ÄÁ°¤Î¥Þ¡¼¥¯¤Èº£¤Î¥Þ¡¼¥¯ */
  BYTE            englishtype;  /* ±Ñ¸ì¥¿¥¤¥×(°Ê²¼¤ò¸«¤è) */
} yomiContextRec, *yomiContext;

/* for generalFlags */
#define CANNA_YOMI_MODE_SAVED		0x01L /* savedFlags ¤Ë¤·¤«»È¤ï¤ì¤Ê¤¤ */

#define CANNA_YOMI_BREAK_ROMAN		0x01L
#define CANNA_YOMI_CHIKUJI_MODE		0x02L
#define CANNA_YOMI_CHGMODE_INHIBITTED	0x04L
#define CANNA_YOMI_END_IF_KAKUTEI	0x08L
#define CANNA_YOMI_DELETE_DONT_QUIT	0x10L

#define CANNA_YOMI_IGNORE_USERSYMBOLS	0x20L
#define CANNA_YOMI_IGNORE_HENKANKEY	0x40L

#define CANNA_YOMI_BASE_CHIKUJI		0x80L /* ¿´¤ÏÃà¼¡ */

/* for generalFlags also used in savedFlags */

/* °Ê²¼¤Î ATTRFUNCS ¤Ë¥Þ¥¹¥¯¤µ¤ì¤ë¥Ó¥Ã¥È¤Ï defmode ¤ÎÂ°À­¤È¤·¤Æ»È¤ï¤ì¤ë */
#define CANNA_YOMI_KAKUTEI		0x0100L
#define CANNA_YOMI_HENKAN		0x0200L
#define CANNA_YOMI_ZENKAKU		0x0400L
#define CANNA_YOMI_HANKAKU		0x0800L /* ¼ÂºÝ¤ËÈ¾³Ñ */
#define CANNA_YOMI_HIRAGANA		0x1000L
#define CANNA_YOMI_KATAKANA		0x2000L
#define CANNA_YOMI_ROMAJI		0x4000L
#define CANNA_YOMI_JISHUFUNCS		0x7c00L
#define CANNA_YOMI_ATTRFUNCS		0x7f00L

#define CANNA_YOMI_BASE_HANKAKU		0x8000L /* ¿´¤ÏÈ¾³Ñ */

/* kind of allowed input keys */
#define CANNA_YOMI_INHIBIT_NONE		0
#define CANNA_YOMI_INHIBIT_HENKAN	1
#define CANNA_YOMI_INHIBIT_JISHU	2
#define CANNA_YOMI_INHIBIT_ASHEX	4
#define CANNA_YOMI_INHIBIT_ASBUSHU	8
#define CANNA_YOMI_INHIBIT_ALL		15

/* ¸õÊä°ìÍ÷¤Î¤¿¤á¤Î¥Õ¥é¥° */
#define NUMBERING 			1
#define CHARINSERT			2

#define CANNA_JISHU_UPPER		1
#define CANNA_JISHU_LOWER		2
#define CANNA_JISHU_CAPITALIZE		3
#define CANNA_JISHU_MAX_CASE		4

/* englishtype */
#define CANNA_ENG_KANA			0 /* ½Ì¾®¤¹¤ë¤³¤È */
#define CANNA_ENG_ENG1			1
#define CANNA_ENG_ENG2			2 /* Î¾Ã¼¤Ë¶õÇò¤¬Æþ¤Ã¤Æ¤¤¤ë */
#define CANNA_ENG_NO			3

/* yc->status ¤Î¥Õ¥é¥°(Ãà¼¡ÍÑ) */

#define	CHIKUJI_ON_BUNSETSU		0x0001 /* Ê¸Àá¾å¤Ë¤¢¤ë */
#define	CHIKUJI_OVERWRAP		0x0002 /* Ê¸Àá¤«¤ÄÆÉ¤ß¾õÂÖ¡© */
#define	CHIKUJI_NULL_STATUS	        0 /* ¾å¤Î¤ò¾Ã¤¹ÍÑ */

/* yc ¤ò»È¤¦¥â¡¼¥É¤Î¶èÊÌ(Í¥Àè½ç) */

#define adjustp(yc) (0< (yc)->bunlen)
#define jishup(yc) (0 < (yc)->jishu_kEndp)
#define chikujip(yc) ((yc)->generalFlags & CANNA_YOMI_CHIKUJI_MODE)
#define henkanp(yc) (0 < (yc)->nbunsetsu)

#define chikujiyomiremain(yc) ((yc)->cStartp < (yc)->kEndp)

typedef struct _ichiranContextRec {
  BYTE id;
  BYTE majorMode, minorMode;
  struct _kanjiMode *prevMode;	/* £±¤ÄÁ°¤Î¥â¡¼¥É */
  mode_context    next;

  int            svIkouho;   /* ¥«¥ì¥ó¥È¸õÊä¤ò°ì»þ¤È¤Ã¤Æ¤ª¤¯(°ìÍ÷É½¼¨¹Ô) */
  int            *curIkouho; /* ¥«¥ì¥ó¥È¸õÊä */
  int            nIkouho;    /* ¸õÊä¤Î¿ô(°ìÍ÷É½¼¨¹Ô) */
  int		 tooSmall;   /* ¥«¥é¥à¿ô¤¬¶¹¤¯¤Æ¸õÊä°ìÍ÷¤¬½Ð¤»¤Ê¤¤¤è¥Õ¥é¥° */
  int            curIchar;   /* Ì¤³ÎÄêÊ¸»úÎó¤¢¤ê¤ÎÃ±¸ìÅÐÏ¿¤ÎÃ±¸ìÆþÎÏ¤Î
    							ÀèÆ¬Ê¸»ú¤Î°ÌÃÖ */
  BYTE           inhibit;
  BYTE           flags;	     /* ²¼¤ò¸«¤Æ¤Í */
  WCHAR_T        **allkouho; /* RkGetKanjiList¤ÇÆÀ¤é¤ì¤ëÊ¸»úÎó¤òÇÛÎó¤Ë¤·¤Æ
				¤È¤Ã¤Æ¤ª¤¯¤È¤³¤í */
  WCHAR_T        *glinebufp; /* ¸õÊä°ìÍ÷¤Î¤¢¤ë°ì¹Ô¤òÉ½¼¨¤¹¤ë¤¿¤á¤ÎÊ¸»ú
				Îó¤Ø¤Î¥Ý¥¤¥ó¥¿ */
  kouhoinfo      *kouhoifp;  /* ¸õÊä°ìÍ÷´Ø·¸¤Î¾ðÊó¤ò³ÊÇ¼¤·¤Æ¤ª¤¯¹½Â¤ÂÎ
				¤Ø¤Î¥Ý¥¤¥ó¥¿ */
  glineinfo      *glineifp;  /* ¸õÊä°ìÍ÷´Ø·¸¤Î¾ðÊó¤ò³ÊÇ¼¤·¤Æ¤ª¤¯¹½Â¤ÂÎ
				¤Ø¤Î¥Ý¥¤¥ó¥¿ */
} ichiranContextRec, *ichiranContext;

/* ¥Õ¥é¥°¤Î°ÕÌ£ */
#define ICHIRAN_ALLOW_CALLBACK 1 /* ¥³¡¼¥ë¥Ð¥Ã¥¯¤ò¤·¤Æ¤âÎÉ¤¤ */
#define ICHIRAN_STAY_LONG    0x02 /* Áª¤Ö¤ÈÈ´¤±¤ë */
#define ICHIRAN_NEXT_EXIT    0x04 /* ¼¡¤Î quit ¤ÇÈ´¤±¤ë */


typedef struct _foirchiranContextRec {
  BYTE id;
  BYTE majorMode, minorMode;
  struct _kanjiMode *prevMode;	/* £±¤ÄÁ°¤Î¥â¡¼¥É */
  mode_context    next;

  int            curIkouho;  /* ¥«¥ì¥ó¥È¸õÊä */
  WCHAR_T        **allkouho; /* RkGetKanjiList¤ÇÆÀ¤é¤ì¤ëÊ¸»úÎó¤òÇÛÎó¤Ë¤·¤Æ
				¤È¤Ã¤Æ¤ª¤¯¤È¤³¤í */
  menustruct     *table;  /* Ê¸»úÎó¤È´Ø¿ô¤Î¥Æ¡¼¥Ö¥ë */
  int            *prevcurp;  /* Á°¤Î¥«¥ì¥ó¥È¸õÊä */
} forichiranContextRec, *forichiranContext;

typedef struct _mountContextRec {
  BYTE id;
  BYTE majorMode, minorMode;
  struct _kanjiMode *prevMode;	/* £±¤ÄÁ°¤Î¥â¡¼¥É */
  mode_context    next;

  BYTE            *mountOldStatus; /* ¥Þ¥¦¥ó¥È¤µ¤ì¤Æ¤¤¤ë¤«¤¤¤Ê¤¤¤« */
  BYTE            *mountNewStatus; /* ¥Þ¥¦¥ó¥È¤µ¤ì¤Æ¤¤¤ë¤«¤¤¤Ê¤¤¤« */
  char            **mountList;   /* ¥Þ¥¦¥ó¥È²ÄÇ½¤Ê¼­½ñ¤Î°ìÍ÷ */
  int            curIkouho;     /* ¥«¥ì¥ó¥È¸õÊä */
} mountContextRec, *mountContext;

typedef struct _tourokuContextRec {
  BYTE id;
  BYTE majorMode, minorMode;
  struct _kanjiMode *prevMode;	/* £±¤ÄÁ°¤Î¥â¡¼¥É */
  mode_context    next;

  WCHAR_T        genbuf[ROMEBUFSIZE];
  WCHAR_T        qbuf[ROMEBUFSIZE];
  WCHAR_T        tango_buffer[ROMEBUFSIZE];
  int            tango_len;  /* Ã±¸ìÅÐÏ¿¤ÎÃ±¸ì¤ÎÊ¸»úÎó¤ÎÄ¹¤µ */
  WCHAR_T        yomi_buffer[ROMEBUFSIZE];
  int            yomi_len;   /* Ã±¸ìÅÐÏ¿¤ÎÆÉ¤ß¤ÎÊ¸»úÎó¤ÎÄ¹¤µ */
  int            curHinshi;  /* ÉÊ»ì¤ÎÁªÂò */
  int            workDic;    /* ºî¶ÈÍÑ¤Î¼­½ñ */
  deldicinfo     *workDic2;  /* Ã±¸ìºï½ü²ÄÇ½¤Ê¼­½ñ */
  int            nworkDic2;  /* Ã±¸ìºï½ü²ÄÇ½¤Ê¼­½ñ¤Î¿ô */
  deldicinfo     *workDic3;  /* Ã±¸ìºï½ü¤¹¤ë¼­½ñ */
  int            nworkDic3;  /* Ã±¸ìºï½ü¤¹¤ë¼­½ñ¤Î¿ô */
  struct dicname *newDic;    /* ÄÉ²Ã¤¹¤ë¼­½ñ */
  WCHAR_T        hcode[INDPHLENGTH];  /* Ã±¸ìÅÐÏ¿¤ÎÉÊ»ì */
  int            katsuyou;   /* Ã±¸ìÅÐÏ¿¤ÎÆ°»ì¤Î³èÍÑ·Á */
  WCHAR_T        **udic;     /* Ã±¸ìÅÐÏ¿¤Ç¤­¤ë¼­½ñ (¼­½ñÌ¾) */
  int            nudic;      /* Ã±¸ìÅÐÏ¿¤Ç¤­¤ë¼­½ñ¤Î¿ô */
  int            delContext; /* Ã±¸ìºï½ü¤Ç£±¤Ä¤Î¼­½ñ¤ò¥Þ¥¦¥ó¥È¤¹¤ë */
} tourokuContextRec, *tourokuContext;

typedef struct _tanContextRec {
  BYTE id;
  BYTE majorMode, minorMode;
  struct _kanjiMode *prevMode;	/* £±¤ÄÁ°¤Î¥â¡¼¥É */
  mode_context    next;

  struct _kanjiMode *curMode;
  struct _tanContextRec	 *left, *right;

  struct RkRxDic *romdic;	/* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë */
  BYTE            myMinorMode;  /* yomiContext ¸ÇÍ­¤Î¥Þ¥¤¥Ê¥â¡¼¥É */
  struct _kanjiMode *myEmptyMode;		/* empty ¥â¡¼¥É¤Ï¤É¤ì¤« */
  long generalFlags, savedFlags; /* yomiContext ¤Î¥³¥Ô¡¼ */
  BYTE		  savedMinorMode;	/* ¥Þ¥¤¥Ê¥â¡¼¥É¤Î¥»¡¼¥Ö */
  BYTE		  allowedChars;		/* see jrkanji.h */
  BYTE		  henkanInhibition;	/* see below */

  WCHAR_T *kanji, *yomi, *roma;
  BYTE *kAttr, *rAttr;
} tanContextRec, *tanContext;

struct moreTodo {
  BYTE          todo; /* ¤â¤Ã¤È¤¢¤ë¤Î¡©¤ò¼¨¤¹ */
  BYTE          fnum; /* ´Ø¿ôÈÖ¹æ¡££°¤Ê¤é¼¡¤ÎÊ¸»ú¤Ç¼¨¤µ¤ì¤ë¤³¤È¤ò¤¹¤ë */
  int		ch;   /* Ê¸»ú */
};

/* ¥â¡¼¥ÉÌ¾¤ò³ÊÇ¼¤¹¤ë¥Ç¡¼¥¿¤Î·¿ÄêµÁ */

struct ModeNameRecs {
  int           alloc;
  WCHAR_T       *name;
};

/* °ìÍ÷¤ÎÈÖ¹æ¤Î¥»¥Ñ¥ì¡¼¥¿¡¼¤Î¥Ç¥Õ¥©¥ë¥È¤ÎÄêµÁ */

#define DEFAULTINDEXSEPARATOR     '.'

/*
   wcKanjiAttribute for internal use
 */

typedef struct {
  wcKanjiAttribute u;
  int len;
  char *sp, *ep;
} wcKanjiAttributeInternal;

/* 

  uiContext ¤Ï¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¡¢¥«¥Ê´Á»úÊÑ´¹¤Ë»È¤ï¤ì¤ë¹½Â¤ÂÎ¤Ç¤¢¤ë¡£
  XLookupKanjiString ¤Ê¤É¤Ë¤è¤ëÊÑ´¹¤Ï¡¢¥¦¥£¥ó¥É¥¦¤ËÊ¬Î¥¤µ¤ì¤¿Ê£¿ô¤ÎÆþ
  ÎÏ¥Ý¡¼¥È¤ËÂÐ±þ¤·¤Æ¤¤¤ë¤Î¤Ç¡¢ÆþÎÏÃæ¤Î¥í¡¼¥Þ»ú¤Î¾ðÊó¤ä¡¢¥«¥Ê´Á»úÊÑ´¹
  ¤ÎÍÍ»Ò¤Ê¤É¤ò¤½¤ì¤¾¤ì¤Î¥¦¥£¥ó¥É¥¦Ëè¤ËÊ¬Î¥¤·¤ÆÊÝ»ý¤·¤Æ¤ª¤«¤Ê¤±¤ì¤Ð¤Ê
  ¤é¤Ê¤¤¡£¤³¤Î¹½Â¤ÂÎ¤Ï¤½¤Î¤¿¤á¤Ë»È¤ï¤ì¤ë¹½Â¤ÂÎ¤Ç¤¢¤ë¡£
 
  ¹½Â¤ÂÎ¤Î¥á¥ó¥Ð¤¬¤É¤Î¤è¤¦¤Ê¤â¤Î¤¬¤¢¤ë¤«¤Ï¡¢ÄêµÁ¤ò»²¾È¤¹¤ë¤³¤È
 
 */

typedef struct _uiContext {

  /* XLookupKanjiString¤Î¥Ñ¥é¥á¥¿ */
  WCHAR_T        *buffer_return;
  int            n_buffer;
  wcKanjiStatus    *kanji_status_return;

  /* XLookupKanjiString¤ÎÌá¤êÃÍ¤Ç¤¢¤ëÊ¸»úÎó¤ÎÄ¹¤µ */
  int		 nbytes;

  /* ¥­¥ã¥é¥¯¥¿ */
  int ch;

  /* ¥»¥ß¥°¥í¡¼¥Ð¥ë¥Ç¡¼¥¿ */
  int		 contextCache;	 /* ÊÑ´¹¥³¥ó¥Æ¥¯¥¹¥È¥­¥ã¥Ã¥·¥å */
  struct _kanjiMode *current_mode;
  BYTE		 majorMode, minorMode;	 /* Ä¾Á°¤Î¤â¤Î */

  short		 curkigo;	 /* ¥«¥ì¥ó¥Èµ­¹æ(µ­¹æÁ´ÈÌ) */
  char           currussia;	 /* ¥«¥ì¥ó¥Èµ­¹æ(¥í¥·¥¢Ê¸»ú) */
  char           curgreek;	 /* ¥«¥ì¥ó¥Èµ­¹æ(¥®¥ê¥·¥ãÊ¸»ú) */
  char           curkeisen;	 /* ¥«¥ì¥ó¥Èµ­¹æ(·ÓÀþ) */
  short          curbushu;       /* ¥«¥ì¥ó¥ÈÉô¼óÌ¾ */
  int            ncolumns;	 /* °ì¹Ô¤Î¥³¥é¥à¿ô¡¢¸õÊä°ìÍ÷¤Î»þ¤ËÍÑ¤¤¤é¤ì¤ë */
  WCHAR_T        genbuf[ROMEBUFSIZE];	/* ÈÆÍÑ¥Ð¥Ã¥Õ¥¡ */
  short          strokecounter;  /* ¥­¡¼¥¹¥È¥í¡¼¥¯¤Î¥«¥¦¥ó¥¿
				    ¥í¡¼¥Þ»ú¥â¡¼¥É¤Ç¥¯¥ê¥¢¤µ¤ì¤ë */
  wcKanjiAttributeInternal *attr;

  /* ¥ê¥¹¥È¥³¡¼¥ë¥Ð¥Ã¥¯´ØÏ¢ */
  char           *client_data;   /* ¥¢¥×¥ê¥±¡¼¥·¥ç¥óÍÑ¥Ç¡¼¥¿ */
  int            (*list_func) (char *, int, WCHAR_T **, int, int *);
                 /* ¥ê¥¹¥È¥³¡¼¥ë¥Ð¥Ã¥¯´Ø¿ô */
  /* ¤½¤ÎÂ¾ */
  char		 flags;		 /* ²¼¤ò¸«¤Æ¤Í */
  char		 status;	 /* ¤É¤Î¤è¤¦¤Ê¾õÂÖ¤ÇÊÖ¤Ã¤¿¤Î¤«¤ò¼¨¤¹ÃÍ
				    ¤½¤Î¥â¡¼¥É¤È¤·¤Æ¡¢
				     ¡¦½èÍýÃæ
				     ¡¦½èÍý½ªÎ»
				     ¡¦½èÍýÃæÃÇ
				     ¡¦¤½¤ÎÂ¾
				    ¤¬¤¢¤ë¡£(²¼¤ò¸«¤è) */

  /* ¥³¡¼¥ë¥Ð¥Ã¥¯¥Á¥§¡¼¥ó */
  struct callback *cb;

  /* ¤â¤Ã¤È¤¹¤ë¤³¤È¤¬¤¢¤ë¤è¤È¤¤¤¦¹½Â¤ÂÎ */
  struct moreTodo more;

  /* ¥¯¥¤¥Ã¥È¥Á¥§¡¼¥ó */
  menustruct *prevMenu;

  /* ³Æ¥á¥Ë¥å¡¼¤ÇÁª¤Ð¤ì¤¿ÈÖ¹æ¤òµ­Ï¿¤·¤Æ¤ª¤¯¹½Â¤ÂÎ¤Ø¤Î¥Ý¥¤¥ó¥¿ */
  menuinfo *minfo;

  /* ³Æ°ìÍ÷¤ÇÁª¤Ð¤ì¤¿ÈÖ¹æ¤òµ­Ï¿¤·¤Æ¤ª¤¯¹½Â¤ÂÎ¤Ø¤Î¥Ý¥¤¥ó¥¿ */
  selectinfo *selinfo;

  /* ¥µ¥Ö¥³¥ó¥Æ¥¯¥¹¥È¤Ø¤Î¥ê¥ó¥¯ */
  mode_context   modec;		/* Á´Éô¤³¤³¤Ë¤Ä¤Ê¤°Í½Äê */
} uiContextRec, *uiContext;

/* uiContext ¤Î flags ¤Î¥Ó¥Ã¥È¤Î°ÕÌ£ */
#define PLEASE_CLEAR_GLINE	1	/* GLine ¤ò¾Ã¤·¤Æ¤Í */
#define PCG_RECOGNIZED		2	/* GLine ¤ò¼¡¤Ï¾Ã¤·¤Þ¤¹¤è */
#define MULTI_SEQUENCE_EXECUTED	4	/* ¤µ¤Ã¤­¥Þ¥ë¥Á¥·¡¼¥±¥ó¥¹¤¬¹Ô¤ï¤ì¤¿ */

#define EVERYTIME_CALLBACK	0
#define EXIT_CALLBACK		1
#define QUIT_CALLBACK		2
#define AUX_CALLBACK		3

/* 
 * ¥«¥Ê´Á»úÊÑ´¹¤Î¤¿¤á¤ÎÍÍ¡¹¤Ê¥­¡¼¥Þ¥Ã¥×¥Æ¡¼¥Ö¥ë 
 * ¥­¡¼¥Þ¥Ã¥×¥Æ¡¼¥Ö¥ë¤Ï½èÍý´Ø¿ô¤Ø¤Î¥Ý¥¤¥ó¥¿¤ÎÇÛÎó¤È¤Ê¤Ã¤Æ¤¤¤ë¡£
 */

struct funccfunc {
  BYTE funcid;
  int (*cfunc) (struct _uiContext *);
};

typedef struct _kanjiMode {
  int (*func) (struct _uiContext *, struct _kanjiMode *, int, int, int);
  BYTE *keytbl;
  int flags;			/* ²¼¤ò¸«¤è */
  struct funccfunc *ftbl;
} *KanjiMode, KanjiModeRec;

struct callback {
  int (*func[NCALLBACK]) (struct _uiContext *, int, mode_context);
  mode_context    env;
  struct callback *next;
};

/* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ë */
     
extern struct RkRxDic *RkwOpenRoma(char *romaji);
extern void RkwCloseRoma(struct RkRxDic *rdic);

/*
 * ¼­½ñ¤ÎÌ¾Á°¤òÆþ¤ì¤Æ¤ª¤¯ÊÑ¿ô
 */

struct dicname {
  struct dicname *next;
  char *name;
  int dictype;
  unsigned long dicflag;
};

/* dictype ¤Ë¤Ï°Ê²¼¤Î¤¤¤º¤ì¤«¤¬Æþ¤ë */
#define DIC_PLAIN 0     /* ÄÌ¾ï¤ÎÍøÍÑ */
#define DIC_USER  1     /* Ã±¸ìÅÐÏ¿ÍÑ¼­½ñ */
#define DIC_BUSHU 2     /* Éô¼óÊÑ´¹ÍÑ¼­½ñ */
#define DIC_GRAMMAR 3   /* Ê¸Ë¡¼­½ñ */
#define DIC_RENGO 4     /* Ï¢¸ì³Ø½¬¼­½ñ */
#define DIC_KATAKANA 5  /* ¥«¥¿¥«¥Ê³Ø½¬¼­½ñ */
#define DIC_HIRAGANA 6  /* ¤Ò¤é¤¬¤Ê³Ø½¬¼­½ñ */

/* dicflag ¤Ë¤Ï°Ê²¼¤Î¤¤¤º¤ì¤«¤¬Æþ¤ë */
#define DIC_NOT_MOUNTED  0
#define DIC_MOUNTED      1
#define DIC_MOUNT_FAILED 2

extern struct dicname *kanjidicnames;

/*
 * ¥¨¥é¡¼¤Î¥á¥Ã¥»¡¼¥¸¤òÆþ¤ì¤Æ¤ª¤¯ÊÑ¿ô
 */

extern char *jrKanjiError;

/*
 * ¥Ç¥Ð¥°Ê¸¤òÉ½¼¨¤¹¤ë¤«¤É¤¦¤«¤Î¥Õ¥é¥°
 */

extern int iroha_debug;

/*
 * ¥­¡¼¥·¡¼¥±¥ó¥¹¤òÈ¯À¸¤¹¤ë¤è¤¦¤Ê¥­¡¼
 */

#define IrohaFunctionKey(key) \
  ((0x80 <= (int)(unsigned char)(key) &&  \
    (int)(unsigned char)(key) <= 0x8b) || \
   (0x90 <= (int)(unsigned char)(key) &&  \
    (int)(unsigned char)(key) <= 0x9b) || \
   (0xe0 <= (int)(unsigned char)(key) &&  \
    (int)(unsigned char)(key) <= 0xff) )

/* selectOne ¤Ç¥³¡¼¥ë¥Ð¥Ã¥¯¤òÈ¼¤¦¤«¤É¤¦¤«¤òÉ½¤¹¥Þ¥¯¥í */

#define WITHOUT_LIST_CALLBACK 0
#define WITH_LIST_CALLBACK    1

/*
 * Rk ´Ø¿ô¤ò¥È¥ì¡¼¥¹¤¹¤ë¤¿¤á¤ÎÌ¾Á°¤Î½ñ¤­´¹¤¨¡£
 */

#ifdef DEBUG
#include "traceRK.h"
#endif /* DEBUG */

/*
 * ¥Ç¥Ð¥°¥á¥Ã¥»¡¼¥¸½ÐÎÏÍÑ¤Î¥Þ¥¯¥í
 */

#ifdef DEBUG
#define debug_message(fmt, x, y, z)	dbg_msg(fmt, x, y, z)
#else /* !DEBUG */
#define debug_message(fmt, x, y, z)
#endif /* !DEBUG */

/*
 * malloc ¤Î¥Ç¥Ð¥°
 */

#ifdef DEBUG_ALLOC
extern char *debug_malloc (int);
extern int fail_malloc;
#define malloc(n) debug_malloc(n)
#endif /* DEBUG_MALLOC */

/*
 * ¿·¤·¤¤¥â¡¼¥É¤òÄêµÁ¤¹¤ë¹½Â¤ÂÎ
 */

typedef struct {
  char           *romaji_table; /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¥Æ¡¼¥Ö¥ëÌ¾(EUC) */
  struct RkRxDic *romdic;	 /* ¥í¡¼¥Þ»ú¼­½ñ¹½Â¤ÂÎ */
  int             romdic_owner;  /* ¥í¡¼¥Þ»ú¼­½ñ¤ò¼«Ê¬¤ÇOpen¤·¤¿¤« */
  long            flags;	 /* flags for yomiContext->generalFlags */
  KanjiMode       emode;	 /* current_mode ¤ËÆþ¤ë¹½Â¤ÂÎ */
} newmode;

/* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¤òÊäÂ­¤¹¤ë¥­¡¼¤ÈÊ¸»ú¤ÎÊÑ´¹¥Æ¡¼¥Ö¥ë */

typedef struct {
  WCHAR_T	key;		/* ¥­¡¼ */
  WCHAR_T       xkey;
  int		groupid;	/* ¥°¥ë¡¼¥×id */
  int           ncand;          /* ¸õÊä¤Î¿ô */
  WCHAR_T       **cand;         /* ¸õÊä¤ÎÇÛÎó */
  WCHAR_T	*fullword;	/* ¸õÊäÎó (¸õÊä1@¸õÊä2@...¸õÊän@@) */
#ifdef WIN_CANLISP
  int		fullwordsize;	/* sizeof fullword by WCHAR_T unit */
#endif
} keySupplement;

#define MAX_KEY_SUP 64

#define HEX_USUAL     0
#define HEX_IMMEDIATE 1

#define ModeInfoStyleIsString		0
#define ModeInfoStyleIsNumeric		1
#define ModeInfoStyleIsExtendedNumeric	2
#define ModeInfoStyleIsBaseNumeric      3
#define MaxModeInfoStyle                ModeInfoStyleIsBaseNumeric

#define killmenu(d) ((d)->prevMenu = (menustruct *)0)
#define	defineEnd(d) killmenu(d)
#define	deleteEnd(d) killmenu(d)

/* defmode¡¢defselection¡¢defmenu ÍÑ¤Î¹½Â¤ÂÎ */

typedef struct _extra_func {
  int  		fnum;		/* ´Ø¿ôÈÖ¹æ */
  int		keyword;	/* ¿·¤·¤¤¥â¡¼¥É¤¬ÄêµÁ¤µ¤ì¤¿¥­¡¼¥ï¡¼¥É */
  WCHAR_T	*display_name;	/* ¥â¡¼¥ÉÉ½¼¨Ì¾ */
  union {
    newmode 	*modeptr;	/* defmode ¤ËÂÐ±þ¤¹¤ë¹½Â¤ÂÎ */
    kigoIchiran	*kigoptr;	/* defselection ¤ËÂÐ±þ¤¹¤ë¹½Â¤ÂÎ */
    menustruct	*menuptr;	/* defmenu ¤ËÂÐ±þ¤¹¤ë¹½Â¤ÂÎ */
  } u;
#ifdef BINARY_CUSTOM
  int           mid;
  char          *symname;
#endif
  struct _extra_func *next;
} extraFunc;

#define EXTRA_FUNC_DEFMODE	1
#define EXTRA_FUNC_DEFSELECTION	2
#define EXTRA_FUNC_DEFMENU	3

#define tanbunMode(d, tan) /* tanContext ´ØÏ¢¥â¡¼¥É¤Ø¤Î°Ü¹Ô */ \
  { extern KanjiModeRec tankouho_mode; (d)->current_mode = &tankouho_mode; \
    (d)->modec = (mode_context)(tan); currentModeInfo(d); }

#define freeForIchiranContext(fc) free((char *)fc)
#define freeIchiranContext(ic) free((char *)ic)
#define freeYomiContext(yc) free((char *)yc)
#define freeCoreContext(cc) free((char *)cc)

#define DEFAULT_CANNA_SERVER_NAME "cannaserver"

#ifndef	_UTIL_FUNCTIONS_DEF_

#define	_UTIL_FUNCTIONS_DEF_

/* ¤«¤ó¤Ê¤Î¥Ð¡¼¥¸¥ç¥ó¤òÄ´¤Ù¤ë */
#define canna_version(majv, minv) ((majv) * 1024 + (minv))

/* ¤è¤¯¥¹¥Ú¥ë¥ß¥¹¤¹¤ë¤Î¤Ç¥³¥ó¥Ñ¥¤¥ë»þ¤Ë¤Ò¤Ã¤«¤«¤ë¤è¤¦¤ËÆþ¤ì¤ë */
extern int RkwGoto (char *, int);

/* storing customize configuration to the following structure. */
struct CannaConfig { /* °Ê²¼¤Î¥³¥á¥ó¥È¤Ï¥À¥¤¥¢¥í¥°¤Ê¤É¤Ëµ­½Ò¤¹¤ë¤È¤­¤Ê¤É¤Ë
			ÍÑ¤¤¤ë¸ì×Ã¡£! ¤¬ÀèÆ¬¤Ë¤Ä¤¤¤Æ¤¤¤ë¤Î¤Ï¥í¥¸¥Ã¥¯¤¬È¿Å¾
			¤·¤Æ¤¤¤ë¤³¤È¤òÉ½¤¹ */
  int CannaVersion;  /* (¸ß´¹ÍÑ) ¤«¤ó¤Ê¤Î¥Ð¡¼¥¸¥ç¥ó */
  int kouho_threshold; /* ÊÑ´¹¥­¡¼¤ò²¿²óÂÇ¤Ä¤È°ìÍ÷¤¬½Ð¤ë¤« */
  int strokelimit;  /* (¸ß´¹ÍÑ) ²¿¥¹¥È¥í¡¼¥¯¥¢¥ë¥Õ¥¡¥Ù¥Ã¥È¤òÆþ¤ì¤ë¤ÈÀÚÃÇ¤« */
  int indexSeparator; /* (¸ß´¹ÍÑ) °ìÍ÷»þ¤Î¥¤¥ó¥Ç¥Ã¥¯¥¹¤Î¥»¥Ñ¥ì¡¼¥¿ */
  BYTE ReverseWidely; /* È¿Å¾ÎÎ°è¤ò¹­¤¯¤È¤ë       */
  BYTE chikuji;       /* Ãà¼¡¼«Æ°ÊÑ´¹             */
  BYTE Gakushu;       /* ³Ø½¬¤¹¤ë¤«¤É¤¦¤«         */
  BYTE CursorWrap;    /* ±¦Ã¼¤Ç±¦¤Çº¸Ã¼¤Ø¹Ô¤¯     */
  BYTE SelectDirect;  /* °ìÍ÷»þ¡¢ÁªÂò¤Ç°ìÍ÷¤òÈ´¤±¤ë */
  BYTE HexkeySelect;  /* (¸ß´¹ÍÑ) 16¿Ê¿ô»ú¤Ç¤â°ìÍ÷ÁªÂò²Ä */
  BYTE BunsetsuKugiri; /* ÊÑ´¹»þÊ¸Àá´Ö¤Ë¶õÇò¤òÁÞÆþ  */
  BYTE ChBasedMove;   /* !¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹Ã±°Ì¤Î¥«¡¼¥½¥ë°ÜÆ°   */
  BYTE ReverseWord;   /* (¸ß´¹ÍÑ) °ìÍ÷¤Ç¸ì¤òÈ¿Å¾¤¹¤ë */
  BYTE QuitIchiranIfEnd; /* °ìÍ÷ËöÈø¤Ç°ìÍ÷¤òÊÄ¤¸¤ë */
  BYTE kakuteiIfEndOfBunsetsu; /* Ê¸ÀáËöÈø¤Ç±¦°ÜÆ°¤Ç³ÎÄê¤¹¤ë */
  BYTE stayAfterValidate; /* !°ìÍ÷¤ÇÁªÂò¸å¼¡¤ÎÊ¸Àá¤Ø°ÜÆ° */
  BYTE BreakIntoRoman;    /* BS¥­¡¼¤Ç¥í¡¼¥Þ»ú¤ØÌá¤¹ */
  BYTE grammaticalQuestion; /* (¸ß´¹ÍÑ) Ã±¸ìÅÐÏ¿»þÊ¸Ë¡Åª¼ÁÌä¤ò¤¹¤ë */
  BYTE forceKana;           /* Isn't this used? */
  BYTE kCount;        /* (¸ß´¹ÍÑ) ¸õÊä¤¬²¿ÈÖÌÜ¤«¤òÉ½¼¨¤¹¤ë */
  BYTE LearnNumericalType;  /* Isn't this used? */
  BYTE BackspaceBehavesAsQuit; /* Ãà¼¡¼«Æ°ÊÑ´¹»þ BS ¥­¡¼¤ÇÁ´ÂÎ¤òÆÉ¤ß¤ËÌá¤¹ */
  BYTE iListCB;       /* (¸ß´¹ÍÑ) ¥ê¥¹¥È¥³¡¼¥ë¥Ð¥Ã¥¯¤ò¶Ø»ß¤¹¤ë */
  BYTE keepCursorPosition;  /* !ÊÑ´¹»þ¤ËBSÂÇ¸°»þ¥«¡¼¥½¥ë°ÌÃÖ¤òËöÈø¤Ë°ÜÆ° */
  BYTE abandonIllegalPhono; /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¤Ë»È¤ï¤ì¤Ê¤¤¥­¡¼¤ò¼Î¤Æ¤ë */
  BYTE hexCharacterDefiningStyle; /* Isn't this used? */
  BYTE kojin;         /* ¸Ä¿Í³Ø½¬ */
  BYTE indexHankaku;  /* (¸ß´¹ÍÑ) °ìÍ÷»þ¤Î¥¤¥ó¥Ç¥Ã¥¯¥¹¤òÈ¾³Ñ¤Ë¤¹¤ë */
  BYTE allowNextInput; /* ¸õÊä°ìÍ÷É½¼¨»þ¡¢¼¡¤ÎÆþÎÏ¤¬²ÄÇ½¤Ë¤¹¤ë */
  BYTE doKatakanaGakushu; /* Isn't this used? */
  BYTE doHiraganaGakushu; /* Isn't this used? */ 
  BYTE ChikujiContinue; /* Ãà¼¡¼«Æ°ÊÑ´¹»þ¼¡¤ÎÆþÎÏ¤Ç´ûÊÑ´¹ÉôÊ¬¤ò³ÎÄê¤·¤Ê¤¤ */
  BYTE RenbunContinue;  /* Ï¢Ê¸ÀáÊÑ´¹»þ¼¡¤ÎÆþÎÏ¤Ç´ûÊÑ´¹ÉôÊ¬¤ò³ÎÄê¤·¤Ê¤¤ */
  BYTE MojishuContinue; /* »ú¼ïÊÑ´¹»þ¼¡¤ÎÆþÎÏ¤Ç´ûÊÑ´¹ÉôÊ¬¤ò³ÎÄê¤·¤Ê¤¤ */
  BYTE chikujiRealBackspace; /* Ãà¼¡¼«Æ°ÊÑ´¹»þBS¤ÇÉ¬¤º°ìÊ¸»ú¾Ãµî¤¹¤ë */
  BYTE ignore_case;   /* ÂçÊ¸»ú¾®Ê¸»ú¤ò¶èÊÌ¤·¤Ê¤¤ */
  BYTE romaji_yuusen; /* ¥í¡¼¥Þ»ú¤«¤ÊÊÑ´¹¤òÍ¥Àè¤¹¤ë */
  BYTE auto_sync;     /* Äê´üÅª¤Ë¼­½ñ¤ò½ñ¤­Ìá¤¹ */
  BYTE quickly_escape; /* (¸ß´¹ÍÑ) °ìÍ÷É½¼¨»þ¡¢ÁªÂò¤ÇÂ¨ºÂ¤Ë°ìÍ÷¤òÈ´¤±¤ë */
  BYTE InhibitHankakuKana; /* È¾³Ñ¥«¥¿¥«¥Ê¤Î¶Ø»ß */
  BYTE code_input;    /* ¥³¡¼¥É(0: jis, 1: sjis, 2: ¶èÅÀ) */
};

#define CANNA_CODE_JIS   0
#define CANNA_CODE_SJIS  1
#define CANNA_CODE_KUTEN 2
#define CANNA_MAX_CODE   3

extern struct CannaConfig cannaconf;
typedef int (* canna_callback_t) (uiContext, int, mode_context);

/* RKkana.c */
int RkCvtZen(unsigned char *zen, int maxzen, unsigned char *han, int maxhan);
int RkCvtHan(unsigned char *han, int maxhan, unsigned char *zen, int maxzen);
int RkCvtKana(unsigned char *kana, int maxkana, unsigned char *hira, int maxhira);
int RkCvtHira(unsigned char *hira, int maxhira, unsigned char *kana, int maxkana);
int RkCvtNone(unsigned char *dst, int maxdst, unsigned char *src, int maxsrc);
int RkCvtEuc(unsigned char *euc, int maxeuc, unsigned char *sj, int maxsj);
int RkCvtSuuji(unsigned char *dst, int maxdst, unsigned char *src, int maxsrc, int format);
int RkwCvtHan(WCHAR_T *dst, int maxdst, WCHAR_T *src, int srclen);
int RkwCvtHira(WCHAR_T *dst, int maxdst, WCHAR_T *src, int srclen);
int RkwCvtKana(WCHAR_T *dst, int maxdst, WCHAR_T *src, int srclen);
int RkwCvtZen(WCHAR_T *dst, int maxdst, WCHAR_T *src, int srclen);
int RkwCvtNone(WCHAR_T *dst, int maxdst, WCHAR_T *src, int srclen);
int RkwMapRoma(struct RkRxDic *romaji, WCHAR_T *dst, int maxdst, WCHAR_T *src, int srclen, int flags, int *status);
int RkwMapPhonogram(struct RkRxDic *romaji, WCHAR_T *dst, int maxdst, WCHAR_T *src, int srclen, WCHAR_T key, int flags, int *ulen, int *dlen, int *tlen, int *rule);
int RkwCvtRoma(struct RkRxDic *romaji, WCHAR_T *dst, int maxdst, WCHAR_T *src, int srclen, int flags);

/* RKroma.c */
int compar(struct romaRec *p, struct romaRec *q);
struct RkRxDic *RkwOpenRoma(char *romaji);
void RkwCloseRoma(struct RkRxDic *rdic);
struct RkRxDic *RkOpenRoma(char *romaji);
void RkCloseRoma(struct RkRxDic *rdic);
int RkMapRoma(struct RkRxDic *rdic, unsigned char *dst, int maxdst, unsigned char *src, int maxsrc, int flags, int *status);
int RkMapPhonogram(struct RkRxDic *rdic, unsigned char *dst, int maxdst, unsigned char *src, int srclen, unsigned key, int flags, int *used_len_return, int *dst_len_return, int *tmp_len_return, int *rule_id_inout);
int RkCvtRoma(struct RkRxDic *rdic, unsigned char *dst, int maxdst, unsigned char *src, int maxsrc, unsigned flags);

/* bunsetsu.c */
int enterAdjustMode(uiContext d, yomiContext yc);
int leaveAdjustMode(uiContext d, yomiContext yc);

/* bushu.c */
int initBushuTable(void);
int getForIchiranContext(uiContext d);
void popForIchiranMode(uiContext d);
int BushuMode(uiContext d);
int ConvertAsBushu(uiContext d);

/* chikuji.c */
void clearHenkanContext(yomiContext yc);
int chikujiInit(uiContext d);
int ChikujiSubstYomi(uiContext d);
int ChikujiTanDeletePrevious(uiContext d);
void moveToChikujiTanMode(uiContext d);
void moveToChikujiYomiMode(uiContext d);

/* commondata.c */
void InitCannaConfig(struct CannaConfig *cf);
void restoreBindings(void);

/* defaultmap.c */
int searchfunc (uiContext, KanjiMode, int, int, int);
int CYsearchfunc(uiContext d, KanjiMode mode, int whattodo, int key, int fnum);

/* ebind.c */
int XLookupKanji2(unsigned int dpy, unsigned int win, char *buffer_return, int bytes_buffer, int nbytes, int functionalChar, jrKanjiStatus *kanji_status_return);
int XKanjiControl2(unsigned int display, unsigned int window, unsigned int request, BYTE *arg);

/* empty.c */
extraFunc *FindExtraFunc(int fnum);
int getBaseMode(yomiContext yc);
void EmptyBaseModeInfo(uiContext d, yomiContext yc);
int EmptyBaseHira(uiContext d);
int EmptyBaseKata(uiContext d);
int EmptyBaseEisu(uiContext d);
int EmptyBaseZen(uiContext d);
int EmptyBaseHan(uiContext d);
int EmptyBaseKana(uiContext d);
int EmptyBaseKakutei(uiContext d);
int EmptyBaseHenkan(uiContext d);

/* engine.c */
int RkSetServerName(char *s);
char *RkGetServerHost(void);
void close_engine(void);

/* henkan.c */
int KanjiInit(void);
int KanjiFin(void);
void freeTanContext(tanContext tan);
int doTanConvertTb(uiContext d, yomiContext yc);
int YomiBubunKakutei(uiContext d);
yomiContext newFilledYomiContext(mode_context next, KanjiMode prev);
int TanBubunMuhenkan(uiContext d);
int prepareHenkanMode(uiContext d);
int doHenkan(uiContext d, int len, WCHAR_T *kanji);
int TanNop(uiContext d);
int TanForwardBunsetsu(uiContext d);
int TanBackwardBunsetsu(uiContext d);
int TanKouhoIchiran(uiContext d);
int TanNextKouho(uiContext d);
int TanPreviousKouho(uiContext d);
int TanHiragana(uiContext d);
int TanKatakana(uiContext d);
int TanRomaji(uiContext d);
int TanUpper(uiContext d);
int TanCapitalize(uiContext d);
int TanZenkaku(uiContext d);
int TanHankaku(uiContext d);
int TanKanaRotate(uiContext d);
int TanRomajiRotate(uiContext d);
int TanCaseRotateForward(uiContext d);
int TanBeginningOfBunsetsu(uiContext d);
int TanEndOfBunsetsu(uiContext d);
int tanMuhenkan(uiContext d, int kCurs);
int TanMuhenkan(uiContext d);
int TanDeletePrevious(uiContext d);
void finishTanKakutei(uiContext d);
int TanKakutei(uiContext d);
int TanPrintBunpou(uiContext d);
void jrKanjiPipeError(void);
void setMode(uiContext d, tanContext tan, int forw);
int TbForward(uiContext d);
int TbBackward(uiContext d);
int TbBeginningOfLine(uiContext d);
int TbEndOfLine(uiContext d);

/* hex.c */
int HexMode(uiContext d);

/* ichiran.c */
int initIchiran(void);
void makeGlineStatus(uiContext d);
void freeIchiranBuf(ichiranContext ic);
void freeGetIchiranList(WCHAR_T **buf);
WCHAR_T **getIchiranList(int context, int *nelem, int *currentkouho);
ichiranContext newIchiranContext(void);
int selectOne (uiContext d, WCHAR_T **buf, int *ck, int nelem, int bangomax, unsigned inhibit, int currentkouho, int allowcallback, canna_callback_t everyTimeCallback, canna_callback_t exitCallback, canna_callback_t quitCallback, canna_callback_t auxCallback);
int allocIchiranBuf(uiContext d);
int tanKouhoIchiran(uiContext d, int step);
int IchiranQuit(uiContext d);
int IchiranNop(uiContext d);
int IchiranForwardKouho(uiContext d);
int IchiranBackwardKouho(uiContext d);
int IchiranPreviousKouhoretsu(uiContext d);
int IchiranNextKouhoretsu(uiContext d);
int IchiranBeginningOfKouho(uiContext d);
int IchiranEndOfKouho(uiContext d);
void ichiranFin(uiContext d);

/* jishu.c */
void enterJishuMode(uiContext d, yomiContext yc);
void leaveJishuMode(uiContext d, yomiContext yc);
int extractJishuString(yomiContext yc, WCHAR_T *s, WCHAR_T *e, WCHAR_T **sr, WCHAR_T **er);

/* jrbind.c */
int XwcLookupKanji2(unsigned int dpy, unsigned int win, WCHAR_T *buffer_return, int nbuffer, int nbytes, int functionalChar, wcKanjiStatus *kanji_status_return);
int XwcKanjiControl2(unsigned int display, unsigned int window, unsigned int request, BYTE *arg);
struct callback *pushCallback(uiContext d, mode_context env, canna_callback_t ev, canna_callback_t ex, canna_callback_t qu, canna_callback_t au);
void popCallback(uiContext d);

/* kctrl.c */
int initRomeStruct(uiContext d, int flg);
void freeRomeStruct(uiContext d);
uiContext keyToContext(unsigned int data1, unsigned int data2);
struct bukRec *internContext(unsigned int data1, unsigned int data2, uiContext context);
void rmContext(unsigned int data1, unsigned int data2);
void addWarningMesg(char *s);
int escapeToBasicStat(uiContext d, int how);
void makeAllContextToBeClosed(int flag);
int _do_func_slightly(uiContext d, int fnum, mode_context mode_c, KanjiMode c_mode);
int _doFunc(uiContext d, int fnum);
int _afterDoFunc(uiContext d, int retval);
int doFunc(uiContext d, int fnum);
int wcCloseKanjiContext (int context);
int jrCloseKanjiContext (int context);
int ToggleChikuji(uiContext d, int flg);
int kanjiControl(int request, uiContext d, caddr_t arg);

/* keydef.c */
int initKeyTables(void);
void restoreDefaultKeymaps(void);
int changeKeyfunc(int modenum, int key, int fnum, unsigned char *actbuff, unsigned char *keybuff);
int changeKeyfuncOfAll(int key, int fnum, unsigned char *actbuff, unsigned char *keybuff);
unsigned char *actFromHash(unsigned char *tbl_ptr, unsigned char key);
struct map *mapFromHash(KanjiMode tbl, unsigned char key, struct map ***ppp);
int askQuitKey(unsigned key);

/* kigo.c */
void initKigoTable(void);
int KigoIchiran(uiContext d);
int makeKigoIchiran(uiContext d, int major_mode);

/* lisp.c */
int WCinit(void);
int clisp_init(void);
void clisp_fin(void);
int YYparse_by_rcfilename(char *s);
int parse_string(char *str);
void clisp_main(void);
int DEFVAR(int Vromkana, int StrAcc, char *, int RomkanaTable);

/* mode.c */
newmode *findExtraKanjiMode(int mnum);
void currentModeInfo(uiContext d);
void initModeNames(void);
void resetModeNames(void);
int JapaneseMode(uiContext d);
int AlphaMode(uiContext d);
int HenkanNyuryokuMode(uiContext d);
int queryMode(uiContext d, WCHAR_T *arg);
int changeModeName(int modeid, char *str);

/* multi.c */
int UseOtherKeymap(uiContext d);
int DoFuncSequence(uiContext d);
int multiSequenceFunc(uiContext d, KanjiMode mode, int whattodo, int key, int fnum);

/* onoff.c */
int initOnoffTable(void);
int selectOnOff(uiContext d, WCHAR_T **buf, int *ck, int nelem, int bangomax, int currentkouho, unsigned char *status, int (*everyTimeCallback )(...), int (*exitCallback )(...), int (*quitCallback )(...), int (*auxCallback )(...));

/* parse.c */
#ifdef BINARY_CUSTOM
int binparse (void);
#else
void parse(void);
#endif

int parse_string(char *str);
void clisp_main(void);

/* romaji.c */
void debug_yomi(yomiContext x);
void kPos2rPos(yomiContext yc, int s, int e, int *rs, int *re);
void makeYomiReturnStruct(uiContext d);
int RomkanaInit(void);
void RomkanaFin(void);
yomiContext newYomiContext(WCHAR_T *buf, int bufsize, int allowedc, int chmodinhibit, int quitTiming, int hinhibit);
yomiContext GetKanjiString(uiContext d, WCHAR_T *buf, int bufsize, int allowedc, int chmodinhibit, int quitTiming, int hinhibit, canna_callback_t everyTimeCallback, canna_callback_t exitCallback, canna_callback_t quitCallback);
void popYomiMode(uiContext d);
void fitmarks(yomiContext yc);
void ReCheckStartp(yomiContext yc);
void removeCurrentBunsetsu(uiContext d, tanContext tan);
void restoreChikujiIfBaseChikuji(yomiContext yc);
int YomiInsert(uiContext d);
int findSup(WCHAR_T key);
void moveStrings(WCHAR_T *str, BYTE *attr, int start, int end, int distance);
int forceRomajiFlushYomi(uiContext d);
int RomajiFlushYomi(uiContext d, WCHAR_T *b, int bsize);
void restoreFlags(yomiContext yc);
int xString(WCHAR_T *str, int len, WCHAR_T *s, WCHAR_T *e);
int appendTan2Yomi(tanContext tan, yomiContext yc);
yomiContext dupYomiContext(yomiContext yc);
void doMuhenkan(uiContext d, yomiContext yc);
int doKakutei(uiContext d, tanContext st, tanContext et, WCHAR_T *s, WCHAR_T *e, yomiContext *yc_return);
int cutOffLeftSide(uiContext d, yomiContext yc, int n);
int YomiKakutei(uiContext d);
void clearYomiContext(yomiContext yc);
void RomajiClearYomi(uiContext d);
int YomiExit(uiContext d, int retval);
void RomajiStoreYomi(uiContext d, WCHAR_T *kana, WCHAR_T *roma);
int KanaDeletePrevious(uiContext d);
coreContext newCoreContext(void);
int alphaMode(uiContext d);
int YomiQuotedInsert(uiContext d);
int convertAsHex(uiContext d);
int selectKeysup(uiContext d, yomiContext yc, int ind);
int containUnconvertedKey(yomiContext yc);
int YomiBaseHiraKataToggle(uiContext d);
int YomiBaseZenHanToggle(uiContext d);
int YomiBaseRotateForw(uiContext d);
int YomiBaseRotateBack(uiContext d);
int YomiBaseKanaEisuToggle(uiContext d);
int YomiBaseKakuteiHenkanToggle(uiContext d);
int YomiModeBackup(uiContext d);
int exitJishu(uiContext d);
int YomiMark(uiContext d);
int Yomisearchfunc(uiContext d, KanjiMode mode, int whattodo, int key, int fnum);
void trimYomi(uiContext d, int sy, int ey, int sr, int er);
int TanBubunKakutei(uiContext d);
int TanBubunKakutei(uiContext d);
void removeKana(uiContext d, yomiContext yc, int k, int r);
int cvtAsHex(uiContext d, WCHAR_T *buf, WCHAR_T *hexbuf, int hexlen);

/* uiutil.c */
menustruct *allocMenu(int n, int nc);
int uiUtilIchiranTooSmall(uiContext d, int retval, mode_context env);
int UiUtilMode(uiContext d);

#ifndef NO_EXTEND_MENU
int initExtMenu(void);
int prevMenuIfExist(uiContext d);
int showmenu(uiContext d, menustruct *table);
void finExtMenu(void);
void freeAllMenuInfo(menuinfo *p);
void freeMenu(menustruct *m);
#endif

/* uldefine.c */
int dicTouroku(uiContext d);
int dicSakujo(uiContext d);
int initHinshiTable(void);
void clearYomi(uiContext d);
int getTourokuContext(uiContext d);
void popTourokuMode(uiContext d);
int uuTTangoQuitCatch(uiContext d, int retval, mode_context env);
WCHAR_T **getUserDicName(uiContext d);
int dicTouroku(uiContext d);
int dicTourokuTango(uiContext d, canna_callback_t quitfunc);
int dicTourokuHinshi(uiContext d);
int dicTourokuControl(uiContext d, WCHAR_T *tango, canna_callback_t quitfunc);

/* uldelete.c */
void freeWorkDic3(tourokuContext tc);
void freeWorkDic(tourokuContext tc);
void freeDic(tourokuContext tc);
void freeAndPopTouroku(uiContext d);
int dicSakujo(uiContext d);

/* ulhinshi.c */
int initHinshiMessage(void);
void EWStrcat(WCHAR_T *buf, char *xxxx);
int initGyouTable(void);
int dicTourokuHinshiDelivery(uiContext d);
int dicTourokuDictionary(uiContext d, int (*exitfunc )(...), int (*quitfunc )(...));

/* ulkigo.c */
int initUlKigoTable(void);
int initUlKeisenTable(void);
int uuKigoGeneralExitCatch(uiContext d, int retval, mode_context env);
int uuKigoMake(uiContext d, WCHAR_T **allkouho, int size, char cur, char mode, int (*exitfunc )(...), int *posp);
int kigoRussia(uiContext d);
int kigoGreek(uiContext d);
int kigoKeisen(uiContext d);

/* ulmount.c */
int getMountContext(uiContext d);
void popMountMode(uiContext d);
int dicMount(uiContext d);

/* ulserver.c */
int serverFin(uiContext d);
int serverChange(uiContext d);

/* util.c */
void GlineClear(uiContext d);
void echostrClear(uiContext d);
int checkGLineLen(uiContext d);
int NothingChanged(uiContext d);
int NothingForGLine(uiContext d);
void CannaBeep(void);
int NothingChanged(uiContext d);
int NothingChangedWithBeep(uiContext d);
int NothingForGLineWithBeep(uiContext d);
int Insertable(unsigned char ch);
int extractTanString(tanContext tan, WCHAR_T *s, WCHAR_T *e);
int extractTanYomi(tanContext tan, WCHAR_T *s, WCHAR_T *e);
int extractTanRomaji(tanContext tan, WCHAR_T *s, WCHAR_T *e);
void makeKanjiStatusReturn(uiContext d, yomiContext yc);
void makeGLineMessage(uiContext d, WCHAR_T *msg, int sz);
void makeGLineMessageFromString(uiContext d, char *msg);
int setWStrings(WCHAR_T **ws, char **s, int sz);
int dbg_msg(char *fmt, int x, int y, int z);
int checkModec(uiContext d);
int showRomeStruct(unsigned int dpy, unsigned int win);
int NoMoreMemory(void);
int GLineNGReturn(uiContext d);
int GLineNGReturnFI(uiContext d);
int specialfunc(uiContext d, int fn);
int GLineNGReturnTK(uiContext d);
int copyAttribute(BYTE *dest, BYTE *src, int n);
char *debug_malloc(int n);
int WCinit(void);
int WStrlen(WCHAR_T *ws);
WCHAR_T *WStrcpy(WCHAR_T *ws1, WCHAR_T *ws2);
WCHAR_T *WStrncpy(WCHAR_T *ws1, WCHAR_T *ws2, int cnt);
WCHAR_T *WStraddbcpy(WCHAR_T *ws1, WCHAR_T *ws2, int cnt);
WCHAR_T *WStrcat(WCHAR_T *ws1, WCHAR_T *ws2);
int WStrcmp(WCHAR_T *w1, WCHAR_T *w2);
int WStrncmp(WCHAR_T *w1, WCHAR_T *w2, int n);
int WWhatGPlain(WCHAR_T wc);
int WIsG0(WCHAR_T wc);
int WIsG1(WCHAR_T wc);
int WIsG2(WCHAR_T wc);
int WIsG3(WCHAR_T wc);
int CANNA_mbstowcs(WCHAR_T *dest, char *src, int destlen);
int CNvW2E(WCHAR_T *src, int srclen, char *dest, int destlen);
int CANNA_wcstombs(char *dest, WCHAR_T *src, int destlen);
int WStringOpen(void);
WCHAR_T *WString(char *s);
void WStringClose(void);
int WSfree(WCHAR_T *s);
void generalReplace(WCHAR_T *buf, BYTE *attr, int *startp, int *cursor, int *endp, int bytes, WCHAR_T *rplastr, int len, int attrmask);
int WToupper(WCHAR_T w);
int WTolower(WCHAR_T w);
WCHAR_T key2wchar(int key, int *check);
int confirmContext(uiContext d, yomiContext yc);
int abandonContext(uiContext d, yomiContext yc);
int makeRkError(uiContext d, char *str);
int canna_alert(uiContext d, char *message, canna_callback_t cnt);
void EWStrcat(WCHAR_T *buf, char *xxxx);
char *KanjiInitError(void);
int wchar2ushort(WCHAR_T *src, int slen, uint32 *dst, int dlen);
int ushort2wchar(uint32 *src, int slen, WCHAR_T *dst, int dlen);
int euc2ushort(char *src, int srclen, uint32 *dest, int destlen);

/* yesno.c */
int getYesNoContext(uiContext d, canna_callback_t everyTimeCallback, canna_callback_t exitCallback, canna_callback_t quitCallback, canna_callback_t auxCallback);

#endif /* _UTIL_FUNCTIONS_DEF_ */
#endif /* !_CANNA_H_ */
