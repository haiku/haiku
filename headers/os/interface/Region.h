/*
 * Copyright 2007, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef	_REGION_H
#define	_REGION_H

#include <Rect.h>

namespace BPrivate {
	class ServerLink;
	class LinkReceiver;
};

/* Integer rect used to define a clipping rectangle. All bounds are inclusive. */
/* Moved from DirectWindow.h */
typedef struct {
	int32	left;
	int32	top;
	int32	right;
	int32	bottom;
} clipping_rect;


class BRegion {
public:
								BRegion();
								BRegion(const BRegion& region);
								BRegion(const BRect rect);
	virtual						~BRegion();

			BRegion&			operator=(const BRegion& from);
			bool				operator==(const BRegion& other) const;

			void				Set(BRect newBounds);
			void				Set(clipping_rect newBounds);

			BRect				Frame() const;
			clipping_rect		FrameInt() const;

			BRect				RectAt(int32 index);
			BRect				RectAt(int32 index) const;
			clipping_rect		RectAtInt(int32 index);
			clipping_rect		RectAtInt(int32 index) const;

			int32				CountRects();
			int32				CountRects() const;

			bool				Intersects(BRect rect) const;
			bool				Intersects(clipping_rect rect) const;

			bool				Contains(BPoint point) const;
			bool				Contains(int32 x, int32 y);
			bool				Contains(int32 x, int32 y) const;

			void				PrintToStream() const;

			void				OffsetBy(const BPoint& point);
			void				OffsetBy(int32 x, int32 y);

			void				MakeEmpty();

			void				Include(BRect rect);
			void				Include(clipping_rect rect);
			void				Include(const BRegion* region);

			void				Exclude(BRect r);
			void				Exclude(clipping_rect r);
			void				Exclude(const BRegion* region);

			void				IntersectWith(const BRegion* region);

			void				ExclusiveInclude(const BRegion* region);

private:
	friend class BDirectWindow;
	friend class BPrivate::ServerLink;
	friend class BPrivate::LinkReceiver;

	class Support;
	friend class Support;

private:
								BRegion(const clipping_rect& rect);

			void				_AdoptRegionData(BRegion& region);
			bool				_SetSize(int32 newSize);

			clipping_rect		_Convert(const BRect& rect) const;
			clipping_rect		_ConvertToInternal(const BRect& rect) const;
			clipping_rect		_ConvertToInternal(
									const clipping_rect& rect) const;

private:
			int32				fCount;
			int32				fDataSize;
			clipping_rect		fBounds;
			clipping_rect*		fData;
};

#endif // _REGION_H
