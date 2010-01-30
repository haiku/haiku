/*	PROJECT:		3Dmov
	AUTHORS:		Zenja Solaja
	COPYRIGHT:		2009 Haiku Inc
	DESCRIPTION:	Haiku version of the famous BeInc demo 3Dmov
					Just drag'n'drop media files to the 3D objects
*/

#ifndef _GL_UTILITY_H_
#define _GL_UTILITY_H_

#include <cmath>

/**************************
	Definitions
***************************/
#define Y_PI			3.14159265358979323846f
#define Y_PI_DIV_180	(Y_PI/180.0f)


/**************************
	Macros
***************************/
#define sind(a)		(sinf((a) * Y_PI_DIV_180))
#define cosd(a)		(cosf((a) * Y_PI_DIV_180))
#define tand(a)		(tanf((a) * Y_PI_DIV_180))

/**************************
	Functions
***************************/
void GLCreateIdentityMatrix(float *m);
void GLMatrixMultiply(float *destination, float *matrix_a, float *matrix_b);
void GLCreatePerspectiveMatrix(float *m, float fov, float aspect, float znear, float zfar);
void GLCreateModelViewMatrix(float *m, float x, float y, float z, float yaw, float pitch, float roll=0.0f);

/**************************
	Quaternion class
***************************/
class Quaternion  
{
public:
				Quaternion();
	
	void		GenerateLocalRotation(float angle, float x_axis, float y_axis, float z_axis);
	void		CreateRotatedQuaternion(float *matrix);
	void		GenerateFromEuler(float roll, float pitch, float yaw);

	//	Operators
	inline const Quaternion operator+=(const Quaternion &q) {x += q.x; y += q.y; z += q.z; w += q.w; return *this;}
	inline const Quaternion operator-=(const Quaternion &q) {x -= q.x; y -= q.y; z -= q.z; w -= q.w; return *this;}
	const Quaternion	operator *(Quaternion q);

private:
	float		x;
	float		y;
	float		z;
	float		w;
};
#endif	//#ifndef _GL_UTILITY_H_
