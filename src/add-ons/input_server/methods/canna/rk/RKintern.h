/* Copyright 1994 NEC Corporation, Tokyo, Japan.
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

/* LINTLIBRARY */
/* $Id: RKintern.h 10525 2004-12-23 21:23:50Z korli $ */
/************************************************************************/
/* THIS SOURCE CODE IS MODIFIED FOR kanBe BY T.MURAI 1997 */
/************************************************************************/
#ifndef		_RKintern_h
#define		_RKintern_h

#define BUNMATU 

/* BUNMATU ¥Þ¥¯¥í¤ò define ¤¹¤ë¤È¡¢ËÌÎ¦ÀèÃ¼²Ê³Øµ»½ÑÂç³Ø±¡Âç³Ø¾ðÊó²Ê³Ø
   ¸¦µæ²Ê¤ÎÆ£»ÞÏÂ¹¨(fujieda@jaist.ac.jp)¤µ¤ó¤¬¡¢¡Ø¤«¤ó¤Ê¡Ù¤Î¤«¤Ê´Á»úÊÑ
   ´¹¸úÎ¨¸þ¾å¤Î¤¿¤á¤Ë¡¢1994 Åß¤ËÁÈ¤ß¹þ¤ó¤À¡ÖÊ¸¾ÏËöµ­¹æ¡×¤¬Í­¸ú¤Ë¤Ê¤ê¤Þ
   ¤¹¡£¤³¤ì¤Ë¤è¤êÊ¸¾ÏËö¤Ë¤·¤«¸½¤ì¤Ê¤¤Ê¸»ú¤¬Ê¸Àá¤ÎÅÓÃæ¤Ë½Ð¤ÆÍè¤ë¤³¤È¤¬
   ¤Ê¤¯¤Ê¤êÊÑ´¹¸úÎ¨¤¬¸þ¾å¤·¤Þ¤¹¡£¤³¤Î¥³¡¼¥É¤ò¤«¤¤¤Æ²¼¤µ¤¤¤Þ¤·¤¿Æ£»Þ¤µ
   ¤ó¤Ë´¶¼Õ¤·¤Þ¤¹¡£ */

//#define FUJIEDA_HACK
/* Æ£»Þ¡÷£Ê£Á£É£Ó£Ô¤Î¥Ï¥Ã¥¯¤òÍ­¸ú¤Ë¤¹¤ë */

#define EXTENSION_NEW

#include "cannaconf.h"
#include "cannabuild.h"

#if defined(__STDC__) || !DONT_HAVE_RENAME
#define HAVE_RENAME
#endif

#include "widedef.h"
#define RK_INTERNAL
#include "canna/RK.h"

#include <stdlib.h>

#ifdef NOT_DEF
#include <errno.h>
int	Rk_errno;
#define RkSetErrno(number) {\
    if (number)
	Rk_errno = (number);
    else
	Rk_errno = errno;
}
#else
#define RkSetErrno(number)
#endif

#ifndef RK_DEBUG
#define	RkDebug(fmt, p, q, r)
#endif

#define	MKDIR_MODE	0775
#define	CREAT_MODE	0664

typedef unsigned char	*pointer;
typedef unsigned char   Wrec;

#define WMASK		(~0x0303)
#define RK_WMASK	WMASK
#define WNILL		(unsigned short)0x0000
#define WNULL		(unsigned short *)0
#define	us_iscodeG0(wc)	(((wc) & 0x8080) == 0x0000)
#define	us_iscodeG1(wc)	(((wc) & 0x8080) == 0x8080)
#define	us_iscodeG2(wc)	(((wc) & 0x8080) == 0x0080)
#define	us_iscodeG3(wc)	(((wc) & 0x8080) == 0x8000)

#define RK_SS2 (unsigned char)0x8e
#define RK_SS3 (unsigned char)0x8f

#define RK_ESC_CHAR ((WCHAR_T)'\\')

//#ifndef isascii
//#define isascii(c) (((c) >= 0 && (c) <= 256))
//#endif

#define	euc_iscodeG0(c)	isascii(c)
#define	euc_iscodeG1(c)	(((c) & 0x80) && !((c) & RK_SS2) && !((c) & RK_SS3))
#define	euc_iscodeG2(c)	((c) & RK_SS2)
#define	euc_iscodeG3(c)	((c) & RK_SS3)

/* RkUnion
 *	private data at each class.
 */
typedef union _rkunion {
  pointer	ptr;
  long		var;
  unsigned long	uvar;
} RkUnion;

/*	kana kanji henkan jisho
 *		MS	870713
 *
 *	+---------------+
 *	|     HEADER	|	jisho header (256 byte)
 *	+---------------+256L
 *	|		|	yomigana no index
 *	|   DIRECTORY	|
 *	+---------------+
 *	|		|	tango jouhou
 *	|		|		yomi, {(row, col), tsuduri, hindo}...
 *	|     WORD	|
 *	|		|
 *	|		|
 *	+---------------+
 *	|		|	setuzoku jouhou(swd nomi)
 *	|   CONJUNCT	|		row...
 *	|		|
 *	+---------------+
 */

/* BASIC TYPE:
 *	subete no data ha MSB first(Motorolla order) de tenkai sareru
 *		unsigned char	w
 *		unsigned short	wx
 *		unsigned long	wxyz
 */	
#define LOMASK(x)	((x)&255)
/* #include	"type.h" */
/*
 * 	byte stream <=> long or short
 */
#define l_to_bst4(src, dst) {\
    dst[0] = (unsigned char)(((long)(src) >> 24) & 0xff);\
    dst[1] = (unsigned char)(((long)(src) >> 16) & 0xff);\
    dst[2] = (unsigned char)(((long)(src) >> 8) & 0xff);\
    dst[3] = (unsigned char)( (long)(src) & 0xff);\
}

#define bst4_to_l(src) \
    (unsigned long)(((long)(src)[0] << 24) |\
		    ((long)(src)[1] << 16) |\
		    ((long)(src)[2] << 8) |\
		     (long)(src)[3])

#define l_to_bst3(src, dst) {\
    dst[0] = (unsigned char)(((long)(src) >> 16) & 0xff);\
    dst[1] = (unsigned char)(((long)(src) >> 8) & 0xff);\
    dst[2] = (unsigned char)( (long)(src) & 0xff);\
}

#define bst3_to_l(src) \
    (unsigned long)(((long)(src)[0] << 16) | ((long)(src)[1] << 8) | (long)(src)[2])

#define s_to_bst2(src, dst) {\
    (dst)[0] = (unsigned char)((src >> 8) & 0xff);\
    (dst)[1] = (unsigned char)(src & 0xff);\
}

#define	bst2_to_s(src) \
    (unsigned short)(((unsigned short)(src)[0] << 8) | (src)[1])

#define l_to_bst	l_to_bst4
#define bst_to_l	bst4_to_l
#define s_to_bst	s_to_bst2

#define	L4TOL(l4)\
	(((((((unsigned long)(l4)[0]<<8)|(l4)[1])<<8) | (l4)[2])<<8)|(l4)[3])
#define	L3TOL(l3)\
	(((((unsigned long)(l3)[0]<<8)|(l3)[1])<<8) | (l3)[2])
#define	S2TOS(s2)	(((unsigned short)(s2)[0]<<8)|(s2)[1])
#define	LTOL4(l, l4)	{\
	(l4)[0] = LOMASK((l)>>24); (l4)[1] = LOMASK((l)>>16);\
	(l4)[2] = LOMASK((l)>> 8); (l4)[3] = LOMASK((l));\
	}
#define	LTOL3(l, l3)	{\
			   (l3)[0] = LOMASK((l)>>16);\
			   (l3)[1] = LOMASK((l)>> 8);\
			   (l3)[2] = LOMASK((l));\
			}
#define	STOS2(s, s2)	{\
			   (s2)[0] = LOMASK((s)>> 8);\
			   (s2)[1] = LOMASK((s));\
			 }

#define RkNumber(ary) (sizeof(ary)/sizeof(ary[0]))

#define rk_isdigit(c) (!((c) & ~0xff) ? isdigit((int)(c)) : 0)
#define rk_isascii(c) (!((c) & ~0xff) ? isascii((int)(c)) : 0)
#define rk_isspace(c) (!((c) & ~0xff) ? isspace((int)(c)) : 0)

#define HD_MAG		0
#define HD_VER		1
#define HD_TIME		2
#define HD_REC		3
#define HD_CAN		4
#define HD_L2P		5
#define HD_L2C		6
#define HD_PAG		7
#define HD_LND		8
#define HD_SND		9
#define HD_SIZ		10
#define HD_HSZ		11
#define HD_DROF		12
#define HD_PGOF		13
#define HD_DMNM		14
#define HD_CODM		15
#define HD_LANG		16
#define HD_WWID		17
#define HD_WTYP		18
#define HD_COPY		19
#define HD_NOTE		20
#define HD_TYPE		21

#define HD_MAXTAG	(HD_TYPE + 1)

#define HD_TAGSIZ	(sizeof(long))
#define HD_MIN_TAGSIZ	(3 * HD_TAGSIZ)

struct HD {
    RkUnion	data[HD_MAXTAG];
    int		flag[HD_MAXTAG];
};

#define	isEndTag(s)	(s[0] == 0 && s[1] == 0 && s[2] == 0 && s[3] == 0)

#define HD_TAG_MAG	"MAG#"
#define HD_TAG_VER	"VER#"
#define HD_TAG_TIME	"TIME"
#define HD_TAG_REC	"#REC"
#define HD_TAG_CAN	"#CAN"
#define HD_TAG_L2P	"L2P#"
#define HD_TAG_L2C	"L2C#"
#define HD_TAG_PAG	"#PAG"
#define HD_TAG_LND	"#LND"
#define HD_TAG_SND	"#SND"
#define HD_TAG_SIZ	"#SIZ"
#define HD_TAG_HSZ	"#HSZ"
#define HD_TAG_DROF	"DROF"
#define HD_TAG_PGOF	"PGOF"
#define HD_TAG_DMNM	"DMNM"
#define HD_TAG_CODM	"CODM"
#define HD_TAG_LANG	"LANG"
#define HD_TAG_WWID	"WWID"
#define HD_TAG_WTYP	"WTYP"
#define HD_TAG_COPY	"(C) "
#define HD_TAG_NOTE	"NOTE"
#define HD_TAG_TYPE	"TYPE"

#define JMWD	1
#define JSWD	2
#define JPRE	3
#define JSUC	4

#define BYTE2		2
#define BYTE4		4

#define DEF_WWID	2

#define WORD_NODE		(0x80)
#define LAST_NODE		(0x40)
const unsigned int BIT_UNIT = 8;

#define NOLOCALE	"NOLOCALE"
#define DEF_WTYP	"W16 "
#define DEF_TYPE	"jmwd"

#define	RK_PG_LOADED		0x01
#define	RK_PG_USED		0x02

#define	isLoadedPage(p)		((p)->flags & RK_PG_LOADED)
#define	isUsedPage(p)		((p)->flags & RK_PG_USED)

#define	thisPWO(p)	(unsigned long)((((unsigned long ) *(p) << 6) & 0x3fc0) \
				   | ((*((p)+1) >> 2) & 0x3f))
#define	thisLVO(p)	(unsigned long)((((unsigned long) *((p)+1) << 13) & 0x6000) \
				   | (((unsigned long) *((p)+2) << 5) & 0x1fe0) \
				   | ((*((p)+3) >> 3) & 0x1f))
#define	thisCSN(p)	(unsigned long)((((unsigned long) *((p)+3) << 8) & 0x700) | *((p)+4))

struct NP {
  unsigned	ndsz;
  unsigned	lnksz;
  unsigned long	lvo;
  unsigned long	csn;
  unsigned	flags;
  int		count;
  unsigned char	*buf;
};

/*
 * definition of off_t added by Kenji Rikitake 27-MAR-1995
 *
 * Include necessary headers for lseek() here. 
 * note: sys/types.h only applies for BSD/OS 2.0. 
 * Use the appropriate files for other systems.
 */

#include <sys/types.h>	

struct ND {
  unsigned long time;
  unsigned long rec;
  unsigned long can;
  unsigned long sz;
  off_t         doff;
  unsigned long drsz;
  unsigned long pgsz;
  unsigned long ttlpg;
  int           fd;
  unsigned char *buf;
  struct NP     *pgs;
};

/* HEADER:
 *	unsigned char	magic[2];
 *	unsigned char	pakeuc;		key ga asshuku sareteiru
 *	unsigned char	name[];		jisho mei
 *					xxx.mwd xxx.swd xxx.pre xxx.suc
 *					\n de shuuryou
 */
#define		ND_HDMAGICSIZ		4
#define		ND_HDRSIZ		256		/* header size */
#if 0 /* iranai? */
#define		ND_HDRMAG (('H'<<8)|'N')/* magic number */
#endif
/* jisho class */
#define		ND_MWD		RK_MWD
#define		ND_SWD		RK_SWD
#define		ND_PRE		RK_PRE
#define		ND_SUC		RK_SUC
#define		ND_EMP		4		/* kuugo */
#define		ND_OPN		5		/* hiraki kakko */
#define		ND_CLS		6		/* toji kakko */
#define		ND_PUN		7		/* kutouten */


struct wcand {
    unsigned char	*addr;
    unsigned short	row;
    unsigned char	klen;
};

struct RkXwrec {
    unsigned		ncands, wrecsz;
    unsigned char	*wrec;
};

/* DIRECTORY:
 *	xxxx NODE....
 *	    xxxx niha, DIRECTORY no ookisa ga byte de hairu.
 *
 *	4 byte no NODE kara kousei 
 *	
 *	+--+--+--+--+
 *	|KY|X1 X2 X3|
 *	+--+--+--+--+
 *	 !  !
 *	 !  +-------------	X1
 *	 !			word record/directory offset
 *	 !			 (X1<<16) | (X2<<8) | (X3)
 *	 !
 *	 +----------------	yomi (ascii/euc hiragara lower byte)
 *				0
 */
#define	ND_NODSIZ	6		/* node size in byte */
#define	ND_DSPENT	189		/* #entry in the dispatch table */
/* mask values */
#define	ND_OFFMASK	0x00ffffffL	/* offset mask (24 bit) */
/* X1 bit flags */

#define	ND_NULLOFF	ND_OFFMASK	/* tango ga sakujo sareta */
#define	VMASK		0x7fffffL

/* predicators */
#define	ND_LAST		0x0080L	/* directory no saishuu node (entry) */
#define	ND_WORD		0x0040L	/* offset ha tango joushou wo sasu */
#define	ND_RFU		0x0020L	/* Mishiyou (offset ni tuika ?) */
#define	ISLASTNOD(flag)	((flag)&ND_LAST)
#define	ISWORDNOD(flag)	((flag)&ND_WORD)


#define HN (unsigned long) 256
#define HashFunc(key) (unsigned long)((key) % HN)

struct WRT{
       unsigned long sz;
       unsigned long cs;
       unsigned long frst;
       unsigned long tm;
       unsigned char *buf;
};

struct CTdata{
       struct CTdata *next;
       unsigned long ct[2];
};       

struct RUT{
       unsigned long sz;
       unsigned long cs;
       unsigned long frst;
       unsigned long tm;
       struct CTdata **dp;
};

#define a_csn(x)  (unsigned long) \
(((((unsigned long)(x)[0] << 8) | (x)[1]) << 4) | (((unsigned long)(x)[2] & 0xf0) >> 4))

#define a_tick(x) (unsigned long) \
((((((unsigned long)(x)[2] & 0x0f) << 8) | (unsigned long)(x)[3]) << 8) | (x)[4]) 

#define WriteVal(csn, tick, buf){\
   (buf)[0] = (unsigned char)(((csn)>>12) & 0xff);\
   (buf)[1] = (unsigned char)(((csn)>>4) & 0xff);\
   (buf)[2] = (unsigned char)((((csn)<< 4) & 0xf0)|(((tick)>>16) & 0x0f));\
   (buf)[3] = (unsigned char)(((tick)>>8) & 0xff);\
   (buf)[4] = (unsigned char)((tick) & 0xff);\
}

#define WriteCT(csn, tick, arr){\
   (arr)[0] = (unsigned long) (csn);\
   (arr)[1] = (unsigned long) (tick);\
}  

#define freeWRT(wruc) {\
     free(wruc->buf);\
     free(wruc);\
}

#define RKmaxDN (unsigned long) 0xff
#define RKmaxCN (unsigned long) 0xffffff


/* WORD:
 *	xxxx {yomi nkouho {flags row col freq kouho}...}...
 *	    xxxx niha, WORD no ookisa ga byte de hairu.
 *
 * tango:
 *	unsigned char	yomi[];			yomigana 
 *	unsigned char	nkouho;			kouho no kazu 
 *		unsigned char	flags;		kouho no nagasa + flags
 *		unsigned char	row;		kouho no row bangou
 *		unsigned char	col;		kouho no col bangou
 *		unsigned char	freq		kouho no shiyou hindo
 *		unsigned char	kouho[];
 *	
 */

#define	NW_PREFIX	2	/* the length of kouho prefix in byte */

/* flags */
#define	NW_RC256	0x80	/* set when rowcol >= 256 */
#define	NW_LEN		0x7f	/* kouho no nagasa ( zenkaku 31 moji) */
#define	rowcol256(flag)	((flag) & NW_RC256)

#define	candlen(flag)	((flag) & NW_LEN)
#define wordlen(flag)	((candlen(flag) << 1) + NW_PREFIX)
/* RkWcand
 *	data structure used to handle the word record 
 */
typedef struct RkWcand {
    Wrec		*addr;		/* houho sentou address */
    short		rcnum;		/* row column number */
    unsigned char	klen;		/* kouho no nagasa */
} Candidate;

/* CONJUNCT:
 *	xxxx rrcc
		row_0  row_1  ... row_rr-1
 *		name_0 name_1 ... name_rr-1
 *	    xxxx niha, CONJUNCT no ookisa ga byte de hairu.
 *	    rr, cc ha sorezore, row, col no kosuu wo simesu.
 *	    row_i ha byte kyoukai kara hajimaru cc bit karanaru bit retu
 *	    name_i ha EOS de owaru mojiretu
 *
 */

#define		crpair(row, col)	((long)(row))
/* ngram
 *	tango kan no setuzoku wo simesu gyouretu
 *	SWD ga open sareruto douji ni yomikomareru
 */
struct RkKxGram {
/* setuzoku jouhou */
    int		ng_rowcol;	/* row no kazu */
    int		ng_rowbyte;	/* row atari no byte suu */
    char	*ng_conj;	/* setuzoku gyouretu/code table */
    char	**ng_strtab;
};

#define	GetGramRow(g, r) 	((g)->ng_conj + (r)*(g)->ng_rowbyte)
#define	TestGram(cnj, col)	((cnj) && ((cnj)[((col)>>3)]&(0x80>>(col&7))))

/* hinshi no bunrui
 *	renbunsetu henkan de siyou sareru
 */
#define	IsShuutan(g, r)		TestGram(GetGramRow(g, r), 0)
#ifdef BUNMATU
#define	IsBunmatu(g, r)		TestGram(GetGramRow(g, r), 1)
#endif
#define	IsTaigen(g, r)		TestGram(GetGramRow(g, R_TAIGEN), r)
#define	IsYougen(g, r)		TestGram(GetGramRow(g, R_YOUGEN), r)
#define	IsKakujs(g, r)		TestGram(GetGramRow(g, R_KAKUJS), r)
#define is_row_num(g, n)	((0 <= (n)) && ((n) < ((g)->ng_rowcol)))

/* RkGram -- grammatical information
 */

struct RkGram {
  int			refcount; /* reference counter */
  struct RkKxGram	*gramdic; /* grammar dictionary */
  int	 		P_BB, P_NN, P_T00, P_T30, P_T35; /* hinshi codes */
#ifdef FUJIEDA_HACK
  int			P_KJ; /* tankanji */
#endif
};

extern struct RkGram SG;


/* ncache
 *	tango record no caching list 
 *  ncache ha, jisho kara itido yomikomareta tango record wo cache kanri suru
 * tameno jouhou kouzou dearu.
 *  tango record ha key (dicnum, address) de skibetu sareru.
 *  taiou suru tango record ha nc_word ni yori simesareru HEAP ryouiki ni aru.
 *	+------------+
 *	| nc_dicnum  |	key part
 *	| nc_address |
 *	+------------+
 *	|  nc_word   |	value part
 *	+------------+
 *  ncache ha 2tu no souhoukou list hash/free list ni tunagareru.
 *  (* koreha UNIX i/o system no buffer cache kara hint wo eta *)
 *  hash list ha key no kensaku wo kousoku ka suru tame ni shiyou sareru.
 *  free list ha kaihou sareta cache ga tunagareru.
 */
struct ncache	{
    struct ncache	*nc_hnext;	/* hash list */
    struct ncache	*nc_hprev;
    struct ncache	*nc_anext;	/* free list */
    struct ncache	*nc_aprev;
    Wrec		*nc_word;	/* heap jouno tango record */
    struct DM		*nc_dic;	/* yomikomareta jisho */
    unsigned short	nc_flags;
    unsigned long	nc_count;	/* word karano sanshou dosuu */
    long		nc_address;	/* word record no file address */
};
/* flags */
#define	NC_DIRTY	1	/* word no naiyou ga henkou sareta */
#define	NC_ERROR	2	/* cache no naiyou ga tadasiku nai */
#define	NC_NHEAP	4	/* word ha heap wo sasite inai */

struct nread {
  struct ncache	*cache;
  unsigned long	offset;
  unsigned long	csn;
  int		nk;
};

/* nword
 *	bunsetsu kaiseki kekka 
 * 	jisho kara no tango yomidasi kekka
 * nword ha, jisho kara search sareta tango record wo kirokusi, 
 * bunsetsu kaiseki ni yori sakusei sareru bunsetsu tree wo hyougen suru.
 */
struct nword {
    struct ncache	*nw_cache;	/* tango record wo fukumu cache */
    short		nw_rowcol;	/* setsuzoku  jouhou */
    unsigned char	nw_ylen;	/* #chars in yomi */
    unsigned char	nw_klen;	/* #chars in kanji */
    unsigned char	nw_class;	/* word class */
    unsigned char	nw_flags;	/* word flags (see below) */
    unsigned char	nw_lit;		/* literal conversion */
    unsigned long	nw_prio;	        /* kouzou ni yoru priority */  /* True ? by tamano */
    unsigned long   nw_csn;
    struct nword	*nw_left;	/* hidari ni tunagaru word */
    struct nword	*nw_next;	/* onaji nw_len wo motu list */
    unsigned char	*nw_kanji;	/* kanji kouho ichi/douteki na kouho */
/* nw_klen ha fuyou ni naru kanousei ari */
    struct DM		*nw_freq;	/* pointer to frequncy file */
};
/* 
 * word flags
 */
#define NW_MWD		0x80
#define NW_SWD		0x40
#define NW_PRE		0x20
#define NW_SUC		0x10
#ifdef FUJIEDA_HACK
#define NW_DUMMY	0x08
#endif
#ifdef BUNMATU
#define NW_BUNMATU	0x04
#endif
#define	NW_FOLLOW	0x02		/* has some words following to me */
#define	NW_NEVER_SPLIT	0x01		/* cannot terminate here */
/* accessor & predicates */
#define	IsFollowed(w)	((w)->nw_flags&NW_FOLLOW)
#define	DontSplitWord(w)	((w)->nw_flags |= NW_NEVER_SPLIT)
#define	CanSplitWord(w)		(!((w)->nw_flags&NW_NEVER_SPLIT))
#ifdef BUNMATU
#define	OnlyBunmatu(w)		((w)->nw_flags & NW_BUNMATU)
#endif

/* conversion number */
#define	LIT2GRP(cn)	((cn)>>4)
#define	LIT2MEM(cn)	((cn)&15)
#define	MAKELIT(g, w)	(((g)<<4)|(w))
#define	MAXLIT		16

#define	LIT_NONE	0
#define	LIT_NUM		1
#define	LIT_ALPHA	2
#define	LIT_HIRA	3

/* nbun
 *	bunsetsu queue
 * renbunsetsu henkan kekka wo kiroku 
 */
struct nbun {
    struct nword	*nb_cand;	/* kouho wo kousei suru word
				         * nw_next niyori sort sareru */
    unsigned short	nb_yoff;	/* yomigana offset in buffer  */
    unsigned short	nb_curlen;	/* current candidate length in char */
    unsigned short	nb_maxcand;	/* number of candidates */
    unsigned short	nb_curcand;	/* current candidate */
    unsigned short	nb_flags;	/* ?? */
};

/* DM/DF/DD
 */
/* MS1214
 *	now DM supports frequency files as well
 */
typedef struct DM {
/* pointers for the file structure */
    struct DM		*dm_next;
    struct DM		*dm_prev;
    struct DF		*dm_file;	/* ptr to the file containing this */
/* attributes */
    char		*dm_dicname;		
    char		*dm_nickname;
    unsigned		dm_class;	/* MWD/SWD/PRE/SUC */
    unsigned		dm_flags;	/* flags */
    unsigned char	dm_packbyte;	/* shouryaku sareta joui byte */
/* reference  count from MD */
    int			dm_rcount;
/* setuzoku jouhou (SWD nomi) */
    struct RkGram	*dm_gram;
    RkUnion		dm_extdata;
    unsigned char	*dm_qbits;	/* MS1214 gakushuu jouhou */
/* */
    struct DDT		*dm_line;	/* pointer to dd line */
    long		dm_offset;
    struct RUT		*dm_rut;
    struct NV		*dm_nv;
} DM;
/* flag values */
#define	DM_EXIST	0x01		/* member exists */
#define	DM_WRITABLE	(DM_EXIST << 1)	/* jisho ga write dekiru */
#define	DM_LOADWORD	(DM_EXIST << 2)	/* word rec wo memory ni yomi */
#define	DM_UPDATED	(DM_EXIST << 4)	/* member changed */
#define DM_READOK	(DM_EXIST << 5) /* jisho no read ken */
#define DM_WRITEOK	(DM_EXIST << 6) /* jisho no write ken */

typedef struct DF {
/* DF doubly linked list */
    struct DF	*df_next;
    struct DF	*df_prev;
    struct DD	*df_direct;	/* backward pointer to the directory */
    struct DM	df_members;	/* the header of the dictionary */
/* attributes */
    char	*df_link; /* link name of the file */
    unsigned	df_type;	/* type of file */
    unsigned	df_flags;	/* access mode */
    int		df_rcount;	/* # the mounted members */
    RkUnion	df_extdata;
    long	df_size;
} DF;
#define	DM2TYPE(dm)	((dm)->dm_file->df_type)
/* jisho no shurui */
#define	DF_PERMDIC	0	/* file ni aru jisho */
#define	DF_TEMPDIC	1	/* zantei jisho */
#define DF_FREQDIC 	2	/* gakushuu hindo jisho MS1214 */
#define DF_RUCDIC 	3	/* gakushuu hindo jisho MS1214 */

/* flag values */
#define	DF_EXIST	1	/* file exists */
#define	DF_WRITABLE	2	/* file is writable */

/*
 * DD
 */
struct DDT	{
   struct DDT		*ddt_next;
   struct DDT		*ddt_prev;
   int			ddt_status;
   char			*ddt_spec;
};

typedef struct DD {
/* DD doubly linked list */
    struct DD		*dd_next;
    struct DD		*dd_prev;
    char		*dd_path;	/* path name of the directory */
    char		*dd_name;	/* unique name of the directory */
    int			dd_rcount;	/* # opened files+reference from DDP */
    struct DF		dd_files;	/* the header of the file list */
    unsigned		dd_flags;	/* flag */
/* dics.dir file */
    struct DDT		dd_text;
} DD, *DDP;

#define	DD_WRITEOK	1	/* directory is allowed to write */
#define DD_READOK	2

/* MD ----------------------------------------------------------------
 *  context ni mount sareta jisho/jisho wo mount siteiru jisho list no taiou
 * ---------------------------------------------------------------- */
typedef struct MD {
    struct MD		*md_next;	/* dictionary list */
    struct MD		*md_prev;
    struct DM		*md_dic;	/* jisho heno pointer */
    struct DM		*md_freq;	/* MS1214 gakushuu jisho heno pointer */
    int			md_flags;	/* mount flags */
} MD, *MDP;
#define	MD_WRITE	1		/* gakushuu shitei */
#define	MD_MPEND	2		/* mount pending */
#define	MD_UPEND	4		/* unmount pending */

/* RkParam
 *	common parameter block
 */
struct RkParam {
/* cache */
    struct ncache	*cache;
    int			maxcache;
/* heap */
    unsigned char	*heap;
    int			maxheap;
    unsigned char	*ham;
    int			maxham;
/* word */
    struct nword	*word;		/* henkyaku sareta word wo tunageru */
    struct nword	*page;		/* word page list */
    int			word_in_use;
    int			page_in_use;
/* jisho */
    struct DD		dd;
    char        	*ddhome;	/* dictionary home directory */
    struct DD		**ddpath;	/* system ddpath */
/* flags */
    unsigned		flag;		/* checks initialize */
/* */
};

extern struct RkParam SX;

#define SX_INITED	1

#define RK_REARRANGED	0x01

struct NVE {
  unsigned char	*data;
  struct NVE	*left;
  struct NVE	*right;
  struct NVE	*next;
};

struct NV {
  unsigned long	sz;
  unsigned long	tsz;
  long		csz;
  long		cnt;
/* int		lst; delete by tamano(tajima) */
  struct NVE	head;
  struct NVE	**buf;
};

struct nstore {
    WCHAR_T		*yomi;		/* yomigana buffer */
    unsigned		nyomi;		/* number of yomigana chars */
    unsigned		maxyomi;	/* maximum number of yomigana chars */
    struct nbun		*bunq;		/* Ê¸Àá¥­¥å¡¼ */
    unsigned		maxbunq;	/* saidai bunnsetu suu */
    unsigned		maxbun;		/* saidai bunsetsu suu */
    int			curbun;		/* ¥«¥ì¥ó¥ÈÊ¸Àá */
    int			maxxq;		/* saidai queue == bunsetu moji suu */
    struct nqueue	*xq;		/* henkan queue */
    struct nword	**xqh;		/* used in _RkparseBun() */
    int			word_in_use;   	/* #words used in context */
};

/* context
 *	ncontext ha rennbunnsetu hennkann client wo seigyo suru jouhou
 *		jisho kankyou
 *		renbunsetsu henkan sagyou ryouiki 
 */
struct 	nqueue {
    struct nword	*tree;
    short		maxlen;
    short		status;
};

typedef struct RkContext {
    struct nstore	*store;
    struct MD		*md[4];		/* mwd/swd/pre/suc */
    struct DD   	**ddpath;	/* dictionary search path */
    struct DM   	*dmprev;	/* dictionary search path : dic */
    struct DM   	*qmprev;	/* dictionary search path : freq */
    unsigned long	time;		/* tic */
    unsigned		flags;		/* context control flags */
    unsigned long	kouhomode;	/* kana kouho mode */
    unsigned long	*litmode;	/* literal conversion table */
    int			concmode;	/* connect words mode */
    int			poss_cont;	/* */
    RkUnion		cx_extdata;	/* etc data */
    struct NV		*nv;
    struct RkGram	*gram;
} RkContext;

/* Extension Data in DM */
typedef struct TD {
    struct TN   *td_node;	/* array of node TN */
    unsigned	td_n;		/* # effective nodes */
    unsigned	td_max;		/* # maximum nodes in the array */
} TD;

struct TW {
  Wrec		*word;
  unsigned long	lucks[2];
};

typedef struct TN {
  unsigned char	tn_flags;	/* type of node (see blow) */
  WCHAR_T		tn_key;
  union {
    struct TD	*tree;
    struct TW	*word;
  }	tn_value;				/* type specific data */
  /* syntax sugar */
#define tn_tree		tn_value.tree
#define tn_word		tn_value.word
} TN;
/* flags  values */
#define	TN_WORD		1		/* is a word */
#define	TN_WDEF		2		/* wrec has been defined */
#define	TN_WDEL		4		/* wrec has been deleted */
#define	IsWordNode(tn)	((tn)->tn_flags&TN_WORD)

/* td_and_n structure 
 *
 * tupple of td and n for RkGetWordTextDic()
 */
struct td_n_tupple {
  char               *td; /* in fact this type is (struct TD *) */
  int                n;
  struct td_n_tupple *next;
};

/* jishowo 1gyouzutu yomu tokino pointer :used in RkGetWordTextDic */
typedef struct _rec {
    int                 gwt_cx;
    unsigned char       *gwt_dicname;
    struct td_n_tupple  *tdn;
} GWT;

/* flag values */
#define	CTX_LIVE		1	/* allocate sareta */
#define	CTX_XFER		2	/* bunsetu henkann chuu */
#define	CTX_XAUT		4	/* jidou henkan chuu */
#define	CTX_XFSH	      010
#define	CTX_NODIC	      020	/* have never tried to mount dic */

#define	MAX_CONTEXT		256
#define	INIT_CONTEXT		32
#define	ADD_CONTEXT		32

#ifndef SYSTEM_DDHOME_NAME
#define SYSTEM_DDHOME_NAME	"canna"
#endif

#define USER_DIC_DIR		"user"
#define GROUP_DIC_DIR		"group"

#define	IS_LIVECTX(cx)	((cx)->flags & CTX_LIVE)
#define IS_XFERCTX(cx)	(IS_LIVECTX(cx) && ((cx)->flags & CTX_XFER))
#define IS_XAUTCTX(cx)	(IS_LIVECTX(cx) && ((cx)->flags & CTX_XAUT))
#define RK_CONNECT_WORD	0x01
#define	IsCxNum(cn) (0<=((unsigned long)cn)&&((unsigned long)cn)<now_context)
#define	IsLiveCxNum(cn) (IsCxNum(cn) && IS_LIVECTX(&CX[cn]))
#define	IsXferCxNum(cn) (IsLiveCxNum(cn) && IS_XFERCTX(&CX[cn]))
#define Is_Word_Connect(cx)\
    ((cx) && (((cx)->concmode & RK_CONNECT_WORD) == RK_CONNECT_WORD))
#define Is_Word_Make(cx)\
    ((cx) && (((cx)->concmode & RK_MAKE_WORD) == RK_MAKE_WORD))


/* RkDST
 *	dictionary switch table
 */
struct RkDST {
  int (*d_open) /* jisho ga open sareta toki */
    (struct DM *, char *, int, struct RkKxGram *);
  int (*d_close) /* jisho ga close sareta toki */
    (struct DM *, char *, struct RkKxGram *);
  int (*d_search) /* jisho kara tango wo search suru */
    (struct RkContext *, struct DM *, WCHAR_T *,
	 int, struct nread *, int, int *);
  int (*d_io) /* jisho he tango to cache no io */
    (struct DM *, struct ncache *, int);
  int (*d_ctl)	/* jisho koyuuno sousa */
    (struct DM *, struct DM *, int, WCHAR_T *, struct RkKxGram *);
  int (*d_sync) /* jisho sync suru */
    (struct RkContext *, struct DM *, struct DM *);
};
extern struct RkDST	_RkDST[];

#define	_RkEnrefCache(a)	((a)->nc_count++)
#define	DST_PROC(dm)		_RkDST[DM2TYPE(dm)]
#define	DST_OPEN(dm, file, mode, gram)\
	(DST_PROC(dm).d_open)(dm, file, mode, gram)
#define	DST_CLOSE(dm, file, gram)	(DST_PROC(dm).d_close)(dm, file, gram)
#define	DST_SEARCH(cx, dm, k, n, cp, max, cf)\
	(DST_PROC(dm).d_search)(cx, dm, k, n, cp, max, cf)
#define	DST_IO(dm, cp, io)	(DST_PROC(dm).d_io)(dm, cp, io)
#define	DST_READ(dm, cp)	DST_IO(dm, cp, 0)
#define	DST_WRITE(dm, cp)	DST_IO(dm, cp, 1)
#define	DST_CTL(dm, qm, what, arg, gram)\
	(DST_PROC(dm).d_ctl)(dm, qm, what, arg, gram)
#define	DST_SYNC(cx, dm, qm)	(DST_PROC(dm).d_sync)(cx, dm, qm)

/* ctl command */
#define DST_DoDefine		0	/* define words */
#define DST_DoDelete		1	/* delete words */
/* more commands (not implemented) */
#define DST_DoQuery		2	/* query on the dictionary info */

#define DST_DoPrint		9999	/* print debug information */

/* Internal Functions */
struct RkParam		*RkGetSystem();
struct DD			*RkGetSystemDD();
struct DD			*RkGetUserDD();
struct RkContext	*RkGetContext(int cx_num);
struct RkContext	*RkGetXContext(int cx_num);

struct RkKxGram		*RkReadGram(int fd);
struct RkKxGram		*RkOpenGram(char *mydic);
struct RkKxGram		*RkDuplicateGram(struct RkKxGram *ogram);
void				RkCloseGram(struct RkKxGram *gram);

int					_RkInitializeCache(int size);
void				_RkFinalizeCache(void);
struct ncache 		*_RkFindCache(struct DM *dm, long addr);
struct ncache 		*_RkReadCache(struct DM *dm, long addr);
void 				_RkFreeCache();
void 				_RkDerefCache(struct ncache *cache);
void 				_RkPurgeCache(struct ncache *cache);
void 				_RkKillCache(struct DM *dm);

int 				_RkRenbun2(struct RkContext *cx, int firstlen);
void 				_RkLearnBun(struct RkContext *cx, int cur, int mode);

int 				RkScanWcand(Wrec *wrec, struct RkWcand *word, int maxword);
int 				RkUniqWcand(struct RkWcand *wc, int nwc);
int 				RkUnionWcand(struct RkWcand *wc1, int nc1, int wlen1, struct RkWcand *wc2, int nc2);
int 				RkSubtractWcand(struct RkWcand *wc1, int nc1, struct RkWcand *wc2, int nc2, unsigned long *lucks);

unsigned char 		*RkGetGramName(struct RkKxGram *gram, int row);
int 				RkGetGramNum(struct RkKxGram *gram, char *name);
WCHAR_T 			*RkUparseWrec();
WCHAR_T 			*_RkUparseWrec(struct RkKxGram *gram, Wrec *src, WCHAR_T *dst, int maxdst, unsigned long *lucks, int add);
Wrec 				*RkParseWrec(struct RkKxGram *gram, WCHAR_T *src, unsigned left, unsigned char *dst, unsigned maxdst);
Wrec 				*RkParseOWrec(struct RkKxGram *gram, WCHAR_T *src, unsigned char *dst, unsigned maxdst, unsigned long *lucks);
WCHAR_T 			*RkUparseGramNum(struct RkKxGram *gram, int row, WCHAR_T *dst, int maxdst);
WCHAR_T 			*RkParseGramNum(struct RkKxGram *gram, WCHAR_T *src, int *row);

/* Context */
void 				_RkEndBun(struct RkContext *cx);
void 				freeDF();

int 				_RkCandNumber(unsigned char *wrec);
int 				_RkWordLength(unsigned char *wrec);
int 				_RkCalcLog2(int n);
int 				_RkCalcUnlog2(int x);

/* etc. */


/* lang dep part */
#define IS_WC_G1_HYPHEN(wc)	((wc) == 0xa1bc)
#define IS_HYPHEN(wc)		(IS_WC_G1_HYPHEN(wc) || ((wc) == '-'))
/* space ¤â¸õÊä¤È¤·¤Æ»È¤¤¤¿¤¤¤¬º£¤Î½ê¤Ï space */
#define IS_DIC_PUNCT(euc)	isspace(euc)

int				_RkRealizeDF();

struct DM		*_RkSearchDDQ(struct DD	**dd, char *name, int type);
struct DM		*_RkSearchDDP(struct DD	**ddp, char	*name);
struct DM		*_RkSearchUDDP(struct DD **ddp, unsigned char *name);
struct DM		*_RkSearchDDMEM(struct DD **ddp, char *name);

int				_RkIsinDDP();
struct DD		** _RkCopyDDP(struct DD	**ddp);
struct DD		**_RkCreateDDP(char		*ddpath);
void 			_RkFreeDDP(struct DD	**ddp);

struct DM		*DMcreate(struct DD		*dd,char		*spec);
int 			DMremove(struct DM	*dm);
int 			DMrename(struct DM		*dm,unsigned char	*nickname);

int 			_RkMountMD(struct RkContext	*cx,struct DM		*dm,struct DM		*qm,int		mode,int		firsttime);
void			_RkUmountMD( struct RkContext	*cx, struct MD		*md);

char 			*_RkCreatePath(struct DD *dd, char *name);
char 			*_RkCreateUniquePath(struct DD *dd, char *proto);
char 			*_RkMakePath(struct DF *df);

unsigned char	*_RkCreateHeader(struct HD *hd, unsigned *size);
int 			_RkReadHeader(int fd, struct HD *hd, off_t off_from_top);
void 			_RkClearHeader(struct HD *hd);
void 			_RkRehashCache(struct ncache *cache, long addr);

/*
 * limits
 */
/*
 *   NOTE: The following number does not includes EOS at the end of string.
 *	RK_PATH_BMAX	maximum path name length
 *	RK_LINK_BMAX	maximum file name length
 *	RK_MEMBER_BMAX	maximum member name length
 *	RK_NICK_BMAX	maximum nickname length
 */
#ifdef PATH_MAX
#define	RK_PATH_BMAX	PATH_MAX
#else
#define RK_PATH_BMAX 1024
#endif
#ifdef  NAME_MAX
#define RK_LINK_BMAX	NAME_MAX
#else
#ifdef	MAXNAMLEN
#define RK_LINK_BMAX	MAXNAMLEN
#else
#define RK_LINK_BMAX	14
#endif 
#endif
#define	RK_MEMBER_BMAX	255
#define	RK_NICK_BMAX	255

#define RK_LINE_BMAX    1024
#define RK_BUFFER_SIZE  1024

/* 
 * RK_KEY_WMAX <= RK_LEN_WMAX
 */
#define	RK_KEY_WMAX		0x7f
#define	RK_LEFT_KEY_WMAX	0x3f
const unsigned long 	RK_LEN_WMAX=0x7f;
/*
 * RK_CAND_WMAX <= RK_LEN_WMAX
 */
/* ¸õÊä¿ô */
#define	NW_MAXCAND	0x07	/* 7 */
#define	EX_NW_MAXCAND	0xfff	/* 4096 */

/* ¸õÊäÄ¹ */
#define	NW_MAXCANDLEN	0x7f
#define RK_CAND_WMAX	0x7f
#define RK_CAND_NMAX    0xfff /* 4095 */

/* wrec len */
#define NW_MAXWREC	0x3f	/* 63 */
#define EX_NW_MAXWREC	0x1fff	/* 8191 */

#define RK_RCNAME_BMAX	16	/* essential ?! */

#define	RK_DIV_NMAX	2048
#define	RK_WREC_BMAX	EX_NW_MAXWREC
#define	RK_MAX_TEXT_LEN	MAX_WREC_BMAX

#define	RK_CONC_NMAX	16	/* ÀÜÂ³¤¹¤ëÉÕÂ°¸ì¤Î¿ô(¸ú¤¤¤Æ¤ë¤Î¤«¤Ê¡©) */

#define RK_MAX_HDRSIZ	1024


/* bits.c */
long _RkPackBits(unsigned char *dst_bits, long dst_offset, int bit_size, unsigned *src_ints, int count);
long _RkUnpackBits(unsigned *dst_ints, unsigned char *src_bits, long src_offset, int bit_size, int count);
long _RkCopyBits(unsigned char *dst_bits, long dst_offset, int bit_size, unsigned char *src_bits, long src_offset, int count);
int _RkSetBitNum(unsigned char *dst_bits, unsigned long dst_offset, int bit_size, int n, int val);
int _RkCalcFqSize(int n);
int _RkPrintPackedBits(unsigned char *bits, int offset, int bit_size, int count);

/* bun.c */
struct nstore *_RkReallocBunStorage(struct nstore *src, unsigned len);
int RkeResize (int cx_num, int len);
int RkeGetStat (int cx_num, RkStat *st);
int RkeGetLex (int cx_num, RkLex *dst, int maxdst);
int RkwStoreRange(int cx_num, WCHAR_T *yomi, int maxyomi);
int RkwSetLocale(int cx_num, unsigned char *locale);

int _RkResize(int cx_num, int len, int t);
int _RkwSync(struct RkContext *cx, char *dicname);

/* context.c */
struct RkParam *RkGetSystem(void);
struct DD *RkGetSystemDD(void);
struct RkContext *RkGetContext(int cx_num);
struct RkContext *RkGetXContext(int cx_num);
int RkwSetAppName(int Context, char *name);
int RkwGetProtocolVersion(int *majorp, int *minorp);

/* dd.c */
int DMcheck(char *spec, char *name);
int DMchmod(struct DM *dm, int mode);
int DDchmod(struct DD *dd, int mode);

int _RkRealizeDF(struct DF *df);
int _RkRealizeDD(struct DD	*dd);
struct DM *_RkSearchDicWithFreq (struct DD **, char *, struct DM **);
int _RkIsInDDP(struct DD	**ddp, struct DD	*dd);

/* dic.c */
int copyFile(struct DM *src, struct DM *dst);
void freeTdn(struct RkContext *cx);

/* fq.c */
struct RUT *allocRUT(unsigned long hn);
unsigned long searchRut(struct RUT *ruc, unsigned long csn);
int entryRut(struct RUT *ruc, unsigned long csn, unsigned long tick);
struct RUT *LoadRUC(int fr);
int FQopen(struct DM *dm, struct DM *qm, char *file, int mode);
void FQclose(struct RkContext *cx, struct DM *dm, struct DM *qm, char *file);
int FQsync(struct RkContext *cx, struct DM *dm, struct DM *qm, char *file);

/* ncache.c */
int _RkRelease(void);

/* ngram.c */
int RkGetGramNum(struct RkKxGram *gram, char *name);
unsigned char *RkGetGramName(struct RkKxGram *gram, int row);
WCHAR_T *RkUparseWrec(struct RkKxGram *gram, Wrec *src, WCHAR_T *dst, int maxdst, unsigned long *lucks);
struct TW *RkCopyWrec(struct TW *src);
struct TW *RkSubtractWrec(struct TW *tw1, struct TW *tw2);
struct TW *RkUnionWrec(struct TW *tw1, struct TW *tw2);

int _RkRowNumber(unsigned char *wrec);

/* nword.c */
void _RkFreeBunq(struct nstore *st);
void _RkFreeQue(struct nstore *st, int s, int e);
int _RkRegisterNV(struct NV *nv, Wrec *yomi, int len, int half);
WCHAR_T *_RkGetKanji(struct nword *cw, WCHAR_T *key, unsigned long mode);
int _RkSubstYomi(struct RkContext *cx, int ys, int ye, WCHAR_T *yomi, int newLen);
int _RkFlushYomi(struct RkContext *cx);

/* permdic.c */
int _Rkpopen(struct DM *dm, char *dfnm, int mode, struct RkKxGram *gram);
int _Rkpclose(struct DM *dm, char *dfnm, struct RkKxGram *gram);
int _RkEql(WCHAR_T *a, unsigned char *b, int n);
int _Rkpsearch(struct RkContext *cx, struct DM *dm, WCHAR_T *key, int n, struct nread *nread, int mc, int *cf);
int _Rkpio(struct DM *dm, struct ncache *cp, int io);
int _Rkpctl(struct DM *dm, struct DM *qm, int what, WCHAR_T *arg, struct RkKxGram *gram);
int _Rkpsync(struct RkContext *cx, struct DM *dm, struct DM *qm);
int _Rktopen(struct DM	*dm,char	*file,int	mode,struct RkKxGram	*gram);
int	 _Rktclose(struct DM	*dm,char	*file,struct RkKxGram	*gram);
int _Rktsearch(struct RkContext	*cx,struct DM		*dm,WCHAR_T		*key,int		n,struct nread	*nread,int		maxcache,int		*cf);
int	 _Rktio( struct DM		*dm,struct ncache	*cp,int		io);
int _Rktctl(struct DM	*dm,struct DM	*qm,int	what,WCHAR_T	*arg,struct RkKxGram	*gram);
int _Rktsync(struct RkContext *cx, struct DM	*dm, struct DM	 *qm);

/* util.c */
int uslen(WCHAR_T *us);
void usncopy(WCHAR_T *dst, WCHAR_T *src, int len);
unsigned char *ustoeuc(WCHAR_T *src, int srclen, unsigned char *dest, int destlen);
WCHAR_T *euctous(unsigned char *src, int srclen, WCHAR_T *dest, int destlen);
void _Rkpanic(char *fmt, int p, int q, int r);
WCHAR_T uniqAlnum(WCHAR_T c);
int set_hdr_var(struct HD *hd, int n, unsigned long var);
int HowManyChars(WCHAR_T *yomi, int len);
int HowManyBytes(WCHAR_T *yomi, int len);
int printWord(struct nword *w);
int showWord(struct nword *w);

int _RkCalcLog2(int n);
unsigned char *_RkCreateHeader(struct HD *hd, unsigned *size);
unsigned long _RkGetTick(int mode);
int _RkGetLink(struct ND *dic, long pgno, unsigned long off, unsigned long *lvo, unsigned long *csn);
unsigned long _RkGetOffset(struct ND *dic, unsigned char *pos);
#endif /* _RKintern_h */
/* don't add stuff after this line */
