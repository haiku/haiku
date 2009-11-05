/*
 * Copyright 2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexandre Deckner
 *
 */

/*
 *
 * This is a refactored and stripped down version of bullet-2.66
 * src\LinearMath\btVector3.h
 * The dependancies on base class btQuadWord have been removed for
 * simplification.
 *
 */

/*
Copyright (c) 2003-2006 Gino van den Bergen / Erwin Coumans
	http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the
use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim
 that you wrote the original software. If you use this software in a product,
 an acknowledgment in the product documentation would be appreciated but is
 not required.
2. Altered source versions must be plainly marked as such, and must not be
 misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/


#ifndef __VECTOR3_H__
#define __VECTOR3_H__

#include "assert.h"
#include "math.h"

///Vector3 can be used to represent 3D points and vectors.

class	Vector3 {
protected:
	float	m_x;
	float	m_y;
	float	m_z;

public:
	inline Vector3() {}


	inline Vector3(const Vector3& v)
	{
		*((Vector3*)this) = v;
	}


	inline Vector3(const float& x, const float& y, const float& z)
	{
		m_x = x, m_y = y, m_z = z;
	}


	inline const float& x() const { return m_x; }


	inline const float& y() const { return m_y; }


	inline const float& z() const { return m_z; }


	inline  void 	setValue(const float& x, const float& y, const float& z)
	{
		m_x = x;
		m_y = y;
		m_z = z;
	}

	inline Vector3& operator+=(const Vector3& v)
	{
		m_x += v.x(); m_y += v.y(); m_z += v.z();
		return *this;
	}


	inline Vector3& operator-=(const Vector3& v)
	{
		m_x -= v.x(); m_y -= v.y(); m_z -= v.z();
		return *this;
	}


	inline Vector3& operator*=(const float& s)
	{
		m_x *= s; m_y *= s; m_z *= s;
		return *this;
	}


	inline Vector3& operator/=(const float& s)
	{
		assert(s != 0.0f);
		return *this *= 1.0f / s;
	}


	inline float dot(const Vector3& v) const
	{
		return m_x * v.x() + m_y * v.y() + m_z * v.z();
	}


	inline float length2() const
	{
		return dot(*this);
	}


	inline float length() const
	{
		return sqrt(length2());
	}


	inline float distance2(const Vector3& v) const;


	inline float distance(const Vector3& v) const;


	inline Vector3& normalize()
	{
		return *this /= length();
	}


	inline Vector3 normalized() const;


	inline Vector3 rotate( const Vector3& wAxis, const float angle );


	inline float angle(const Vector3& v) const
	{
		float s = sqrt(length2() * v.length2());
		assert(s != 0.0f);
		return acos(dot(v) / s);
	}


	inline Vector3 absolute() const
	{
		return Vector3(
			fabs(m_x),
			fabs(m_y),
			fabs(m_z));
	}


	inline Vector3 cross(const Vector3& v) const
	{
		return Vector3(
			m_y * v.z() - m_z * v.y(),
			m_z * v.x() - m_x * v.z(),
			m_x * v.y() - m_y * v.x());
	}


	inline float triple(const Vector3& v1, const Vector3& v2) const
	{
		return m_x * (v1.y() * v2.z() - v1.z() * v2.y())
			+ m_y * (v1.z() * v2.x() - v1.x() * v2.z())
			+ m_z * (v1.x() * v2.y() - v1.y() * v2.x());
	}


	inline int minAxis() const
	{
		return m_x < m_y ? (m_x < m_z ? 0 : 2) : (m_y < m_z ? 1 : 2);
	}


	inline int maxAxis() const
	{
		return m_x < m_y ? (m_y < m_z ? 2 : 1) : (m_x < m_z ? 2 : 0);
	}


	inline int furthestAxis() const
	{
		return absolute().minAxis();
	}


	inline int closestAxis() const
	{
		return absolute().maxAxis();
	}


	inline void setInterpolate3(const Vector3& v0, const Vector3& v1, float rt)
	{
		float s = 1.0f - rt;
		m_x = s * v0.x() + rt * v1.x();
		m_y = s * v0.y() + rt * v1.y();
		m_z = s * v0.z() + rt * v1.z();
		// don't do the unused w component
		//		m_co[3] = s * v0[3] + rt * v1[3];
	}


	inline Vector3 lerp(const Vector3& v, const float& t) const
	{
		return Vector3(m_x + (v.x() - m_x) * t,
			m_y + (v.y() - m_y) * t,
			m_z + (v.z() - m_z) * t);
	}


	inline Vector3& operator*=(const Vector3& v)
	{
		m_x *= v.x(); m_y *= v.y(); m_z *= v.z();
		return *this;
	}

};


inline Vector3
operator+(const Vector3& v1, const Vector3& v2)
{
	return Vector3(v1.x() + v2.x(), v1.y() + v2.y(), v1.z() + v2.z());
}


inline Vector3
operator*(const Vector3& v1, const Vector3& v2)
{
	return Vector3(v1.x() * v2.x(), v1.y() * v2.y(), v1.z() * v2.z());
}


inline Vector3
operator-(const Vector3& v1, const Vector3& v2)
{
	return Vector3(v1.x() - v2.x(), v1.y() - v2.y(), v1.z() - v2.z());
}


inline Vector3
operator-(const Vector3& v)
{
	return Vector3(-v.x(), -v.y(), -v.z());
}


inline Vector3
operator*(const Vector3& v, const float& s)
{
	return Vector3(v.x() * s, v.y() * s, v.z() * s);
}


inline Vector3
operator*(const float& s, const Vector3& v)
{
	return v * s;
}


inline Vector3
operator/(const Vector3& v, const float& s)
{
	assert(s != 0.0f);
	return v * (1.0f / s);
}


inline Vector3
operator/(const Vector3& v1, const Vector3& v2)
{
	return Vector3(v1.x() / v2.x(),v1.y() / v2.y(),v1.z() / v2.z());
}


inline float
dot(const Vector3& v1, const Vector3& v2)
{
	return v1.dot(v2);
}


inline float
distance2(const Vector3& v1, const Vector3& v2)
{
	return v1.distance2(v2);
}


inline float
distance(const Vector3& v1, const Vector3& v2)
{
	return v1.distance(v2);
}


inline float
angle(const Vector3& v1, const Vector3& v2)
{
	return v1.angle(v2);
}


inline Vector3
cross(const Vector3& v1, const Vector3& v2)
{
	return v1.cross(v2);
}


inline float
triple(const Vector3& v1, const Vector3& v2, const Vector3& v3)
{
	return v1.triple(v2, v3);
}


inline Vector3
lerp(const Vector3& v1, const Vector3& v2, const float& t)
{
	return v1.lerp(v2, t);
}


inline bool
operator==(const Vector3& p1, const Vector3& p2)
{
	return p1.x() == p2.x() && p1.y() == p2.y() && p1.z() == p2.z();
}


inline float
Vector3::distance2(const Vector3& v) const
{
	return (v - *this).length2();
}


inline float
Vector3::distance(const Vector3& v) const
{
	return (v - *this).length();
}


inline Vector3
Vector3::normalized() const
{
	return *this / length();
}


inline Vector3
Vector3::rotate( const Vector3& wAxis, const float angle )
{
	// wAxis must be a unit lenght vector

	Vector3 o = wAxis * wAxis.dot( *this );
	Vector3 x = *this - o;
	Vector3 y;

	y = wAxis.cross( *this );

	return ( o + x * cos( angle ) + y * sin( angle ) );
}

#endif //__VECTOR3_H__
