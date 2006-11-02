#ifndef _MATRIX_H
#define _MATRIX_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Point.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

// BMatrix class ---------------------------------------------------------------
class BMatrix {

public:
				BMatrix();

		void	Translate(float tx, float ty);
		void	Rotate(float angle);
		void	Scale(float scaleFactorX, float scaleFactorY);
		void	SkewX(float angle);
		void	SkewY(float angle);

		void	Transform(BPoint *point);
		BPoint	Transform(BPoint point);
		void	Transform(BShape &shape);

		BMatrix &operator*=(const BMatrix &matrix);

		float a, b, c, d, e, f;
};
// BTransformIterator class ----------------------------------------------------
class BTransformIterator : public BShapeIterator {

public:
						BTransformIterator(BMatrix *matrix);

virtual	status_t		IterateMoveTo(BPoint *point);
virtual	status_t		IterateLineTo(int32 lineCount, BPoint *linePts);
virtual	status_t		IterateBezierTo(int32 bezierCount, BPoint *bezierPts);
virtual	status_t		IterateClose();

private:

		BMatrix			*fMatrix;
};
//------------------------------------------------------------------------------
inline BMatrix::BMatrix()
{
	a = d = 1.0f;
	b = c = e = f = 0.0f;
}
//------------------------------------------------------------------------------
inline void BMatrix::Translate(float tx, float ty)
{
	e += a * tx + c * ty;
	f += b * tx + d * ty;
}
//------------------------------------------------------------------------------
inline void BMatrix::Rotate(float angle)
{
	BMatrix m;
	angle = (float)(angle * 3.1415926535897932384626433832795 / 180.0);
	float ca = (float)cos(angle), sa = (float)sin(angle);

	m.a = a * ca + c * sa;
	m.b = b * ca + d * sa;
	m.c = - a * sa + c * ca;
	m.d = - b * sa + d * ca;
	m.e = e;
	m.f = f;

	*this = m;
}
//------------------------------------------------------------------------------
inline void BMatrix::Scale(float scaleFactorX, float scaleFactorY)
{
	BMatrix m;

	m.a = a * scaleFactorX;
	m.b = b * scaleFactorX;
	m.c = c * scaleFactorY;
	m.d = d * scaleFactorY;
	m.e = e;
	m.f = f;

	*this = m;
}
//------------------------------------------------------------------------------
inline void BMatrix::SkewX(float angle)
{
	BMatrix m;
	angle = (float)(angle * 3.1415926535897932384626433832795 / 180.0);
	float x = (float)tan(angle);

	m.a = a;
	m.b = b;
	m.c = a * x + c;
	m.d = b * x + d;
	m.e = e;
	m.f = f;

	*this = m;
}
//------------------------------------------------------------------------------
inline void BMatrix::SkewY(float angle)
{
	BMatrix m;
	angle = (float)(angle * 3.1415926535897932384626433832795 / 180.0);
	float y = (float)tan(angle);

	m.a = a + c * y;
	m.b = b + d * y;
	m.c = c;
	m.d = d;
	m.e = e;
	m.f = f;

	*this = m;
}
//------------------------------------------------------------------------------
inline void BMatrix::Transform(BPoint *point)
{
	float x = point->x;
	float y = point->y;

	point->x = x * a + y * c + e;
	point->y = x * b + y * d + f;
}
//------------------------------------------------------------------------------
inline BPoint BMatrix::Transform(BPoint point)
{
	Transform(&point);
	return point;
}
//------------------------------------------------------------------------------
inline void BMatrix::Transform(BShape &shape)
{
	BTransformIterator transform(this);
	transform.Iterate(&shape);
}
//------------------------------------------------------------------------------
inline BMatrix &BMatrix::operator*=(const BMatrix &matrix)
{
	BMatrix m;

	m.a = a * matrix.a + c * matrix.b;
	m.b = b * matrix.a + d * matrix.b;
	m.c = a * matrix.c + c * matrix.d;
	m.d = b * matrix.c + d * matrix.d;
	m.e = a * matrix.e + c * matrix.f + e;
	m.f = b * matrix.e + d * matrix.f + f;

	*this = m;

	return *this;
}
//------------------------------------------------------------------------------
inline BTransformIterator::BTransformIterator(BMatrix *matrix)
{
	fMatrix = matrix;
}
//------------------------------------------------------------------------------
inline status_t BTransformIterator::IterateMoveTo(BPoint *point)
{
	fMatrix->Transform(point);
	return B_OK;
}
//------------------------------------------------------------------------------
inline status_t BTransformIterator::IterateLineTo(int32 lineCount, BPoint *linePts)
{
	for (int32 i = 0; i < lineCount; i++)
		fMatrix->Transform(linePts++);
	return B_OK;
}
//------------------------------------------------------------------------------
inline status_t BTransformIterator::IterateBezierTo(int32 bezierCount,
											 BPoint *bezierPts)
{
	for (int32 i = 0; i < bezierCount * 3; i++)
		fMatrix->Transform(bezierPts++);
	return B_OK;
}
//------------------------------------------------------------------------------
inline status_t BTransformIterator::IterateClose()
{
	return B_OK;
}
//------------------------------------------------------------------------------

#endif // _MATRIX_H
