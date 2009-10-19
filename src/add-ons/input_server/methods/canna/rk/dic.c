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

#if !defined(lint) && !defined(__CODECENTER__)
static char rcsid[]="@(#) 102.1 $Id: dic.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif
/*LINTLIBRARY*/

#include "RKintern.h"

#include <stdio.h> /* for sprintf */
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define dm_td		dm_extdata.ptr
#define cx_gwt		cx_extdata.ptr
#define Is_Gwt_CTX(cx)

#define ND2RK(s)	((0x80 >> (int)(s)) & 0xff)
#define	STRCMP(d, s)	strcmp((char *)(d), (char *)(s))

#define FREQ_TEMPLATE	"frq%d.cld"
#define USER_TEMPLATE	"usr%d.ctd"
#define PERM_TEMPLATE	"bin%d.cbd"

#define DEFAULT_PERMISSION	"w"

static int locatepath(struct DD *userDDP[], struct DD *ddpath[], int mode);
static struct td_n_tupple *pushTdn(struct RkContext *cx, struct TD *tdp);

/* locatepath -- ¼­½ñ¥µ¡¼¥Á¥Ñ¥¹¤ò mode ¤Ë±þ¤¸¤ÆÄ¥¤ë

   return value:
       0: À®¸ù
   ACCES: ¥¨¥é¡¼(¥°¥ë¡¼¥×¤ò»ØÄê¤·¤¿¤Î¤Ë DDPATH ¤ËÂ¸ºß¤·¤Ê¤¤)
 */

static int
locatepath(struct DD *userDDP[], struct DD *ddpath[], int mode)
{
  /* find dictionary under system and user/group directory */
  if (mode & RK_SYS_DIC) {
    if (ddpath[2]) {
      userDDP[0] = ddpath[2];
    }
    else {
      return ACCES;
    }
  } else  
  if (mode & RK_GRP_DIC) {
    if (ddpath[1] && ddpath[2]) {
      /* ¥°¥ë¡¼¥×¼­½ñ¤È¥·¥¹¥Æ¥à¼­½ñ¤¬¤Á¤ã¤ó¤È¤¢¤ì¤Ð */
      userDDP[0] = ddpath[1];
    }
    else {
      return ACCES;
    }
  }
  else { /* ¥æ¡¼¥¶¼­½ñ */
    userDDP[0] = ddpath[0];
  }
  userDDP[1] = (struct DD*)0;
  return 0;
}


/* int
 * RkwCreateDic(cx_num, dicname, mode)
 *
 * °ú¤­¿ô
 *         int            cx_num    ¥³¥ó¥Æ¥¯¥¹¥È¥Ê¥ó¥Ð¡¼
 *	   unsigned char  *dicname  ¼­½ñ¤Ø¤Î¥Ý¥¤¥ó¥¿
 *	   int            mode      ¼­½ñ¤Î¼ïÎà¤È¶¯À©¥â¡¼¥É¤ÎOR
 *             ¼­½ñ¤Î¼ïÎà 
 *                 #define	Rk_MWD		0x80
 *                 #define	Rk_SWD		0x40
 *                 #define	Rk_PRE		0x20
 *                 #define	Rk_SUC		0x10
 *             ¶¯À©¥â¡¼¥É
 *                 #define KYOUSEI	        0x01
 *                 ¶¯À©¤·¤Ê¤¤¾ì¹ç           0x00
#define PL_DIC		(0x0100)
#define PL_ALLOW	(PL_DIC << 1)
#define PL_INHIBIT	(PL_DIC << 2)
#define PL_FORCE	(PL_DIC << 3)
 *
 * ¥ê¥¿¡¼¥óÃÍ
 *             À®¸ù¤·¤¿¾ì¹ç                                 0
 *             À®¸ù¤·¤¿¾ì¹ç(¾å½ñ¤­¤·¤¿¾ì¹ç)                 1
 *             ¥¢¥í¥±¡¼¥·¥ç¥ó¤Ë¼ºÇÔ¤·¤¿¾ì¹ç                -6  NOTALC
 *             ¼­½ñ¤¬¥Ð¥¤¥Ê¥ê¼­½ñ¤Ç¤¢¤Ã¤¿¾ì¹ç              -9    BADF
 *             dics.dir¤Ë°Û¾ï¤¬¤¢¤Ã¤¿¾ì¹ç                 -10   BADDR
 *             GetDicFilenameÊÖ¤êÃÍ¤¬-1¤Î¾ì¹ç           -13   ACCES
 *             MakeDicFile¤Ë¼ºÇÔ¤·¤¿¾ì¹ç                  -13   ACCES
 *             CreatDic¤Ë¼ºÇÔ¤·¤¿¾ì¹ç                     -13   ACCES
 *             ¼­½ñ¤¬¥Þ¥¦¥ó¥ÈÃæ¤Ç¤¢¤Ã¤¿¾ì¹ç               -16   MOUNT
 *             ¼­½ñ¤¬¤¹¤Ç¤Ë¤¢¤ë¾ì¹ç(¶¯À©¤Ç¤Ê¤¤¾ì¹ç)       -17   EXIST
 *             ¼­½ñ¤¬»ÈÍÑÃæ¤Ç¤¢¤Ã¤¿¾ì¹ç                   -26  TXTBSY
 *             mode¤¬°Û¾ïÃÍ¤Ç¤¢¤Ã¤¿¾ì¹ç                   -99  BADARG
 *             ¥³¥ó¥Æ¥¯¥¹¥È¹½Â¤ÂÎ¤¬Â¸ºß¤·¤Ê¤¤¾ì¹ç        -100 BADCONT
 */

int
RkwCreateDic(int cx_num, char *dicname, int mode)
{
  struct RkParam	*sx = RkGetSystem();
						  
  struct RkContext	*cx = RkGetContext(cx_num);
  struct DM             *sm, *um, *tm;
  int			type;
  struct DD		*userDDP[2], *systemDDP[2];
  char	      		*filename, extent[5];
  int ret;
#ifndef USE_MALLOC_FOR_BIG_ARRAY
  char            	spec[RK_LINE_BMAX];
#else
  char *spec = (char *)malloc(RK_LINE_BMAX);
  if (!spec) {
    return NOTALC;
  }
#endif
  
  if(!dicname || !dicname[0]) {
    ret = ACCES;
    goto return_ret;
  }
  if (strlen(dicname) >= (unsigned)RK_NICK_BMAX) {
    ret = INVAL;
    goto return_ret;
  }
  if ( !cx || !cx->ddpath || !cx->ddpath[0] ) {
    ret = BADCONT;
    goto return_ret;
  }
  if ( !sx || !sx->ddpath || !sx->ddpath[0] ) {
    ret = BADCONT;
    goto return_ret;
  }
#ifndef STANDALONE /* Is it true ? */
  if ( cx->ddpath[0] == sx->ddpath[0] ) {
    ret = BADCONT;
    goto return_ret;
  }
#endif

  if (locatepath(userDDP, cx->ddpath, mode) < 0) {
    ret = ACCES;
    goto return_ret;
  }
  if (!(userDDP[0]->dd_flags & DD_WRITEOK)) {
    ret = ACCES;
    goto return_ret;
  }

  systemDDP[0] = sx->ddpath[0];
  systemDDP[1] = (struct DD *)0;

  type = (mode & PL_DIC) ? DF_FREQDIC : DF_TEMPDIC;
/* find dictionary in current mount list */
  sm = _RkSearchDDQ(systemDDP, dicname, type);
  um = _RkSearchDDQ(userDDP, dicname, type);

  if (um && !(mode & KYOUSEI)) {
    ret = EXIST;
    goto return_ret;
  }

  if (mode & PL_DIC) {
    if (!sm) {
      if(_RkSearchDDQ(systemDDP, dicname, DF_TEMPDIC)) {
	ret = BADF;
	goto return_ret;
      }
      ret = NOENT;
      goto return_ret;
    }
    if (!um) {
      struct DM	*dm;
      
      if (!(filename = _RkCreateUniquePath(userDDP[0], FREQ_TEMPLATE))) {
	ret = ACCES;
	goto return_ret;
      }
      (void)sprintf(spec, "%s(%s) -%s--%s-\n",
		    filename, sm->dm_dicname, sm->dm_nickname,
		    DEFAULT_PERMISSION);
      if (!DMcheck(spec, dicname)) {
	ret = NOENT;
	goto return_ret;
      }
      if (!(dm = DMcreate(userDDP[0], spec))) {
	ret = NOTALC;
	goto return_ret;
      }
      if (copyFile(sm, dm)) {
	ret = ACCES;
	goto return_ret;
      }else{
	ret = 0;
	goto return_ret;
      }
    } else {
      if (!(ND2RK(um->dm_class) & mode)) {
	ret = INVAL;
	goto return_ret;
      }
      if ( um->dm_rcount > 0 ) {
	ret = TXTBSY;
	goto return_ret;
      }
      if ( !um->dm_file ) {
	ret = BADCONT;	/* INVAL SHOULD BE REPLACED... MAKO 1225 */
	goto return_ret;
      }

      if(_RkRealizeDF(um->dm_file)) {/* ¤³¤ì¤¤¤é¤Ê¤¤¤ó¤¸¤ã¤Ê¤¤¡© kon 1993.11 */
	ret = ACCES;
	goto return_ret;
      }
      if ( copyFile(sm, um) ) {
	ret = ACCES;
	goto return_ret;
      }else{
	ret = 1;
	goto return_ret;
      }
    }
  } else {
    /*    um = _RkSearchDDQ(userDDP, dicname, DF_TEMPDIC);*/
    tm = _RkSearchDDP(userDDP, dicname);
    if (tm != um) {
      ret = BADF;
      goto return_ret;
    }
    if (!um) {
      if (!(filename = _RkCreateUniquePath(userDDP[0], USER_TEMPLATE))) {
	ret = ACCES;
	goto return_ret;
      }
      if (mode & Rk_MWD) {
	  (void)strcpy(extent, "mwd");
      } else if (mode & Rk_SWD) {
	  (void)strcpy(extent, "swd");
      } else if (mode & Rk_PRE) {
	  (void)strcpy(extent, "pre");
      } else if (mode & Rk_SUC) {
	  (void)strcpy(extent, "suc");
      } else {
	  /* return INVAL;	*/
	  (void)strcpy(extent, "mwd");
      };
      (void)sprintf(spec, "%s(.%s) -%s--%s-\n", filename, extent, dicname,
		    DEFAULT_PERMISSION);
      if (!DMcheck(spec, dicname)) {
	ret = NOENT;
	goto return_ret;
      }
      if (!DMcreate(userDDP[0], spec)) {
	ret = NOTALC;
	goto return_ret;
      }
      _RkRealizeDD(userDDP[0]);
      ret = 0;
      goto return_ret;
    } else {
      if ( um->dm_rcount > 0 ) {
	ret = TXTBSY;
	goto return_ret;
      }
      if ( !um->dm_file ) {
	ret = BADCONT;	/* INVAL SHOULD BE REPLACED... MAKO 1225 */
	goto return_ret;
      }
      sprintf(spec, "%s(%s) -%s--%s%s-\n",
	      um->dm_file->df_link, um->dm_dicname, um->dm_nickname,
	      (um->dm_flags & DM_READOK) ? "r" : "",
	      (um->dm_flags & DM_WRITEOK) ? "w" : "");
      if(_RkRealizeDF(um->dm_file)) {
	ret = ACCES;
	goto return_ret;
      }
      ret = 1;	/* backward compatiblity ... 1224 Xmas */
    }
  }
 return_ret:
#ifdef USE_MALLOC_FOR_BIG_ARRAY
  free(spec);
#endif
  return ret;
}

int copyFile(struct DM *src, struct DM *dst)
{
  struct DF	*srcF = src->dm_file;
  struct DD	*srcD = srcF->df_direct;
  struct DF	*dstF = dst->dm_file;
  struct DD	*dstD = dstF->df_direct;
  char		*srcN, *dstN;
#ifndef WIN
  int		srcFd, dstFd;
  int		n;
#else
  HANDLE srcFd, dstFd;
  DWORD n, nextn;
#endif
  int		ecount = 0;

  srcN = _RkCreatePath(srcD, srcF->df_link);
  if (srcN) {
#ifdef WIN
    srcFd = CreateFile(srcN, GENERIC_READ,
		       FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#else
    srcFd = open(srcN, 0);
#endif
    free(srcN);
    if
#ifdef WIN
      (srcFd != INVALID_HANDLE_VALUE)
#else
      (srcFd >= 0)
#endif
    {
      dstN = _RkCreatePath(dstD, dstF->df_link);
      if (dstN) {
#ifdef WIN
	dstFd = CreateFile(dstN, GENERIC_WRITE,
			   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
#else
	dstFd = creat(dstN, 0666);
#endif
	free(dstN);

	if
#ifdef WIN
	  (dstFd != INVALID_HANDLE_VALUE)
#else
	  (dstFd >= 0)
#endif
	{
	  char b[RK_BUFFER_SIZE];
	  /* I will leave this array on the stack because it may be a rare
	     case to use this function. 1996.6.5 kon */

	  _RkRealizeDD(dstD);

	  while 
#ifdef WIN
	    (ReadFile(srcFd, b, RK_BUFFER_SIZE, &n, NULL) && n > 0)
#else
	    ((n = read(srcFd, b, RK_BUFFER_SIZE)) > 0)
#endif
	  { /* do copy */
	    if
#ifdef WIN
	      (!WriteFile(dstFd, b, n, &nextn, NULL) || nextn != n)
#else
	      ( write(dstFd, b, n) != n )
#endif
	    {
	      ecount++;
	      break;
	    }
	  }
	  if (
#ifdef WIN
	    !CloseHandle(dstFd)
#else
	    close(dstFd) < 0
#endif
	      || n < 0)
	  {
	    ecount++;
	  }
	}
      }
#ifdef WIN
      CloseHandle(srcFd);
#else
      close(srcFd);
#endif
    }
  }
  return ecount ? -1 : 0;
}

/*
 * RkwListDic(cx_num, dirname, buf, size)
 * int  cx_num;             ¥³¥ó¥Æ¥¯¥¹¥È¥Ê¥ó¥Ð¡¼
 * unsigned char *dirname;  ¼­½ñ¥ê¥¹¥È¤ò½ÐÎÏ¤·¤¿¤¤¥Ç¥£¥ì¥¯¥È¥êÌ¾
 * unsigned char *buf;      ¼­½ñ¥ê¥¹¥È¤¬ÊÖ¤Ã¤Æ¤¯¤ë¥Ð¥Ã¥Õ¥¡
 * int  size;               ¥Ð¥Ã¥Õ¥¡¤Î¥µ¥¤¥º
 *
 * ¥ê¥¿¡¼¥óÃÍ               
 *             À®¸ù¤·¤¿¾ì¹ç                      ¼­½ñ¤Î¿ô
 *             ¥³¥ó¥Æ¥¯¥¹¥È¥Ê¥ó¥Ð¡¼¤¬Éé¤Î¾ì¹ç          BADCONT
 *             RkwCreateContext¤Ë¼ºÇÔ¤·¤¿¾ì¹ç           BADCONT
 *             RkwSetDicPath¤Ë¼ºÇÔ¤·¤¿¾ì¹ç              NOTALC
 */
int
RkwListDic(int cx_num, char *dirname, char *buf, int size)
{
  int dicscnt;
  int new_cx_num;

  if(!dirname || !strlen(dirname))
    return 0;
  if (cx_num < 0)
    return BADCONT;
  if((new_cx_num = RkwCreateContext()) < 0)
    return BADCONT;
  if (RkwSetDicPath(new_cx_num, dirname) == -1) {
    RkwCloseContext(new_cx_num);
    return NOTALC;
  }
  dicscnt = RkwGetDicList(new_cx_num, buf, size);
  (void)RkwCloseContext(new_cx_num);
  return (dicscnt);
}

/* int
 * RkwRemoveDic(cx_num, dicname, mode)
 *
 * »ØÄê¤µ¤ì¤¿¥³¥ó¥Æ¥¯¥¹¥È¤Ë»ØÄê¤µ¤ì¤¿¼­½ñ¤¬Â¸ºß¤¹¤ì¤Ð
 * ¤½¤Î¼­½ñ¤òºï½ü¤¹¤ë¡£
 *
 * °ú¤­¿ô
 *             int            cx_num     ¥³¥ó¥Æ¥¯¥¹¥È¥Ê¥ó¥Ð¡¼
 *             unsigned char  *dicname   ¼­½ñÌ¾
 *
 * ¥ê¥¿¡¼¥óÃÍ
 *             À®¸ù¤·¤¿¾ì¹ç                             0
 *             ¼­½ñ¤¬Â¸ºß¤·¤Ê¤¤¾ì¹ç                    -2   NOENT
 *             ¼­½ñ¤¬¥Ð¥¤¥Ê¥ê¼­½ñ¤Ç¤¢¤Ã¤¿¾ì¹ç          -9    BADF
 *             RemoveDic¤ÎÊÖ¤êÃÍ¤¬-1¤Î¾ì¹ç            -13   ACCES
 *             ¥Þ¥¦¥ó¥È¤·¤Æ¤¤¤¿¾ì¹ç                   -26  TXTBSY
 *             ¥³¥ó¥Æ¥¯¥¹¥È¹½Â¤ÂÎ¤¬Â¸ºß¤·¤Ê¤¤¾ì¹ç    -100 BADCONT
 */
int
RkwRemoveDic(int cx_num, char *dicname, int mode)
{
  struct RkContext	*cx = RkGetContext(cx_num);
/*  struct RkParam	*sx = RkGetSystem();	*/
  struct DD		*userDDP[2], *dum_direct;
  struct DM             *dm;
  char			*path;
  int res;

  if(!dicname)
    return NOENT;
  if ( !cx || !cx->ddpath || !cx->ddpath[0] )
      return BADCONT;

  if (locatepath(userDDP, cx->ddpath, mode) < 0) {
    return ACCES;
  }

  /* find dictionary in current mount list */
  dm = _RkSearchDDP(userDDP, (char *)dicname);
  if (!dm || ((mode & PL_DIC) && dm->dm_file->df_type != DF_FREQDIC)) {
    return NOENT;
  }
  if ( dm->dm_rcount > 0 ) 
	return TXTBSY;
  if ( !dm->dm_file ) /* ? */
	return BADCONT;
  if (!(dm->dm_file->df_direct->dd_flags & DD_WRITEOK) ||
      (!(dm->dm_flags & DM_WRITEOK) && !(mode & KYOUSEI))) {
    return ACCES;
  }
  if (!(path = _RkMakePath(dm->dm_file)))
	return NOTALC;
  res = unlink(path);
  free(path);  
  if(res)
    return ACCES;
  dum_direct = dm->dm_file->df_direct;
  DMremove(dm);
  (void)_RkRealizeDD(dum_direct);
  return 0;
}

/* int
 * RkwRenameDic(cx_num, oldnick, newnick, mode)
 *
 * »ØÄê¤µ¤ì¤¿¥³¥ó¥Æ¥¯¥¹¥È¤Ë»ØÄê¤µ¤ì¤¿¼­½ñ¤¬Â¸ºß¤¹¤ì¤Ð
 * ¤½¤Î¼­½ñ¤ÎÌ¾Á°¤òÊÑ¹¹¤¹¤ë¡£
 *
 * °ú¤­¿ô
 *          int            cx_num           ¥³¥ó¥Æ¥¯¥¹¥È¥Ê¥ó¥Ð¡¼
 *          unsigned char  *oldnick        ÊÑ¹¹¸µ¼­½ñÌ¾
 *          unsigned char  *newnick        ÊÑ¹¹Àè¼­½ñÌ¾
 *          int            mode             ¶¯À©¥â¡¼¥É
 *
 * ÊÖ¤êÃÍ (RKdic.h»²¾È)
 *          À®¸ù¤·¤¿¾ì¹ç                             0
 *          oldnick¤¬Â¸ºß¤·¤Ê¤¤¾ì¹ç                -2     NOENT
 *          RemoveDic¤ÎÊÖ¤êÃÍ¤¬-1¤Î¾ì¹ç             -2     NOENT
 *          ¼­½ñ¤¬¥Ð¥¤¥Ê¥ê¼­½ñ¤Ç¤¢¤Ã¤¿¾ì¹ç          -9      BADF
 *          RenameDicFile¤ÎÊÖ¤êÃÍ¤¬-1¤Î¾ì¹ç        -13     ACCES
 *          newnick¤¬Â¸ºß¤¹¤ë¾ì¹ç                 -17     EXIST
 *          oldnick¤ò¥Þ¥¦¥ó¥È¤·¤Æ¤¤¤¿¾ì¹ç         -26    TXTBSY
 *          newnick¤ò¥Þ¥¦¥ó¥È¤·¤Æ¤¤¤¿¾ì¹ç         -26    TXTBSY
 *          ¥³¥ó¥Æ¥¯¥¹¥È¹½Â¤ÂÎ¤¬Â¸ºß¤·¤Ê¤¤¾ì¹ç    -100   BADCONT
 */
int
RkwRenameDic(int cx_num, char *old, char *newc, int mode)
{
  struct RkContext	*cx = RkGetContext(cx_num);
  struct DD		*userDDP[2], *dd;
  struct DM		*dm1, *dm2;
  char			*path;
  char            	spec[RK_LINE_BMAX];
  /* I leave this array on the stack because this will not glow so big.
   1996.6.5 kon */
  
  if(!old || !*old)
    return NOENT;
  if(!newc || !*newc)
    return ACCES;
  if (!cx || !cx->ddpath || !cx->ddpath[0])
    return BADCONT;
  if (strlen((char *)newc) >= (unsigned)RK_NICK_BMAX) {
    return INVAL;
  }

  if (locatepath(userDDP, cx->ddpath, mode) < 0) {
    return ACCES;
  }

  dm1 = _RkSearchDDP(userDDP, (char *)old);
  if (!dm1) {
    return NOENT;
  }

  dd = dm1->dm_file->df_direct;
  if (!(dd->dd_flags & DD_WRITEOK)) {
    return ACCES;
  }

  dm2 = _RkSearchDDP(userDDP, (char *)newc);

  if (dm1->dm_rcount > 0) 
    return TXTBSY;
  if (dm2) { /* ¿·¤·¤¤Ì¾Á°¤¬¡¢´û¤Ë¼­½ñ¤È¤·¤ÆÂ¸ºß¤¹¤ì¤Ð */
    if (dm2->dm_rcount > 0) 
      return TXTBSY;
    if (!(mode & KYOUSEI))
      return EXIST;
    if (!(path = _RkMakePath(dm2->dm_file)))
      return NOTALC;
    (void)unlink(path);
    free(path);
    DMremove(dm2);
    DMrename(dm1, (unsigned char *)newc);
    (void)_RkRealizeDD(dd);
    return 1;
  } else {
    (void)sprintf(spec, "%s(.%s) -%s--%s%s-\n", "tmp.ctd", "mwd", newc,
		  (dm1->dm_flags & DM_READOK) ? "r" : "",
		  (dm1->dm_flags & DM_WRITEOK) ? "w" : "");
    if (!DMcheck(spec, newc))
      return NOENT; /* ¤Ê¤ó¤Ê¤ó¤À¤«ÎÉ¤¯Ê¬¤«¤é¤Ê¤¤ (1993.11 º£) */
    /* ¤¿¤á¤·¤Ë¤ä¤Ã¤Æ¤ß¤Æ¤¤¤ë¤Î¤«¤Ê¡© (1993.11 º£) */
    DMrename(dm1, (unsigned char *)newc);
    (void)_RkRealizeDD(dd);
    return 0;
  }
}

/* int
 * RkwCopyDic(cx, dir, from, to, mode)
 *
 * ¼­½ñ¤ò¥³¥Ô¡¼¤¹¤ë¡£
 *
 * °ú¤­¿ô
 *          int            cx              ¥³¥ó¥Æ¥¯¥¹¥È¥Ê¥ó¥Ð¡¼
 *          char           *dir	           ¥Ç¥£¥ì¥¯¥È¥êÌ¾
 *          char           *from           ¥³¥Ô¡¼¸µ¼­½ñÌ¾
 *          char           *to             ¥³¥Ô¡¼Àè¼­½ñÌ¾
 *          int            mode            ¥â¡¼¥É
 *
 * ÊÖ¤êÃÍ (RKdic.h»²¾È)
 *          À®¸ù¤·¤¿¾ì¹ç                           0
 *          oldnick¤¬Â¸ºß¤·¤Ê¤¤¾ì¹ç               -2     NOENT
 *          RemoveDic¤ÎÊÖ¤êÃÍ¤¬-1¤Î¾ì¹ç           -2     NOENT
 *          ¥Ç¥£¥ì¥¯¥È¥ê¤Î»ØÄê¤¬¤ª¤«¤·¤¤¾ì¹ç      -9     BADF
 *          RenameDicFile¤ÎÊÖ¤êÃÍ¤¬-1¤Î¾ì¹ç       -13    ACCES
 *          ¥á¥â¥ê¤¬Â­¤ê¤Ê¤«¤Ã¤¿¾ì¹ç                     NOTALC
 *          ¼­½ñÌ¾¤¬Ä¹¤¹¤®¤ë¾ì¹ç                         INVAL
 *          newnick¤¬Â¸ºß¤¹¤ë¾ì¹ç                 -17    EXIST
 *          oldnick¤ò¥Þ¥¦¥ó¥È¤·¤Æ¤¤¤¿¾ì¹ç         -26    TXTBSY
 *          newnick¤ò¥Þ¥¦¥ó¥È¤·¤Æ¤¤¤¿¾ì¹ç         -26    TXTBSY
 *          ¥³¥ó¥Æ¥¯¥¹¥È¹½Â¤ÂÎ¤¬Â¸ºß¤·¤Ê¤¤¾ì¹ç    -100   BADCONT
 */

int
RkwCopyDic(int co, char *dir, char *from, char *to, int mode)
{
  struct RkContext	*cx;
  struct DD		*userDDP[2];
  struct DM		*dm1, *dm2;
  char			*path, *perm = DEFAULT_PERMISSION;
  char *myddname;
  int res, v, con;
  
  if (!dir || !*dir) {
    return BADF;
  }
  if (!from || !*from)
    return NOENT;
  if (!to || !*to)
    return ACCES;
  if (strlen((char *)to) >= (unsigned)RK_NICK_BMAX) {
    return INVAL;
  }

  res = BADCONT;
  con = RkwCreateContext();

  cx = RkGetContext(co);
  if (!cx || !cx->ddpath || !cx->ddpath[0]) {
    if (con >= 0)
      RkwCloseContext(con);
    return BADCONT;
  }

  if (con >= 0) {
    int n = 2; /* for system dic */
    switch (mode & (RK_GRP_DIC | RK_SYS_DIC)) {
    case RK_GRP_DIC:
      n = 1; /* for group dic */
    case RK_SYS_DIC:
      if (!cx->ddpath[2]) {
	return BADCONT;
      }
      myddname = cx->ddpath[n]->dd_name;
      break;
    default:
      myddname = cx->ddpath[0]->dd_name;
      break;
    }

    res = NOTALC;
    path = (char *)malloc(strlen(dir) + 1 + strlen(myddname) + 1);
    if (path) {
      sprintf(path, "%s:%s", dir, myddname);

      res = NOTALC;
      v = RkwSetDicPath(con, path);
      free(path);
      if (v >= 0) {
	struct RkContext *cy = RkGetContext(con);

	res = ACCES;
	if (cy->ddpath[1]->dd_flags & DD_WRITEOK) {
	  userDDP[0] = cy->ddpath[0];
	  userDDP[1] = (struct DD *)0;

	  res = NOENT;
	  dm1 = _RkSearchDDP(userDDP, from);
	  if (dm1) {
	    int type = dm1->dm_file->df_type;

	    res = BADF;
	    if (type != DF_RUCDIC) {
	      userDDP[0] = cy->ddpath[1];
	      userDDP[1] = (struct DD *)0;

	      dm2 = _RkSearchDDP(userDDP, to);
	      if (dm2) { /* to ¤¬¤¢¤Ã¤Æ¡¢¶¯À©¥â¡¼¥É¤Ê¤é¾Ã¤¹ */
		if (dm2->dm_rcount > 0) {
		  res = TXTBSY;
		  goto newdicUsed;
		}
		if (!(mode & KYOUSEI)) {
		  res = EXIST;
		  goto newdicUsed;
		}
		if (!(path = _RkMakePath(dm2->dm_file))) {
		  res = NOTALC;
		  goto newdicUsed;
		}
		(void)unlink(path);
		free(path);
		switch (dm2->dm_flags & (DM_READOK | DM_WRITEOK)) {
		case (DM_READOK | DM_WRITEOK):
		  perm = "rw";
		  break;
		case DM_READOK:
		  perm = "r";
		  break;
		case DM_WRITEOK:
		  perm = "w";
		  break;
		default:
		  perm = "";
		  break;
		}
		DMremove(dm2);
	      }

	      { /* ¤¤¤è¤¤¤è¼­½ñ¤òºî¤ë */
		char *ptemplate, *filename;

		RkwSync(co, from); /* sometimes, this failes to an error */
		ptemplate =
		  (type == DF_FREQDIC) ? (char*)FREQ_TEMPLATE :
		    (type == DF_TEMPDIC) ? (char*)USER_TEMPLATE :
		      (char*)PERM_TEMPLATE;

		res = ACCES;
		filename = _RkCreateUniquePath(userDDP[0], ptemplate);
		if (filename) {
		  char spec[RK_LINE_BMAX];
  /* I leave this array on the stack because this will not glow so big.
   1996.6.5 kon */

		  (void)sprintf(spec, "%s(%s) -%s--%s-\n",
				filename, dm1->dm_dicname, to, perm);
		  res = NOTALC;
		  dm2 = DMcreate(userDDP[0], spec);
		  if (dm2) {
		    res = ACCES;
		    if (copyFile(dm1, dm2) == 0) {
		      (void)_RkRealizeDD(userDDP[0]);
		      res = 0;
		    }
		    else {
		      DMremove(dm2);
		    }
		  }
		}
	      }
	    }
	  }
	newdicUsed:;
	}
      }
    }
    RkwCloseContext(con);
  }
  return res;
}

/* int
 * RkwChmodDic(cx_num, dicname, mode)
 *
 * ¼­½ñ¤Î¥â¡¼¥É¤òÊÑ¹¹¤¹¤ë¡£
 *
 * °ú¤­¿ô
 *          int   cx_num           ¥³¥ó¥Æ¥¯¥¹¥È
 *          char  dicname          ¼­½ñÌ¾
 *          int   mode             ¥â¡¼¥É
 *
 * ÊÖ¤êÃÍ (RKdic.h»²¾È)
 *          À®¸ù¤·¤¿¾ì¹ç                             0
 *          dicname¤¬Â¸ºß¤·¤Ê¤¤¾ì¹ç                 -2     NOENT
 *          DMchmod ¤ÎÊÖ¤êÃÍ¤¬-1¤Î¾ì¹ç             -13     ACCES
 *          ¥³¥ó¥Æ¥¯¥¹¥È¹½Â¤ÂÎ¤¬Â¸ºß¤·¤Ê¤¤¾ì¹ç    -100   BADCONT
 */
int
RkwChmodDic(int cx_num, char *dicname, int mode)
{
  struct RkContext	*cx = RkGetContext(cx_num);
  struct DD		*dd, *userDDP[2];
  struct DM		*dm;
  int res;
  unsigned dirmode;

  res = BADCONT;
  if (cx && cx->ddpath && cx->ddpath[0]) {
    dirmode = mode & RK_DIRECTORY;
    if (dirmode != 0) { /* ¥Ç¥£¥ì¥¯¥È¥ê */
      switch (dirmode) {
      case RK_SYS_DIR:
	dd = (struct DD *)0; /* or SX.ddpath[0] */
	break;
      case RK_GRP_DIR:
	if (cx->ddpath[1] && cx->ddpath[2]) {
	  dd = cx->ddpath[1];
	}
	break;
      default: /* RK_USR_DIR */
	dd = cx->ddpath[0];
	break;
      }
      res = dd ? DDchmod(dd, mode) : ACCES;
    }
    else { /* ¥Õ¥¡¥¤¥ë */
      res = ACCES;
      if (locatepath(userDDP, cx->ddpath, mode) == 0) {
	res = NOENT;
	if(dicname && *dicname) {
	  dm = _RkSearchDDP(userDDP, dicname);
	  res = NOENT;
	  if (dm) {
	    struct DD *dd = dm->dm_file->df_direct;

	    res = DMchmod(dm, mode);
	    if (res >= 0) {
	      (void)_RkRealizeDD(dd);
	    }
	    else {
	      res = ACCES;
	    }
	  }
	}
      }
    }
  }
  return res;
}

/*
 * GetLine(cx, gram, tdp, line)
 * struct RkContext            *cx
 * struct RkKxGram	*gram
 * struct TD            *tdp
 * WCHAR_T	*line
 *
 * ÊÖ¤êÃÍ À®¸ù  0
 *        ¼ºÇÔ -1
 */
static struct td_n_tupple *
pushTdn(struct RkContext *cx, struct TD *tdp)
{
  struct td_n_tupple	*newtd;
  struct _rec		*gwt;
  if (!cx || !(gwt = (struct _rec *)cx->cx_gwt) ||
      !(newtd = (struct td_n_tupple *)malloc(sizeof(struct td_n_tupple)))) {
    return (struct td_n_tupple *)0;
  }
  newtd->td = (char *)tdp;
  newtd->n = 0;
  newtd->next = (struct td_n_tupple *)gwt->tdn;
  gwt->tdn = (struct td_n_tupple *)newtd;
  return newtd;
}

void
freeTdn(struct RkContext *cx)
{
  struct td_n_tupple *work;
  struct _rec	*gwt = (struct _rec *)cx->cx_gwt;
  if (gwt) {
    while((work = gwt->tdn) != (struct td_n_tupple *)0) {
      gwt->tdn = work->next;
      free(work);
    };
  };
}

inline void
popTdn(struct RkContext *cx)
{
  struct td_n_tupple *work;
  struct _rec	*gwt = (struct _rec *)cx->cx_gwt;
  work = gwt->tdn;
  if (work) {
    gwt->tdn = work->next;
    free(work);
  }
}

inline int
GetLine(struct RkContext *cx, struct RkKxGram *gram, struct TD *tdp, WCHAR_T *line, int size)
{
  struct TD	*vtd;
  struct TN	*vtn;
  struct _rec	*gwt = (struct _rec *)cx->cx_gwt;
  
  if (tdp) {
    if (gwt->tdn)
      freeTdn(cx);
    if(!pushTdn(cx, tdp))
      return NOTALC;
  }
  while (gwt->tdn && gwt->tdn->n >= (int)((struct TD *)gwt->tdn->td)->td_n)
    popTdn(cx);
  if (gwt->tdn == (struct td_n_tupple *)0)
    return -1;
  vtd = (struct TD *)gwt->tdn->td;
  vtn = vtd->td_node + gwt->tdn->n;
  while ( !IsWordNode(vtn) ) {
    gwt->tdn->n++;
    if(!pushTdn(cx, vtn->tn_tree))
      return NOTALC;
    vtd = (struct TD *)gwt->tdn->td;
    vtn = vtd->td_node;
  }
  if (RkUparseWrec(gram, vtn->tn_word->word, line, size, vtn->tn_word->lucks)) {
    gwt->tdn->n++;
    return 0;
  } else
    return -1;
}

/*
 * RkwGetWordTextDic(cx_num, dirname, dicname, info, infolen)
 *
 * int            cx_num      ¥³¥ó¥Æ¥¯¥¹¥ÈNO
 * unsigned char  *dirname    ¥Ç¥£¥ì¥¯¥È¥êÌ¾
 * unsigned char  *dicname    ¼­½ñÌ¾
 * unsigned char  *info       ¥Ð¥Ã¥Õ¥¡
 * int            infolen     ¥Ð¥Ã¥Õ¥¡¤ÎÄ¹¤µ
 *
 * ÊÖ¤êÃÍ : ¼ÂºÝ¤Ëinfo¤ËÆþ¤Ã¤¿Ä¹¤µ
 *          ºÇ¸å¤Þ¤ÇÆÉ¤ó¤Ç¤¤¤¿¤é          £°¤òÊÖ¤¹
 *          RkwCreateContext¤Ë¼ºÇÔ¤·¤¿     BADCONT
 *          RkwDuplicateContext¤Ë¼ºÇÔ¤·¤¿  BADCONT
 *          RkGetContext¤Ë¼ºÇÔ¤·¤¿        BADCONT
 *          RkwSetDicPath¤Ë¼ºÇÔ¤·¤¿        NOTALC
 *          RkwMountDic¤Ë¼ºÇÔ¤·¤¿          NOENT
 *          SearchUDDP¤Ë¼ºÇÔ¤·¤¿          NOENT
 *          ¥Ð¥¤¥Ê¥ê¼­½ñ¤À¤Ã¤¿                          -9   BADF
 *          dics.dir¤Ë°Û¾ï¤¬¤¢¤Ã¤¿¾ì¹ç                 -10   BADDR
 */
int
RkwGetWordTextDic(int cx_num, unsigned char *dirname, unsigned char *dicname, WCHAR_T *info, int infolen)
{
  struct RkContext *new_cx, *cx;
  struct DM *dm;
  int new_cx_num;
  struct TD *initial_td;
  unsigned size;
  unsigned char *buff = 0;
  struct _rec	*gwt;

  if (!dicname || !dirname || !info || !(cx = RkGetContext(cx_num)) || 
      !(gwt = (struct _rec *)cx->cx_gwt))
    return BADCONT;

  if(dicname[0] != '\0') {
    size = strlen((char *)dicname) + 1;
    if (!(buff = (unsigned char *)malloc(size)))
      return (NOTALC);
    (void)strcpy((char *)buff, (char *)dicname);

    if(dirname[0] != '\0') {
      if((new_cx_num = RkwCreateContext()) < 0) {
	free(buff);
	return BADCONT;
      }
      if(RkwSetDicPath(new_cx_num, (char *)dirname) < 0) {
	RkwCloseContext(new_cx_num);
	free(buff);
	return NOTALC;
      }
    } else {
      if ((new_cx_num = RkwDuplicateContext(cx_num)) < 0) {
	free(buff);
	return BADCONT;
      }
    }
    if (!(cx = RkGetContext(cx_num)) || !(gwt = (struct _rec *)cx->cx_gwt)) {
      RkwCloseContext(new_cx_num);
      free(buff);
      return BADCONT;
    }

    if (!(new_cx = RkGetContext(new_cx_num))) {
      if(dirname[0] != '\0') {
	RkwCloseContext(new_cx_num);
	free(buff);
	return BADCONT;
      }
    }
    if (gwt->gwt_cx >= 0) {
      RkwCloseContext(gwt->gwt_cx);
      gwt->gwt_cx = -1;
    }
    
    if(!STRCMP(dirname, SYSTEM_DDHOME_NAME)) {
      if (!(dm = _RkSearchDDP(new_cx->ddpath, (char *)dicname))) {
	if (dirname[0] != '\0') {
	  RkwCloseContext(new_cx_num);
	}
	free(buff);
	return NOENT;
      }
    } else {
      if (!(dm = _RkSearchUDDP(new_cx->ddpath, dicname))) {
	if(dirname[0] != '\0') {
	  RkwCloseContext(new_cx_num);
	}
	free(buff);
	return NOENT;
      }
    }
    if (DM2TYPE(dm) != DF_TEMPDIC ) {
      if(dirname[0] != '\0') {
	RkwCloseContext(new_cx_num);
      }
      free(buff);
      return BADF;
    }
    if(RkwMountDic(new_cx_num, (char *)dicname,0) == -1) {
      RkwCloseContext(new_cx_num);
      free(buff);
      return NOMOUNT;
    }

    if (!_RkSearchDDP(new_cx->ddpath, (char *)dicname)) {
      RkwCloseContext(new_cx_num);
      free(buff);
      return BADDR;
    }
    gwt->gwt_cx = new_cx_num;
    if (gwt->gwt_dicname)
      free(gwt->gwt_dicname);
    gwt->gwt_dicname = buff;
    initial_td = (struct TD *)dm->dm_td;
  }
  else {
    if ((new_cx_num = gwt->gwt_cx) < 0
	|| !(new_cx = RkGetContext(new_cx_num))) {
      if (gwt->gwt_dicname)
	free(gwt->gwt_dicname);
      gwt->gwt_dicname = (unsigned char *)0;
      return BADCONT;
    }
    initial_td = (struct TD *)0;
  }
  if (GetLine(new_cx, cx->gram->gramdic, (struct TD *)initial_td,
	      info, infolen) < 0) {
    RkwUnmountDic(new_cx_num, (char *)gwt->gwt_dicname);
    RkwCloseContext(new_cx_num);
    gwt->gwt_cx = -1;
    return 0;
  }
  infolen = uslen((WCHAR_T *)info);
  return infolen;
}
