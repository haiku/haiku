/*******************************************************************************
/
/	File:			Region.h
/
/   Description:    BRegion represents an area that's composed of individual
/                   rectangles.
/
/	Copyright 1992-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_REGION_H
#define	_REGION_H

#include <BeBuild.h>
#include <Rect.h>


/* Integer rect used to define a cliping rectangle. All bounds are included */
/* Moved from DirectWindow.h */
typedef struct {
	int32	left;
	int32	top;
	int32	right;
	int32	bottom;
} clipping_rect;

namespace BPrivate {
	class ServerLink;
};

/*----- BRegion class --------------------------------------------*/
class ClipRegion;
class BDirectWindow;
class BRegion {
public:
				BRegion();
				BRegion(const BRegion &region);
				BRegion(const BRect rect);
virtual			~BRegion();	

		BRegion	&operator=(const BRegion &from);

		BRect	Frame() const;
clipping_rect	FrameInt() const;
		BRect	RectAt(int32 index);
clipping_rect	RectAtInt(int32 index);
		int32	CountRects();
		void	Set(BRect newBounds);
		void	Set(clipping_rect newBounds);
		bool	Intersects(BRect r) const;
		bool	Intersects(clipping_rect r) const;
		bool	Contains(BPoint pt) const;
		bool	Contains(int32 x, int32 y);
		void	PrintToStream() const;
		void	OffsetBy(int32 dh, int32 dv);
		void	MakeEmpty();
		void	Include(BRect r);
		void	Include(clipping_rect r);
		void	Include(const BRegion*);
		void	Exclude(BRect r);
		void	Exclude(clipping_rect r);
		void	Exclude(const BRegion*);
		void	IntersectWith(const BRegion*);

/*----- Private or reserved -----------------------------------------*/
private:
friend class BPrivate::ServerLink;
friend class BDirectWindow;

		ClipRegion *fRegion;
		char padding[24];
};

#endif /* _REGION_H */
