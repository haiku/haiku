/* $Xorg: Region.c,v 1.6 2001/02/09 02:03:35 xorgcvs Exp $ */
/************************************************************************

Copyright 1987, 1988, 1998  The Open Group

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


Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

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
/* $XFree86: xc/lib/X11/Region.c,v 1.9 2002/06/04 22:19:57 dawes Exp $ */
/*
 * The functions in this file implement the BRegion* abstraction, similar to one
 * used in the X11 sample server. A BRegion* is simply an area, as the name
 * implies, and is implemented as a "y-x-banded" array of rectangles. To
 * explain: Each BRegion* is made up of a certain number of rectangles sorted
 * by y coordinate first, and then by x coordinate.
 *
 * Furthermore, the rectangles are banded such that every rectangle with a
 * given upper-left y coordinate (top) will have the same lower-right y
 * coordinate (bottom) and vice versa. If a rectangle has scanlines in a band, it
 * will span the entire vertical distance of the band. This means that some
 * areas that could be merged into a taller rectangle will be represented as
 * several shorter rectangles to account for shorter rectangles to its left
 * or right but within its "vertical scope".
 *
 * An added constraint on the rectangles is that they must cover as much
 * horizontal area as possible. E.g. no two rectangles in a band are allowed
 * to touch.
 *
 * Whenever possible, bands will be merged together to cover a greater vertical
 * distance (and thus reduce the number of rectangles). Two bands can be merged
 * only if the bottom of one touches the top of the other and they have
 * rectangles in the same places (of the same width, of course). This maintains
 * the y-x-banding that's so nice to have...
 */

#include "RegionSupport.h"

#include <stdlib.h>
#include <new>

using std::nothrow;

#include <SupportDefs.h>


#ifdef DEBUG
#include <stdio.h>
#define assert(expr) {if (!(expr)) fprintf(stderr,\
"Assertion failed file %s, line %d: " #expr "\n", __FILE__, __LINE__); }
#else
#define assert(expr)
#endif


/*  1 if two clipping_rects overlap.
 *  0 if two clipping_rects do not overlap.
 *  Remember, right and bottom are not in the region
 */
#define EXTENTCHECK(r1, r2) \
	((r1)->right > (r2)->left && \
	 (r1)->left < (r2)->right && \
	 (r1)->bottom > (r2)->top && \
	 (r1)->top < (r2)->bottom)

/*
 *  update region fBounds
 */
#define EXTENTS(r,idRect){\
            if ((r)->left < (idRect)->fBounds.left)\
              (idRect)->fBounds.left = (r)->left;\
            if ((r)->top < (idRect)->fBounds.top)\
              (idRect)->fBounds.top = (r)->top;\
            if ((r)->right > (idRect)->fBounds.right)\
              (idRect)->fBounds.right = (r)->right;\
            if ((r)->bottom > (idRect)->fBounds.bottom)\
              (idRect)->fBounds.bottom = (r)->bottom;\
        }

/*
 *   Check to see if there is enough memory in the present region.
 */
#define MEMCHECK(reg, rect, firstrect){\
        if ((reg)->fCount >= ((reg)->fDataSize - 1)){\
          (firstrect) = (clipping_rect *) realloc \
          ((char *)(firstrect), (unsigned) (2 * (sizeof(clipping_rect)) * ((reg)->fDataSize)));\
          if ((firstrect) == 0)\
            return(0);\
          (reg)->fDataSize *= 2;\
          (rect) = &(firstrect)[(reg)->fCount];\
         }\
       }

/*  this routine checks to see if the previous rectangle is the same
 *  or subsumes the new rectangle to add.
 */

#define CHECK_PREVIOUS(Reg, R, Rx1, Ry1, Rx2, Ry2)\
               (!(((Reg)->fCount > 0)&&\
                  ((R-1)->top == (Ry1)) &&\
                  ((R-1)->bottom == (Ry2)) &&\
                  ((R-1)->left <= (Rx1)) &&\
                  ((R-1)->right >= (Rx2))))

/*  add a rectangle to the given BRegion */
#define ADDRECT(reg, r, rx1, ry1, rx2, ry2){\
    if (((rx1) < (rx2)) && ((ry1) < (ry2)) &&\
        CHECK_PREVIOUS((reg), (r), (rx1), (ry1), (rx2), (ry2))){\
              (r)->left = (rx1);\
              (r)->top = (ry1);\
              (r)->right = (rx2);\
              (r)->bottom = (ry2);\
              EXTENTS((r), (reg));\
              (reg)->fCount++;\
              (r)++;\
            }\
        }



/*  add a rectangle to the given BRegion */
#define ADDRECTNOX(reg, r, rx1, ry1, rx2, ry2){\
            if ((rx1 < rx2) && (ry1 < ry2) &&\
                CHECK_PREVIOUS((reg), (r), (rx1), (ry1), (rx2), (ry2))){\
              (r)->left = (rx1);\
              (r)->top = (ry1);\
              (r)->right = (rx2);\
              (r)->bottom = (ry2);\
              (reg)->fCount++;\
              (r)++;\
            }\
        }

#define EMPTY_REGION(pReg) pReg->MakeEmpty()

#define REGION_NOT_EMPTY(pReg) pReg->fCount

#define INBOX(r, x, y) \
      ( ( ((r).right >  x)) && \
        ( ((r).left <= x)) && \
        ( ((r).bottom >  y)) && \
        ( ((r).top <= y)) )



/*	Create a new empty region	*/
BRegion*
BRegion::Support::CreateRegion(void)
{
    return new (nothrow) BRegion();
}

void
BRegion::Support::DestroyRegion(BRegion* r)
{
	delete r;
}


/*-
 *-----------------------------------------------------------------------
 * miSetExtents --
 *	Reset the fBounds of a region to what they should be. Called by
 *	miSubtract and miIntersect b/c they can't figure it out along the
 *	way or do so easily, as miUnion can.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The region's 'fBounds' structure is overwritten.
 *
 *-----------------------------------------------------------------------
 */
void
BRegion::Support::miSetExtents(BRegion* pReg)
{
    register clipping_rect*	pBox;
	clipping_rect* pBoxEnd;
	clipping_rect* pExtents;

    if (pReg->fCount == 0)
    {
	pReg->fBounds.left = 0;
	pReg->fBounds.top = 0;
	pReg->fBounds.right = 0;
	pReg->fBounds.bottom = 0;
	return;
    }

    pExtents = &pReg->fBounds;
    pBox = pReg->fData;
    pBoxEnd = &pBox[pReg->fCount - 1];

    /*
     * Since pBox is the first rectangle in the region, it must have the
     * smallest top and since pBoxEnd is the last rectangle in the region,
     * it must have the largest bottom, because of banding. Initialize left and
     * right from  pBox and pBoxEnd, resp., as good things to initialize them
     * to...
     */
    pExtents->left = pBox->left;
    pExtents->top = pBox->top;
    pExtents->right = pBoxEnd->right;
    pExtents->bottom = pBoxEnd->bottom;

    assert(pExtents->top < pExtents->bottom);
    while (pBox <= pBoxEnd)
    {
	if (pBox->left < pExtents->left)
	{
	    pExtents->left = pBox->left;
	}
	if (pBox->right > pExtents->right)
	{
	    pExtents->right = pBox->right;
	}
	pBox++;
    }
    assert(pExtents->left < pExtents->right);
}


/* TranslateRegion(pRegion, x, y)
   translates in place
   added by raymond
*/
void
BRegion::Support::XOffsetRegion(
    register BRegion* pRegion,
    register int x,
    register int y)
{
    register int nbox;
    register clipping_rect *pbox;

    pbox = pRegion->fData;
    nbox = pRegion->fCount;

    while(nbox--)
    {
	pbox->left += x;
	pbox->right += x;
	pbox->top += y;
	pbox->bottom += y;
	pbox++;
    }
    pRegion->fBounds.left += x;
    pRegion->fBounds.right += x;
    pRegion->fBounds.top += y;
    pRegion->fBounds.bottom += y;
}

/*
   Utility procedure Compress:
   Replace r by the region r', where
     p in r' iff (Quantifer m <= dx) (p + m in r), and
     Quantifier is Exists if grow is true, For all if grow is false, and
     (x,y) + m = (x+m,y) if xdir is true; (x,y+m) if xdir is false.

   Thus, if xdir is true and grow is false, r is replaced by the region
   of all points p such that p and the next dx points on the same
   horizontal scan line are all in r.  We do this using by noting
   that p is the head of a run of length 2^i + k iff p is the head
   of a run of length 2^i and p+2^i is the head of a run of length
   k. Thus, the loop invariant: s contains the region corresponding
   to the runs of length shift.  r contains the region corresponding
   to the runs of length 1 + dxo & (shift-1), where dxo is the original
   value of dx.  dx = dxo & ~(shift-1).  As parameters, s and t are
   scratch regions, so that we don't have to allocate them on every
   call.
*/

#if 0
#define ZOpRegion(a,b,c) if (grow) BRegion::Support::XUnionRegion(a,b,c); \
			 else BRegion::Support::XIntersectRegion(a,b,c)
#define ZShiftRegion(a,b) if (xdir) BRegion::Support::XOffsetRegion(a,b,0); \
			  else BRegion::Support::XOffsetRegion(a,0,b)
#define ZCopyRegion(a,b) BRegion::Support::XUnionRegion(a,a,b)

static void
Compress(
    BRegion* r, BRegion* s, BRegion* t,
    register unsigned dx,
    register int xdir, register int grow)
{
    register unsigned shift = 1;

    ZCopyRegion(r, s);
    while (dx) {
        if (dx & shift) {
            ZShiftRegion(r, -(int)shift);
            ZOpRegion(r, s, r);
            dx -= shift;
            if (!dx) break;
        }
        ZCopyRegion(s, t);
        ZShiftRegion(s, -(int)shift);
        ZOpRegion(s, t, s);
        shift <<= 1;
    }
}

#undef ZOpRegion
#undef ZShiftRegion
#undef ZCopyRegion

int
XShrinkRegion(
    BRegion* r,
    int dx, int dy)
{
    BRegion* s;
    BRegion* t;
    int grow;

    if (!dx && !dy) return 0;
    if ((! (s = CreateRegion()))  || (! (t = CreateRegion()))) return 0;
    if ((grow = (dx < 0))) dx = -dx;
    if (dx) Compress(r, s, t, (unsigned) 2*dx, true, grow);
    if ((grow = (dy < 0))) dy = -dy;
    if (dy) Compress(r, s, t, (unsigned) 2*dy, false, grow);
    XOffsetRegion(r, dx, dy);
    DestroyRegion(s);
    DestroyRegion(t);
    return 0;
}

#ifdef notdef
/***********************************************************
 *     Bop down the array of fData until we have passed
 *     scanline y.  fCount is the fDataSize of the array.
 ***********************************************************/

static clipping_rect*
IndexRects(
    register clipping_rect *rect,
    register int rectCount,
    register int y)
{
     while ((rectCount--) && (rect->bottom <= y))
        rect++;
     return(rect);
}
#endif
#endif // 0

/*======================================================================
 *	    BRegion* Intersection
 *====================================================================*/
/*-
 *-----------------------------------------------------------------------
 * miIntersectO --
 *	Handle an overlapping band for miIntersect.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Rectangles may be added to the region.
 *
 *-----------------------------------------------------------------------
 */
int
BRegion::Support::miIntersectO (
    register BRegion*	pReg,
    register clipping_rect*	r1,
    clipping_rect*  	  	r1End,
    register clipping_rect*	r2,
    clipping_rect*  	  	r2End,
    int    	  	top,
    int    	  	bottom)
{
    register int  	left;
    register int  	right;
    register clipping_rect*	pNextRect;

    pNextRect = &pReg->fData[pReg->fCount];

    while ((r1 != r1End) && (r2 != r2End))
    {
	left = max_c(r1->left,r2->left);
	right = min_c(r1->right,r2->right);

	/*
	 * If there's any overlap between the two rectangles, add that
	 * overlap to the new region.
	 * There's no need to check for subsumption because the only way
	 * such a need could arise is if some region has two rectangles
	 * right next to each other. Since that should never happen...
	 */
	if (left < right)
	{
	    assert(top<bottom);

	    MEMCHECK(pReg, pNextRect, pReg->fData);
	    pNextRect->left = left;
	    pNextRect->top = top;
	    pNextRect->right = right;
	    pNextRect->bottom = bottom;
	    pReg->fCount += 1;
	    pNextRect++;
	    assert(pReg->fCount <= pReg->fDataSize);
	}

	/*
	 * Need to advance the pointers. Shift the one that extends
	 * to the right the least, since the other still has a chance to
	 * overlap with that region's next rectangle, if you see what I mean.
	 */
	if (r1->right < r2->right)
	{
	    r1++;
	}
	else if (r2->right < r1->right)
	{
	    r2++;
	}
	else
	{
	    r1++;
	    r2++;
	}
    }
    return 0;	/* lint */
}

int
BRegion::Support::XIntersectRegion(
    const BRegion* 	  	reg1,
    const BRegion*	  	reg2,          /* source regions     */
    register BRegion* 	newReg)               /* destination BRegion* */
{
   /* check for trivial reject */
    if ( (!(reg1->fCount)) || (!(reg2->fCount))  ||
	(!EXTENTCHECK(&reg1->fBounds, &reg2->fBounds)))
        newReg->fCount = 0;
    else
	miRegionOp (newReg, reg1, reg2,
    		miIntersectO, NULL, NULL);

    /*
     * Can't alter newReg's fBounds before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the fBounds of those regions being the same. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    miSetExtents(newReg);
    return 1;
}

void
BRegion::Support::miRegionCopy(
    register BRegion* dstrgn,
    register const BRegion* rgn)

{
	*dstrgn = *rgn;
}

#if 0
#ifdef notdef

/*
 *  combinRegs(newReg, reg1, reg2)
 *    if one region is above or below the other.
*/

static void
combineRegs(
    register BRegion* newReg,
    BRegion* reg1,
    BRegion* reg2)
{
    register BRegion* tempReg;
    register clipping_rect *rects_;
    register clipping_rect *rects1;
    register clipping_rect *rects2;
    register int total;

    rects1 = reg1->fData;
    rects2 = reg2->fData;

    total = reg1->fCount + reg2->fCount;
    if (! (tempReg = CreateRegion()))
	return;
    tempReg->fDataSize = total;
    /*  region 1 is below region 2  */
    if (reg1->fBounds.top > reg2->fBounds.top)
    {
        miRegionCopy(tempReg, reg2);
        rects_ = &tempReg->fData[tempReg->fCount];
        total -= tempReg->fCount;
        while (total--)
            *rects_++ = *rects1++;
    }
    else
    {
        miRegionCopy(tempReg, reg1);
        rects_ = &tempReg->fData[tempReg->fCount];
        total -= tempReg->fCount;
        while (total--)
            *rects_++ = *rects2++;
    }
    tempReg->fBounds = reg1->fBounds;
    tempReg->fCount = reg1->fCount + reg2->fCount;
    EXTENTS(&reg2->fBounds, tempReg);
    miRegionCopy(newReg, tempReg);

    DestroyRegion(tempReg);
}

/*
 *  QuickCheck checks to see if it does not have to go through all the
 *  the ugly code for the region call.  It returns 1 if it did all
 *  the work for Union, otherwise 0 - still work to be done.
*/

static int
QuickCheck(BRegion* newReg, BRegion* reg1, BRegion* reg2)
{

    /*  if unioning with itself or no fData to union with  */
    if ( (reg1 == reg2) || (!(reg1->fCount)) )
    {
        miRegionCopy(newReg, reg2);
        return true;
    }

    /*   if nothing to union   */
    if (!(reg2->fCount))
    {
        miRegionCopy(newReg, reg1);
        return true;
    }

    /*   could put an extent check to see if add above or below */

    if ((reg1->fBounds.top >= reg2->fBounds.bottom) ||
        (reg2->fBounds.top >= reg1->fBounds.bottom) )
    {
        combineRegs(newReg, reg1, reg2);
        return true;
    }
    return false;
}

/*   TopRects(fData, reg1, reg2)
 * N.B. We now assume that reg1 and reg2 intersect.  Therefore we are
 * NOT checking in the two while loops for stepping off the end of the
 * region.
 */

static int
TopRects(
    register BRegion* newReg,
    register clipping_rect *rects_,
    register BRegion* reg1,
    register BRegion* reg2,
    clipping_rect *FirstRect)
{
    register clipping_rect *tempRects;

    /*  need to add some fData from region 1 */
    if (reg1->fBounds.top < reg2->fBounds.top)
    {
        tempRects = reg1->fData;
        while(tempRects->top < reg2->fBounds.top)
	{
	    MEMCHECK(newReg, rects_, FirstRect);
            ADDRECTNOX(newReg,rects_, tempRects->left, tempRects->top,
		       tempRects->right, min_c(tempRects->bottom, reg2->fBounds.top));
            tempRects++;
	}
    }
    /*  need to add some fData from region 2 */
    if (reg2->fBounds.top < reg1->fBounds.top)
    {
        tempRects = reg2->fData;
        while (tempRects->top < reg1->fBounds.top)
        {
            MEMCHECK(newReg, rects_, FirstRect);
            ADDRECTNOX(newReg, rects_, tempRects->left,tempRects->top,
		       tempRects->right, min_c(tempRects->bottom, reg1->fBounds.top));
            tempRects++;
	}
    }
    return 1;
}
#endif // notdef
#endif // 0

/*======================================================================
 *	    Generic BRegion* Operator
 *====================================================================*/

/*-
 *-----------------------------------------------------------------------
 * miCoalesce --
 *	Attempt to merge the boxes in the current band with those in the
 *	previous one. Used only by miRegionOp.
 *
 * Results:
 *	The new index for the previous band.
 *
 * Side Effects:
 *	If coalescing takes place:
 *	    - rectangles in the previous band will have their bottom fields
 *	      altered.
 *	    - pReg->fCount will be decreased.
 *
 *-----------------------------------------------------------------------
 */
int
BRegion::Support::miCoalesce(
    register BRegion*	pReg,	    	/* BRegion* to coalesce */
    int	    	  	prevStart,  	/* Index of start of previous band */
    int	    	  	curStart)   	/* Index of start of current band */
{
    register clipping_rect*	pPrevBox;   	/* Current box in previous band */
    register clipping_rect*	pCurBox;    	/* Current box in current band */
    register clipping_rect*	pRegEnd;    	/* End of region */
    int	    	  	curNumRects;	/* Number of rectangles in current
					 * band */
    int	    	  	prevNumRects;	/* Number of rectangles in previous
					 * band */
    int	    	  	bandY1;	    	/* Y1 coordinate for current band */

    pRegEnd = &pReg->fData[pReg->fCount];

    pPrevBox = &pReg->fData[prevStart];
    prevNumRects = curStart - prevStart;

    /*
     * Figure out how many rectangles are in the current band. Have to do
     * this because multiple bands could have been added in miRegionOp
     * at the end when one region has been exhausted.
     */
    pCurBox = &pReg->fData[curStart];
    bandY1 = pCurBox->top;
    for (curNumRects = 0;
	 (pCurBox != pRegEnd) && (pCurBox->top == bandY1);
	 curNumRects++)
    {
	pCurBox++;
    }

    if (pCurBox != pRegEnd)
    {
	/*
	 * If more than one band was added, we have to find the start
	 * of the last band added so the next coalescing job can start
	 * at the right place... (given when multiple bands are added,
	 * this may be pointless -- see above).
	 */
	pRegEnd--;
	while (pRegEnd[-1].top == pRegEnd->top)
	{
	    pRegEnd--;
	}
	curStart = pRegEnd - pReg->fData;
	pRegEnd = pReg->fData + pReg->fCount;
    }

    if ((curNumRects == prevNumRects) && (curNumRects != 0)) {
	pCurBox -= curNumRects;
	/*
	 * The bands may only be coalesced if the bottom of the previous
	 * matches the top scanline of the current.
	 */
	if (pPrevBox->bottom == pCurBox->top)
	{
	    /*
	     * Make sure the bands have boxes in the same places. This
	     * assumes that boxes have been added in such a way that they
	     * cover the most area possible. I.e. two boxes in a band must
	     * have some horizontal space between them.
	     */
	    do
	    {
		if ((pPrevBox->left != pCurBox->left) ||
		    (pPrevBox->right != pCurBox->right))
		{
		    /*
		     * The bands don't line up so they can't be coalesced.
		     */
		    return (curStart);
		}
		pPrevBox++;
		pCurBox++;
		prevNumRects -= 1;
	    } while (prevNumRects != 0);

	    pReg->fCount -= curNumRects;
	    pCurBox -= curNumRects;
	    pPrevBox -= curNumRects;

	    /*
	     * The bands may be merged, so set the bottom y of each box
	     * in the previous band to that of the corresponding box in
	     * the current band.
	     */
	    do
	    {
		pPrevBox->bottom = pCurBox->bottom;
		pPrevBox++;
		pCurBox++;
		curNumRects -= 1;
	    } while (curNumRects != 0);

	    /*
	     * If only one band was added to the region, we have to backup
	     * curStart to the start of the previous band.
	     *
	     * If more than one band was added to the region, copy the
	     * other bands down. The assumption here is that the other bands
	     * came from the same region as the current one and no further
	     * coalescing can be done on them since it's all been done
	     * already... curStart is already in the right place.
	     */
	    if (pCurBox == pRegEnd)
	    {
		curStart = prevStart;
	    }
	    else
	    {
		do
		{
		    *pPrevBox++ = *pCurBox++;
		} while (pCurBox != pRegEnd);
	    }

	}
    }
    return (curStart);
}

/*-
 *-----------------------------------------------------------------------
 * miRegionOp --
 *	Apply an operation to two regions. Called by miUnion, miInverse,
 *	miSubtract, miIntersect...
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The new region is overwritten.
 *
 * Notes:
 *	The idea behind this function is to view the two regions as sets.
 *	Together they cover a rectangle of area that this function divides
 *	into horizontal bands where points are covered only by one region
 *	or by both. For the first case, the nonOverlapFunc is called with
 *	each the band and the band's upper and lower fBounds. For the
 *	second, the overlapFunc is called to process the entire band. It
 *	is responsible for clipping the rectangles in the band, though
 *	this function provides the boundaries.
 *	At the end of each band, the new region is coalesced, if possible,
 *	to reduce the number of rectangles in the region.
 *
 *-----------------------------------------------------------------------
 */
void
BRegion::Support::miRegionOp(
    register BRegion* 	newReg,	    	    	/* Place to store result */
    const BRegion*	  	reg1,	    	    	/* First region in operation */
    const BRegion*	  	reg2,	    	    	/* 2d region in operation */
    int    	  	(*overlapFunc)(
        register BRegion*     pReg,
        register clipping_rect*     r1,
        clipping_rect*              r1End,
        register clipping_rect*     r2,
        clipping_rect*              r2End,
        int               top,
        int               bottom),                /* Function to call for over-
						 * lapping bands */
    int    	  	(*nonOverlap1Func)(
        register BRegion*     pReg,
        register clipping_rect*     r,
        clipping_rect*              rEnd,
        register int      top,
        register int      bottom),                /* Function to call for non-
						 * overlapping bands in region
						 * 1 */
    int    	  	(*nonOverlap2Func)(
        register BRegion*     pReg,
        register clipping_rect*     r,
        clipping_rect*              rEnd,
        register int      top,
        register int      bottom))                /* Function to call for non-
						 * overlapping bands in region
						 * 2 */
{
    register clipping_rect*	r1; 	    	    	/* Pointer into first region */
    register clipping_rect*	r2; 	    	    	/* Pointer into 2d region */
    clipping_rect*  	  	r1End;	    	    	/* End of 1st region */
    clipping_rect*  	  	r2End;	    	    	/* End of 2d region */
    register int  	ybot;	    	    	/* Bottom of intersection */
    register int  	ytop;	    	    	/* Top of intersection */
 //   clipping_rect*  	  	oldRects;   	    	/* Old fData for newReg */
    int	    	  	prevBand;   	    	/* Index of start of
						 * previous band in newReg */
    int	    	  	curBand;    	    	/* Index of start of current
						 * band in newReg */
    register clipping_rect* 	r1BandEnd;  	    	/* End of current band in r1 */
    register clipping_rect* 	r2BandEnd;  	    	/* End of current band in r2 */
    int     	  	top;	    	    	/* Top of non-overlapping
						 * band */
    int     	  	bot;	    	    	/* Bottom of non-overlapping
						 * band */

    /*
     * Initialization:
     *	set r1, r2, r1End and r2End appropriately, preserve the important
     * parts of the destination region until the end in case it's one of
     * the two source regions, then mark the "new" region empty, allocating
     * another array of rectangles for it to use.
     */
    r1 = reg1->fData;
    r2 = reg2->fData;
    r1End = r1 + reg1->fCount;
    r2End = r2 + reg2->fCount;

//    oldRects = newReg->fData;

    EMPTY_REGION(newReg);

    /*
     * Allocate a reasonable number of rectangles for the new region. The idea
     * is to allocate enough so the individual functions don't need to
     * reallocate and copy the array, which is time consuming, yet we don't
     * have to worry about using too much memory. I hope to be able to
     * nuke the realloc() at the end of this function eventually.
     */
    if (!newReg->_SetSize(max_c(reg1->fCount,reg2->fCount) * 2)) {
		return;
    }

    /*
     * Initialize ybot and ytop.
     * In the upcoming loop, ybot and ytop serve different functions depending
     * on whether the band being handled is an overlapping or non-overlapping
     * band.
     * 	In the case of a non-overlapping band (only one of the regions
     * has points in the band), ybot is the bottom of the most recent
     * intersection and thus clips the top of the rectangles in that band.
     * ytop is the top of the next intersection between the two regions and
     * serves to clip the bottom of the rectangles in the current band.
     *	For an overlapping band (where the two regions intersect), ytop clips
     * the top of the rectangles of both regions and ybot clips the bottoms.
     */
    if (reg1->fBounds.top < reg2->fBounds.top)
	ybot = reg1->fBounds.top;
    else
	ybot = reg2->fBounds.top;

    /*
     * prevBand serves to mark the start of the previous band so rectangles
     * can be coalesced into larger rectangles. qv. miCoalesce, above.
     * In the beginning, there is no previous band, so prevBand == curBand
     * (curBand is set later on, of course, but the first band will always
     * start at index 0). prevBand and curBand must be indices because of
     * the possible expansion, and resultant moving, of the new region's
     * array of rectangles.
     */
    prevBand = 0;

    do
    {
	curBand = newReg->fCount;

	/*
	 * This algorithm proceeds one source-band (as opposed to a
	 * destination band, which is determined by where the two regions
	 * intersect) at a time. r1BandEnd and r2BandEnd serve to mark the
	 * rectangle after the last one in the current band for their
	 * respective regions.
	 */
	r1BandEnd = r1;
	while ((r1BandEnd != r1End) && (r1BandEnd->top == r1->top))
	{
	    r1BandEnd++;
	}

	r2BandEnd = r2;
	while ((r2BandEnd != r2End) && (r2BandEnd->top == r2->top))
	{
	    r2BandEnd++;
	}

	/*
	 * First handle the band that doesn't intersect, if any.
	 *
	 * Note that attention is restricted to one band in the
	 * non-intersecting region at once, so if a region has n
	 * bands between the current position and the next place it overlaps
	 * the other, this entire loop will be passed through n times.
	 */
	if (r1->top < r2->top)
	{
	    top = max_c(r1->top,ybot);
	    bot = min_c(r1->bottom,r2->top);

	    if ((top != bot) && (nonOverlap1Func != NULL))
	    {
		(* nonOverlap1Func) (newReg, r1, r1BandEnd, top, bot);
	    }

	    ytop = r2->top;
	}
	else if (r2->top < r1->top)
	{
	    top = max_c(r2->top,ybot);
	    bot = min_c(r2->bottom,r1->top);

	    if ((top != bot) && (nonOverlap2Func != NULL))
	    {
		(* nonOverlap2Func) (newReg, r2, r2BandEnd, top, bot);
	    }

	    ytop = r1->top;
	}
	else
	{
	    ytop = r1->top;
	}

	/*
	 * If any rectangles got added to the region, try and coalesce them
	 * with rectangles from the previous band. Note we could just do
	 * this test in miCoalesce, but some machines incur a not
	 * inconsiderable cost for function calls, so...
	 */
	if (newReg->fCount != curBand)
	{
	    prevBand = miCoalesce (newReg, prevBand, curBand);
	}

	/*
	 * Now see if we've hit an intersecting band. The two bands only
	 * intersect if ybot > ytop
	 */
	ybot = min_c(r1->bottom, r2->bottom);
	curBand = newReg->fCount;
	if (ybot > ytop)
	{
	    (* overlapFunc) (newReg, r1, r1BandEnd, r2, r2BandEnd, ytop, ybot);

	}

	if (newReg->fCount != curBand)
	{
	    prevBand = miCoalesce (newReg, prevBand, curBand);
	}

	/*
	 * If we've finished with a band (bottom == ybot) we skip forward
	 * in the region to the next band.
	 */
	if (r1->bottom == ybot)
	{
	    r1 = r1BandEnd;
	}
	if (r2->bottom == ybot)
	{
	    r2 = r2BandEnd;
	}
    } while ((r1 != r1End) && (r2 != r2End));

    /*
     * Deal with whichever region still has rectangles left.
     */
    curBand = newReg->fCount;
    if (r1 != r1End)
    {
	if (nonOverlap1Func != NULL)
	{
	    do
	    {
		r1BandEnd = r1;
		while ((r1BandEnd < r1End) && (r1BandEnd->top == r1->top))
		{
		    r1BandEnd++;
		}
		(* nonOverlap1Func) (newReg, r1, r1BandEnd,
				     max_c(r1->top,ybot), r1->bottom);
		r1 = r1BandEnd;
	    } while (r1 != r1End);
	}
    }
    else if ((r2 != r2End) && (nonOverlap2Func != NULL))
    {
	do
	{
	    r2BandEnd = r2;
	    while ((r2BandEnd < r2End) && (r2BandEnd->top == r2->top))
	    {
		 r2BandEnd++;
	    }
	    (* nonOverlap2Func) (newReg, r2, r2BandEnd,
				max_c(r2->top,ybot), r2->bottom);
	    r2 = r2BandEnd;
	} while (r2 != r2End);
    }

    if (newReg->fCount != curBand)
    {
	(void) miCoalesce (newReg, prevBand, curBand);
    }

    /*
     * A bit of cleanup. To keep regions from growing without bound,
     * we shrink the array of rectangles to match the new number of
     * rectangles in the region. This never goes to 0, however...
     *
     * Only do this stuff if the number of rectangles allocated is more than
     * twice the number of rectangles in the region (a simple optimization...).
     */
//    if (newReg->fCount < (newReg->fDataSize >> 1))
//    {
//	if (REGION_NOT_EMPTY(newReg))
//	{
//	    clipping_rect* prev_rects = newReg->fData;
//	    newReg->fDataSize = newReg->fCount;
//	    newReg->fData = (clipping_rect*) realloc ((char *) newReg->fData,
//				   (unsigned) (sizeof(clipping_rect) * newReg->fDataSize));
//	    if (! newReg->fData)
//		newReg->fData = prev_rects;
//	}
//	else
//	{
//	    /*
//	     * No point in doing the extra work involved in an realloc if
//	     * the region is empty
//	     */
//	    newReg->fDataSize = 1;
//	    free((char *) newReg->fData);
//	    newReg->fData = (clipping_rect*) malloc(sizeof(clipping_rect));
//	}
//    }
//    free ((char *) oldRects);
    return;
}


/*======================================================================
 *	    BRegion* Union
 *====================================================================*/

/*-
 *-----------------------------------------------------------------------
 * miUnionNonO --
 *	Handle a non-overlapping band for the union operation. Just
 *	Adds the rectangles into the region. Doesn't have to check for
 *	subsumption or anything.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	pReg->fCount is incremented and the final rectangles overwritten
 *	with the rectangles we're passed.
 *
 *-----------------------------------------------------------------------
 */
int
BRegion::Support::miUnionNonO(register BRegion* pReg,
	register clipping_rect* r, clipping_rect* rEnd,
    register int top, register int bottom)
{
	register clipping_rect*	pNextRect = &pReg->fData[pReg->fCount];

	assert(top < bottom);

	while (r != rEnd) {
		assert(r->left < r->right);
		MEMCHECK(pReg, pNextRect, pReg->fData);
		pNextRect->left = r->left;
		pNextRect->top = top;
		pNextRect->right = r->right;
		pNextRect->bottom = bottom;
		pReg->fCount += 1;
		pNextRect++;

		assert(pReg->fCount<=pReg->fDataSize);
		r++;
	}
	return 0;
}


/*-
 *-----------------------------------------------------------------------
 * miUnionO --
 *	Handle an overlapping band for the union operation. Picks the
 *	left-most rectangle each time and merges it into the region.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Rectangles are overwritten in pReg->fData and pReg->fCount will
 *	be changed.
 *
 *-----------------------------------------------------------------------
 */

int
BRegion::Support::miUnionO (
    register BRegion*	pReg,
    register clipping_rect*	r1,
    clipping_rect*  	  	r1End,
    register clipping_rect*	r2,
    clipping_rect*  	  	r2End,
    register int	top,
    register int	bottom)
{
    register clipping_rect*	pNextRect;

    pNextRect = &pReg->fData[pReg->fCount];

#define MERGERECT(r) \
    if ((pReg->fCount != 0) &&  \
	(pNextRect[-1].top == top) &&  \
	(pNextRect[-1].bottom == bottom) &&  \
	(pNextRect[-1].right >= r->left))  \
    {  \
	if (pNextRect[-1].right < r->right)  \
	{  \
	    pNextRect[-1].right = r->right;  \
	    assert(pNextRect[-1].left<pNextRect[-1].right); \
	}  \
    }  \
    else  \
    {  \
	MEMCHECK(pReg, pNextRect, pReg->fData);  \
	pNextRect->top = top;  \
	pNextRect->bottom = bottom;  \
	pNextRect->left = r->left;  \
	pNextRect->right = r->right;  \
	pReg->fCount += 1;  \
        pNextRect += 1;  \
    }  \
    assert(pReg->fCount<=pReg->fDataSize);\
    r++;

    assert (top<bottom);
    while ((r1 != r1End) && (r2 != r2End))
    {
	if (r1->left < r2->left)
	{
	    MERGERECT(r1);
	}
	else
	{
	    MERGERECT(r2);
	}
    }

    if (r1 != r1End)
    {
	do
	{
	    MERGERECT(r1);
	} while (r1 != r1End);
    }
    else while (r2 != r2End)
    {
	MERGERECT(r2);
    }
    return 0;	/* lint */
}

int
BRegion::Support::XUnionRegion(
    const BRegion* 	  reg1,
    const BRegion*	  reg2,             /* source regions     */
    BRegion* 	  newReg)                  /* destination BRegion* */
{
    /*  checks all the simple cases */

    /*
     * BRegion* 1 and 2 are the same or region 1 is empty
     */
    if ( (reg1 == reg2) || (!(reg1->fCount)) )
    {
        if (newReg != reg2)
            miRegionCopy(newReg, reg2);
        return 1;
    }

    /*
     * if nothing to union (region 2 empty)
     */
    if (!(reg2->fCount))
    {
        if (newReg != reg1)
            miRegionCopy(newReg, reg1);
        return 1;
    }

    /*
     * BRegion* 1 completely subsumes region 2
     */
    if ((reg1->fCount == 1) &&
	(reg1->fBounds.left <= reg2->fBounds.left) &&
	(reg1->fBounds.top <= reg2->fBounds.top) &&
	(reg1->fBounds.right >= reg2->fBounds.right) &&
	(reg1->fBounds.bottom >= reg2->fBounds.bottom))
    {
        if (newReg != reg1)
            miRegionCopy(newReg, reg1);
        return 1;
    }

    /*
     * BRegion* 2 completely subsumes region 1
     */
    if ((reg2->fCount == 1) &&
	(reg2->fBounds.left <= reg1->fBounds.left) &&
	(reg2->fBounds.top <= reg1->fBounds.top) &&
	(reg2->fBounds.right >= reg1->fBounds.right) &&
	(reg2->fBounds.bottom >= reg1->fBounds.bottom))
    {
        if (newReg != reg2)
            miRegionCopy(newReg, reg2);
        return 1;
    }

    miRegionOp (newReg, reg1, reg2, miUnionO,
    		miUnionNonO, miUnionNonO);

    newReg->fBounds.left = min_c(reg1->fBounds.left, reg2->fBounds.left);
    newReg->fBounds.top = min_c(reg1->fBounds.top, reg2->fBounds.top);
    newReg->fBounds.right = max_c(reg1->fBounds.right, reg2->fBounds.right);
    newReg->fBounds.bottom = max_c(reg1->fBounds.bottom, reg2->fBounds.bottom);

    return 1;
}


/*======================================================================
 * 	    	  BRegion* Subtraction
 *====================================================================*/

/*-
 *-----------------------------------------------------------------------
 * miSubtractNonO --
 *	Deal with non-overlapping band for subtraction. Any parts from
 *	region 2 we discard. Anything from region 1 we add to the region.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	pReg may be affected.
 *
 *-----------------------------------------------------------------------
 */
int
BRegion::Support::miSubtractNonO1 (
    register BRegion*	pReg,
    register clipping_rect*	r,
    clipping_rect*  	  	rEnd,
    register int  	top,
    register int   	bottom)
{
    register clipping_rect*	pNextRect;

    pNextRect = &pReg->fData[pReg->fCount];

    assert(top<bottom);

    while (r != rEnd)
    {
	assert(r->left<r->right);
	MEMCHECK(pReg, pNextRect, pReg->fData);
	pNextRect->left = r->left;
	pNextRect->top = top;
	pNextRect->right = r->right;
	pNextRect->bottom = bottom;
	pReg->fCount += 1;
	pNextRect++;

	assert(pReg->fCount <= pReg->fDataSize);

	r++;
    }
    return 0;	/* lint */
}

/*-
 *-----------------------------------------------------------------------
 * miSubtractO --
 *	Overlapping band subtraction. left is the left-most point not yet
 *	checked.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	pReg may have rectangles added to it.
 *
 *-----------------------------------------------------------------------
 */
int
BRegion::Support::miSubtractO(
    register BRegion*	pReg,
    register clipping_rect*	r1,
    clipping_rect*  	  	r1End,
    register clipping_rect*	r2,
    clipping_rect*  	  	r2End,
    register int  	top,
    register int  	bottom)
{
    register clipping_rect*	pNextRect;
    register int  	left;

    left = r1->left;

    assert(top<bottom);
    pNextRect = &pReg->fData[pReg->fCount];

    while ((r1 != r1End) && (r2 != r2End))
    {
	if (r2->right <= left)
	{
	    /*
	     * Subtrahend missed the boat: go to next subtrahend.
	     */
	    r2++;
	}
	else if (r2->left <= left)
	{
	    /*
	     * Subtrahend preceeds minuend: nuke left edge of minuend.
	     */
	    left = r2->right;
	    if (left >= r1->right)
	    {
		/*
		 * Minuend completely covered: advance to next minuend and
		 * reset left fence to edge of new minuend.
		 */
		r1++;
		if (r1 != r1End)
		    left = r1->left;
	    }
	    else
	    {
		/*
		 * Subtrahend now used up since it doesn't extend beyond
		 * minuend
		 */
		r2++;
	    }
	}
	else if (r2->left < r1->right)
	{
	    /*
	     * Left part of subtrahend covers part of minuend: add uncovered
	     * part of minuend to region and skip to next subtrahend.
	     */
	    assert(left<r2->left);
	    MEMCHECK(pReg, pNextRect, pReg->fData);
	    pNextRect->left = left;
	    pNextRect->top = top;
	    pNextRect->right = r2->left;
	    pNextRect->bottom = bottom;
	    pReg->fCount += 1;
	    pNextRect++;

	    assert(pReg->fCount<=pReg->fDataSize);

	    left = r2->right;
	    if (left >= r1->right)
	    {
		/*
		 * Minuend used up: advance to new...
		 */
		r1++;
		if (r1 != r1End)
		    left = r1->left;
	    }
	    else
	    {
		/*
		 * Subtrahend used up
		 */
		r2++;
	    }
	}
	else
	{
	    /*
	     * Minuend used up: add any remaining piece before advancing.
	     */
	    if (r1->right > left)
	    {
		MEMCHECK(pReg, pNextRect, pReg->fData);
		pNextRect->left = left;
		pNextRect->top = top;
		pNextRect->right = r1->right;
		pNextRect->bottom = bottom;
		pReg->fCount += 1;
		pNextRect++;
		assert(pReg->fCount<=pReg->fDataSize);
	    }
	    r1++;
	    if (r1 != r1End)
		left = r1->left;
	}
    }

    /*
     * Add remaining minuend rectangles to region.
     */
    while (r1 != r1End)
    {
	assert(left<r1->right);
	MEMCHECK(pReg, pNextRect, pReg->fData);
	pNextRect->left = left;
	pNextRect->top = top;
	pNextRect->right = r1->right;
	pNextRect->bottom = bottom;
	pReg->fCount += 1;
	pNextRect++;

	assert(pReg->fCount<=pReg->fDataSize);

	r1++;
	if (r1 != r1End)
	{
	    left = r1->left;
	}
    }
    return 0;	/* lint */
}

/*-
 *-----------------------------------------------------------------------
 * miSubtract --
 *	Subtract regS from regM and leave the result in regD.
 *	S stands for subtrahend, M for minuend and D for difference.
 *
 * Results:
 *	true.
 *
 * Side Effects:
 *	regD is overwritten.
 *
 *-----------------------------------------------------------------------
 */

int
BRegion::Support::XSubtractRegion(
    const BRegion* 	  	regM,
    const BRegion*	  	regS,
    register BRegion*	regD)
{
   /* check for trivial reject */
    if ( (!(regM->fCount)) || (!(regS->fCount))  ||
	(!EXTENTCHECK(&regM->fBounds, &regS->fBounds)) )
    {
	miRegionCopy(regD, regM);
        return 1;
    }

    miRegionOp (regD, regM, regS, miSubtractO,
    		miSubtractNonO1, NULL);

    /*
     * Can't alter newReg's fBounds before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the fBounds of those regions being the unaltered. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    miSetExtents (regD);
    return 1;
}

int
BRegion::Support::XXorRegion(const BRegion* sra, const BRegion* srb,
	BRegion* dr)
{
    BRegion* tra = NULL;
    BRegion* trb = NULL;

    if ((! (tra = CreateRegion())) || (! (trb = CreateRegion())))
    {
        DestroyRegion(tra);
        DestroyRegion(trb);
		return 0;
    }
    (void) XSubtractRegion(sra,srb,tra);
    (void) XSubtractRegion(srb,sra,trb);
    (void) XUnionRegion(tra,trb,dr);
    DestroyRegion(tra);
    DestroyRegion(trb);
    return 0;
}


bool
BRegion::Support::XPointInRegion(
    const BRegion* pRegion,
    int x, int y)
{
	// TODO: binary search by "y"!
    int i;

    if (pRegion->fCount == 0)
        return false;
    if (!INBOX(pRegion->fBounds, x, y))
        return false;
    for (i=0; i<pRegion->fCount; i++)
    {
        if (INBOX (pRegion->fData[i], x, y))
	    return true;
    }
    return false;
}

int
BRegion::Support::XRectInRegion(
    register const BRegion* region,
    const clipping_rect& rect)
{
    register clipping_rect* pbox;
    register clipping_rect* pboxEnd;
    register const clipping_rect* prect = &rect;
    bool      partIn, partOut;

    int rx = prect->left;
    int ry = prect->top;

    /* this is (just) a useful optimization */
    if ((region->fCount == 0) || !EXTENTCHECK(&region->fBounds, prect))
        return(RectangleOut);

    partOut = false;
    partIn = false;

    /* can stop when both partOut and partIn are true, or we reach prect->bottom */
    for (pbox = region->fData, pboxEnd = pbox + region->fCount;
	 pbox < pboxEnd;
	 pbox++)
    {

	if (pbox->bottom <= ry)
	   continue;	/* getting up to speed or skipping remainder of band */

	if (pbox->top > ry)
	{
	   partOut = true;	/* missed part of rectangle above */
	   if (partIn || (pbox->top >= prect->bottom))
	      break;
	   ry = pbox->top;	/* x guaranteed to be == prect->left */
	}

	if (pbox->right <= rx)
	   continue;		/* not far enough over yet */

	if (pbox->left > rx)
	{
	   partOut = true;	/* missed part of rectangle to left */
	   if (partIn)
	      break;
	}

	if (pbox->left < prect->right)
	{
	    partIn = true;	/* definitely overlap */
	    if (partOut)
	       break;
	}

	if (pbox->right >= prect->right)
	{
	   ry = pbox->bottom;	/* finished with this band */
	   if (ry >= prect->bottom)
	      break;
	   rx = prect->left;	/* reset x out to left again */
	} else
	{
	    /*
	     * Because boxes in a band are maximal width, if the first box
	     * to overlap the rectangle doesn't completely cover it in that
	     * band, the rectangle must be partially out, since some of it
	     * will be uncovered in that band. partIn will have been set true
	     * by now...
	     */
	    break;
	}

    }

    return(partIn ? ((ry < prect->bottom) ? RectanglePart : RectangleIn) :
		RectangleOut);
}
