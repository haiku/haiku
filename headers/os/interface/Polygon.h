/*
 * Copyright 2001-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers, mflerackers@androme.be
 */
#ifndef _POLYGON_H
#define _POLYGON_H


#include <BeBuild.h>
#include <InterfaceDefs.h>
#include <Rect.h>


class BPolygon {
	public:
		BPolygon(const BPoint *ptArray, int32 numPoints);
		BPolygon(const BPolygon *polygon);
		BPolygon();
		virtual ~BPolygon();

		BPolygon &operator=(const BPolygon &from);

		BRect Frame() const;
		void AddPoints(const BPoint *ptArray, int32 numPoints);
		int32 CountPoints() const;
		void MapTo(BRect srcRect, BRect dstRect);
		void PrintToStream() const;

	private:
		friend class BView;

		void _ComputeBounds();
		void _MapPoint(BPoint *point, BRect srcRect, BRect dstRect);
		void _MapRectangle(BRect *rect, BRect srcRect, BRect dstRect);

	private:
		BRect	fBounds;
		int32	fCount;
		BPoint	*fPoints;
};

#endif // _POLYGON_H_
