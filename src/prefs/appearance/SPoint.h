#ifndef	_SPOINT_H
#define	_SPOINT_H

#include <BeBuild.h>
#include <SupportDefs.h>

class SRect;

#ifndef BE_SUPPORT
#define BE_SUPPORT
#endif

#ifdef BE_SUPPORT
#include <Rect.h>
#include <Point.h>
#endif

class SPoint 
{
public:
	float x;
	float y;

	SPoint();
	SPoint(float X, float Y);
	SPoint(const SPoint &p);
		
	SPoint	&operator=(const SPoint &p);
	void	Set(float X, float Y);

	void	ConstrainTo(SRect r);
	void	PrintToStream() const;
			
	SPoint	operator+(const SPoint &p) const;
	SPoint	operator-(const SPoint &p) const;
	SPoint	operator*(const SPoint &p) const;
	SPoint	operator/(const SPoint &p) const;
	SPoint&	operator+=(const SPoint &p);
	SPoint&	operator-=(const SPoint &p);

	bool	operator!=(const SPoint &p) const;
	bool	operator==(const SPoint &p) const;
	bool	operator>=(const SPoint &p) const;
	bool	operator<=(const SPoint &p) const;
	bool	operator>(const SPoint &p) const;
	bool	operator<(const SPoint &p) const;

#ifdef BE_SUPPORT
	BPoint Point(void) { return BPoint(x,y); };
#endif
};

inline SPoint::SPoint()
{
	x = y = 0;
}
inline SPoint::SPoint(float X, float Y)
{
	x = X;
	y = Y;
}
inline SPoint::SPoint(const SPoint& pt)
{
	x = pt.x;
	y = pt.y;
}
inline SPoint &SPoint::operator=(const SPoint& from)
{
	x = from.x;
	y = from.y;
	return *this;
}
inline void SPoint::Set(float X, float Y)
{
	x = X;
	y = Y;
}


#endif
