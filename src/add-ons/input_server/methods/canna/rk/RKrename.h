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

/* $Id: RKrename.h 10525 2004-12-23 21:23:50Z korli $ */

#define RkGetProtocolVersion	wRkGetProtocolVersion
#define RkGetServerName		wRkGetServerName
#define RkGetServerVersion	wRkGetServerVersion
#define RkwInitialize		wRkwInitialize
#define RkwFinalize		wRkwFinalize
#define RkwCreateContext	wRkwCreateContext
#define RkwDuplicateContext	wRkwDuplicateContext
#define RkwCloseContext		wRkwCloseContext
#define RkwSetDicPath		wRkwSetDicPath
#define RkwCreateDic		wRkwCreateDic
#define RkwGetDicList		wRkwGetDicList
#define RkwGetMountList		wRkwGetMountList
#define RkwMountDic		wRkwMountDic
#define RkwRemountDic		wRkwRemountDic
#define RkwUnmountDic		wRkwUnmountDic
#define RkwDefineDic		wRkwDefineDic
#define RkwDeleteDic		wRkwDeleteDic
#define RkwGetHinshi		wRkwGetHinshi
#define RkwGetKanji		wRkwGetKanji
#define RkwGetYomi		wRkwGetYomi
#define RkwGetLex		wRkwGetLex
#define RkwGetStat		wRkwGetStat
#define RkwGetKanjiList		wRkwGetKanjiList
#define RkwFlushYomi		wRkwFlushYomi
#define RkwGetLastYomi		wRkwGetLastYomi
#define RkwRemoveBun		wRkwRemoveBun
#define RkwSubstYomi		wRkwSubstYomi
#define RkwBgnBun		wRkwBgnBun
#define RkwEndBun		wRkwEndBun
#define RkwGoTo			wRkwGoTo
#define RkwLeft			wRkwLeft
#define RkwRight		wRkwRight
#define RkwNext			wRkwNext
#define RkwPrev			wRkwPrev
#define RkwNfer			wRkwNfer
#define RkwXfer			wRkwXfer
#define RkwResize		wRkwResize
#define RkwEnlarge		wRkwEnlarge
#define RkwShorten		wRkwShorten
#define RkwStoreYomi		wRkwStoreYomi
#define RkwSetAppName		wRkwSetAppName
