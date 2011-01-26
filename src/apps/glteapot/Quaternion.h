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
 * This is a refactored and stripped down version of bullet-2.66 src\LinearMath\btQuaternion.h
 * The dependancies on base class btQuadWord have been removed for simplification.
 * Added gl matrix conversion method.
 * 
 */

/*
Copyright (c) 2003-2006 Gino van den Bergen / Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/


#ifndef __QUATERNION_H__
#define __QUATERNION_H__

#include "Vector3.h"

class Quaternion {
protected:
	float	m_x;
	float	m_y;
	float	m_z;
	float	m_w;
public:
	Quaternion() {}
	
	Quaternion(const Quaternion& q)
	{
		*((Quaternion*)this) = q;
	}
	
	Quaternion(const float& x, const float& y, const float& z,const float& w) 
	{
		m_x = x, m_y = y, m_z = z, m_w = w;
	}

	Quaternion(const Vector3& axis, const float& angle) 
	{ 
		setRotation(axis, angle); 
	}

	Quaternion(const float& yaw, const float& pitch, const float& roll)
	{ 
		setEuler(yaw, pitch, roll); 
	}
	
	inline const float& x() const { return m_x; }


	inline const float& y() const { return m_y; }


	inline const float& z() const { return m_z; }


	inline const float& w() const { return m_w; }	
	
		
	void setValue(const float& x, const float& y, const float& z)
	{
		m_x=x;
		m_y=y;
		m_z=z;
		m_w = 0.f;
	}


	void setValue(const float& x, const float& y, const float& z,const float& w)
	{
		m_x=x;
		m_y=y;
		m_z=z;
		m_w=w;
	}
	 

	void setRotation(const Vector3& axis, const float& angle)
	{
		float d = axis.length();
		assert(d != 0.0f);
		float s = sin(angle * 0.5f) / d;
		setValue(axis.x() * s, axis.y() * s, axis.z() * s, 
			cos(angle * 0.5f));
	}


	void setEuler(const float& yaw, const float& pitch, const float& roll)
	{
		float halfYaw = yaw * 0.5f;  
		float halfPitch = pitch * 0.5f;  
		float halfRoll = roll * 0.5f;  
		float cosYaw = cos(halfYaw);
		float sinYaw = sin(halfYaw);
		float cosPitch = cos(halfPitch);
		float sinPitch = sin(halfPitch);
		float cosRoll = cos(halfRoll);
		float sinRoll = sin(halfRoll);
		setValue(cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw,
			cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw,
			sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw,
			cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw);
	}


	Quaternion& operator+=(const Quaternion& q)
	{
		m_x += q.x(); m_y += q.y(); m_z += q.z(); m_w += q.m_w;
		return *this;
	}


	Quaternion& operator-=(const Quaternion& q) 
	{
		m_x -= q.x(); m_y -= q.y(); m_z -= q.z(); m_w -= q.m_w;
		return *this;
	}


	Quaternion& operator*=(const float& s)
	{
		m_x *= s; m_y *= s; m_z *= s; m_w *= s;
		return *this;
	}


	Quaternion& operator*=(const Quaternion& q)
	{
		setValue(m_w * q.x() + m_x * q.m_w + m_y * q.z() - m_z * q.y(),
			m_w * q.y() + m_y * q.m_w + m_z * q.x() - m_x * q.z(),
			m_w * q.z() + m_z * q.m_w + m_x * q.y() - m_y * q.x(),
			m_w * q.m_w - m_x * q.x() - m_y * q.y() - m_z * q.z());
		return *this;
	}


	float dot(const Quaternion& q) const
	{
		return m_x * q.x() + m_y * q.y() + m_z * q.z() + m_w * q.m_w;
	}


	float length2() const
	{
		return dot(*this);
	}


	float length() const
	{
		return sqrt(length2());
	}


	Quaternion& normalize() 
	{
		return *this /= length();
	}


	inline Quaternion
	operator*(const float& s) const
	{
		return Quaternion(x() * s, y() * s, z() * s, m_w * s);
	}


	Quaternion operator/(const float& s) const
	{
		assert(s != 0.0f);
		return *this * (1.0f / s);
	}


	Quaternion& operator/=(const float& s) 
	{
		assert(s != 0.0f);
		return *this *= 1.0f / s;
	}


	Quaternion normalized() const 
	{
		return *this / length();
	} 


	float angle(const Quaternion& q) const 
	{
		float s = sqrt(length2() * q.length2());
		assert(s != 0.0f);
		return acos(dot(q) / s);
	}


	float getAngle() const 
	{
		float s = 2.0f * acos(m_w);
		return s;
	}


	Quaternion inverse() const
	{
		return Quaternion(m_x, m_y, m_z, -m_w);
	}


	inline Quaternion
	operator+(const Quaternion& q2) const
	{
		const Quaternion& q1 = *this;
		return Quaternion(q1.x() + q2.x(), q1.y() + q2.y(), q1.z() + q2.z(), q1.m_w + q2.m_w);
	}


	inline Quaternion
	operator-(const Quaternion& q2) const
	{
		const Quaternion& q1 = *this;
		return Quaternion(q1.x() - q2.x(), q1.y() - q2.y(), q1.z() - q2.z(), q1.m_w - q2.m_w);
	}


	inline Quaternion operator-() const
	{
		const Quaternion& q2 = *this;
		return Quaternion( - q2.x(), - q2.y(),  - q2.z(),  - q2.m_w);
	}


	inline Quaternion farthest( const Quaternion& qd) const 
	{
		Quaternion diff,sum;
		diff = *this - qd;
		sum = *this + qd;
		if( diff.dot(diff) > sum.dot(sum) )
			return qd;
		return (-qd);
	}


	Quaternion slerp(const Quaternion& q, const float& t) const
	{
		float theta = angle(q);
		if (theta != 0.0f)
		{
			float d = 1.0f / sin(theta);
			float s0 = sin((1.0f - t) * theta);
			float s1 = sin(t * theta);   
			return Quaternion((m_x * s0 + q.x() * s1) * d,
				(m_y * s0 + q.y() * s1) * d,
				(m_z * s0 + q.z() * s1) * d,
				(m_w * s0 + q.m_w * s1) * d);
		}
		else
		{
			return *this;
		}
	}


	void toOpenGLMatrix(float m[4][4]){
	
	    float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;
		
	    // calculate coefficients
	    x2 = m_x + m_x; y2 = m_y + m_y;
	    z2 = m_z + m_z;
	    xx = m_x * x2; xy = m_x * y2; xz = m_x * z2;
	    yy = m_y * y2; yz = m_y * z2; zz = m_z * z2;
	    wx = m_w * x2; wy = m_w * y2; wz = m_w * z2;
	
	
	    m[0][0] = 1.0 - (yy + zz); m[1][0] = xy - wz;
	    m[2][0] = xz + wy; m[3][0] = 0.0;
	
	    m[0][1] = xy + wz; m[1][1] = 1.0 - (xx + zz);
	    m[2][1] = yz - wx; m[3][1] = 0.0;
	
	
	    m[0][2] = xz - wy; m[1][2] = yz + wx;
	    m[2][2] = 1.0 - (xx + yy); m[3][2] = 0.0;
	
	
	    m[0][3] = 0; m[1][3] = 0;
	    m[2][3] = 0; m[3][3] = 1;	
	}	
};


inline Quaternion
operator-(const Quaternion& q)
{
	return Quaternion(-q.x(), -q.y(), -q.z(), -q.w());
}


inline Quaternion
operator*(const Quaternion& q1, const Quaternion& q2) {
	return Quaternion(q1.w() * q2.x() + q1.x() * q2.w() + q1.y() * q2.z() - q1.z() * q2.y(),
		q1.w() * q2.y() + q1.y() * q2.w() + q1.z() * q2.x() - q1.x() * q2.z(),
		q1.w() * q2.z() + q1.z() * q2.w() + q1.x() * q2.y() - q1.y() * q2.x(),
		q1.w() * q2.w() - q1.x() * q2.x() - q1.y() * q2.y() - q1.z() * q2.z()); 
}


inline Quaternion
operator*(const Quaternion& q, const Vector3& w)
{
	return Quaternion( q.w() * w.x() + q.y() * w.z() - q.z() * w.y(),
		q.w() * w.y() + q.z() * w.x() - q.x() * w.z(),
		q.w() * w.z() + q.x() * w.y() - q.y() * w.x(),
		-q.x() * w.x() - q.y() * w.y() - q.z() * w.z()); 
}


inline Quaternion
operator*(const Vector3& w, const Quaternion& q)
{
	return Quaternion( w.x() * q.w() + w.y() * q.z() - w.z() * q.y(),
		w.y() * q.w() + w.z() * q.x() - w.x() * q.z(),
		w.z() * q.w() + w.x() * q.y() - w.y() * q.x(),
		-w.x() * q.x() - w.y() * q.y() - w.z() * q.z()); 
}


inline float 
dot(const Quaternion& q1, const Quaternion& q2) 
{ 
	return q1.dot(q2); 
}


inline float
length(const Quaternion& q) 
{ 
	return q.length(); 
}


inline float
angle(const Quaternion& q1, const Quaternion& q2) 
{ 
	return q1.angle(q2); 
}


inline Quaternion
inverse(const Quaternion& q) 
{
	return q.inverse();
}


inline Quaternion
slerp(const Quaternion& q1, const Quaternion& q2, const float& t) 
{
	return q1.slerp(q2, t);
}


inline Quaternion 
shortestArcQuat(const Vector3& v0, const Vector3& v1) // Game Programming Gems 2.10. make sure v0,v1 are normalized
{
	Vector3 c = v0.cross(v1);
	float  d = v0.dot(v1);

	if (d < -1.0 + FLT_EPSILON)
		return Quaternion(0.0f,1.0f,0.0f,0.0f); // just pick any vector

	float  s = sqrt((1.0f + d) * 2.0f);
	float rs = 1.0f / s;

	return Quaternion(c.x()*rs, c.y()*rs, c.z()*rs, s * 0.5f);
}


inline Quaternion 
shortestArcQuatNormalize2(Vector3& v0,Vector3& v1)
{
	v0.normalize();
	v1.normalize();
	return shortestArcQuat(v0,v1);
}
#endif



