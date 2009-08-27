/*
 * Copyright 2001-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _POLYGON_H
#define _POLYGON_H


#include <InterfaceDefs.h>
#include <Rect.h>

//namespace BPrivate { class BAffineTransform; }
//using namespace BPrivate;


class BPolygon {
public:
								BPolygon(const BPoint* points, int32 count);
								BPolygon(const BPolygon& other);
								BPolygon(const BPolygon* other);
								BPolygon();
	virtual						~BPolygon();

			BPolygon&			operator=(const BPolygon& other);

			BRect				Frame() const;
			void				AddPoints(const BPoint* points, int32 count);
			int32				CountPoints() const;
			void				MapTo(BRect srcRect, BRect dstRect);
			void				PrintToStream() const;

//			void				TransformBy(const BAffineTransform& transform);
//			BPolygon&			TransformBySelf(
//									const BAffineTransform& transform);
//			BPolygon			TransformByCopy(
//									const BAffineTransform& transform) const;

private:
	friend class BView;

			bool				_AddPoints(const BPoint* points, int32 count,
									bool computeBounds);
			void				_ComputeBounds();
			void				_MapPoint(BPoint* point, const BRect& srcRect,
										const BRect& dstRect);
			void				_MapRectangle(BRect* rect,
									const BRect& srcRect,
									const BRect& dstRect);

private:
			BRect				fBounds;
			uint32				fCount;
			BPoint*				fPoints;
};

#endif // _POLYGON_H_
