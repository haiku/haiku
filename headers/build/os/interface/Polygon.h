//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Polygon.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BPolygon represents a n-sided area.
//------------------------------------------------------------------------------

#ifndef _POLYGON_H
#define _POLYGON_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <InterfaceDefs.h>
#include <Rect.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

// BPolygon class --------------------------------------------------------------
class BPolygon {

public:
					BPolygon(const BPoint *ptArray, int32 numPoints);
					BPolygon();
					BPolygon(const BPolygon *poly);
virtual				~BPolygon();

					BPolygon	&operator=(const BPolygon &from);

		BRect		Frame() const;
		void		AddPoints(const BPoint *ptArray, int32 numPoints);
		int32		CountPoints() const;
		void		MapTo(BRect srcRect, BRect dstRect);
		void		PrintToStream() const;

private:

friend class BView;

		void	compute_bounds();
		void	map_pt(BPoint *point, BRect srcRect, BRect dstRect);
		void	map_rect(BRect *rect, BRect srcRect, BRect dstRect);

		BRect	fBounds;
		int32	fCount;
		BPoint	*fPts;
};
//------------------------------------------------------------------------------

#endif // _POLYGON_H_

/*
 * $Log $
 *
 * $Id  $
 *
 */

