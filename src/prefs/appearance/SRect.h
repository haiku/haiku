#ifndef	_SRECT_H_
#define	_SRECT_H_

// define used when this class also needs to support BRects
#ifndef BE_SUPPORT
#define BE_SUPPORT
#endif

#include <math.h>
#include <SupportDefs.h>
#include "SPoint.h"

#ifdef BE_SUPPORT
#include <Rect.h>
#endif

class SRect 
{
public:
	float	left;
	float	top;
	float	right;
	float	bottom;

	SRect();
	SRect(const SRect &r);
	SRect(float l, float t, float r, float b);
	SRect(SPoint lt, SPoint rb);

	SRect	&operator=(const SRect &r);
	void	Set(float l, float t, float r, float b);

	void	PrintToStream() const;

	SPoint	LeftTop() const;
	SPoint	RightBottom() const;
	SPoint	LeftBottom() const;
	SPoint	RightTop() const;

	void	SetLeftTop(const SPoint p);
	void	SetRightBottom(const SPoint p);
	void	SetLeftBottom(const SPoint p);
	void	SetRightTop(const SPoint p);

	// transformation
	void	InsetBy(SPoint p);
	void	InsetBy(float dx, float dy);
	void	OffsetBy(SPoint p);
	void	OffsetBy(float dx, float dy);
	void	OffsetTo(SPoint p);
	void	OffsetTo(float x, float y);

	// expression transformations
	SRect &	InsetBySelf(SPoint);
	SRect &	InsetBySelf(float dx, float dy);
	SRect	InsetByCopy(SPoint);
	SRect	InsetByCopy(float dx, float dy);
	SRect &	OffsetBySelf(SPoint);
	SRect &	OffsetBySelf(float dx, float dy);
	SRect	OffsetByCopy(SPoint);
	SRect	OffsetByCopy(float dx, float dy);
	SRect &	OffsetToSelf(SPoint);
	SRect &	OffsetToSelf(float dx, float dy);
	SRect	OffsetToCopy(SPoint);
	SRect	OffsetToCopy(float dx, float dy);

	// comparison
	bool	operator==(SRect r) const;
	bool	operator!=(SRect r) const;

	// intersection and union
	SRect	operator&(SRect r) const;
	SRect	operator|(SRect r) const;

	bool	Intersects(SRect r) const;
	bool	IsValid() const;
	float	Width() const;
	int32	IntegerWidth() const;
	float	Height() const;
	int32	IntegerHeight() const;
	bool	Contains(SPoint p) const;
	bool	Contains(SRect r) const;

#ifdef BE_SUPPORT
	BRect  Rect(void) { return BRect(left,top,right,bottom); };
#endif
};


// inline definitions ----------------------------------------------------------

inline SPoint SRect::LeftTop() const
{
	return(*((const SPoint*)&left));
}

inline SPoint SRect::RightBottom() const
{
	return(*((const SPoint*)&right));
}

inline SPoint SRect::LeftBottom() const
{
	return(SPoint(left, bottom));
}

inline SPoint SRect::RightTop() const
{
	return(SPoint(right, top));
}

inline SRect::SRect()
{
	top = left = 0;
	bottom = right = -1;
}

inline SRect::SRect(float l, float t, float r, float b)
{
	left = l;
	top = t;
	right = r;
	bottom = b;
}

inline SRect::SRect(const SRect &r)
{
	left = r.left;
	top = r.top;
	right = r.right;
	bottom = r.bottom;
}

inline SRect::SRect(SPoint leftTop, SPoint rightBottom)
{
	left = leftTop.x;
	top = leftTop.y;
	right = rightBottom.x;
	bottom = rightBottom.y;
}

inline SRect &SRect::operator=(const SRect& from)
{
	left = from.left;
	top = from.top;
	right = from.right;
	bottom = from.bottom;
	return *this;
}

inline void SRect::Set(float l, float t, float r, float b)
{
	left = l;
	top = t;
	right = r;
	bottom = b;
}

inline bool SRect::IsValid() const
{
	if (left <= right && top <= bottom)
		return true;
	else
		return false;
}

inline int32 SRect::IntegerWidth() const
{
	return((int32)ceil(right - left));
}

inline float SRect::Width() const
{
	return(right - left);
}

inline int32 SRect::IntegerHeight() const
{
	return((int32)ceil(bottom - top));
}

inline float SRect::Height() const
{
	return(bottom - top);
}

#endif
