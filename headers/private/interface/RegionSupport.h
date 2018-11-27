/* $Xorg: region.h,v 1.4 2001/02/09 02:03:40 xorgcvs Exp $ */
/************************************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

************************************************************************/

#ifndef __REGION_SUPPORT_H
#define __REGION_SUPPORT_H

#include <Region.h>

class BRegion::Support {
 public:
	static	int					XUnionRegion(const BRegion* reg1,
									const BRegion* reg2, BRegion* newReg);

	static	int					XIntersectRegion(const BRegion* reg1,
									const BRegion* reg2, BRegion* newReg);

	static	int					XSubtractRegion(const BRegion* regM,
									const BRegion* regS, BRegion* regD);

	static	int					XXorRegion(const BRegion* sra,
									const BRegion* srb, BRegion* dr);

	static	bool				XPointInRegion(const BRegion* region,
									int x, int y);

	enum {
		RectangleOut = 0,
		RectanglePart = 1,
		RectangleIn = 2
	};

	static	int					XRectInRegion(const BRegion* region,
									const clipping_rect& rect);

 private:
	static	BRegion*			CreateRegion();
	static	void				DestroyRegion(BRegion* r);

	static	void				XOffsetRegion(BRegion* pRegion, int x, int y);

	static	void				miSetExtents(BRegion* pReg);
	static	int					miIntersectO(BRegion* pReg,
									clipping_rect* r1, clipping_rect* r1End,
									clipping_rect* r2, clipping_rect* r2End,
									int top, int bottom);
	static	void				miRegionCopy(BRegion* dstrgn, const BRegion* rgn);
	static	int					miCoalesce(BRegion* pReg,
									int prevStart, int curStart);
	static	int					miUnionNonO(BRegion* pReg, clipping_rect*	r,
									clipping_rect* rEnd, int top, int bottom);
	static	int					miUnionO(BRegion*	pReg,
									clipping_rect* r1, clipping_rect* r1End,
									clipping_rect* r2, clipping_rect* r2End,
									int	top, int bottom);
	static	int					miSubtractO(BRegion* pReg,
									clipping_rect* r1, clipping_rect* r1End,
									clipping_rect* r2, clipping_rect* r2End,
									int top, int bottom);
	static	int					miSubtractNonO1(BRegion* pReg,
									clipping_rect* r, clipping_rect* rEnd,
									int top, int bottom);



	typedef	int (*overlapProcp)(
		BRegion* pReg,
		clipping_rect* r1,
		clipping_rect* r1End,
		register clipping_rect* r2,
		clipping_rect* r2End,
		int top,
		int bottom);

	typedef int (*nonOverlapProcp)(
		BRegion* pReg,
		clipping_rect* r,
		clipping_rect* rEnd,
		int top,
		int bottom);

	static	void				miRegionOp(BRegion* newReg,
									const BRegion* reg1, const BRegion* reg2,
    int (*overlapFunc)(
        register BRegion*     pReg,
        register clipping_rect*     r1,
        clipping_rect*              r1End,
        register clipping_rect*     r2,
        clipping_rect*              r2End,
        int               top,
        int               bottom),
    int (*nonOverlap1Func)(
        register BRegion*     pReg,
        register clipping_rect*     r,
        clipping_rect*              rEnd,
        register int      top,
        register int      bottom),

    int (*nonOverlap2Func)(
        register BRegion*     pReg,
        register clipping_rect*     r,
        clipping_rect*              rEnd,
        register int      top,
        register int      bottom));

};

#endif // __REGION_SUPPORT_H
