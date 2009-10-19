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

/************************************************************************/
/* THIS SOURCE CODE IS MODIFIED FOR TKO BY T.MURAI 1997
/************************************************************************/


#if !defined(lint) && !defined(__CODECENTER__)
static char rcsid[] = "$Id: bun.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif

/* LINTLIBRARY */

#include <string.h>
#include "RKintern.h"


#define NEED_DEF
#ifdef RkSetErrno
#undef RkSetErrno
#define RkSetErrno(no)
#endif

const unsigned  OVERRUN_MARGIN = 0;

#define	STRCMP(d, s)	strcmp((char *)(d), (char *)(s))

inline void
usncopy(WCHAR_T *dst, WCHAR_T *src, int len)
{
	memcpy(dst, src, len * sizeof(WCHAR_T));
}

static void
freeBunStorage(struct nstore *s)
{
  if (s) {
    if (s->yomi)
      free((s->yomi-OVERRUN_MARGIN));
    if (s->bunq)
      free((s->bunq-OVERRUN_MARGIN));
    if (s->xq)
      free((s->xq-OVERRUN_MARGIN));
    if (s->xqh)
      free((s->xqh-OVERRUN_MARGIN));
    free(s);
  }
}

static struct nstore *
allocBunStorage(unsigned len)
{
  struct nstore	*s;

  s = (struct nstore *)malloc((unsigned)sizeof(struct nstore));
  if (s) {
    WCHAR_T	*p, *q, pat;
    int			i;

    s->yomi = (WCHAR_T *)0;
    s->bunq = (struct nbun *)0;
    s->xq = (struct nqueue *)0;
    s->xqh = (struct nword **)0;
    s->nyomi = (unsigned)0;
    s->maxyomi = (unsigned)len;

    s->yomi = (WCHAR_T *)calloc((s->maxyomi+1+2*OVERRUN_MARGIN), sizeof(WCHAR_T));
    s->maxbunq = (unsigned)len;
    s->maxbun = (unsigned)0;
    s->curbun = 0;
    s->bunq = (struct nbun *)calloc((unsigned)(s->maxbunq+1+2*OVERRUN_MARGIN),
				    sizeof(struct nbun));
    s->maxxq = len;
    s->xq = (struct nqueue *)calloc((unsigned)(s->maxxq+1+2*OVERRUN_MARGIN),
				    sizeof(struct nqueue));
    s->xqh = (struct nword **)calloc((unsigned)(s->maxxq+1+2*OVERRUN_MARGIN),
				     sizeof(struct nword *));
    if (!s->yomi || !s->bunq || !s->xq || !s->xqh) {
      RkSetErrno(RK_ERRNO_ENOMEM);
      freeBunStorage(s);
      return (struct nstore *)0;
    }
    s->yomi += OVERRUN_MARGIN;
    s->bunq += OVERRUN_MARGIN;
    s->xq += OVERRUN_MARGIN;
    s->xqh += OVERRUN_MARGIN;
    p = (WCHAR_T*)&s->yomi[0];
    q = (WCHAR_T*)&s->yomi[s->maxyomi+1];
    for (i = 0; pat = (WCHAR_T)~i, i < OVERRUN_MARGIN; i++)
      p[-i-1] = q[i] = pat;
    p = (WCHAR_T*)&s->bunq[0];
    q = (WCHAR_T*)&s->bunq[s->maxbunq+1];
    for (i = 0; pat = (WCHAR_T)~i, i < OVERRUN_MARGIN; i++)
      p[-i-1] = q[i] = pat;
    p = (WCHAR_T*)&s->xq[0];
    q = (WCHAR_T*)&s->xq[s->maxxq+1];
    for (i = 0; pat = (WCHAR_T)~i, i < OVERRUN_MARGIN; i++)
      p[-i-1] = q[i] = pat;
    p = (WCHAR_T*)&s->xqh[0];
    q = (WCHAR_T*)&s->xqh[s->maxxq+1];
    for (i = 0; pat = (WCHAR_T)~i, i < OVERRUN_MARGIN; i++)
      p[-i-1] = q[i] = pat;
    s->word_in_use = 0;
  };
  if (!s) /* EMPTY */RkSetErrno(RK_ERRNO_ENOMEM);
  return s;
}

struct nstore	*
_RkReallocBunStorage(struct nstore *src, unsigned len)
{
  struct nstore	*dst = allocBunStorage(len);

  if (dst) {
    int		i;
    
    if (src->yomi) {
      for (i = 0; i <= (int)src->maxyomi; i++)
	dst->yomi[i] = src->yomi[i];
      free((src->yomi-OVERRUN_MARGIN));
    };
    dst->nyomi = src->nyomi;
    if (src->bunq) {
      for (i = 0; i <= (int)src->maxbun; i++) 
	dst->bunq[i] = src->bunq[i];
      free((src->bunq-OVERRUN_MARGIN));
    };
    dst->maxbun = src->maxbun;
    dst->curbun = src->curbun;
    if (src->xq) {
      for (i = 0; i <= src->maxxq; i++)
	dst->xq[i] = src->xq[i];
      free((src->xq-OVERRUN_MARGIN));
    };
    if (src->xqh) {
      for (i = 0; i <= src->maxxq; i++)
	dst->xqh[i] = src->xqh[i];
      free((src->xqh-OVERRUN_MARGIN));
    };
    dst->word_in_use = src->word_in_use;
    free(src);
    return(dst);
  }
  return((struct nstore *)0);
}

inline struct nbun *
getCurrentBun(struct nstore *store)
{
  if (store && 0 <= store->curbun && store->curbun < (int)store->maxbun) 
    return &store->bunq[store->curbun];
  return (struct nbun *)0;
}

/* RkBgnBun
 *	renbunsetu henkan wo kaishi surutameno shokisettei wo okonau
 * reuturns:
 *	# >=0	shoki bunsetsu no kosuu
 *	-1	shoki ka sippai
 *		RK_ERRNO_ECTXNO
 *		RK_ERRNO_EINVAL
 *		RK_ERRNO_ENOMEM
 */
int
RkwBgnBun(int cx_num, WCHAR_T *yomi, int n, int kouhomode)
{
  struct RkContext	*cx;
  unsigned long		mask1, mask2;
  int			asset = 0;

  if (!(cx = RkGetContext(cx_num))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return(-1);
  }
  if (IS_XFERCTX(cx)) {
    RkSetErrno(0);
    return(-1);
  }
  for (mask1 = (unsigned long) kouhomode, mask2 = 0L;
                           mask1; mask1 >>= RK_XFERBITS) {
    if ((mask1 & (unsigned long)RK_XFERMASK) == (unsigned long)RK_CTRLHENKAN) {
      mask1 >>= RK_XFERBITS;
      asset = 1;
      break;
    }
    mask2 = (mask2 << RK_XFERBITS) | ((unsigned long)RK_XFERMASK);
  }
  if (!(cx->store = allocBunStorage((unsigned)n))) {
    RkSetErrno(RK_ERRNO_ENOMEM);
    return(-1);
  }
  cx->flags |= (unsigned)CTX_XFER;
  cx->concmode = (RK_CONNECT_WORD
		  | (asset ? ((int)mask1 & ~RK_TANBUN) : 
		     (RK_MAKE_KANSUUJI | RK_MAKE_WORD | RK_MAKE_EISUUJI)));
  cx->kouhomode = ((unsigned long)kouhomode) & mask2;
  if (yomi) {
    int	i;
	
    if (n <= 0) {
      RkSetErrno(RK_ERRNO_EINVAL);
      RkwEndBun(cx_num, 0);
      return(-1);
    };
    for (i = 0; i < n; i++)
      cx->store->yomi[i] = yomi[i];
    cx->store->yomi[n] = 0;
    cx->store->nyomi = n;
    cx->store->bunq[0].nb_yoff = 0;
    i = _RkRenbun2(cx, mask1 & RK_TANBUN ? n : 0);
    return(i);
  } else {
    cx->concmode |= RK_MAKE_WORD;
    cx->flags |= (unsigned) CTX_XAUT;
    if (n < 0) {
      RkSetErrno(RK_ERRNO_EINVAL);
      RkwEndBun(cx_num, 0);
      return(-1);
    };
    return(0);
  }
}

/* RkEndBun
 *	bunsetsu henkan wo shuuryou suru
 *	hituyou ni oujite, henkan kekka wo motoni gakushuu wo okonau 
 *
 *	return	0
 *		-1(RK_ERRNO_ECTX)
 */
int
RkwEndBun(int cx_num, int mode)
{
struct RkContext	*cx;
struct nstore		*store;    
int					i;
const int			DO_LEARN = 1;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store)) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return(-1);
  }
  if (mode) {
    if (mode != DO_LEARN) {
      RkSetErrno(RK_ERRNO_EINVAL);
      return -1;
    };
  }
  for (i = 0; i < (int)store->maxbun; i++)
    (void)_RkLearnBun(cx, i, mode);
  if (cx->flags & CTX_XAUT)
    _RkFreeQue(store, 0, store->maxxq + 1);
  cx->concmode &= ~(RK_CONNECT_WORD | RK_MAKE_WORD |
		    RK_MAKE_KANSUUJI | RK_MAKE_EISUUJI);
  _RkEndBun(cx);
  freeBunStorage(store);
  return(0);
}

/* RkRemoveBun
 *	current bunsetu made wo sakujo suru
 *	current bunsetu ha 0 ni naru.
 */

int
RkwRemoveBun(int cx_num, int mode)
{
  struct RkContext	*cx;
  struct nstore		*store;
  int			i, c;

  if (!(cx = RkGetXContext(cx_num))
      || !(store = cx->store)
      || !IS_XFERCTX(cx)
      || store->maxbun <= 0) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return -1;
  }
  for (i = 0; i <= store->curbun; i++)
    _RkLearnBun(cx, i, mode);
  c = store->bunq[store->curbun + 1].nb_yoff;
  for (i = store->curbun + 1; i <= (int)store->maxbun; i++) {
    store->bunq[i - store->curbun - 1] = store->bunq[i];
    store->bunq[i - store->curbun - 1].nb_yoff -= c;
  }
  store->nyomi -= c;
  usncopy(store->yomi, store->yomi + c, (unsigned)store->nyomi);
  store->maxbun -= store->curbun + 1;
  store->curbun = 0;
  return(store->maxbun);
}

/* RkSubstYomi
 *	change the contents of hiragana buffer
 * returns:
 *	# bunsetu
 */

int 
RkwSubstYomi(int cx_num, int ys, int ye, WCHAR_T *yomi, int newLen)
{
  struct RkContext	*cx;
  struct nstore	*store;
  struct nbun	*bun;
  
  if (!(cx = RkGetContext(cx_num))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return(-1);
  }
  if (!(store = cx->store) ||
      !(bun = &store->bunq[store->maxbun]) ||
      !IS_XFERCTX(cx) ||
      !IS_XAUTCTX(cx) ||
      !(0 <= ys && ys <= ye && ye <= (int)(store->nyomi - bun->nb_yoff)) ||
      (newLen < 0)) {
    RkSetErrno(RK_ERRNO_EINVAL);
    return -1;
  }
  return _RkSubstYomi(cx, ys, ye, yomi, newLen);
}

/* RkFlushYomi
 *	force to convert the remaining hiragana
 * returns:
 *	# bunsetu
 */

int
RkwFlushYomi(int cx_num)
{
  struct RkContext	*cx;
  if (!(cx = RkGetContext(cx_num)) ||
      !IS_XFERCTX(cx) ||
      !IS_XAUTCTX(cx)) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return(-1);
  }
  return(_RkFlushYomi(cx));
}

/* RkResize/RkEnlarge/RkShorten
 *	current bunsetsu no ookisa wo henkou
 */
int
_RkResize(int cx_num, int len, int t)
{
  struct RkContext	*cx;
  struct nbun		*bun;
  struct nstore		*store;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store) ||
      !(bun = getCurrentBun(store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return(-1);
  }
  if (t)
    len = HowManyChars(store->yomi + store->bunq[store->curbun].nb_yoff, len);
  if (0 < len && (unsigned)(bun->nb_yoff + len) <= store->nyomi) {
    bun->nb_flags |= RK_REARRANGED;
    return(_RkRenbun2(cx, len));
  }
  return(store->maxbun);
}

int 
RkwResize(int cx_num, int len)
{
  return(_RkResize(cx_num, len, 0));
}

int
RkeResize(int cx_num, int len)
{
  return(_RkResize(cx_num, len, 1));
}

int
RkwEnlarge(int cx_num)
{
  struct RkContext	*cx;
  struct nstore		*store;
  struct nbun		*bun;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store)||
      !(bun = getCurrentBun(store))) {
    RkSetErrno(RK_ERRNO_ENOMEM);
    return(0);
  }
  if (store->nyomi > (unsigned)(bun->nb_yoff + bun->nb_curlen) &&
	   store->yomi[bun->nb_yoff + bun->nb_curlen]) {
    bun->nb_flags |= RK_REARRANGED;
    return(_RkRenbun2(cx, (int)(bun->nb_curlen + 1)));
  }
  return(store->maxbun);
}

int
RkwShorten(int cx_num)
{
  struct RkContext	*cx;
  struct nstore		*store;
  struct nbun		*bun;
    
  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store) ||
      !(bun = getCurrentBun(store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return 0;
  }
  if (bun->nb_curlen > 1) {
    bun->nb_flags |= RK_REARRANGED;
    return(_RkRenbun2(cx, (int)(bun->nb_curlen - 1)));
  }
  return(store->maxbun);
}

/* RkStoreYomi
 *	current bunsetu no yomi wo sitei sareta mono to okikaeru
 *	okikaeta noti, saihen kan suru
 */

int
RkwStoreYomi(int cx_num, WCHAR_T *yomi, int nlen)
{
  unsigned		nmax, omax, cp;
  WCHAR_T			*s, *d, *e;
  int			i, olen, diff;
  struct RkContext	*cx;
  struct nstore		*store;
  struct nbun		*bun;
    
  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store) ||
      !(bun = getCurrentBun(store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return -1;
  }
  if (!yomi || nlen < 0 || uslen(yomi) < nlen) {
    RkSetErrno(RK_ERRNO_EINVAL);
    return -1;
  }
  nmax = store->nyomi + (diff = nlen - (olen = bun->nb_curlen));
  omax = store->nyomi;
  /* nobiru */
  if (nlen > olen) {
    if (!(store = _RkReallocBunStorage(store, store->maxyomi + diff))) {
      RkSetErrno(RK_ERRNO_ENOMEM);
      return -1;
    }
    cx->store = store;
    bun = getCurrentBun(store);
    /* shift yomi */
    s = store->yomi + omax;
    d = store->yomi + nmax;
    e = store->yomi + bun->nb_yoff + olen;
    while (s > e)
      *--d = *--s;
  } else if (nlen < olen) {	/* chizimu */
    s = store->yomi + bun->nb_yoff + olen;
    d = store->yomi + bun->nb_yoff + nlen;
    e = store->yomi + omax;
    while (s < e)
      *d++ = *s++;
  }
  store->yomi[nmax] = (WCHAR_T)0;
  store->nyomi = nmax;
  for (i = store->curbun + 1; i <= (int)store->maxbun; i++) 
    store->bunq[i].nb_yoff += diff;
  cp = store->curbun;
  if (!nlen) {
    _RkFreeBunq(store);
    for (i = store->curbun; i < (int)store->maxbun; i++)
      store->bunq[i] = store->bunq[i + 1];
    store->maxbun--;
    cp = store->curbun;
    if (cp >= store->maxbun && cp > 0)
      cp -= 1;
  } else
    usncopy((store->yomi + bun->nb_yoff), yomi, (unsigned)nlen);
  if ((i = _RkRenbun2(cx, 0)) != -1)
    store->curbun = cp;
  return(i);
}

/* RkGoTo/RkLeft/RkRight
 * 	current bunsetu no idou
 */

int
RkwGoTo(int cx_num, int bnum)
{
  struct RkContext	*cx;
  struct nstore	*store;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store) ||
      !store->maxbun) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return 0;
  }
  if ((0 <= bnum) && (bnum < (int)store->maxbun))
    store->curbun = bnum;
  return(store->curbun);
}

int
RkwLeft(int cx_num)
{
  struct RkContext	*cx;
  struct nstore	*store;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store) ||
      !store->maxbun) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return 0;
  }
  if (--store->curbun < 0)
    store->curbun = store->maxbun - 1;
  return store->curbun;
}

int
RkwRight(int cx_num)
{
  struct RkContext	*cx;
  struct nstore	*store;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store) ||
      !store->maxbun) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return 0;
  }
  if (++store->curbun >= (int)store->maxbun)
    store->curbun = 0;
  return(store->curbun);
}

/* RkXfer/RkNfer/RkNext/RkPrev
 *	current kouho wo henkou
 */
static int
countCand(struct RkContext *cx)
{
  struct nbun		*bun;
  int			maxcand = 0;
  unsigned long		mask;
  
  bun = getCurrentBun(cx->store);
  if (bun) {
    maxcand = bun->nb_maxcand;
    for (mask = cx->kouhomode; mask; mask >>= RK_XFERBITS)
      maxcand++;
  };
  return(maxcand);
}

static int
getXFER(struct RkContext *cx, int cnum)
{
  struct nbun	*bun = getCurrentBun(cx->store);

  cnum -= ((int)bun->nb_maxcand);
  return(cnum < 0 ? RK_NFER : (cx->kouhomode>>(RK_XFERBITS*cnum))&RK_XFERMASK);
}

int
RkwXfer(int cx_num, int knum)
{
  struct RkContext	*cx;
  struct nbun		*bun;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(cx->store) ||
      !(bun = getCurrentBun(cx->store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return 0;
  }
  if (0 <= knum && knum < countCand(cx))
    bun->nb_curcand = knum;
  return(bun->nb_curcand);
}

int
RkwNfer(int cx_num)
{
  struct RkContext	*cx;
  struct nbun	*bun;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(cx->store) ||
      !(bun = getCurrentBun(cx->store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return(0);
  }
  return(bun->nb_curcand = bun->nb_maxcand);
}

int RkwNext (int);

int
RkwNext(int cx_num)
{
  struct RkContext	*cx;
  struct nbun	*bun;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(cx->store) ||
      !(bun = getCurrentBun(cx->store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return(0);
  }
  if (++bun->nb_curcand >= (WCHAR_T)countCand(cx))
    bun->nb_curcand = 0;
  return(bun->nb_curcand);
}

int
RkwPrev(int cx_num)
{
  struct RkContext	*cx;
  struct nbun		*bun;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(cx->store) ||
      !(bun = getCurrentBun(cx->store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return(0);
  }
  
  if (!bun->nb_curcand)
    bun->nb_curcand = countCand(cx);
  return(--bun->nb_curcand);
}

/* findBranch
 * 	shiteisareta kouho wo fukumu path wo motomeru
 */
static struct nword *
findBranch(struct nstore *store, int cnum)
{
  struct nbun		*bun;
  struct nword		*w;
  
  if (!(bun = getCurrentBun(store)) ||
      (0 > cnum) ||
      (cnum >= (int)bun->nb_maxcand))
    return((struct nword *)0);
  for (w = bun->nb_cand; w; w = w->nw_next) {
    if (CanSplitWord(w) && bun->nb_curlen == w->nw_ylen) {
      if (cnum-- <= 0)
	return(w);
    }
  }
  return((struct nword *)0);
}

/* RkGetStat
 */
int
RkwGetStat(int cx_num, RkStat *st)
{
  struct RkContext	*cx;
  struct nstore		*store;
  struct nbun		*bun;
  struct nword		*cw, *lw;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store) ||
      !(bun = getCurrentBun(store)) ||
      !st) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return -1;
  }
  /* set up void values */
  st->bunnum = st->candnum = st->maxcand =
    st->diccand = st->ylen = st->klen = st->tlen = 0;
  st->bunnum  = store->curbun;
  st->candnum = bun->nb_curcand;
  
  st->maxcand = countCand(cx);
  st->diccand = bun->nb_maxcand;
  st->ylen    = bun->nb_curlen;
  st->klen    = bun->nb_curlen;
  st->tlen    = 1;
  /* look up the word node containing the current candidate */
  cw = findBranch(store, (int)bun->nb_curcand);
  if (cw) {
    st->klen = st->tlen = 0;
    for (; cw; cw = cw->nw_left) {
      if (!(lw = cw->nw_left))
	break;
      if (cw->nw_klen == lw->nw_klen)
	st->klen += (cw->nw_ylen - lw->nw_ylen);
      else
	st->klen += (cw->nw_klen - lw->nw_klen);
      st->tlen++;
    }
  } else {
    WCHAR_T	*yomi = store->yomi + bun->nb_yoff;
    switch(getXFER(cx, (int)bun->nb_curcand)) {
    default:
    case RK_XFER:
      st->klen = RkwCvtHira((WCHAR_T *)0, 0, yomi, st->ylen);
      break;
    case RK_KFER:
      st->klen = RkwCvtKana((WCHAR_T *)0, 0, yomi, st->ylen);
      break;
    case RK_HFER:
      st->klen = RkwCvtHan((WCHAR_T *)0, 0, yomi, st->ylen);
      break;
    case RK_ZFER:
      st->klen = RkwCvtZen((WCHAR_T *)0, 0, yomi, st->ylen);
      break;
    }
  }
  return 0;
}

/* RkGetStat
 */
int
RkeGetStat(int cx_num, RkStat *st)
{
  struct RkContext *cx;
  struct nstore *store;
  WCHAR_T *yomi;
  int res, klen;
  WCHAR_T *kanji = (WCHAR_T *)malloc(sizeof(WCHAR_T) * (RK_LEN_WMAX + 1));
  if (!kanji) {
    return -1;
  }
  
  if (!(cx = RkGetXContext(cx_num)) || !(store = cx->store)) {
    RkSetErrno(RK_MSG_ECTXNO);
    res = -1;
    goto return_res;
  }

  yomi = store->yomi + store->bunq[store->curbun].nb_yoff;
  klen = RkwGetKanji(cx_num, kanji, RK_LEN_WMAX + 1);
  res = RkwGetStat(cx_num, st);
  if (res < 0 || klen != st->klen) {
    res = -1;
    goto return_res;
  }
  if (st) {
    st->ylen = HowManyBytes(yomi, st->ylen);
    st->klen = HowManyBytes(kanji, klen);
  }
 return_res:
  free(kanji);
  return res;
}

static int
addIt(struct nword * cw, WCHAR_T * key, int (*proc) (...), WCHAR_T * dst, int ind, int maxdst, unsigned long mode, RkContext * cx)
{
	struct nword       *lw;
	WCHAR_T              *y;
	RkLex               lex;

	lw = cw->nw_left;
	if (lw) {
		ind = addIt(lw, key, proc, dst, ind, maxdst, mode, cx);
		y = key + lw->nw_ylen;
		lex.ylen = cw->nw_ylen - lw->nw_ylen;
		lex.klen = cw->nw_klen - lw->nw_klen;
		lex.rownum = cw->nw_rowcol;
#ifdef NEED_DEF
		lex.colnum = cw->nw_rowcol;
#endif
		lex.dicnum = cw->nw_class;
		ind = (*proc) (dst, ind, maxdst, y, _RkGetKanji(cw, y, mode), &lex, cx);
	}
	return ind;
}


inline int
getIt(RkContext * cx, int cnum, int (*proc) (...), WCHAR_T * dst, int max)
{
	struct nstore      *store = cx->store;
	struct nbun        *bun;
	struct nword       *w;

	if (!(bun = getCurrentBun(store)) ||
	    !(w = findBranch(store, cnum)))
		return (-1);
	return addIt(w, store->yomi + bun->nb_yoff, proc, dst, 0, max,
		     cx->concmode, cx);
}

/*ARGSUSED*/
inline int
addYomi(WCHAR_T *dst, int ind, int max, WCHAR_T *yomi, WCHAR_T *kanji, RkLex *lex)
{
  int		ylen;
    
  ylen = lex->ylen;
  while (ylen--) {
    if (ind < max) {
      if (dst)
	dst[ind] = *yomi++;
      ind++;
    }
  }
  return ind;
}
/* RkGetYomi
 *	current bunsetu no yomi wo toru
 */

int RkwGetYomi (int, WCHAR_T *, int);

int
RkwGetYomi(int cx_num, WCHAR_T *yomi, int maxyomi)
{
  struct RkContext	*cx;
  struct nbun	*bun;
  RkLex	lex;
  int		i;
  struct nstore	*store;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store) ||
      !(bun = getCurrentBun(store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return -1;
  };
  if (!yomi) {
    RkSetErrno(RK_ERRNO_EINVAL);
    return -1;
  };
  lex.ylen = bun->nb_curlen;
  i = addYomi(yomi, 0, maxyomi - 1,
	      store->yomi + bun->nb_yoff,
	      store->yomi+bun->nb_yoff,
	      &lex);
  if (yomi && i < maxyomi)
    yomi[i] = (WCHAR_T)0;
  return i;
}

int
RkwGetLastYomi(int cx_num, WCHAR_T *yomi, int maxyomi)
{
  struct RkContext	*cx;
  struct nbun	*bun;
  struct nstore	*store;
  int		nyomi;
    
  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store) ||
      !(bun = &store->bunq[store->maxbun])) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return -1;
  }
  if (!(cx->flags & CTX_XAUT) || maxyomi < 0) {
    RkSetErrno(RK_ERRNO_EINVAL);
    return -1;
  }
  nyomi = store->nyomi - bun->nb_yoff;
  if (yomi) {
    usncopy(yomi, store->yomi + bun->nb_yoff, (unsigned)(maxyomi));
    if (nyomi + 1 < maxyomi) {
      yomi[nyomi] = (WCHAR_T)0;
    } else {
      yomi[maxyomi - 1] = (WCHAR_T)0;
    };
  }
  return nyomi;
}

/*ARGSUSED*/
static int
addKanji(WCHAR_T *dst, int ind, int max, WCHAR_T *yomi, WCHAR_T *kanji, RkLex *lex, struct RkContext *cx) /* ARGSUSED */
{
  int		klen;
  
  klen = lex->klen;
  while (klen-- > 0) {
    if (ind < max) {
      if (dst)
	dst[ind] = *kanji++;
      ind++;
    }
  }
  return ind;
}

static int
getKanji(struct RkContext *cx, int cnum, WCHAR_T *dst, int maxdst)
{
  struct nbun	*bun = getCurrentBun(cx->store);
  WCHAR_T		*yomi;
  int		i, ylen;

  i = getIt(cx, cnum, (int(*)(...))addKanji, dst, maxdst - 1);
  if (i < 0) {
    yomi = cx->store->yomi + bun->nb_yoff;
    ylen = bun->nb_curlen;
    switch(getXFER(cx, cnum)) {
    default:
    case RK_XFER:
      i = RkwCvtHira(dst, maxdst, yomi, ylen); break;
    case RK_KFER:
      i = RkwCvtKana(dst, maxdst, yomi, ylen); break;
    case RK_HFER:
      i = RkwCvtHan(dst, maxdst, yomi, ylen); break;
    case RK_ZFER:
      i = RkwCvtZen(dst, maxdst, yomi, ylen); break;
    }
  }
  if (dst && i < maxdst)
    dst[i] = (WCHAR_T)0;
  return i;
}

/* RkGetKanji
 *	current bunsetu no kanji tuduri wo toru
 */

int
RkwGetKanji(int cx_num, WCHAR_T *dst, int maxdst)
{
  RkContext	*cx;
  struct nbun	*bun;
  int		i;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(cx->store) ||
      !(bun = getCurrentBun(cx->store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return -1;
  }
  
  i = getKanji(cx, (int)bun->nb_curcand, dst, maxdst);
  if (dst && i < maxdst)
    dst[i] = 0;
  return i;
}

/* RkGetKanjiList
 * 	genzai sentaku sareta kouho mojiretu wo toridasu
 */
int
RkwGetKanjiList(int cx_num, WCHAR_T *dst, int maxdst)
{
  struct RkContext	*cx;
  int			i, len, ind = 0, num = 0;
  int			maxcand;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(cx->store)) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return -1;
  }
  maxcand = countCand(cx);
  for (i = 0; i < maxcand; i++) {
    if (dst)
      len = getKanji(cx, i, dst + ind, maxdst - ind - 1);
    else
      len = getKanji(cx, i, dst, maxdst - ind - 1);
    if (0 < len && ind + len + 1 < maxdst - 1) {
      if (dst)
	dst[ind + len] = (WCHAR_T)0;
      ind += len + 1;
      num++;
    }
  }
  if (dst && ind < maxdst)
    dst[ind] = (WCHAR_T)0;
  return num;
}

/* RkGetLex
 *	current bunsetu no hishi jouhou wo toru
 */
/*ARGSUSED*/
static int
addLex(RkLex *dst, int ind, int max, WCHAR_T *yomi, WCHAR_T *kanji, RkLex *lex, struct RkContext *cx) /* ARGSUSED */
{
  if (ind + 1 <= max) {
    if (dst)
      dst[ind] = *lex;
    ind++;
  }
  return ind;
}

int
RkwGetLex(int cx_num, RkLex *dst, int maxdst)
{
  RkContext	*cx;
  struct nbun	*bun;
  int	i;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(cx->store) ||
      !(bun = getCurrentBun(cx->store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return -1;
  }
  i = getIt(cx, (int)bun->nb_curcand, (int(*)(...))addLex, (WCHAR_T *)dst, maxdst - 1);
  if (i < 0) {
    if (dst && 1 < maxdst) {
      dst[0].ylen = bun->nb_curlen;
      dst[0].klen = bun->nb_curlen;
      dst[0].rownum  = cx->gram->P_BB; /* Ê¸Àá */
      dst[0].colnum  = cx->gram->P_BB; /* Ê¸Àá */
      dst[0].dicnum  = ND_EMP;
    }
    i = 1;
  }
  return i;
}

/* RkeGetLex -- ¤Û¤Ü RkwGetLex ¤ÈÆ±¤¸¤À¤¬Ä¹¤µ¤Ï¥Ð¥¤¥ÈÄ¹¤ÇÊÖ¤ë */

int
RkeGetLex(int cx_num, RkLex *dst, int maxdst)
{
  struct RkContext *cx;
  struct nstore *store;
  WCHAR_T *yomi, *kp;
  int nwords, i;
  WCHAR_T *kanji = (WCHAR_T *)malloc(sizeof(WCHAR_T) * (RK_LEN_WMAX + 1));
  if (!kanji) {
    return -1;
  }

  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store)) {
    RkSetErrno(RK_MSG_ECTXNO);
    nwords = -1;
    goto return_nwords;
  }

  yomi = store->yomi + store->bunq[store->curbun].nb_yoff;
  (void)RkwGetKanji(cx_num, kanji, RK_LEN_WMAX + 1);
  kp = kanji;
  nwords = RkwGetLex(cx_num, dst, maxdst);
  if (dst) {
    for (i = 0; i < nwords; i++) {
      int tmp;
      tmp = dst[i].ylen;
      dst[i].ylen = HowManyBytes(yomi, tmp); yomi += tmp;
      tmp = dst[i].klen;
      dst[i].klen = HowManyBytes(kp, tmp); kp += tmp;
    }
  }
 return_nwords:
  free(kanji);
  return nwords;
}

/*ARGSUSED*/
static int
addHinshi(WCHAR_T *dst, int ind, int max, WCHAR_T *yomi, WCHAR_T *kanji, RkLex *lex, struct RkContext *cx)
{
  int	bytes;
  WCHAR_T	*p;
#ifndef USE_MALLOC_FOR_BIG_ARRAY
  WCHAR_T	hinshi[256];
#else
  WCHAR_T *hinshi = (WCHAR_T *)malloc(sizeof(WCHAR_T) * 256);
  if (!hinshi) {
    return ind;
  }
#endif

  if (cx) {
    p = RkUparseGramNum(cx->gram->gramdic, lex->rownum, hinshi, 256);
    if (p) {
      bytes = p - hinshi;
      if (ind + bytes  < max) {
	if (dst)
	  usncopy(dst + ind, hinshi, bytes);
	ind += bytes;
      }
    }
  }
#ifdef USE_MALLOC_FOR_BIG_ARRAY
  free(hinshi);
#endif
  return ind;
}

/* RkGetHinshi
 *	current bunsetu no hinshi mojiretu wo toru
 */
int
RkwGetHinshi(int cx_num, WCHAR_T *dst, int maxdst)
{
  struct RkContext	*cx;
  struct nbun		*bun;
  int			i;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(cx->store) ||
      !(bun = getCurrentBun(cx->store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return(-1);
  }
  i = getIt(cx, (int)bun->nb_curcand, (int(*)(...))addHinshi, dst, maxdst - 1);
  if (i < 0) {
    if (dst && 1 < maxdst) 
      dst[0] = (WCHAR_T)0;
    i = 1;
  }
  return(i);
}

#include <sys/types.h>
#include <sys/stat.h>

#define	CloseContext(a)	{if ((a) != cx_num) RkwCloseContext(a);}

int
RkwQueryDic(int cx_num, char *dirname, char *dicname, struct DicInfo *status)
{
  struct RkContext	*cx;
  int			new_cx_num, size;
  unsigned char		*buff;
  char			*file;
  struct DM		*dm;
  struct DF		*df;
  struct stat		st;
    
  if (!(cx = RkGetContext(cx_num))
      || !status || !dirname || !dicname || !dicname[0])
    return(-1);
  size = strlen(dicname) + 1;
  if (!(buff = (unsigned char *)malloc(size)))
    return(-1);
  (void)strcpy((char *)buff, dicname);
  if (*dirname && strcmp(dirname, cx->ddpath[0]->dd_name)
      && strcmp(dirname, (char *)SYSTEM_DDHOME_NAME)) {
    if((new_cx_num = RkwCreateContext()) < 0) {
      free(buff);
      return BADCONT;
    }
    if(RkwSetDicPath(new_cx_num, dirname) < 0) {
      CloseContext(new_cx_num);
      free(buff);
      return NOTALC;
    }
    if (!(cx = RkGetContext(new_cx_num))) {
      free(buff);
      return(-1);
    }
  } else {
    if (!strcmp(dirname, (char *)SYSTEM_DDHOME_NAME))
      dirname = SYSTEM_DDHOME_NAME;
    else
      dirname = cx->ddpath[0]->dd_name;
    new_cx_num = cx_num;
  }
  if (!strcmp(dirname, (char *)SYSTEM_DDHOME_NAME)) {
    if (!(dm = _RkSearchDDP(cx->ddpath, dicname))) {
      CloseContext(new_cx_num);
      free(buff);
      return NOENT;
    }
  } else {
    if (!(dm = _RkSearchUDDP(cx->ddpath, (unsigned char *)dicname))) {
      CloseContext(new_cx_num);
      free(buff);
      return NOENT;
    }
  }
  df = dm->dm_file;
  if (df) {
    file = _RkCreatePath(df->df_direct, df->df_link);
    if (file) {
      status->di_dic = (unsigned char *)dm->dm_nickname;
      status->di_file = (unsigned char *)df->df_link;
      status->di_form = (DM2TYPE(dm) == DF_TEMPDIC) ? RK_TXT : RK_BIN;
      status->di_kind = dm->dm_class;
      status->di_count = stat(file, &st) >= 0 ? st.st_size : 0;
      status->di_time = stat(file, &st) >= 0 ? st.st_ctime : 0;
      status->di_mode = 0;
      CloseContext(new_cx_num);
      free(file);
      free(buff);
      return(0);
    }
  }
  free(buff);
  CloseContext(new_cx_num);
  return(-1);
}

int
_RkwSync(struct RkContext *cx, char *dicname)
{
  struct DM	*dm, *qm;

  dm = _RkSearchDicWithFreq(cx->ddpath, dicname, &qm);  

  if (dm)
    return(DST_SYNC(cx, dm, qm));
  else 
    return (-1);
}


int
RkwSync(int cx_num, char *dicname)
{
  struct RkContext	*cx;
  int ret = -1;
  const off_t DL_SIZE = 1024;

  if (!(cx = RkGetContext(cx_num)))
    return (-1);

  if (!dicname || !*dicname) {
    int i, rv;
    char *p;
#ifndef USE_MALLOC_FOR_BIG_ARRAY
    char diclist[DL_SIZE];
#else
    char *diclist = (char *)malloc(DL_SIZE);
    if (!diclist) {
      return -1;
    }
#endif
    
    if (!(i = RkwGetDicList(cx_num, diclist, DL_SIZE))) {
      ret = 0;
    }
    else {
      if (i > 0) {
	for (p = diclist, rv = 0; *p;) {
	  rv += _RkwSync(cx, p);
	  p += strlen(p) + 1;
	}
	if (!rv) {
	  ret = 0;
	  goto return_ret;
	}
      }
      ret = -1;
    }
  return_ret:;
#ifdef USE_MALLOC_FOR_BIG_ARRAY
    free(diclist);
#endif
  } else {
    ret = _RkwSync(cx, dicname);
  }
  return ret;
}

/*ARGSUSED*/
int RkwGetSimpleKanji(int cxnum, char *dicname, WCHAR_T *yomi, int maxyomi, WCHAR_T *kanjis, int maxkanjis, char *hinshis, int maxhinshis)
{
  return -1;
}

/*ARGSUSED*/
int
RkwStoreRange(int cx_num, WCHAR_T *yomi, int maxyomi)
{
  return(0);
}

/*ARGSUSED*/
int
RkwSetLocale(int cx_num, unsigned char *locale)
{
  return(0);
}
