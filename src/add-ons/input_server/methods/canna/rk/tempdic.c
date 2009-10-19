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
	static char rcsid[]="$Id: tempdic.c 14875 2005-11-12 21:25:31Z bonefish $";
#endif
/*LINTLIBRARY*/

#include	"RKintern.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <bsd_mem.h>

#define	dm_td	dm_extdata.ptr

static void freeTD(struct TD *td);
static TD *newTD(void);
static TN *extendTD(struct TD *tdic, WCHAR_T key, struct TW *tw);
static int yomi_equal(Wrec *x, Wrec *y, int n);
static WCHAR_T nthKey(Wrec *w, int n);
static TN *defineTD(struct DM *dm, struct TD *tab, int n, struct TW *newTW, int nlen);
static int enterTD(struct DM *dm, struct TD *td, struct RkKxGram *gram, WCHAR_T *word);
static void shrinkTD(struct TD *td, WCHAR_T key);
static int deleteTD(struct DM *dm, struct TD **tab, int n, Wrec *newW);

static void
freeTD(struct TD *td)
{
	int	i;
	for (i = 0; i < (int)td->td_n; i++) {
		struct TN	*tn = &td->td_node[i];
		if (IsWordNode(tn)) {
			free(tn->tn_word->word);
			free(tn->tn_word);
		} else
		freeTD(tn->tn_tree);
	}
	if (td) {
		if (td->td_node)
			free(td->td_node);
		free(td);
	}
}

/* newTD: allocates a fresh node */
static TD *
newTD(void)
{
	struct TD	*td;
	
	td = (struct TD *)malloc(sizeof(struct TD));
	if (td) {
		td->td_n = 0;
		td->td_max = 1;
		if (!(td->td_node = (struct TN *)calloc(td->td_max, sizeof(struct TN)))) {
			freeTD(td);
			return((struct TD *)0);
		}
	}
	return(td);
}

/*
* INSERT
*/
static TN *
extendTD(struct TD *tdic, WCHAR_T key, struct TW *tw)
{
	int		i, j;
	struct TN	*tp;
	struct TW	*ntw;
	
	if (!(ntw = RkCopyWrec(tw)))
		return (struct TN *)0;
	tp = tdic->td_node;
	if (tdic->td_n >= tdic->td_max) {
	if (!(tp = (struct TN *)calloc(tdic->td_max + 1, sizeof(struct TN)))) {
		free(ntw->word);
		free(ntw);
		return (struct TN *)0;
	}
		for (i = 0; i < (int)tdic->td_n; i++)
			tp[i] = tdic->td_node[i];
		free(tdic->td_node);
		tdic->td_max++;
		tdic->td_node = tp;
	}
	for (i = 0; i < (int)tdic->td_n; i++)
		if (key < tp[i].tn_key)
			break;
	for (j = tdic->td_n; j > i; j--)
		tp[j] = tp[j - 1];
	tp[i].tn_flags = TN_WORD|TN_WDEF;
	tp[i].tn_key = key;
	tp[i].tn_word = ntw;
	tdic->td_n++;
	return(tp + i);
}

static
int yomi_equal(Wrec *x, Wrec *y, int n)
{
	int l;
	
	if ((l = (*y >> 1) & 0x3f) == ((*x >> 1) & 0x3f)) {
		if (*y & 0x80)
			y += 2;
		if (*x & 0x80)
			x += 2;
		x += (n << 1) + 2;
		y += (n << 1) + 2;
		for (; n < l ; n++) {
			if (*x++ != *y++ || *x++ != *y++)
				return(0);
		}
		return(1);
	}
	return(0);
}

static
WCHAR_T
nthKey(Wrec *w, int n)
{
if (n < (int)((*w >> 1) & 0x3f)) {
	if (*w & 0x80)
		w += 2;
	w += (n << 1) + 2;
	return((WCHAR_T)((w[0] << 8) | w[1]));
} else
	return((WCHAR_T)0);
}

/*
* defineTD -- ¥Æ¥­¥¹¥È¼­½ñ¤ËÄêµÁ¤¹¤ë
*
* °ú¿ô
*    dm    ¼­½ñ
*    tab   ¥Æ¥­¥¹¥È¼­½ñ¥Ý¥¤¥ó¥¿
*    n     ²¿Ê¸»úÌÜ¤«(Ã±°Ì:Ê¸»ú)
	*    newW  ÅÐÏ¿¤¹¤ë¥ï¡¼¥É¥ì¥³¡¼¥É
*    nlen  ÉÔÌÀ
*/

static TN *
defineTD(struct DM *dm, struct TD *tab, int n, struct TW *newTW, int nlen)
{
	int		i;
	WCHAR_T		key;
	struct TN	*tn;
	struct ncache	*cache;
	struct TW	*mergeTW, *oldTW;
	Wrec		*oldW, *newW = newTW->word;
	
	key = nthKey(newW, n); n++;
	tn = tab->td_node;
	for (i = 0; i < (int)tab->td_n && tn->tn_key <= key; i++, tn++) {
	if (key == tn->tn_key) {
	if (IsWordNode(tn)) {
		struct TD	*td;
		
		oldTW = tn->tn_word;
		oldW = oldTW->word;
		if (!key|| yomi_equal(newW, oldW, n)) {
			if ((cache = _RkFindCache(dm, (long)oldTW)) && cache->nc_count > 0)
				return((struct TN *)0);
			if (!(mergeTW = RkUnionWrec(newTW, oldTW)))
				return((struct TN *)0);
			tn->tn_word = mergeTW;
			free(oldW);
			free(oldTW);
			tn->tn_flags |= TN_WDEF;
			if (cache) {
				_RkRehashCache(cache, (long)tn->tn_word);
				cache->nc_word = mergeTW->word;
			}
			return tn;
		}
		if (!(td = newTD()))
			return((struct TN *)0);
		td->td_n = 1;
		key = nthKey(oldW, n);
		td->td_node[0].tn_key = key;
		td->td_node[0].tn_flags |= TN_WORD;
		td->td_node[0].tn_word = oldTW;
		tn->tn_flags &= ~TN_WORD;
		tn->tn_tree = td;
	}
		return defineTD(dm, tn->tn_tree, n, newTW, nlen);
	}
	}
	return extendTD(tab, key, newTW);
}

static
int enterTD(struct DM *dm, struct TD *td, struct RkKxGram *gram, WCHAR_T *word)
{
	struct TW	tw;
	int ret = -1;
#ifndef USE_MALLOC_FOR_BIG_ARRAY
	Wrec		wrec[RK_LINE_BMAX*10];
#else
	Wrec *wrec = (Wrec *)malloc(sizeof(Wrec) * RK_LINE_BMAX * 10);
	if (!wrec) {
		return ret;
	}
#endif
	
	if (word[0] == (WCHAR_T)'#') {
		ret = 1;
	}
else if (RkParseOWrec(gram, word, wrec, RK_LINE_BMAX * 10, tw.lucks)) {
	struct TN *tn;
	
	tw.word = wrec;
	tn = defineTD(dm, td, 0, &tw, RK_LINE_BMAX * 10);
	if (tn) {
		tn->tn_flags &= ~(TN_WDEF|TN_WDEL);
		ret = 0;
		} else {
		ret = -1;
	}
}
else {
ret = 1;
}
#ifdef USE_MALLOC_FOR_BIG_ARRAY
	free(wrec);
#endif
	return ret;
}

/*
* DELETE
*/
static void
shrinkTD(struct TD *td, WCHAR_T key)
{
	int		i;
	struct TN	*tn = td->td_node;
	
	for (i = 0; i < (int)td->td_n; i++) {
	if (key == tn[i].tn_key) {
		while (++i < (int)td->td_n) tn[i - 1] = tn[i];
		td->td_n--;
		break;
	}
	}
}

/*
* deleteTD -- ¥Æ¥­¥¹¥È¼­½ñ¤«¤éÃ±¸ìÄêµÁ¤ò¼è¤ê½ü¤¯
*
* °ú¿ô
*    dm    ¼­½ñ
*    tab   ¥Æ¥­¥¹¥È¼­½ñ
*    n     ²¿Ê¸»úÌÜ¤«
*    newW  ÄêµÁ¤¹¤ë¥ï¡¼¥É¥ì¥³¡¼¥É
*/
static int
deleteTD(struct DM *dm, struct TD **tab, int n, Wrec *newW)
{
	struct TD	*td = *tab;
	int		i;
	WCHAR_T		key;
	
	key = nthKey(newW, n); n ++;
	for (i = 0; i < (int)td->td_n; i++) {
		struct TN	*tn = &td->td_node[i];
		
		if (key == tn->tn_key ) {
		if (IsWordNode(tn)) {
			struct TW	*oldTW = tn->tn_word;
			Wrec		*oldW = oldTW->word;
			
			if (!key || yomi_equal(newW, oldW, n)) {
				struct ncache	*cache = _RkFindCache(dm, (long)oldTW);
				
				if (!cache || cache->nc_count <= 0) {
					struct TW	*subW, newTW;
					
					newTW.word = newW;
					subW = RkSubtractWrec(oldTW, &newTW);
					free(oldW);
					free(oldTW);
					if (subW) {
						tn->tn_word = subW;
						tn->tn_flags |= TN_WDEL;
						if (cache) {
							_RkRehashCache(cache, (long)subW);
							cache->nc_word = subW->word;
						}
						return(0);
						} else {
						if (cache)
							_RkPurgeCache(cache);
						shrinkTD(td, key);
					}
				}
			}
		} else
			if (deleteTD(dm, &tn->tn_tree, n, newW))
				shrinkTD(td, key);
				if (td->td_n <= 0) {
					free((td->td_node));
					free(td);
					*tab = (struct TD *)0;
					return(1);
				} else
			return(0);
		}
	}
	return(0);
}

/*
* OPEN
*/
/*ARGSUSED*/
int
_Rktopen(
	struct DM	*dm,
	char	*file,
	int	mode,
	struct RkKxGram	*gram)
{
	struct DF	*df = dm->dm_file;
	struct DD	*dd = df->df_direct;
	struct TD	*xdm;
	FILE		*f;
	long		offset = 0L;
	int		ecount = 0;
	int ret = -1;
	char *line = (char *)malloc(RK_LINE_BMAX * 10);
	WCHAR_T *wcline = (WCHAR_T *)malloc(sizeof(WCHAR_T) * RK_LINE_BMAX * 10);
	if (!line || !wcline) {
		if (line) free(line);
		if (wcline) free(wcline);
		return ret;
	}
	
	if (!df->df_rcount) {
		if (close(open(file, 0)))
			goto return_ret;
		df->df_flags |= DF_EXIST;
		dm->dm_flags |= DM_EXIST;
		if (!close(open(file, 2)))
			dm->dm_flags |= DM_WRITABLE;
	}
	if (!(dm->dm_flags & DM_EXIST))
		;
	else if (!(f = fopen(file, "r"))) {
		df->df_flags &= ~DF_EXIST;
		dm->dm_flags &= ~DM_EXIST;
	}
else if (!(xdm = newTD())) {
	fclose(f);
}
else {
while (fgets((char *)line, RK_LINE_BMAX*10, f))
{
	int		sz;
	
	offset += strlen(line);
	sz = RkCvtWide(wcline, RK_LINE_BMAX*10, line, strlen(line));
	if (sz < RK_LINE_BMAX*10 - 1) {
		if ( enterTD(dm, xdm, gram, wcline) < 0 )
			ecount++;
	} else
	ecount++;
}
(void)fclose(f);
dm->dm_offset = 0L;
df->df_size = offset;
if (ecount) {
	freeTD((struct TD *)xdm);
	dm->dm_td = (pointer)0;
	dm->dm_flags &= ~DM_EXIST;
	ret = -1;
	} else {
	dm->dm_td = (pointer)xdm;
	if (df->df_rcount++ == 0)
		dd->dd_rcount++;
	ret = 0;
};
}
return_ret:
	free(wcline);
	free(line);
	return ret;
}

/*
* CLOSE
*/
static int writeTD (struct TD *, struct RkKxGram *, int);

static int
writeTD(
	struct TD		*td,
	struct RkKxGram	*gram,
	int		fdes
)
{
	int	i, tmpres;
	int	ecount = 0;
	WCHAR_T *wcline = (WCHAR_T *)0;
	
	wcline = (WCHAR_T *)malloc(RK_WREC_BMAX * sizeof(WCHAR_T));
	if (wcline) {
	for (i = 0; i < (int)td->td_n; i++) {
		struct TN		*tn = &td->td_node[i];
		unsigned char	*line;
		WCHAR_T		*wc;
		int		sz;
		
		if (IsWordNode(tn)) {
			wc = _RkUparseWrec(gram, tn->tn_word->word, wcline,
				RK_LINE_BMAX * sizeof(WCHAR_T), tn->tn_word->lucks, 1);
			if (wc) {
				sz = RkCvtNarrow((char *)0, 9999, wcline, (int)(wc - wcline));
				if (sz > RK_LINE_BMAX
				&& !(wc = _RkUparseWrec(gram, tn->tn_word->word, wcline,
					RK_LINE_BMAX * sizeof(WCHAR_T), tn->tn_word->lucks, 0)))
					ecount++;
				else {
					line = (unsigned char *)malloc( RK_LINE_BMAX*3 );
					if(line) {
						sz = RkCvtNarrow((char *)line, RK_LINE_BMAX,
							wcline, (int)(wc - wcline));
						line[sz++] = '\n';
						line[sz] = 0;
						tmpres = write(fdes, line, sz);
						if (tmpres != sz)
							ecount++;
						free(line);
					} else
					ecount++;
				}
			} else
			ecount++;
		} else
		ecount += writeTD(tn->tn_tree, gram, fdes);
	}
		free(wcline);
		return(ecount);
	}
	return 0;
}

int
_Rktclose(
	struct DM	*dm,
	char	*file,
	struct RkKxGram	*gram)
{
	struct DF	*df = dm->dm_file;
	struct TD	*xdm = (struct TD *)dm->dm_td;
	int		ecount;
	int		fdes;
	char *backup, *header, *whattime;
	backup = (char *)malloc(RK_PATH_BMAX);
	header = (char *)malloc(RK_LINE_BMAX);
	whattime = (char *)malloc(RK_LINE_BMAX);
	if (!backup || !header || !whattime) {
		if (backup) free(backup);
		if (header) free(header);
		if (whattime) free(whattime);
		return 0;
	}
	
	_RkKillCache(dm);
	if (dm->dm_flags & DM_UPDATED) {
		char	*p = rindex(file, '/');
		
		if (p) {
			strcpy(backup, file);
			p++;
			backup[(int)(p - file)] = '#';
			strcpy(&backup[(int)(p-file) + 1], p);
		};
		
		fdes = creat(backup, (unsigned)0666);
		ecount = 0;
		if
		(fdes >= 0)
		{
			int	n;
			
			time_t	tloc;
			
			tloc = time(0);
			strcpy(whattime, ctime(&tloc));
			whattime[strlen(whattime)-1] = 0;
			n = sprintf(header, "#*DIC %s [%s]\n", dm->dm_nickname, whattime);
			//n = strlen(header);
			if
			(write(fdes, header, n) != n)
			{
				ecount++;
			}
			ecount += writeTD(xdm, gram, fdes);
			(void)close(fdes);
		} else
		ecount++;
		
		if (!ecount) {
			rename(backup, file);
		}
		dm->dm_flags &= ~DM_UPDATED;
	};
	freeTD((struct TD *)xdm);
	
	--df->df_rcount;
	free(backup);
	free(header);
	free(whattime);
	return 0;
}

int
_Rktsearch(
	struct RkContext	*cx,
	struct DM		*dm,
	WCHAR_T		*key,
	int		n,
	struct nread	*nread,
	int		maxcache,
	int		*cf)
{
	struct TD	*xdm = (struct TD *)dm->dm_td;
	int		nc = 0;
	int		i, j;
	
	*cf = 0;
	for (j = 0; j < n;) {
		WCHAR_T	k = uniqAlnum(key[j++]);
		struct TN	*tn;
		
		tn = xdm->td_node;
		for (i = 0; i < (int)xdm->td_n && tn->tn_key <= k; i++, tn++) {
		if (k == tn->tn_key) {
		if (IsWordNode(tn)) {
			Wrec	*w;
			int	l;
			
			w = tn->tn_word->word;
			l = (*w >> 1) & 0x3f;
			if (*w & 0x80)
				w += 2;
			w += 2;
			if (_RkEql(key, w, l)) {
				if (l > n)
					(*cf)++;
				else if (nc < maxcache) {
					nread[nc].cache = _RkReadCache(dm, (long)tn->tn_word);
					if (nread[nc].cache) {
						nread[nc].nk = l;
						nread[nc].csn = 0L;
						nread[nc].offset = 0L;
						nc++;
					} else
					(*cf)++;
				} else
				(*cf)++;
			}
			return nc;
			} else {
			struct TD	*ct = tn->tn_tree;
			struct TN	*n0 = &ct->td_node[0];
			
			if (ct->td_n && !n0->tn_key) {
				unsigned char	*w;
				int			l;
				w = n0->tn_word->word;
				l = (*w >> 1) & 0x3f;
				if (*w & 0x80)
					w += 2;
				w += 2;
				if (_RkEql(key, w, l)) {
					if (l > n)
						(*cf)++;
					else if (nc < maxcache) {
						nread[nc].cache = _RkReadCache(dm, (long)n0->tn_word);
						if (nread[nc].cache) {
							nread[nc].nk = l;
							nread[nc].csn = 0L;
							nread[nc].offset = 0L;
							nc++;
						} else
						(*cf)++;
					} else
					(*cf)++;
				}
			}
			xdm = ct;
			goto	cont;
		}
		}
		}
		break;
	cont:
		;
	}
	if (n <= 0)
		cx->poss_cont++;
	return nc;
}

/*
* IO
*/
/*ARGSUSED*/
int
_Rktio(
	struct DM		*dm,
	struct ncache	*cp,
	int		io)
{
if (io == 0) {
	cp->nc_word = ((struct TW *)cp->nc_address)->word;
	cp->nc_flags |= NC_NHEAP;
}
	return 0;
}

#define TEMPDIC_WRECSIZE 2048

/*
* CTL
*/
int
_Rktctl(
	struct DM	*dm,
	struct DM	*qm, /* no use : dummy*/
	int	what,
	WCHAR_T	*arg,
	struct RkKxGram	*gram)
	/* ARGSUSED */
	{
		struct TD	*xdm = (struct TD *)dm->dm_td;
		int		status = 0;
		struct TW	tw;
		unsigned long	lucks[2];
		Wrec *wrec = (Wrec *)malloc(sizeof(Wrec) * TEMPDIC_WRECSIZE);
		if (!wrec) {
			return status;
		}
		
		switch(what) {
		case DST_DoDefine:
			if ((dm->dm_flags & (DM_WRITABLE | DM_WRITEOK)) !=
			(DM_WRITABLE | DM_WRITEOK)) /* !(writable and write ok) */
				status = -1;
			else if (!RkParseOWrec(gram, arg, wrec, TEMPDIC_WRECSIZE, lucks))
				status = -1;
			else {
				tw.word = wrec;
				status = defineTD(dm, xdm, 0, &tw, TEMPDIC_WRECSIZE) ? 0 : -1;
				dm->dm_flags |= DM_UPDATED;
			}
			break;
		case DST_DoDelete:
			if ((dm->dm_flags & (DM_WRITABLE | DM_WRITEOK)) !=
			(DM_WRITABLE | DM_WRITEOK)) /* !(writable and write ok) */
				status = -1;
			else if (!RkParseOWrec(gram, arg, wrec, TEMPDIC_WRECSIZE, lucks))
				status = -1;
			else if (deleteTD(dm, &xdm, 0, wrec)) {
				xdm = newTD();
				dm->dm_td = (pointer)xdm;
			}
			dm->dm_flags |= DM_UPDATED;
			break;
		}
		free(wrec);
		return status;
	}

int
_Rktsync(
	struct RkContext *cx,
	struct DM	*dm,
	struct DM	 *qm)
	/* ARGSUSED */
	{
		struct RkKxGram  *gram = cx->gram->gramdic;
		struct DF	*df_p;
		struct DD     *dd_p;
		struct TD	*xdm = (struct TD *)dm->dm_td;
		int		ecount;
		char	        *file;
		int ret = -1;
		
		int		fdes;
		
		char *backup, *header, *whattime;
		backup = (char *)malloc(RK_PATH_BMAX);
		header = (char *)malloc(RK_LINE_BMAX);
		whattime = (char *)malloc(RK_LINE_BMAX);
		if (!backup || !header || !whattime) {
			if (backup) free(backup);
			if (header) free(header);
			if (whattime) free(whattime);
			return ret;
		}
		
		df_p = dm->dm_file;
		dd_p = df_p->df_direct;
		file  = _RkCreatePath(dd_p, df_p->df_link);
		if (file) {
		if (dm->dm_flags & DM_UPDATED) {
			char	*p = rindex(file, '/');
			
			if (p) {
				strcpy(backup, file);
				p++;
				backup[(int)(p - file)] = '#';
				strcpy(&backup[(int)(p-file) + 1], p);
			};
			
			fdes = creat(backup, (unsigned)0666);
			ecount = 0;
			if
			(fdes >= 0)
			{
				int	n;
				time_t	tloc;
				
				tloc = time(0);
				strcpy(whattime, ctime(&tloc));
				whattime[strlen(whattime)-1] = 0;
				n = sprintf(header, "#*DIC %s [%s]\n", dm->dm_nickname, whattime);
				//	n = strlen(header);
				if
				(write(fdes, header, n) != n)
				{
					ecount++;
				}
				ecount += writeTD(xdm, gram, fdes);
				(void)close(fdes);
			} else
			ecount++;
			
			if (!ecount) {
				rename(backup, file);
				dm->dm_flags &= ~DM_UPDATED;
				} else {
				free(file);
				ret = -1;
				goto return_ret;
			}
		};
			free(file);
			ret = 0;
		}
	return_ret:
		
		free(backup);
		free(header);
		free(whattime);
		
		return ret;
	}
